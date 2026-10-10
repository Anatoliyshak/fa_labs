#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/* статусы ошибок */
enum Status {
    ST_OK = 0,
    ST_ERR_ARGS,
    ST_ERR_FLAG,
    ST_ERR_NUM,
    ST_ERR_RANGE,
    ST_ERR_OVERFLOW,
    ST_ERR_NULL_PTR,
    ST_ERR_MEMORY
};

/* перевод символа в цифру */
static int char_to_digit_uppercase_only(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

/* перевод цифры в прописной символ */
static char digit_to_char(int d) {
    if (d >= 0 && d <= 9) return (char)('0' + d);
    if (d >= 10 && d <= 35) return (char)('A' + d - 10);
    return '?';
}

/* перевод числа типа long long в строковое представление в системе счисления base */
enum Status convert_to_base(long long val, int base, char *buf, size_t cap) {
    if (!buf) return ST_ERR_NULL_PTR;
    if (base < 2 || base > 36) return ST_ERR_RANGE;
    if (cap < 2) return ST_ERR_RANGE;

    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return ST_OK;
    }

    int is_neg = (val < 0);
    unsigned long long uval = is_neg ? (0ULL - (unsigned long long)val) : (unsigned long long)val;

    char temp[128];
    int len = 0;

    while (uval > 0ULL) {
        int rem = (int)(uval % (unsigned long long)base);
        temp[len++] = digit_to_char(rem);
        uval /= (unsigned long long)base;
    }

    size_t total_needed = (size_t)len + (is_neg ? 1 : 0) + 1;
    if (cap < total_needed) return ST_ERR_RANGE;

    int pos = 0;
    if (is_neg) {
        buf[pos++] = '-';
    }

    for (int i = len - 1; i >= 0; --i) {
        buf[pos++] = temp[i];
    }
    buf[pos] = '\0';

    return ST_OK;
}

/* разбор числа в системе счисления base со строгим требованием прописных букв */
enum Status parse_base_number(const char *raw, int base, long long *val_out, char **cleaned_str) {
    if (!raw || !val_out || !cleaned_str) return ST_ERR_NULL_PTR;
    if (base < 2 || base > 36) return ST_ERR_RANGE;

    /* пропуск пробелов в начале */
    int i = 0;
    while (raw[i] && isspace((unsigned char)raw[i])) {
        ++i;
    }

    if (raw[i] == '\0') return ST_ERR_NUM;

    int is_neg = 0;
    if (raw[i] == '+' || raw[i] == '-') {
        if (raw[i] == '-') is_neg = 1;
        ++i;
    }

    if (raw[i] == '\0' || isspace((unsigned char)raw[i])) return ST_ERR_NUM;

    int start_digits = i;

    /* валидация цифр */
    for (int k = start_digits; raw[k] != '\0' && !isspace((unsigned char)raw[k]); ++k) {
        int d = char_to_digit_uppercase_only(raw[k]);
        if (d < 0 || d >= base) {
            return ST_ERR_NUM;
        }
    }

    /* пропуск ведущих нулей */
    int d_idx = start_digits;
    while (raw[d_idx] == '0' && raw[d_idx + 1] != '\0' && !isspace((unsigned char)raw[d_idx + 1])) {
        ++d_idx;
    }

    int is_zero = (raw[d_idx] == '0' && (raw[d_idx + 1] == '\0' || isspace((unsigned char)raw[d_idx + 1])));
    if (is_zero) is_neg = 0;

    int num_len = 0;
    while (raw[d_idx + num_len] != '\0' && !isspace((unsigned char)raw[d_idx + num_len])) {
        ++num_len;
    }

    char *res_str = (char *)malloc((size_t)num_len + (is_neg ? 2 : 1));
    if (!res_str) return ST_ERR_MEMORY;

    int pos = 0;
    if (is_neg) {
        res_str[pos++] = '-';
    }
    memcpy(&res_str[pos], &raw[d_idx], (size_t)num_len);
    res_str[pos + num_len] = '\0';
    *cleaned_str = res_str;

    /* вычисление числового значения */
    unsigned long long val = 0ULL;
    unsigned long long limit = is_neg ? (unsigned long long)LLONG_MAX + 1ULL : (unsigned long long)LLONG_MAX;
    unsigned int max_rem = (unsigned int)(limit % (unsigned long long)base);

    for (int k = 0; k < num_len; ++k) {
        int d = char_to_digit_uppercase_only(raw[d_idx + k]);

        if (val > limit / (unsigned long long)base) {
            free(res_str);
            *cleaned_str = NULL;
            return ST_ERR_OVERFLOW;
        }
        if (val == limit / (unsigned long long)base && (unsigned int)d > max_rem) {
            free(res_str);
            *cleaned_str = NULL;
            return ST_ERR_OVERFLOW;
        }

        val = val * (unsigned long long)base + (unsigned long long)d;
    }

