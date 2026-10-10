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
    ST_ERR_MEMORY,
    ST_ERR_FILE_OPEN,
    ST_ERR_FILE_READ,
    ST_ERR_FILE_WRITE
};

/* перевод символа в числовое значение цифры (прописные и строчные буквы отождествляются) */
static int char_to_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}

/* чтение следующей лексемы из файла с динамическим буфером */
enum Status read_next_token(FILE *in, char **buf, size_t *cap, int *has_token) {
    if (!in || !buf || !cap || !has_token) return ST_ERR_NULL_PTR;

    *has_token = 0;
    int c;

    while ((c = fgetc(in)) != EOF && isspace((unsigned char)c)) {
    }

    if (c == EOF) {
        return ST_OK;
    }

    size_t len = 0;
    if (*cap == 0) {
        *cap = 64;
        *buf = (char *)malloc(*cap);
        if (!*buf) return ST_ERR_MEMORY;
    }

    while (c != EOF && !isspace((unsigned char)c)) {
        if (len + 1 >= *cap) {
            size_t new_cap = *cap * 2;
            char *new_buf = (char *)realloc(*buf, new_cap);
            if (!new_buf) return ST_ERR_MEMORY;
            *buf = new_buf;
            *cap = new_cap;
        }

        (*buf)[len++] = (char)c;
        c = fgetc(in);
    }

    (*buf)[len] = '\0';
    *has_token = 1;
    return ST_OK;
}

/* обработка числа: определение минимального основания, удаление нулей и перевод в СС 10 */
enum Status process_number(const char *raw, char **cleaned_str, int *min_base, long long *val_base10) {
    if (!raw || !cleaned_str || !min_base || !val_base10) return ST_ERR_NULL_PTR;
    if (!*raw) return ST_ERR_NUM;

    int i = 0;
    int is_neg = 0;

    if (raw[i] == '+' || raw[i] == '-') {
        if (raw[i] == '-') is_neg = 1;
        ++i;
    }

    if (raw[i] == '\0') return ST_ERR_NUM;

    int max_digit = 0;
    int start_digits = i;

    /* поиск максимальной цифры и проверка корректности символов */
    for (; raw[i] != '\0'; ++i) {
        int d = char_to_digit(raw[i]);
        if (d < 0 || d > 35) return ST_ERR_NUM;
        if (d > max_digit) {
            max_digit = d;
        }
    }

    int base = max_digit + 1;
    if (base < 2) base = 2;
    *min_base = base;

    /* пропуск ведущих нулей */
    int d_idx = start_digits;
    while (raw[d_idx] == '0' && raw[d_idx + 1] != '\0') {
        ++d_idx;
    }

    /* если число равно нулю, знак не нужен */
    int is_zero = (raw[d_idx] == '0' && raw[d_idx + 1] == '\0');
    if (is_zero) is_neg = 0;

    size_t needed_len = strlen(&raw[d_idx]) + (is_neg ? 2 : 1);
    char *res_str = (char *)malloc(needed_len);
    if (!res_str) return ST_ERR_MEMORY;

    int pos = 0;
    if (is_neg) {
        res_str[pos++] = '-';
    }
    strcpy(&res_str[pos], &raw[d_idx]);
    *cleaned_str = res_str;

    /* перевод в 10 систему счисления с контролем переполнения */
    unsigned long long val = 0ULL;
    unsigned long long limit = is_neg ? (unsigned long long)LLONG_MAX + 1ULL : (unsigned long long)LLONG_MAX;
    unsigned int max_rem = (unsigned int)(limit % (unsigned long long)base);

    for (int k = d_idx; raw[k] != '\0'; ++k) {
        int d = char_to_digit(raw[k]);

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
            *val_base10 = LLONG_MIN;
        } else {
            *val_base10 = -(long long)val;
        }
    } else {
        *val_base10 = (long long)val;
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
    if (argc != 3) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    const char *in_path = argv[1];
    const char *out_path = argv[2];

    if (strcmp(in_path, out_path) == 0) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    FILE *fin = fopen(in_path, "r");
    if (!fin) {
        print_error(ST_ERR_FILE_OPEN);
        return ST_ERR_FILE_OPEN;
    }

    FILE *fout = fopen(out_path, "w");
    if (!fout) {
        fclose(fin);
        print_error(ST_ERR_FILE_OPEN);
        return ST_ERR_FILE_OPEN;
    }

    char *buf = NULL;
    size_t cap = 0;
    int has_tok = 0;

    enum Status st = read_next_token(fin, &buf, &cap, &has_tok);

    while (st == ST_OK && has_tok) {
        char *cleaned = NULL;
        int base = 0;
        long long val10 = 0;

        /* Некорректное число во входном файле является ошибкой входных данных, программа прерывает обработку и сообщает об ошибке */
        st = process_number(buf, &cleaned, &base, &val10);
        if (st == ST_OK) {
            if (fprintf(fout, "%s %d %lld\n", cleaned, base, val10) < 0) {
                st = ST_ERR_FILE_WRITE;
            }
            free(cleaned);
        } else {
            /* при ошибке разбора конкретного числа буфер buf освобождается ниже перед закрытием файлов */
            break;
        }

        st = read_next_token(fin, &buf, &cap, &has_tok);
    }

    /* проверка на наличие ошибок чтения потока */
    if (st == ST_OK && ferror(fin)) {
        st = ST_ERR_FILE_READ;
    }

    free(buf);
    fclose(fin);
    fclose(fout);

    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    printf("Numbers processed successfully. Results written to: %s\n", out_path);
    return ST_OK;
}
