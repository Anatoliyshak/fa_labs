#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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

/* структура позиции вхождения подстроки в файле */
struct Match {
    size_t line;    /* номер строки (индексируется с 1) */
    size_t col;     /* номер символа в строке (индексируется с 1) */
};

/* результат поиска по одному файлу */
struct FileSearchResult {
    char *filepath;         /* путь к файлу */
    struct Match *matches;  /* динамический массив найденных позиций */
    size_t match_count;     /* количество вхождений */
    enum Status status;     /* статус обработки файла */
};

/* вычисление длины строки без использования string.h */
static size_t my_strlen(const char *s) {
    size_t len = 0;
    if (!s) return 0;
    while (s[len] != '\0') {
        ++len;
    }
    return len;
}

/* безопасное дублирование строки в куче */
static char *my_strdup(const char *s) {
    if (!s) return NULL;
    const size_t len = my_strlen(s);
    char *res = (char *)malloc(len + 1);
    if (!res) return NULL;
    for (size_t i = 0; i < len; ++i) {
        res[i] = s[i];
    }
    res[len] = '\0';
    return res;
}

/* освобождение массива результатов поиска */
void free_search_results(struct FileSearchResult *results, const size_t count) {
    if (!results) return;
    for (size_t i = 0; i < count; ++i) {
        if (results[i].filepath) {
            free(results[i].filepath);
            results[i].filepath = NULL;
        }
        if (results[i].matches) {
            free(results[i].matches);
            results[i].matches = NULL;
        }
    }
    free(results);
}

/* чистая функция: поиск всех вхождений подстроки в буфере памяти */
/* категорически запрещено использование strstr и strncmp из string.h */
enum Status find_matches_in_buffer(const char *content, const size_t content_len,
                                   const char *substr, const size_t sub_len,
                                   struct Match **out_matches, size_t *out_count) {
    if (!content || !substr || !out_matches || !out_count) {
        return ST_ERR_NULL_PTR;
    }

    *out_matches = NULL;
    *out_count = 0;

    if (sub_len == 0) return ST_ERR_ARGS;
    if (content_len < sub_len) return ST_OK;

    size_t cap = 16;
    struct Match *matches = (struct Match *)malloc(cap * sizeof(struct Match));
    if (!matches) return ST_ERR_MEMORY;

    size_t count = 0;
    size_t cur_line = 1;
    size_t cur_col = 1;
    const size_t max_search_idx = content_len - sub_len;

    for (size_t i = 0; i <= max_search_idx; ++i) {
        int is_match = 1;
        /* посимвольное сопоставление подстроки произвольной длины */
        for (size_t k = 0; k < sub_len; ++k) {
            if (content[i + k] != substr[k]) {
                is_match = 0;
                break;
            }
        }

        if (is_match) {
            if (count >= cap) {
                const size_t new_cap = cap * 2;
                struct Match *new_matches = (struct Match *)realloc(matches, new_cap * sizeof(struct Match));
                if (!new_matches) {
                    free(matches);
                    return ST_ERR_MEMORY;
                }
                matches = new_matches;
                cap = new_cap;
            }

            matches[count].line = cur_line;
            matches[count].col = cur_col;
            ++count;
        }

        /* инкрементальный пересчет строки и колонки без выделения доп. памяти */
        if (content[i] == '\n') {
            ++cur_line;
            cur_col = 1;
        } else if (content[i] == '\r') {
            if (i + 1 < content_len && content[i + 1] == '\n') {
                /* в последовательности CRLF перевод строки выполнит символ \n */
            } else {
                ++cur_line;
                cur_col = 1;
            }
        } else {
            ++cur_col;
        }
    }

    if (count == 0) {
        free(matches);
        *out_matches = NULL;
        *out_count = 0;
        return ST_OK;
    }

    *out_matches = matches;
    *out_count = count;
    return ST_OK;
}

