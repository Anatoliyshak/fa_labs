#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* проверка символа на латинскую букву без локалезависимости */
static int is_latin_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/* перевод прописной латинской буквы в строчную без локалезависимости */
static char to_lower_latin(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c - 'A' + 'a');
    }
    return c;
}

/* перевод байта в 4-ичную систему с фиксированной шириной 4 цифры */
static void char_to_base4_fixed(unsigned char val, char out[5]) {
    out[0] = (char)('0' + ((val >> 6) & 3));
    out[1] = (char)('0' + ((val >> 4) & 3));
    out[2] = (char)('0' + ((val >> 2) & 3));
    out[3] = (char)('0' + (val & 3));
    out[4] = '\0';
}

/* проверка флага */
enum Status parse_flag(const char *s, char *f) {
    if (!s || !f) return ST_ERR_NULL_PTR;
    if ((s[0] != '-' && s[0] != '/') || !s[1] || s[2] != '\0') return ST_ERR_FLAG;

    if (s[1] == 'r' || s[1] == 'a') {
        *f = s[1];
        return ST_OK;
    }
    return ST_ERR_FLAG;
}

/* динамическое чтение следующей лексемы из файла (контекст ввода-вывода) */
enum Status read_next_lexeme(FILE *in, char **buf, size_t *cap, int *has_lexeme) {
    if (!in || !buf || !cap || !has_lexeme) return ST_ERR_NULL_PTR;

    *has_lexeme = 0;
    int c;

