#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/* статусы ошибок */
enum Status {
    ST_OK = 0,          /* успешное выполнение */
    ST_ERR_ARGS,        /* неверное количество или формат аргументов командной строки */
    ST_ERR_FLAG,        /* некорректный или неподдерживаемый флаг */
    ST_ERR_NUM,         /* ошибка разбора числа */
    ST_ERR_RANGE,       /* выход значения за допустимый диапазон */
    ST_ERR_OVERFLOW,    /* переполнение разрядной сетки */
    ST_ERR_NULL_PTR,    /* передан недопустимый нулевой указатель */
    ST_ERR_MEMORY,      /* ошибка выделения динамической памяти */
    ST_ERR_FILE_OPEN,   /* не удалось открыть файл */
    ST_ERR_FILE_READ,   /* ошибка чтения из файла */
    ST_ERR_FILE_WRITE   /* ошибка записи в выходной файл */
};

/* описание файла на сервере */
struct FileInfo {
    char *name;                         /* непустое имя файла без пробелов */
    unsigned long long size;           /* размер файла (положительное целое) */
    unsigned long long importance;     /* важность файла (неотрицательное целое) */
    size_t original_index;             /* порядковый номер во входных данных */
};

/* результат выбора файлов для резервного копирования */
struct BackupResult {
    struct FileInfo *selected_files;   /* массив выбранных файлов */
    size_t count;                       /* количество выбранных файлов */
    unsigned long long total_size;      /* суммарный размер */
    unsigned long long total_importance;/* суммарная важность */
};

/* вычисление длины строки без string.h */
static size_t my_strlen(const char *s) {
    size_t len = 0;
    if (!s) return 0;
    while (s[len] != '\0') ++len;
    return len;
}

/* дублирование строки в куче */
static char *my_strdup(const char *src) {
    if (!src) return NULL;
    const size_t len = my_strlen(src);
    char *dst = (char *)malloc(len + 1);
    if (!dst) return NULL;
    for (size_t i = 0; i < len; ++i) dst[i] = src[i];
    dst[len] = '\0';
    return dst;
}

/* освобождение структуры результата */
void free_backup_result(struct BackupResult *res) {
    if (!res) return;
    if (res->selected_files) {
        for (size_t i = 0; i < res->count; ++i) {
            if (res->selected_files[i].name) {
                free(res->selected_files[i].name);
                res->selected_files[i].name = NULL;
            }
        }
        free(res->selected_files);
        res->selected_files = NULL;
    }
    res->count = 0;
    res->total_size = 0;
    res->total_importance = 0;
}

/* освобождение массива файлов */
void free_file_infos(struct FileInfo *files, const size_t count) {
    if (!files) return;
    for (size_t i = 0; i < count; ++i) {
        if (files[i].name) {
            free(files[i].name);
            files[i].name = NULL;
        }
    }
    free(files);
}

/* чистая функция: ДП для задачи о рюкзаке 0/1 */
static enum Status solve_knapsack_dp(const struct FileInfo *files, const size_t n,
                                     const unsigned long long capacity,
                                     int **out_chosen) {
    if (!files || !out_chosen) return ST_ERR_NULL_PTR;

    const size_t cap = (size_t)capacity;
    const size_t stride = cap + 1;

    unsigned long long *dp = (unsigned long long *)malloc((n + 1) * stride * sizeof(unsigned long long));
    if (!dp) return ST_ERR_MEMORY;

    for (size_t w = 0; w <= cap; ++w) {
        dp[0 * stride + w] = 0;
    }

    for (size_t i = 1; i <= n; ++i) {
        const unsigned long long item_size = files[i - 1].size;
        const unsigned long long item_imp = files[i - 1].importance;

        for (size_t w = 0; w <= cap; ++w) {
            unsigned long long best = dp[(i - 1) * stride + w];
            if (w >= item_size) {
                const unsigned long long with_item = dp[(i - 1) * stride + (w - (size_t)item_size)] + item_imp;
                if (with_item > best) {
                    best = with_item;
                }
            }
            dp[i * stride + w] = best;
        }
    }

    int *chosen = (int *)malloc(n * sizeof(int));
    if (!chosen) {
        free(dp);
        return ST_ERR_MEMORY;
    }

    for (size_t i = 0; i < n; ++i) chosen[i] = 0;

    size_t curr_w = cap;
    for (size_t i = n; i > 0; --i) {
        if (dp[i * stride + curr_w] != dp[(i - 1) * stride + curr_w]) {
            chosen[i - 1] = 1;
            curr_w -= (size_t)files[i - 1].size;
        }
    }

    free(dp);
    *out_chosen = chosen;
    return ST_OK;
}