/* чтение содержимого файла в оперативную память */
static enum Status read_file_to_buffer(const char *filepath, char **out_buf, size_t *out_len) {
    if (!filepath || !out_buf || !out_len) return ST_ERR_NULL_PTR;

    *out_buf = NULL;
    *out_len = 0;

    FILE *f = fopen(filepath, "rb");
    if (!f) return ST_ERR_FILE_OPEN;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return ST_ERR_FILE_READ;
    }

    const long file_sz = ftell(f);
    if (file_sz < 0) {
        fclose(f);
        return ST_ERR_FILE_READ;
    }

    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return ST_ERR_FILE_READ;
    }

    char *buf = (char *)malloc((size_t)file_sz + 1);
    if (!buf) {
        fclose(f);
        return ST_ERR_MEMORY;
    }

    const size_t n_read = fread(buf, 1, (size_t)file_sz, f);
    if (n_read != (size_t)file_sz && ferror(f)) {
        free(buf);
        fclose(f);
        return ST_ERR_FILE_READ;
    }

    buf[n_read] = '\0';
    fclose(f);

    *out_buf = buf;
    *out_len = n_read;
    return ST_OK;
}

/* вариадический обработчик списка файлов */
enum Status vfind_substring_in_files(const char *substr,
                                     struct FileSearchResult **out_results,
                                     const size_t file_count, va_list ap) {
    if (!substr || !out_results) return ST_ERR_NULL_PTR;

    *out_results = NULL;
    if (file_count == 0) return ST_OK;

    const size_t sub_len = my_strlen(substr);
    if (sub_len == 0) return ST_ERR_ARGS;

    struct FileSearchResult *res_arr = (struct FileSearchResult *)malloc(file_count * sizeof(struct FileSearchResult));
    if (!res_arr) return ST_ERR_MEMORY;

    for (size_t i = 0; i < file_count; ++i) {
        res_arr[i].filepath = NULL;
        res_arr[i].matches = NULL;
        res_arr[i].match_count = 0;
        res_arr[i].status = ST_OK;
    }

    for (size_t i = 0; i < file_count; ++i) {
        const char *filepath = va_arg(ap, const char *);
        if (!filepath) {
            res_arr[i].status = ST_ERR_NULL_PTR;
            continue;
        }

        res_arr[i].filepath = my_strdup(filepath);
        if (!res_arr[i].filepath) {
            res_arr[i].status = ST_ERR_MEMORY;
            continue;
        }

        char *content = NULL;
        size_t content_len = 0;
        enum Status st = read_file_to_buffer(filepath, &content, &content_len);
        if (st != ST_OK) {
            res_arr[i].status = st;
            continue;
        }

        st = find_matches_in_buffer(content, content_len, substr, sub_len,
                                    &res_arr[i].matches, &res_arr[i].match_count);
        free(content);
        res_arr[i].status = st;
    }

    *out_results = res_arr;
    return ST_OK;
}