    /* пропуск пробельных символов */
    while ((c = fgetc(in)) != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
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

    while (c != EOF && c != ' ' && c != '\t' && c != '\n' && c != '\r') {
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
    *has_lexeme = 1;
    return ST_OK;
}

/* чистая функция: слияние пары лексем в буфер памяти для флага -r */
enum Status proc_r_pair(const char *s1, const char *s2, char **out) {
    if (!out) return ST_ERR_NULL_PTR;
    if (!s1 && !s2) return ST_ERR_ARGS;

    size_t len1 = s1 ? strlen(s1) : 0;
    size_t len2 = s2 ? strlen(s2) : 0;
    size_t total_len = len1 + len2 + ((s1 && s2) ? 2 : 1);

    char *res = (char *)malloc(total_len);
    if (!res) return ST_ERR_MEMORY;

    if (s1 && s2) {
        strcpy(res, s1);
        strcat(res, " ");
        strcat(res, s2);
    } else if (s1) {
        strcpy(res, s1);
    } else {
        strcpy(res, s2);
    }

    *out = res;
    return ST_OK;
}

/* чистая функция: трансформация лексемы по правилам индекса для флага -a */
enum Status transform_lexeme(const char *in, long long index, char **out) {
    if (!in || !out) return ST_ERR_NULL_PTR;

    size_t in_len = strlen(in);
    char *res = NULL;

    if (index % 10 == 0) {
        /* каждая 10-я: латиница в строчные, все символы в base 4 с фиксированной шириной 4 цифры */
        res = (char *)malloc(in_len * 4 + 1);
        if (!res) return ST_ERR_MEMORY;

        size_t pos = 0;
        for (size_t i = 0; i < in_len; ++i) {
            char c = in[i];
            if (is_latin_letter(c)) {
                c = to_lower_latin(c);
            }
            char b4[5];
            char_to_base4_fixed((unsigned char)c, b4);
            memcpy(res + pos, b4, 4);
            pos += 4;
        }
        res[pos] = '\0';

    } else if (index % 2 == 0) {
        /* каждая 2-я (не 10-я): только латинские буквы переводятся в строчные */
        res = (char *)malloc(in_len + 1);
        if (!res) return ST_ERR_MEMORY;

        for (size_t i = 0; i < in_len; ++i) {
            char c = in[i];
            if (is_latin_letter(c)) {
                res[i] = to_lower_latin(c);
            } else {
                res[i] = c;
            }
        }
        res[in_len] = '\0';

    } else if (index % 5 == 0) {
        /* каждая 5-я (не 10-я): все символы в base 8 с фиксированной шириной 3 цифры */
        res = (char *)malloc(in_len * 3 + 1);
        if (!res) return ST_ERR_MEMORY;

        size_t pos = 0;
        for (size_t i = 0; i < in_len; ++i) {
            unsigned char uc = (unsigned char)in[i];
            sprintf(res + pos, "%03o", (unsigned int)uc);
            pos += 3;
        }
        res[pos] = '\0';

    } else {
        /* остальные лексемы без изменений */
        res = (char *)malloc(in_len + 1);
        if (!res) return ST_ERR_MEMORY;
        strcpy(res, in);
    }

    *out = res;
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
    if (argc < 4) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    char flag = '\0';
    enum Status st = parse_flag(argv[1], &flag);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    if (flag == 'r') {
        if (argc != 5) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        const char *p1 = argv[2];
        const char *p2 = argv[3];
        const char *pout = argv[4];

        if (strcmp(p1, pout) == 0 || strcmp(p2, pout) == 0) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        FILE *f1 = fopen(p1, "r");
        if (!f1) {
            print_error(ST_ERR_FILE_OPEN);
            return ST_ERR_FILE_OPEN;
        }

        FILE *f2 = fopen(p2, "r");
        if (!f2) {
            fclose(f1);
            print_error(ST_ERR_FILE_OPEN);
            return ST_ERR_FILE_OPEN;
        }

        FILE *fout = fopen(pout, "w");
        if (!fout) {
            fclose(f1);
            fclose(f2);
            print_error(ST_ERR_FILE_OPEN);
            return ST_ERR_FILE_OPEN;
        }

        char *buf1 = NULL;
        size_t cap1 = 0;
        char *buf2 = NULL;
        size_t cap2 = 0;
        int has1 = 0;
        int has2 = 0;
        int need_space = 0;

        st = read_next_lexeme(f1, &buf1, &cap1, &has1);
        if (st == ST_OK) st = read_next_lexeme(f2, &buf2, &cap2, &has2);

        while (st == ST_OK && (has1 || has2)) {
            char *merged = NULL;
            st = proc_r_pair(has1 ? buf1 : NULL, has2 ? buf2 : NULL, &merged);
            if (st != ST_OK) break;

            if (need_space) {
                if (fputc(' ', fout) == EOF) {
                    free(merged);
                    st = ST_ERR_FILE_WRITE;
                    break;
                }
            }

            if (fputs(merged, fout) == EOF) {
                free(merged);
                st = ST_ERR_FILE_WRITE;
                break;
            }
            need_space = 1;
            free(merged);

            if (has1) st = read_next_lexeme(f1, &buf1, &cap1, &has1);
            if (st == ST_OK && has2) st = read_next_lexeme(f2, &buf2, &cap2, &has2);
        }

        if (st == ST_OK && (ferror(f1) || ferror(f2))) {
            st = ST_ERR_FILE_READ;
        }

        free(buf1);
        free(buf2);
        fclose(f1);
        fclose(f2);
        fclose(fout);

        if (st != ST_OK) {
            print_error(st);
            return st;
        }

        printf("Successfully merged files into: %s\n", pout);

    } else if (flag == 'a') {
        if (argc != 4) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        const char *pin = argv[2];
        const char *pout = argv[3];

        if (strcmp(pin, pout) == 0) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        FILE *fin = fopen(pin, "r");
        if (!fin) {
            print_error(ST_ERR_FILE_OPEN);
            return ST_ERR_FILE_OPEN;
        }

        FILE *fout = fopen(pout, "w");
        if (!fout) {
            fclose(fin);
            print_error(ST_ERR_FILE_OPEN);
            return ST_ERR_FILE_OPEN;
        }

        char *buf = NULL;
        size_t cap = 0;
        int has_lex = 0;
        long long index = 1;
        int need_space = 0;

        st = read_next_lexeme(fin, &buf, &cap, &has_lex);

        while (st == ST_OK && has_lex) {
            char *transformed = NULL;
            st = transform_lexeme(buf, index, &transformed);
            if (st != ST_OK) break;

            if (need_space) {
                if (fputc(' ', fout) == EOF) {
                    free(transformed);
                    st = ST_ERR_FILE_WRITE;
                    break;
                }
            }

            if (fputs(transformed, fout) == EOF) {
                free(transformed);
                st = ST_ERR_FILE_WRITE;
                break;
            }
            need_space = 1;
            free(transformed);

            ++index;
            st = read_next_lexeme(fin, &buf, &cap, &has_lex);
        }

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

        printf("Successfully transformed file into: %s\n", pout);

    } else {
        print_error(ST_ERR_FLAG);
        return ST_ERR_FLAG;
    }

    return ST_OK;
}