/* рекурсивный метод ветвей и границ с оценкой суффиксных сумм */
static void knapsack_branch_and_bound(const struct FileInfo *files, const size_t n,
                                      const size_t idx, const unsigned long long current_weight,
                                      const unsigned long long current_imp,
                                      const unsigned long long capacity,
                                      const unsigned long long *suffix_imp,
                                      int *current_chosen,
                                      unsigned long long *best_imp,
                                      unsigned long long *best_size,
                                      int *best_chosen) {
    if (idx == n) {
        if (current_imp > *best_imp || (current_imp == *best_imp && current_weight < *best_size)) {
            *best_imp = current_imp;
            *best_size = current_weight;
            for (size_t i = 0; i < n; ++i) {
                best_chosen[i] = current_chosen[i];
            }
        }
        return;
    }

    /* отсечение ветвей по верхней границе оставшейся важности */
    if (current_imp + suffix_imp[idx] < *best_imp) {
        return;
    }
    if (current_imp + suffix_imp[idx] == *best_imp && current_weight >= *best_size) {
        return;
    }

    /* ветка 1: включить текущий файл */
    if (current_weight + files[idx].size <= capacity) {
        current_chosen[idx] = 1;
        knapsack_branch_and_bound(files, n, idx + 1,
                                  current_weight + files[idx].size,
                                  current_imp + files[idx].importance,
                                  capacity, suffix_imp, current_chosen,
                                  best_imp, best_size, best_chosen);
        current_chosen[idx] = 0;
    }

    /* ветка 2: не включать текущий файл */
    knapsack_branch_and_bound(files, n, idx + 1, current_weight, current_imp,
                              capacity, suffix_imp, current_chosen,
                              best_imp, best_size, best_chosen);
}

/* чистая функция: выбор файлов для резервного копирования */
enum Status select_backup_files(const struct FileInfo *files, const size_t count,
                                const unsigned long long capacity,
                                struct BackupResult *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (count > 0 && !files) return ST_ERR_NULL_PTR;

    res->selected_files = NULL;
    res->count = 0;
    res->total_size = 0;
    res->total_importance = 0;

    if (count == 0 || capacity == 0) {
        return ST_OK;
    }

    int *chosen = NULL;
    enum Status st = ST_OK;

    /*
     * Порог 16M ячеек DP: таблица (n+1)×(capacity+1) типа unsigned long long
     * занимает ~128 МБ при 16M ячеек. Выше порога — branch-and-bound с
     * суффиксными отсечениями, чтобы не исчерпать оперативную память.
     */
    if (capacity <= 2000000ULL && (count + 1) * (capacity + 1) <= 16000000ULL) {
        st = solve_knapsack_dp(files, count, capacity, &chosen);
    } else {
        int *curr_chosen = (int *)calloc(count, sizeof(int));
        int *best_chosen = (int *)calloc(count, sizeof(int));
        unsigned long long *suffix_imp = (unsigned long long *)malloc((count + 1) * sizeof(unsigned long long));

        if (!curr_chosen || !best_chosen || !suffix_imp) {
            if (curr_chosen) free(curr_chosen);
            if (best_chosen) free(best_chosen);
            if (suffix_imp) free(suffix_imp);
            return ST_ERR_MEMORY;
        }

        /* предподсчет суффиксных сумм для быстрой оценки границ */
        suffix_imp[count] = 0;
        for (size_t i = count; i > 0; --i) {
            suffix_imp[i - 1] = suffix_imp[i] + files[i - 1].importance;
        }

        unsigned long long best_imp = 0;
        unsigned long long best_size = 0;

        knapsack_branch_and_bound(files, count, 0, 0, 0, capacity, suffix_imp,
                                  curr_chosen, &best_imp, &best_size, best_chosen);

        free(curr_chosen);
        free(suffix_imp);
        chosen = best_chosen;
        st = ST_OK;
    }

    if (st != ST_OK) return st;

    size_t sel_count = 0;
    unsigned long long sum_size = 0;
    unsigned long long sum_imp = 0;

    for (size_t i = 0; i < count; ++i) {
        if (chosen[i]) {
            ++sel_count;
            sum_size += files[i].size;
            sum_imp += files[i].importance;
        }
    }

    if (sel_count == 0) {
        free(chosen);
        return ST_OK;
    }

    struct FileInfo *out_files = (struct FileInfo *)malloc(sel_count * sizeof(struct FileInfo));
    if (!out_files) {
        free(chosen);
        return ST_ERR_MEMORY;
    }

    size_t k = 0;
    for (size_t i = 0; i < count; ++i) {
        if (chosen[i]) {
            out_files[k].name = my_strdup(files[i].name);
            if (!out_files[k].name) {
                for (size_t j = 0; j < k; ++j) free(out_files[j].name);
                free(out_files);
                free(chosen);
                return ST_ERR_MEMORY;
            }
            out_files[k].size = files[i].size;
            out_files[k].importance = files[i].importance;
            out_files[k].original_index = files[i].original_index;
            ++k;
        }
    }

    free(chosen);

    res->selected_files = out_files;
    res->count = sel_count;
    res->total_size = sum_size;
    res->total_importance = sum_imp;

    return ST_OK;
}

