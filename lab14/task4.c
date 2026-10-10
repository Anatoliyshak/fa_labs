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

/* проверка символа на принадлежность латинскому алфавиту без локалезависимости */
static int is_latin(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/* проверка и разбор флага */
enum Status parse_flag(const char *s, char *action, int *has_custom_out) {
    if (!s || !action || !has_custom_out) return ST_ERR_NULL_PTR;
    if (s[0] != '-' && s[0] != '/') return ST_ERR_FLAG;

    if (s[1] == 'n') {
        if (!s[2] || s[3] != '\0') return ST_ERR_FLAG;
        if (s[2] == 'd' || s[2] == 'i' || s[2] == 's' || s[2] == 'a') {
            *has_custom_out = 1;
            *action = s[2];
            return ST_OK;
        }
    } else {
        if (!s[1] || s[2] != '\0') return ST_ERR_FLAG;
        if (s[1] == 'd' || s[1] == 'i' || s[1] == 's' || s[1] == 'a') {
            *has_custom_out = 0;
            *action = s[1];
            return ST_OK;
        }
    }

    return ST_ERR_FLAG;
}

/* генерация имени выходного файла с префиксом out_ */
enum Status generate_out_path(const char *in_path, char **out_path) {
    if (!in_path || !out_path) return ST_ERR_NULL_PTR;

    size_t len = strlen(in_path);
    const char *last_slash = strrchr(in_path, '/');
    const char *last_bslash = strrchr(in_path, '\\');
    const char *sep = last_slash;
    if (!sep || (last_bslash && last_bslash > sep)) {
        sep = last_bslash;
    }

    char *res = (char *)malloc(len + 8);
    if (!res) return ST_ERR_MEMORY;

    if (sep) {
        size_t dir_len = (size_t)(sep - in_path + 1);
        memcpy(res, in_path, dir_len);
        res[dir_len] = '\0';
        strcat(res, "out_");
        strcat(res, sep + 1);
    } else {
        strcpy(res, "out_");
        strcat(res, in_path);
    }

    *out_path = res;
    return ST_OK;
}

/* исключение арабских цифр из буфера памяти */
enum Status proc_d(const char *in, size_t len, char *out, size_t out_sz, size_t *out_len) {
    if (!in || !out || !out_len) return ST_ERR_NULL_PTR;

    size_t written = 0;
    for (size_t i = 0; i < len; ++i) {
        char c = in[i];
        if (c < '0' || c > '9') {
            if (written >= out_sz) return ST_ERR_RANGE;
            out[written++] = c;
        }
    }

    *out_len = written;
    return ST_OK;
}

/*замена всех символов кроме цифр на 16-ричный ASCII-код в буфере памяти */
enum Status proc_a(const char *in, size_t len, char *out, size_t out_sz, size_t *out_len) {
    if (!in || !out || !out_len) return ST_ERR_NULL_PTR;

    static const char hex_digits[] = "0123456789ABCDEF";
    size_t written = 0;

    for (size_t i = 0; i < len; ++i) {
        char c = in[i];
        if (c >= '0' && c <= '9') {
            if (written >= out_sz) return ST_ERR_RANGE;
            out[written++] = c;
        } else {
            if (written + 2 > out_sz) return ST_ERR_RANGE;
            unsigned char uc = (unsigned char)c;
            out[written++] = hex_digits[(uc >> 4) & 0x0F];
            out[written++] = hex_digits[uc & 0x0F];
        }
    }

    *out_len = written;
    return ST_OK;
}

/* чистая функция: подсчёт количества латинских букв в строке */
enum Status proc_i(const char *line, int *count) {
    if (!line || !count) return ST_ERR_NULL_PTR;

    int cnt = 0;
    for (size_t i = 0; line[i] != '\0'; ++i) {
        if (is_latin(line[i])) {
            ++cnt;
        }
    }

    *count = cnt;
    return ST_OK;
}

/* чистая функция: подсчёт символов, отличных от латинских букв, цифр и пробела */
enum Status proc_s(const char *line, int *count) {
    if (!line || !count) return ST_ERR_NULL_PTR;

    int cnt = 0;
    for (size_t i = 0; line[i] != '\0'; ++i) {
        char c = line[i];
        if (!is_latin(c) && (c < '0' || c > '9') && c != ' ') {
            ++cnt;
        }
    }

    *count = cnt;
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
    if (argc < 3 || argc > 4) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    char action = '\0';
    int has_custom_out = 0;
    enum Status st = parse_flag(argv[1], &action, &has_custom_out);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    if (has_custom_out && argc != 4) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }
    if (!has_custom_out && argc != 3) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    const char *in_path = argv[2];
    char *allocated_out_path = NULL;
    const char *out_path = NULL;

    if (has_custom_out) {
        out_path = argv[3];
    } else {
        st = generate_out_path(in_path, &allocated_out_path);
        if (st != ST_OK) {
            print_error(st);
            return st;
        }
        out_path = allocated_out_path;
    }

    if (strcmp(in_path, out_path) == 0) {
        if (allocated_out_path) free(allocated_out_path);
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    FILE *in = fopen(in_path, "r");
    if (!in) {
        if (allocated_out_path) free(allocated_out_path);
        print_error(ST_ERR_FILE_OPEN);
        return ST_ERR_FILE_OPEN;
    }

    FILE *out = fopen(out_path, "w");
    if (!out) {
        fclose(in);
        if (allocated_out_path) free(allocated_out_path);
        print_error(ST_ERR_FILE_OPEN);
        return ST_ERR_FILE_OPEN;
    }

    /* контекст ввода-вывода локализован в main */
    if (action == 'd' || action == 'a') {
        char in_buf[4096];
        char out_buf[4096 * 2 + 1];
        size_t n_read = 0;

        while ((n_read = fread(in_buf, 1, sizeof(in_buf), in)) > 0) {
            size_t out_len = 0;

            if (action == 'd') {
                st = proc_d(in_buf, n_read, out_buf, sizeof(out_buf), &out_len);
            } else {
                st = proc_a(in_buf, n_read, out_buf, sizeof(out_buf), &out_len);
            }

            if (st != ST_OK) break;

            if (out_len > 0) {
                size_t n_written = fwrite(out_buf, 1, out_len, out);
                if (n_written != out_len) {
                    st = ST_ERR_FILE_WRITE;
                    break;
                }
            }
        }

        if (st == ST_OK && ferror(in)) {
            st = ST_ERR_FILE_READ;
        }
    } else if (action == 'i' || action == 's') {
        char line_buf[8192];

        while (fgets(line_buf, sizeof(line_buf), in)) {
            /* корректное отбрасывание \r и \n для строк Windows/Unix */
            size_t l = strlen(line_buf);
            while (l > 0 && (line_buf[l - 1] == '\n' || line_buf[l - 1] == '\r')) {
                line_buf[--l] = '\0';
            }

            int count = 0;
            if (action == 'i') {
                st = proc_i(line_buf, &count);
            } else {
                st = proc_s(line_buf, &count);
            }

            if (st != ST_OK) break;

            if (fprintf(out, "%d\n", count) < 0) {
                st = ST_ERR_FILE_WRITE;
                break;
            }
        }

        if (st == ST_OK && ferror(in)) {
            st = ST_ERR_FILE_READ;
        }
    } else {
        st = ST_ERR_FLAG;
    }

    fclose(in);
    fclose(out);
    if (allocated_out_path) free(allocated_out_path);

    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    printf("Operation completed successfully. Output written to: %s\n", out_path);
    return ST_OK;
}
