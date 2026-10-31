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

/* вычисление длины строки без string.h */
static size_t my_strlen(const char *s) {
    size_t len = 0;
    if (!s) return 0;
    while (s[len] != '\0') ++len;
    return len;
}

/* значение цифры в системе счисления base [2..36] */
static int char_to_digit(const char c, const int base) {
    int d = -1;
    if (c >= '0' && c <= '9') {
        d = c - '0';
    } else if (c >= 'a' && c <= 'z') {
        d = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'Z') {
        d = c - 'A' + 10;
    }

    if (d < 0 || d >= base) return -1;
    return d;
}

/* преобразование цифры в символ */
static char digit_to_char(const int d) {
    if (d >= 0 && d <= 9) return (char)('0' + d);
    if (d >= 10 && d <= 35) return (char)('A' + (d - 10));
    return '?';
}

/* удаление ведущих нулей из строки */
static char *strip_leading_zeros(const char *s) {
    if (!s) return NULL;
    size_t i = 0;
    while (s[i] == '0' && s[i + 1] != '\0') {
        ++i;
    }

    const size_t len = my_strlen(s + i);
    char *res = (char *)malloc(len + 1);
    if (!res) return NULL;

    for (size_t k = 0; k < len; ++k) {
        res[k] = s[i + k];
    }
    res[len] = '\0';
    return res;
}

/* функция сложения в столбик двух длинных чисел в системе счисления base [2..36] */
enum Status add_in_base(const char *a, const char *b, const int base, char **out_res) {
    if (!a || !b || !out_res) return ST_ERR_NULL_PTR;
    if (base < 2 || base > 36) return ST_ERR_RANGE;

    *out_res = NULL;

    const size_t len_a = my_strlen(a);
    const size_t len_b = my_strlen(b);

    if (len_a == 0 || len_b == 0) return ST_ERR_NUM;

    const size_t max_len = (len_a > len_b ? len_a : len_b) + 2;
    char *raw_sum = (char *)malloc(max_len);
    if (!raw_sum) return ST_ERR_MEMORY;

    int carry = 0;
    size_t i = 0;

    int idx_a = (int)len_a - 1;
    int idx_b = (int)len_b - 1;

    while (idx_a >= 0 || idx_b >= 0 || carry > 0) {
        int d_a = 0;
        int d_b = 0;

        if (idx_a >= 0) {
            d_a = char_to_digit(a[idx_a], base);
            if (d_a < 0) {
                free(raw_sum);
                return ST_ERR_NUM;
            }
            --idx_a;
        }

        if (idx_b >= 0) {
            d_b = char_to_digit(b[idx_b], base);
            if (d_b < 0) {
                free(raw_sum);
                return ST_ERR_NUM;
            }
            --idx_b;
        }

        const int s = d_a + d_b + carry;
        raw_sum[i++] = digit_to_char(s % base);
        carry = s / base;
    }

    raw_sum[i] = '\0';

    /* разворот строки */
    char *reversed = (char *)malloc(i + 1);
    if (!reversed) {
        free(raw_sum);
        return ST_ERR_MEMORY;
    }

    for (size_t k = 0; k < i; ++k) {
        reversed[k] = raw_sum[i - 1 - k];
    }
    reversed[i] = '\0';
    free(raw_sum);

    /* удаление ведущих нулей */
    char *final_res = strip_leading_zeros(reversed);
    free(reversed);

    if (!final_res) return ST_ERR_MEMORY;

    *out_res = final_res;
    return ST_OK;
}

/* функция с переменным числом аргументов: сложение нескольких чисел в системе base */
enum Status sum_in_base(char **out_res, const int base, const size_t count, ...) {
    if (!out_res) return ST_ERR_NULL_PTR;
    if (base < 2 || base > 36) return ST_ERR_RANGE;

    *out_res = NULL;

    if (count == 0) {
        char *zero_str = (char *)malloc(2);
        if (!zero_str) return ST_ERR_MEMORY;
        zero_str[0] = '0';
        zero_str[1] = '\0';
        *out_res = zero_str;
        return ST_OK;
    }

    va_list ap;
    va_start(ap, count);

    char *total = NULL;

    for (size_t i = 0; i < count; ++i) {
        const char *next_num = va_arg(ap, const char *);
        if (!next_num) {
            va_end(ap);
            if (total) free(total);
            return ST_ERR_NULL_PTR;
        }

        if (next_num[0] == '\0') {
            va_end(ap);
            if (total) free(total);
            return ST_ERR_NUM;
        }

        if (i == 0) {
            /* валидация первого числа и удаление нулей */
            for (size_t k = 0; next_num[k] != '\0'; ++k) {
                if (char_to_digit(next_num[k], base) < 0) {
                    va_end(ap);
                    return ST_ERR_NUM;
                }
            }
            total = strip_leading_zeros(next_num);
            if (!total) {
                va_end(ap);
                return ST_ERR_MEMORY;
            }
        } else {
            char *new_total = NULL;
            const enum Status st = add_in_base(total, next_num, base, &new_total);
            free(total);
            if (st != ST_OK) {
                va_end(ap);
                return st;
            }
            total = new_total;
        }
    }

    va_end(ap);
    *out_res = total;
    return ST_OK;
}

int main(void) {
    printf("=== Demonstration of Column Addition in Base [2..36] ===\n\n");

    /* Тест 1: Двоичная система (base 2) */
    char *res_bin = NULL;
    enum Status st = sum_in_base(&res_bin, 2, 4, "101", "110", "1", "000");
    if (st == ST_OK) {
        printf("Base 2 sum: 101 + 110 + 1 + 000 = %s (expected 1100, which is 12)\n", res_bin);
        free(res_bin);
    }

    /* Тест 2: Шестнадцатеричная система (base 16) */
    char *res_hex = NULL;
    st = sum_in_base(&res_hex, 16, 3, "1A", "2F", "FF");
    if (st == ST_OK) {
        /* 26 + 47 + 255 = 328 = 0x148 */
        printf("Base 16 sum: 1A + 2F + FF = %s (expected 148)\n", res_hex);
        free(res_hex);
    }

    /* Тест 3: Система с основанием 36 с очень длинными числами */
    char *res_base36 = NULL;
    st = sum_in_base(&res_base36, 36, 3,
                     "ZZZZZZZZZZZZZZZZZZZZ",
                     "1",
                     "0000A");
    if (st == ST_OK) {
        printf("Base 36 sum: ZZZ...ZZZ + 1 + 0000A = %s\n", res_base36);
        free(res_base36);
    }

    /* Тест 4: Сложение двух чисел напрямую через add_in_base */
    char *res_add = NULL;
    st = add_in_base("999999999999999999999999999999", "1", 10, &res_add);
    if (st == ST_OK) {
        printf("Base 10 big sum: 999...999 + 1 = %s\n", res_add);
        free(res_add);
    }

    return (int)ST_OK;
}

