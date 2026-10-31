#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>

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

/* структура, описывающая сотрудника */
struct Employee {
    unsigned int id;
    char *first_name;
    char *last_name;
    double salary;
};

/* проверка пробельного символа */
static int my_isspace(const char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

/* проверка, является ли символ цифрой */
static int my_isdigit(const char c) {
    return (c >= '0' && c <= '9');
}

/* проверка принадлежности символа к латинскому алфавиту */
static int is_latin_alpha(const char c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

/* валидация строки: непустая и содержит только латинские буквы */
static int is_valid_name(const char *s, const size_t len) {
    if (!s || len == 0) return 0;
    for (size_t i = 0; i < len; ++i) {
        if (!is_latin_alpha(s[i])) {
            return 0;
        }
    }
    return 1;
}

/* безопасное дублирование строки в динамической памяти */
static char *my_strdup(const char *src, const size_t len) {
    if (!src) return NULL;
    char *dst = (char *)malloc(len + 1);
    if (!dst) return NULL;
    for (size_t i = 0; i < len; ++i) {
        dst[i] = src[i];
    }
    dst[len] = '\0';
    return dst;
}

/* разбор беззнакового 32-битного целого числа без использования errno */
static enum Status parse_uint(const char *s, unsigned int *res) {
    if (!s || !res || !*s || my_isspace(*s)) return ST_ERR_NUM;

    int i = 0;
    if (s[i] == '+') {
        ++i;
    }

    if (s[i] == '\0') return ST_ERR_NUM;

    unsigned long long val = 0ULL;
    const unsigned long long limit = (unsigned long long)UINT_MAX;
    const unsigned int max_digit = (unsigned int)(limit % 10ULL);

    for (; s[i] != '\0'; ++i) {
        if (!my_isdigit(s[i])) return ST_ERR_NUM;
        const unsigned int d = (unsigned int)(s[i] - '0');

        if (val > limit / 10ULL) return ST_ERR_OVERFLOW;
        if (val == limit / 10ULL && d > max_digit) return ST_ERR_OVERFLOW;

        val = val * 10ULL + (unsigned long long)d;
    }

    *res = (unsigned int)val;
    return ST_OK;
}

/* разбор вещественного числа с проверкой на конечность */
static enum Status parse_double(const char *s, double *res) {
    if (!s || !res || !*s || my_isspace(*s)) return ST_ERR_NUM;

    char *end = NULL;
    const double val = strtod(s, &end);

    if (end == s || *end != '\0') return ST_ERR_NUM;
    if (!isfinite(val)) return ST_ERR_OVERFLOW;

    *res = val;
    return ST_OK;
}

/* проверка и разбор флага сортировки */
enum Status parse_flag(const char *s, char *f) {
    if (!s || !f) return ST_ERR_FLAG;
    if ((s[0] != '-' && s[0] != '/') || !s[1] || s[2] != '\0') {
        return ST_ERR_FLAG;
    }

    if (s[1] == 'a' || s[1] == 'd') {
        *f = s[1];
        return ST_OK;
    }

    return ST_ERR_FLAG;
}

/* освобождение массива структур сотрудников */
void free_employees(struct Employee *arr, const size_t count) {
    if (!arr) return;
    for (size_t i = 0; i < count; ++i) {
        if (arr[i].first_name) {
            free(arr[i].first_name);
            arr[i].first_name = NULL;
        }
        if (arr[i].last_name) {
            free(arr[i].last_name);
            arr[i].last_name = NULL;
        }
    }
    free(arr);
}

/*
 * Аксессор для передачи eps в компаратор qsort.
 * qsort в C99 имеет фиксированную сигнатуру int (*)(const void *, const void *)
 * и не принимает пользовательский контекст. qsort_r — не C99.
 * Своя сортировка запрещена заданием. Поэтому eps хранится в static-переменной,
 * изолированной внутри этой функции; сам eps проброшен параметром через
 * sort_employees(..., eps) и parse_employee_line(..., eps).
 */
static double get_set_cmp_eps(const double new_eps, const int set_flag) {
    static double s_eps = 1e-9;
    if (set_flag) {
        s_eps = new_eps;
    }
    return s_eps;
}

/* компаратор по возрастанию (-a / /a) */
static int cmp_employee_asc(const void *a, const void *b) {
    const struct Employee *e1 = (const struct Employee *)a;
    const struct Employee *e2 = (const struct Employee *)b;
    const double eps = get_set_cmp_eps(0.0, 0);

    /* 1. Первично по заработной плате с учетом параметра eps */
    const double diff = e1->salary - e2->salary;
    if (fabs(diff) >= eps) {
        if (diff > 0.0) return 1;
        return -1;
    }

    /* 2. Далее (если зарплаты равны) - по фамилии */
    const int cmp_last = strcmp(e1->last_name, e2->last_name);
    if (cmp_last != 0) {
        return cmp_last;
    }

    /* 3. Далее (если фамилии равны) - по имени */
    const int cmp_first = strcmp(e1->first_name, e2->first_name);
    if (cmp_first != 0) {
        return cmp_first;
    }

    /* 4. Наконец (если имена равны) - по id */
    if (e1->id > e2->id) return 1;
    if (e1->id < e2->id) return -1;
    return 0;
}

/* компаратор по убыванию (-d / /d) */
static int cmp_employee_desc(const void *a, const void *b) {
    return cmp_employee_asc(b, a);
}

/* чистая функция: сортировка коллекции сотрудников в памяти */
enum Status sort_employees(struct Employee *arr, const size_t count,
                           const char flag, const double eps) {
    if (!arr && count > 0) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    if (count <= 1) {
        return ST_OK;
    }

    get_set_cmp_eps(eps, 1);

    if (flag == 'a') {
        qsort(arr, count, sizeof(struct Employee), cmp_employee_asc);
    } else if (flag == 'd') {
        qsort(arr, count, sizeof(struct Employee), cmp_employee_desc);
    } else {
        return ST_ERR_FLAG;
    }

    return ST_OK;
}

/* извлечение следующей подстроки-лексемы, разделенной пробелами */
static const char *get_next_token(const char *p, const char **token_start, size_t *token_len) {
    while (*p != '\0' && my_isspace(*p)) {
        ++p;
    }

    if (*p == '\0') {
        *token_start = NULL;
        *token_len = 0;
        return p;
    }

    *token_start = p;
    size_t len = 0;
    while (*p != '\0' && !my_isspace(*p)) {
        ++len;
        ++p;
    }
    *token_len = len;
    return p;
}

/* чистая функция: разбор одной строки с данными сотрудника в памяти */
enum Status parse_employee_line(const char *line, const double eps,
                                struct Employee *emp, int *is_empty) {
    if (!line || !emp || !is_empty) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    *is_empty = 0;
    const char *p = line;

    const char *tok1 = NULL; size_t len1 = 0;
    p = get_next_token(p, &tok1, &len1);
    if (!tok1) {
        *is_empty = 1;
        return ST_OK;
    }

    const char *tok2 = NULL; size_t len2 = 0;
    p = get_next_token(p, &tok2, &len2);
    if (!tok2) return ST_ERR_FILE_READ;

    const char *tok3 = NULL; size_t len3 = 0;
    p = get_next_token(p, &tok3, &len3);
    if (!tok3) return ST_ERR_FILE_READ;

    const char *tok4 = NULL; size_t len4 = 0;
    p = get_next_token(p, &tok4, &len4);
    if (!tok4) return ST_ERR_FILE_READ;

    const char *tok5 = NULL; size_t len5 = 0;
    p = get_next_token(p, &tok5, &len5);
    if (tok5) return ST_ERR_FILE_READ;

    char buf_id[64];
    if (len1 >= sizeof(buf_id)) return ST_ERR_OVERFLOW;
    for (size_t i = 0; i < len1; ++i) buf_id[i] = tok1[i];
    buf_id[len1] = '\0';

    unsigned int id_val = 0;
    enum Status st = parse_uint(buf_id, &id_val);
    if (st != ST_OK) return st;

    if (!is_valid_name(tok2, len2)) return ST_ERR_FILE_READ;
    if (!is_valid_name(tok3, len3)) return ST_ERR_FILE_READ;

    char buf_salary[128];
    if (len4 >= sizeof(buf_salary)) return ST_ERR_OVERFLOW;
    for (size_t i = 0; i < len4; ++i) buf_salary[i] = tok4[i];
    buf_salary[len4] = '\0';

    double salary_val = 0.0;
    st = parse_double(buf_salary, &salary_val);
    if (st != ST_OK) return st;

    /* проверка неотрицательности зарплаты строго через переданный параметр eps */
    if (salary_val < -eps) return ST_ERR_RANGE;
    if (fabs(salary_val) < eps) salary_val = 0.0;

    char *fn = my_strdup(tok2, len2);
    if (!fn) return ST_ERR_MEMORY;

    char *ln = my_strdup(tok3, len3);
    if (!ln) {
        free(fn);
        return ST_ERR_MEMORY;
    }

    emp->id = id_val;
    emp->first_name = fn;
    emp->last_name = ln;
    emp->salary = salary_val;

    return ST_OK;
}

/* чтение одной строки произвольной длины из входного потока */
static enum Status read_line(FILE *in, char **line, size_t *cap, int *has_line) {
    if (!in || !line || !cap || !has_line) return ST_ERR_NULL_PTR;

    *has_line = 0;
    int c = fgetc(in);
    if (c == EOF) {
        return ST_OK;
    }

    if (*cap == 0) {
        *cap = 128;
        *line = (char *)malloc(*cap);
        if (!*line) return ST_ERR_MEMORY;
    }

    size_t len = 0;
    while (c != EOF && c != '\n') {
        if (c != '\r') {
            if (len + 1 >= *cap) {
                const size_t new_cap = *cap * 2;
                char *new_buf = (char *)realloc(*line, new_cap);
                if (!new_buf) return ST_ERR_MEMORY;
                *line = new_buf;
                *cap = new_cap;
            }
            (*line)[len++] = (char)c;
        }
        c = fgetc(in);
    }

    (*line)[len] = '\0';
    *has_line = 1;
    return ST_OK;
}

/* чтение всех записей сотрудников из файла */
enum Status read_employees_from_file(FILE *fin, const double eps,
                                     struct Employee **out_arr, size_t *out_count) {
    if (!fin || !out_arr || !out_count) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    *out_arr = NULL;
    *out_count = 0;

    size_t capacity = 16;
    struct Employee *arr = (struct Employee *)malloc(capacity * sizeof(struct Employee));
    if (!arr) return ST_ERR_MEMORY;

    char *line = NULL;
    size_t line_cap = 0;
    int has_line = 0;
    size_t count = 0;
    enum Status st = ST_OK;

    while ((st = read_line(fin, &line, &line_cap, &has_line)) == ST_OK && has_line) {
        int is_empty = 0;
        struct Employee emp;
        emp.first_name = NULL;
        emp.last_name = NULL;

        st = parse_employee_line(line, eps, &emp, &is_empty);
        if (st != ST_OK) {
            break;
        }

        if (is_empty) {
            continue;
        }

        if (count >= capacity) {
            const size_t new_cap = capacity * 2;
            struct Employee *new_arr = (struct Employee *)realloc(arr, new_cap * sizeof(struct Employee));
            if (!new_arr) {
                free(emp.first_name);
                free(emp.last_name);
                st = ST_ERR_MEMORY;
                break;
            }
            arr = new_arr;
            capacity = new_cap;
        }

        arr[count++] = emp;
    }

    if (line) {
        free(line);
    }

    if (st != ST_OK) {
        free_employees(arr, count);
        return st;
    }

    *out_arr = arr;
    *out_count = count;
    return ST_OK;
}

/* запись отсортированной коллекции в файл */
enum Status write_employees_to_file(FILE *fout, const struct Employee *arr, const size_t count) {
    if (!fout || (!arr && count > 0)) return ST_ERR_NULL_PTR;

    for (size_t i = 0; i < count; ++i) {
        if (fprintf(fout, "%u %s %s %.2f\n", arr[i].id, arr[i].first_name, arr[i].last_name, arr[i].salary) < 0) {
            return ST_ERR_FILE_WRITE;
        }
    }

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

int main(int argc, char **argv) {
    if (argc != 4) {
        print_error(ST_ERR_ARGS);
        return (int)ST_ERR_ARGS;
    }

    const double eps = 1e-9;
    const char *in_path = NULL;
    const char *out_path = NULL;
    char flag = '\0';

    if (parse_flag(argv[2], &flag) == ST_OK) {
        in_path = argv[1];
        out_path = argv[3];
    } else if (parse_flag(argv[1], &flag) == ST_OK) {
        in_path = argv[2];
        out_path = argv[3];
    } else {
        print_error(ST_ERR_FLAG);
        return (int)ST_ERR_FLAG;
    }

    if (strcmp(in_path, out_path) == 0) {
        print_error(ST_ERR_ARGS);
        return (int)ST_ERR_ARGS;
    }

    FILE *fin = fopen(in_path, "r");
    if (!fin) {
        print_error(ST_ERR_FILE_OPEN);
        return (int)ST_ERR_FILE_OPEN;
    }

    FILE *fout = fopen(out_path, "w");
    if (!fout) {
        fclose(fin);
        print_error(ST_ERR_FILE_OPEN);
        return (int)ST_ERR_FILE_OPEN;
    }

    struct Employee *employees = NULL;
    size_t count = 0;
    enum Status st = read_employees_from_file(fin, eps, &employees, &count);
    fclose(fin);

    if (st != ST_OK) {
        fclose(fout);
        print_error(st);
        return (int)st;
    }

    st = sort_employees(employees, count, flag, eps);
    if (st != ST_OK) {
        fclose(fout);
        free_employees(employees, count);
        print_error(st);
        return (int)st;
    }

    st = write_employees_to_file(fout, employees, count);
    fclose(fout);
    free_employees(employees, count);

    if (st != ST_OK) {
        print_error(st);
        return (int)st;
    }

    printf("Operation completed successfully. Processed %zu records.\n", count);
    return (int)ST_OK;
}
