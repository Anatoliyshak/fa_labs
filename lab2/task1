#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>

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

/* проверка, является ли символ цифрой */
static int my_isdigit(const char c) {
    return (c >= '0' && c <= '9');
}

/* проверка, является ли символ буквой латинского алфавита */
static int my_isalpha(const char c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

/* перевод строчной латинской буквы в прописную */
static char my_toupper(const char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

/* проверка пробельного символа */
static int my_isspace(const char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

/* вычисление длины строки без использования функций из string.h */
static size_t my_strlen(const char *s) {
    size_t len = 0;
    if (!s) return 0;
    while (s[len] != '\0') {
        ++len;
    }
    return len;
}

/* проверка и разбор флага */
enum Status parse_flag(const char *s, char *f) {
    if (!s || !f) return ST_ERR_FLAG;
    if ((s[0] != '-' && s[0] != '/') || !s[1] || s[2] != '\0') {
        return ST_ERR_FLAG;
    }

    if (s[1] == 'l' || s[1] == 'r' || s[1] == 'u' ||
        s[1] == 'n' || s[1] == 'c') {
        *f = s[1];
        return ST_OK;
    }

    return ST_ERR_FLAG;
}

/* разбор беззнакового 32-битного числа без использования errno с защитой от переполнения */
enum Status parse_uint(const char *s, unsigned int *res) {
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

        /* проверка на переполнение до выполнения умножения */
        if (val > limit / 10ULL) return ST_ERR_OVERFLOW;
        if (val == limit / 10ULL && d > max_digit) return ST_ERR_OVERFLOW;

        val = val * 10ULL + (unsigned long long)d;
    }

    *res = (unsigned int)val;
    return ST_OK;
}

/* флаг -l: подсчет длины строки */
enum Status do_l(const char *str, size_t *out_len) {
    if (!str || !out_len) return ST_ERR_NULL_PTR;

    *out_len = my_strlen(str);
    return ST_OK;
}

/* флаг -r: переворот строки с размещением результата в динамической памяти */
enum Status do_r(const char *str, char **out_str) {
    if (!str || !out_str) return ST_ERR_NULL_PTR;

    const size_t len = my_strlen(str);
    char *res = (char *)malloc(len + 1);
    if (!res) return ST_ERR_MEMORY;

    for (size_t i = 0; i < len; ++i) {
        res[i] = str[len - 1 - i];
    }
    res[len] = '\0';

    *out_str = res;
    return ST_OK;
}

/* флаг -u: перевод символов на нечетных позициях в верхний регистр */
enum Status do_u(const char *str, char **out_str) {
    if (!str || !out_str) return ST_ERR_NULL_PTR;

    const size_t len = my_strlen(str);
    char *res = (char *)malloc(len + 1);
    if (!res) return ST_ERR_MEMORY;

    for (size_t i = 0; i < len; ++i) {
        if (i % 2 != 0) {
            res[i] = my_toupper(str[i]);
        } else {
            res[i] = str[i];
        }
    }
    res[len] = '\0';

    *out_str = res;
    return ST_OK;
}

/* флаг -n: переупорядочивание символов (цифры, затем буквы, затем прочие) */
enum Status do_n(const char *str, char **out_str) {
    if (!str || !out_str) return ST_ERR_NULL_PTR;

    const size_t len = my_strlen(str);
    char *res = (char *)malloc(len + 1);
    if (!res) return ST_ERR_MEMORY;

    size_t pos = 0;

    /* 1. Цифры в исходном порядке следования */
    for (size_t i = 0; i < len; ++i) {
        if (my_isdigit(str[i])) {
            res[pos++] = str[i];
        }
    }

    /* 2. Буквы в исходном порядке следования */
    for (size_t i = 0; i < len; ++i) {
        if (my_isalpha(str[i])) {
            res[pos++] = str[i];
        }
    }

    /* 3. Все остальные символы в исходном порядке следования */
    for (size_t i = 0; i < len; ++i) {
        if (!my_isdigit(str[i]) && !my_isalpha(str[i])) {
            res[pos++] = str[i];
        }
    }

    res[pos] = '\0';
    *out_str = res;
    return ST_OK;
}

/* флаг -c: конкатенация строк в псевдослучайном порядке */
enum Status do_c(const char **strings, const size_t count, const unsigned int seed, char **out_str) {
    if (!strings || !out_str) return ST_ERR_NULL_PTR;

    for (size_t i = 0; i < count; ++i) {
        if (!strings[i]) return ST_ERR_NULL_PTR;
    }

    if (count == 0) {
        char *empty_str = (char *)malloc(1);
        if (!empty_str) return ST_ERR_MEMORY;
        empty_str[0] = '\0';
        *out_str = empty_str;
        return ST_OK;
    }

    /* подсчет суммарной длины строк с контролем переполнения */
    size_t total_len = 0;
    for (size_t i = 0; i < count; ++i) {
        const size_t cur_len = my_strlen(strings[i]);
        if (total_len + cur_len < total_len) {
            return ST_ERR_OVERFLOW;
        }
        total_len += cur_len;
    }

    /* создание и инициализация массива индексов */
    size_t *indices = (size_t *)malloc(count * sizeof(size_t));
    if (!indices) return ST_ERR_MEMORY;

    for (size_t i = 0; i < count; ++i) {
        indices[i] = i;
    }

    /* тасование Фишера-Йетса */
    srand(seed);
    for (size_t i = count - 1; i > 0; --i) {
        const size_t j = (size_t)rand() % (i + 1);
        const size_t tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
    }

    /* выделение памяти под результирующую строку */
    char *res = (char *)malloc(total_len + 1);
    if (!res) {
        free(indices);
        return ST_ERR_MEMORY;
    }

    /* копирование строк согласно псевдослучайному порядку индексов */
    size_t pos = 0;
    for (size_t i = 0; i < count; ++i) {
        const char *cur = strings[indices[i]];
        for (size_t k = 0; cur[k] != '\0'; ++k) {
            res[pos++] = cur[k];
        }
    }
    res[pos] = '\0';

    free(indices);
    *out_str = res;
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
    if (argc < 3) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    char flag = '\0';
    enum Status st = parse_flag(argv[1], &flag);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    if (flag == 'l') {
        if (argc != 3) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        size_t len = 0;
        st = do_l(argv[2], &len);
        if (st == ST_OK) {
            printf("String length: %zu\n", len);
        }
    } else if (flag == 'r') {
        if (argc != 3) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        char *res = NULL;
        st = do_r(argv[2], &res);
        if (st == ST_OK) {
            printf("Reversed string: %s\n", res);
            free(res);
        }
    } else if (flag == 'u') {
        if (argc != 3) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        char *res = NULL;
        st = do_u(argv[2], &res);
        if (st == ST_OK) {
            printf("Uppercase odd string: %s\n", res);
            free(res);
        }
    } else if (flag == 'n') {
        if (argc != 3) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        char *res = NULL;
        st = do_n(argv[2], &res);
        if (st == ST_OK) {
            printf("Reordered string: %s\n", res);
            free(res);
        }
    } else if (flag == 'c') {
        if (argc < 4) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        unsigned int seed = 0;
        st = parse_uint(argv[3], &seed);
        if (st != ST_OK) {
            print_error(st);
            return st;
        }

        /* вычисление количества строк: argv[2] + строки, начиная с argv[4] */
        const size_t count = 1 + (size_t)(argc > 4 ? (argc - 4) : 0);
        const char **strings = (const char **)malloc(count * sizeof(const char *));
        if (!strings) {
            print_error(ST_ERR_MEMORY);
            return ST_ERR_MEMORY;
        }

        strings[0] = argv[2];
        for (int i = 4; i < argc; ++i) {
            strings[1 + (size_t)(i - 4)] = argv[i];
        }

        char *res = NULL;
        st = do_c(strings, count, seed, &res);
        free(strings);

        if (st == ST_OK) {
            printf("Concatenated string: %s\n", res);
            free(res);
        }
    } else {
        st = ST_ERR_FLAG;
    }

    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    return ST_OK;
}