/* целевая функция с переменным числом аргументов по заданию */
enum Status find_substring_in_files(const char *substr,
                                    struct FileSearchResult **out_results,
                                    const size_t file_count, ...) {
    if (!substr || !out_results) return ST_ERR_NULL_PTR;
    if (file_count == 0) return ST_OK;

    va_list ap;
    va_start(ap, file_count);
    const enum Status st = vfind_substring_in_files(substr, out_results, file_count, ap);
    va_end(ap);

    return st;
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

/* печать результатов в контексте main */
static void print_search_results(const struct FileSearchResult *results, const size_t count) {
    for (size_t i = 0; i < count; ++i) {
        printf("File: %s\n", results[i].filepath ? results[i].filepath : "(null)");
        if (results[i].status != ST_OK) {
            printf("  Status: ");
            print_error(results[i].status);
            continue;
        }

        if (results[i].match_count == 0) {
            printf("  No occurrences found.\n");
        } else {
            printf("  Total occurrences: %zu\n", results[i].match_count);
            for (size_t k = 0; k < results[i].match_count; ++k) {
                printf("    Occurrence #%zu: Line %zu, Character %zu\n",
                       k + 1, results[i].matches[k].line, results[i].matches[k].col);
            }
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    printf("=== Demonstration of Substring Search in Files ===\n\n");

    /* 1. Если аргументы переданы через командную строку: <substr> <f1> [f2] ... */
    if (argc >= 3) {
        const char *substr = argv[1];
        const size_t file_count = (size_t)(argc - 2);

        struct FileSearchResult *results = NULL;
        enum Status st = ST_OK;

        if (file_count == 1) {
            st = find_substring_in_files(substr, &results, 1, argv[2]);
        } else if (file_count == 2) {
            st = find_substring_in_files(substr, &results, 2, argv[2], argv[3]);
        } else if (file_count == 3) {
            st = find_substring_in_files(substr, &results, 3, argv[2], argv[3], argv[4]);
        } else if (file_count == 4) {
            st = find_substring_in_files(substr, &results, 4, argv[2], argv[3], argv[4], argv[5]);
        } else if (file_count == 5) {
            st = find_substring_in_files(substr, &results, 5, argv[2], argv[3], argv[4], argv[5], argv[6]);
        } else if (file_count == 6) {
            st = find_substring_in_files(substr, &results, 6, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);
        } else if (file_count == 7) {
            st = find_substring_in_files(substr, &results, 7, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
        } else if (file_count == 8) {
            st = find_substring_in_files(substr, &results, 8, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]);
        /*
         * В C нет стандартного способа собрать va_list из массива argv во время
         * выполнения. Поэтому для >8 файлов используется последовательный вызов
         * find_substring_in_files с одним файлом; результаты объединяются в all_res.
         * Для 1..8 файлов — прямые вариадические вызовы (см. выше).
         */
        } else {
            struct FileSearchResult *all_res = (struct FileSearchResult *)malloc(file_count * sizeof(struct FileSearchResult));
            if (!all_res) {
                print_error(ST_ERR_MEMORY);
                return (int)ST_ERR_MEMORY;
            }
            for (size_t i = 0; i < file_count; ++i) {
                struct FileSearchResult *single = NULL;
                st = find_substring_in_files(substr, &single, 1, argv[2 + i]);
                if (st == ST_OK && single) {
                    all_res[i] = single[0];
                    free(single);
                } else {
                    all_res[i].filepath = my_strdup(argv[2 + i]);
                    all_res[i].matches = NULL;
                    all_res[i].match_count = 0;
                    all_res[i].status = st;
                }
            }
            results = all_res;
            st = ST_OK;
        }

        if (st != ST_OK) {
            print_error(st);
            return (int)st;
        }

        print_search_results(results, file_count);
        free_search_results(results, file_count);
        return (int)ST_OK;
    }

    /* 2. Автономная демонстрация: создание тестовых файлов с многострочным текстом */
    const char *test_f1 = "test_sub1.txt";
    const char *test_f2 = "test_sub2.txt";

    FILE *f1 = fopen(test_f1, "wb");
    if (f1) {
        fprintf(f1, "Hello world\nThis is a test\nHello again\nMultiline\nhello match");
        fclose(f1);
    }

    FILE *f2 = fopen(test_f2, "wb");
    if (f2) {
        fprintf(f2, "Another file without that word\nJust a test\nEnd of file");
        fclose(f2);
    }

    const char *target = "Hello";
    printf("Searching for \"%s\" in demo files:\n\n", target);

    struct FileSearchResult *demo_results = NULL;
    enum Status st = find_substring_in_files(target, &demo_results, 2, test_f1, test_f2);
    if (st == ST_OK) {
        print_search_results(demo_results, 2);
        free_search_results(demo_results, 2);
    } else {
        print_error(st);
    }

    remove(test_f1);
    remove(test_f2);

    return (int)ST_OK;
}