/* вывод сообщений об ошибках */
void print_error(const enum Status st) {
    if (st == ST_ERR_ARGS) {
        printf("Error: problem with arguments.\n");
    } else if (st == ST_ERR_FLAG) {
        printf("Error: problem with flag.\n");
    } else if (st == ST_ERR_NUM) {
        printf("Error: problem with number.\n");
    } else if (st == ST_ERR_RANGE) {
        printf("Error: problem with range.\n");
    } else if (st == ST_ERR_OVERFLOW) {
        printf("Error: problem with overflow.\n");
    } else if (st == ST_ERR_NULL_PTR) {
        printf("Error: problem with null pointer.\n");
    } else if (st == ST_ERR_MEMORY) {
        printf("Error: memory allocation failed.\n");
    } else if (st == ST_ERR_FILE_OPEN) {
        printf("Error: failed to open file.\n");
    } else if (st == ST_ERR_FILE_READ) {
        printf("Error: failed to read file.\n");
    } else if (st == ST_ERR_FILE_WRITE) {
        printf("Error: failed to write to file.\n");
    }
}

int main(void) {
    size_t n = 0;
    unsigned long long capacity = 0;

    if (scanf("%zu %llu", &n, &capacity) != 2) {
        print_error(ST_ERR_NUM);
        return (int)ST_ERR_NUM;
    }

    /* количество файлов должно быть строго положительным числом */
    if (n == 0) {
        print_error(ST_ERR_RANGE);
        return (int)ST_ERR_RANGE;
    }

    struct FileInfo *files = NULL;
    if (n > 0) {
        files = (struct FileInfo *)malloc(n * sizeof(struct FileInfo));
        if (!files) {
            print_error(ST_ERR_MEMORY);
            return (int)ST_ERR_MEMORY;
        }

        char name_buf[1024];
        for (size_t i = 0; i < n; ++i) {
            unsigned long long fsize = 0;
            unsigned long long fimp = 0;

            if (scanf("%1023s %llu %llu", name_buf, &fsize, &fimp) != 3) {
                free_file_infos(files, i);
                print_error(ST_ERR_FILE_READ);
                return (int)ST_ERR_FILE_READ;
            }

            if (fsize == 0) {
                free_file_infos(files, i);
                print_error(ST_ERR_RANGE);
                return (int)ST_ERR_RANGE;
            }

            /* проверка уникальности имени файла */
            for (size_t j = 0; j < i; ++j) {
                const char *s1 = files[j].name;
                const char *s2 = name_buf;
                size_t p = 0;
                while (s1[p] != '\0' && s1[p] == s2[p]) ++p;
                if (s1[p] == '\0' && s2[p] == '\0') {
                    free_file_infos(files, i);
                    print_error(ST_ERR_ARGS);
                    return (int)ST_ERR_ARGS;
                }
            }

            files[i].name = my_strdup(name_buf);
            if (!files[i].name) {
                free_file_infos(files, i);
                print_error(ST_ERR_MEMORY);
                return (int)ST_ERR_MEMORY;
            }
            files[i].size = fsize;
            files[i].importance = fimp;
            files[i].original_index = i;
        }
    }

    struct BackupResult result;
    const enum Status st = select_backup_files(files, n, capacity, &result);
    free_file_infos(files, n);

    if (st != ST_OK) {
        print_error(st);
        return (int)st;
    }

    printf("TOTAL_SIZE %llu\n", result.total_size);
    printf("TOTAL_IMPORTANCE %llu\n", result.total_importance);
    printf("FILES %zu\n", result.count);

    for (size_t i = 0; i < result.count; ++i) {
        printf("%s\n", result.selected_files[i].name);
    }

    free_backup_result(&result);
    return (int)ST_OK;
}