    if (is_neg) {
        if (val > (unsigned long long)LLONG_MAX) {
            *val_out = LLONG_MIN;
        } else {
            *val_out = -(long long)val;
        }
    } else {
        *val_out = (long long)val;
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
    }
}

/* Функция контекста вывода данных, вызываемая из main */
/* вывод числа в системах счисления 9, 18, 27, 36 */
static void print_in_target_bases(long long val) {
    char buf[128];
    const int target_bases[4] = {9, 18, 27, 36};

    for (int i = 0; i < 4; ++i) {
        int tb = target_bases[i];
        if (convert_to_base(val, tb, buf, sizeof(buf)) == ST_OK) {
            printf("  Base %2d: %s\n", tb, buf);
        }
    }
}

int main(void) {
    char line[4096];

    printf("Enter number base (2..36): ");
    if (!fgets(line, sizeof(line), stdin)) {
        print_error(ST_ERR_NUM);
        return ST_ERR_NUM;
    }

    char *end_ptr = NULL;
    long base_l = strtol(line, &end_ptr, 10);
    while (end_ptr && *end_ptr && isspace((unsigned char)*end_ptr)) {
        ++end_ptr;
    }
    if (end_ptr == line || (*end_ptr != '\0' && *end_ptr != '\n' && *end_ptr != '\r')) {
        print_error(ST_ERR_NUM);
        return ST_ERR_NUM;
    }
    if (base_l < 2 || base_l > 36) {
        print_error(ST_ERR_RANGE);
        return ST_ERR_RANGE;
    }

    int base = (int)base_l;
    printf("Enter numbers in base %d (uppercase digits for > 9, Stop to finish):\n", base);

    int count = 0;
    long long sum = 0;
    long long max_abs_val = 0;
    unsigned long long max_abs_mag = 0;
    char *max_abs_str = NULL;

    while (1) {
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        size_t len = strlen(line);
        if (len == sizeof(line) - 1 && line[len - 1] != '\n' && !feof(stdin)) {
            print_error(ST_ERR_RANGE);
            if (max_abs_str) free(max_abs_str);
            return ST_ERR_RANGE;
        }

        /* удаление символов перевода строки */
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || isspace((unsigned char)line[len - 1]))) {
            line[--len] = '\0';
        }

        /* пропуск пустых строк */
        char *p = line;
        while (*p && isspace((unsigned char)*p)) ++p;
        if (*p == '\0') continue;

        if (strcmp(p, "Stop") == 0) {
            break;
        }

        long long cur_val = 0;
        char *cleaned = NULL;
        enum Status st = parse_base_number(p, base, &cur_val, &cleaned);

        if (st != ST_OK) {
            print_error(st);
            continue;
        }

        /* проверка переполнения при суммировании */
        if (cur_val > 0 && sum > LLONG_MAX - cur_val) {
            print_error(ST_ERR_OVERFLOW);
            free(cleaned);
            if (max_abs_str) free(max_abs_str);
            return ST_ERR_OVERFLOW;
        }
        if (cur_val < 0 && sum < LLONG_MIN - cur_val) {
            print_error(ST_ERR_OVERFLOW);
            free(cleaned);
            if (max_abs_str) free(max_abs_str);
            return ST_ERR_OVERFLOW;
        }
        sum += cur_val;

        /* модуль числа */
        unsigned long long cur_mag = (cur_val < 0) ? (0ULL - (unsigned long long)cur_val) : (unsigned long long)cur_val;

        if (count == 0 || cur_mag > max_abs_mag) {
            max_abs_mag = cur_mag;
            max_abs_val = cur_val;
            if (max_abs_str) free(max_abs_str);
            max_abs_str = cleaned;
        } else {
            free(cleaned);
        }

        ++count;
    }

    if (count == 0) {
        printf("No numbers were entered.\n");
        return ST_OK;
    }

    printf("\n=== Results ===\n");

    printf("1. Maximum by absolute value: %s (value = %lld)\n",
           base, max_abs_str, max_abs_val);
    printf("   Representations of max by absolute value:\n");
    print_in_target_bases(max_abs_val);

    char sum_buf[128];
    convert_to_base(sum, base, sum_buf, sizeof(sum_buf));
    printf("\n2. Sum of all numbers: %s (value = %lld)\n",
           base, sum_buf, sum);
    printf("   Representations of sum:\n");
    print_in_target_bases(sum);

    if (max_abs_str) {
        free(max_abs_str);
    }

    return ST_OK;
}
