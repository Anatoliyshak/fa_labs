#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdint.h>

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

/* абстрактный поток ввода */
struct InStream {
    FILE *fp;
    const char *str;
    size_t pos;
    int hit_eof;
};

/* чтение одного символа из абстрактного потока */
static int in_getc(struct InStream *in) {
    if (in->fp) {
        int c = fgetc(in->fp);
        if (c == EOF) in->hit_eof = 1;
        return c;
    }
    if (in->str) {
        if (in->str[in->pos] != '\0') {
            return (int)(unsigned char)in->str[in->pos++];
        } else {
            in->hit_eof = 1;
            return EOF;
        }
    }
    in->hit_eof = 1;
    return EOF;
}

/* возврат символа в абстрактный поток */
static void in_ungetc(struct InStream *in, const int c) {
    if (c == EOF) return;
    in->hit_eof = 0;
    if (in->fp) {
        ungetc(c, in->fp);
    } else if (in->str && in->pos > 0) {
        --in->pos;
    }
}

/* пропуск пробельных символов во входном потоке */
static void skip_spaces(struct InStream *in) {
    int c;
    while ((c = in_getc(in)) != EOF) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f') {
            in_ungetc(in, c);
            break;
        }
    }
}

/* значение римского символа */
static int roman_char_val(const char c) {
    if (c == 'I' || c == 'i') return 1;
    if (c == 'V' || c == 'v') return 5;
    if (c == 'X' || c == 'x') return 10;
    if (c == 'L' || c == 'l') return 50;
    if (c == 'C' || c == 'c') return 100;
    if (c == 'D' || c == 'd') return 500;
    if (c == 'M' || c == 'm') return 1000;
    return -1;
}

/* парсинг римского числа (%Ro) */
static int scan_roman(struct InStream *in, int *out_val) {
    skip_spaces(in);

    int c = in_getc(in);
    if (c == EOF) return 0;

    int neg = 0;
    if (c == '-' || c == '+') {
        neg = (c == '-');
        c = in_getc(in);
    }

    char buf[64];
    size_t len = 0;

    while (c != EOF && roman_char_val((char)c) > 0 && len + 1 < sizeof(buf)) {
        buf[len++] = (char)c;
        c = in_getc(in);
    }
    in_ungetc(in, c);

    if (len == 0) return 0;

    int total = 0;
    for (size_t i = 0; i < len; ++i) {
        const int cur = roman_char_val(buf[i]);
        int next = 0;
        if (i + 1 < len) {
            next = roman_char_val(buf[i + 1]);
        }
        if (cur < next) {
            total -= cur;
        } else {
            total += cur;
        }
    }

    *out_val = neg ? -total : total;
    return 1;
}

/* парсинг цекендорфова представления (%Zr) */
/* коэффициенты 0 и 1 от младшего к старшему с единицей на конце (терминатор 11) */
static int scan_zeckendorf(struct InStream *in, unsigned int *out_val) {
    skip_spaces(in);

    char bits[64];
    size_t len = 0;
    int c;

    while ((c = in_getc(in)) != EOF) {
        if (c == '0' || c == '1') {
            bits[len++] = (char)c;
            /* завершение записи происходит при первой встрече двух единиц подряд */
            if (len >= 2 && bits[len - 2] == '1' && bits[len - 1] == '1') {
                break;
            }
            if (len >= sizeof(bits) - 1) {
                break;
            }
        } else {
            in_ungetc(in, c);
            break;
        }
    }

    /* проверка наличия корректного признака окончания (две единицы в конце) */
    if (len < 2 || bits[len - 2] != '1' || bits[len - 1] != '1') {
        return 0;
    }

    /* генерация чисел Фибоначчи F_2 = 1, F_3 = 2, F_4 = 3... */
    unsigned int fib[64];
    fib[0] = 1;
    fib[1] = 2;
    for (size_t i = 2; i < 64; ++i) {
        fib[i] = fib[i - 1] + fib[i - 2];
    }

    /* отбрасываем завершающий служебный бит 1 */
    const size_t coeff_count = len - 1;
    unsigned int res = 0;

    for (size_t i = 0; i < coeff_count; ++i) {
        if (bits[i] == '1') {
            res += fib[i];
        }
    }

    *out_val = res;
    return 1;
}

/* парсинг целого числа в системе base [2..36] (%Cv, %CV) */
static int scan_base_int(struct InStream *in, int *out_val, int base, const int upper) {
    if (base < 2 || base > 36) {
        base = 10;
    }

    skip_spaces(in);
    int c = in_getc(in);
    if (c == EOF) return 0;

    int neg = 0;
    if (c == '-' || c == '+') {
        neg = (c == '-');
        c = in_getc(in);
    }

    int read_any = 0;
    long long res = 0;

    while (c != EOF) {
        int d = -1;
        if (c >= '0' && c <= '9') {
            d = c - '0';
        } else if (upper && (c >= 'A' && c <= 'Z')) {
            d = c - 'A' + 10;
        } else if (!upper && (c >= 'a' && c <= 'z')) {
            d = c - 'a' + 10;
        }

        if (d < 0 || d >= base) {
            in_ungetc(in, c);
            break;
        }

        read_any = 1;
        res = res * base + d;
        c = in_getc(in);
    }

    if (!read_any) return 0;

    *out_val = neg ? (int)(-res) : (int)res;
    return 1;
}

/* чтение десятичного int (%d, %i) */
static int scan_dec_int(struct InStream *in, int *out_val) {
    return scan_base_int(in, out_val, 10, 0);
}

/* чтение беззнакового числа (%u) */
static int scan_dec_uint(struct InStream *in, unsigned int *out_val) {
    int v = 0;
    if (scan_base_int(in, &v, 10, 0)) {
        *out_val = (unsigned int)v;
        return 1;
    }
    return 0;
}

/*
 * Сигнатура oversscanf/overfscanf идентична стандартным sscanf/fscanf.
 * Чтение %s без ограничения ширины соответствует поведению стандартного %s.
 * Задание: «валидация данных, находящихся во входном потоке, не требуется».
 * Ответственность за размер буфера — на вызывающем коде.
 */
static int scan_string(struct InStream *in, char *out_str) {
    skip_spaces(in);
    int c = in_getc(in);
    if (c == EOF) return 0;

    size_t len = 0;
    while (c != EOF && c != ' ' && c != '\t' && c != '\n' && c != '\r') {
        out_str[len++] = (char)c;
        c = in_getc(in);
    }
    in_ungetc(in, c);
    out_str[len] = '\0';
    return 1;
}

/* чтение шестнадцатеричного числа (%x, %X) */
static int scan_hex_uint(struct InStream *in, unsigned int *out_val) {
    skip_spaces(in);
    int c = in_getc(in);
    if (c == EOF) return 0;

    int read_any = 0;
    unsigned int res = 0;

    while (c != EOF) {
        int d = -1;
        if (c >= '0' && c <= '9') {
            d = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            d = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            d = c - 'A' + 10;
        }

        if (d < 0) {
            in_ungetc(in, c);
            break;
        }

        read_any = 1;
        res = res * 16U + (unsigned int)d;
        c = in_getc(in);
    }

    if (!read_any) return 0;

    *out_val = res;
    return 1;
}

/* чтение восьмеричного числа (%o) */
static int scan_oct_uint(struct InStream *in, unsigned int *out_val) {
    int v = 0;
    if (scan_base_int(in, &v, 8, 0)) {
        *out_val = (unsigned int)v;
        return 1;
    }
    return 0;
}

/* чтение вещественного числа float/double (%f, %lf) */
static int scan_float(struct InStream *in, float *out_val) {
    skip_spaces(in);
    char buf[128];
    size_t len = 0;
    int c = in_getc(in);
    if (c == EOF) return 0;
    if (c == '+' || c == '-') {
        buf[len++] = (char)c;
        c = in_getc(in);
    }
    int has_digits = 0;
    while (c != EOF && ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-')) {
        if (len + 1 < sizeof(buf)) buf[len++] = (char)c;
        if (c >= '0' && c <= '9') has_digits = 1;
        c = in_getc(in);
    }
    in_ungetc(in, c);
    buf[len] = '\0';
    if (!has_digits) return 0;
    char *end = NULL;
    *out_val = strtof(buf, &end);
    return (end != buf);
}

static int scan_double(struct InStream *in, double *out_val) {
    skip_spaces(in);
    char buf[128];
    size_t len = 0;
    int c = in_getc(in);
    if (c == EOF) return 0;
    if (c == '+' || c == '-') {
        buf[len++] = (char)c;
        c = in_getc(in);
    }
    int has_digits = 0;
    while (c != EOF && ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-')) {
        if (len + 1 < sizeof(buf)) buf[len++] = (char)c;
        if (c >= '0' && c <= '9') has_digits = 1;
        c = in_getc(in);
    }
    in_ungetc(in, c);
    buf[len] = '\0';
    if (!has_digits) return 0;
    char *end = NULL;
    *out_val = strtod(buf, &end);
    return (end != buf);
}

/* общий синтаксический анализатор overfscanf и oversscanf */
static int voverscanf(struct InStream *in, const char *format, va_list ap) {
    if (!in || !format) return -1;

    int assigned_count = 0;

    for (size_t i = 0; format[i] != '\0'; ++i) {
        if (format[i] == ' ' || format[i] == '\t' || format[i] == '\n') {
            skip_spaces(in);
            continue;
        }

        if (format[i] != '%') {
            int c = in_getc(in);
            if (c != (int)(unsigned char)format[i]) {
                in_ungetc(in, c);
                break;
            }
            continue;
        }

        ++i;
        if (format[i] == '\0') break;

        /* кастомные спецификаторы */
        if (format[i] == 'R' && format[i + 1] == 'o') {
            ++i;
            int *ptr = va_arg(ap, int *);
            if (scan_roman(in, ptr)) {
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 'Z' && format[i + 1] == 'r') {
            ++i;
            unsigned int *ptr = va_arg(ap, unsigned int *);
            if (scan_zeckendorf(in, ptr)) {
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 'C' && format[i + 1] == 'v') {
            ++i;
            int *ptr = va_arg(ap, int *);
            int base = va_arg(ap, int);
            if (scan_base_int(in, ptr, base, 0)) {
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 'C' && format[i + 1] == 'V') {
            ++i;
            int *ptr = va_arg(ap, int *);
            int base = va_arg(ap, int);
            if (scan_base_int(in, ptr, base, 1)) {
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 'd' || format[i] == 'i') {
            int *ptr = va_arg(ap, int *);
            if (scan_dec_int(in, ptr)) {
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 'u') {
            unsigned int *ptr = va_arg(ap, unsigned int *);
            if (scan_dec_uint(in, ptr)) {
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 'x' || format[i] == 'X') {
            unsigned int *ptr = va_arg(ap, unsigned int *);
            if (scan_hex_uint(in, ptr)) {
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 'o') {
            unsigned int *ptr = va_arg(ap, unsigned int *);
            if (scan_oct_uint(in, ptr)) {
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 'l' && format[i + 1] == 'f') {
            ++i;
            double *ptr = va_arg(ap, double *);
            if (scan_double(in, ptr)) {
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 'f') {
            float *ptr = va_arg(ap, float *);
            if (scan_float(in, ptr)) {
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 'p') {
            void **ptr = va_arg(ap, void **);
            int v = 0;
            if (scan_base_int(in, &v, 16, 0) || scan_base_int(in, &v, 16, 1)) {
                *ptr = (void *)(uintptr_t)(unsigned int)v;
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 's') {
            char *ptr = va_arg(ap, char *);
            if (scan_string(in, ptr)) {
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == 'c') {
            char *ptr = va_arg(ap, char *);
            int c = in_getc(in);
            if (c != EOF) {
                *ptr = (char)c;
                ++assigned_count;
            } else {
                break;
            }
        } else if (format[i] == '%') {
            int c = in_getc(in);
            if (c != '%') {
                in_ungetc(in, c);
                break;
            }
        } else {
            break;
        }
    }

    if (assigned_count == 0 && in->hit_eof) {
        return EOF;
    }

    return assigned_count;
}

/* функция overfscanf */
int overfscanf(FILE *stream, const char *format, ...) {
    if (!stream || !format) return -1;
    struct InStream in = { stream, NULL, 0, 0 };
    va_list ap;
    va_start(ap, format);
    const int res = voverscanf(&in, format, ap);
    va_end(ap);
    return res;
}

/* функция oversscanf */
int oversscanf(const char *str, const char *format, ...) {
    if (!str || !format) return -1;
    struct InStream in = { NULL, str, 0, 0 };
    va_list ap;
    va_start(ap, format);
    const int res = voverscanf(&in, format, ap);
    va_end(ap);
    return res;
}

int main(void) {
    printf("=== Demonstration of overfscanf and oversscanf ===\n\n");

    /* 1. Тестирование oversscanf */
    printf("--- 1. Testing oversscanf ---\n");

    /* Римские числа %Ro */
    const char *input_roman = "MCMLXXXIV -XIV XLII";
    int r1 = 0, r2 = 0, r3 = 0;
    int matched = oversscanf(input_roman, "%Ro %Ro %Ro", &r1, &r2, &r3);
    printf("[%%Ro] Matched %d items from \"%s\":\n", matched, input_roman);
    printf("  r1 = %d (1984), r2 = %d (-14), r3 = %d (42)\n\n", r1, r2, r3);

    /* Цекендорфово представление %Zr */
    /* 1 = F_2 -> "11" */
    /* 4 = F_2 + F_4 -> "1011" */
    /* 10 = F_3 + F_6 -> "010011" */
    const char *input_zeck = "11 1011 010011";
    unsigned int z1 = 0, z2 = 0, z3 = 0;
    matched = oversscanf(input_zeck, "%Zr %Zr %Zr", &z1, &z2, &z3);
    printf("[%%Zr] Matched %d items from \"%s\":\n", matched, input_zeck);
    printf("  z1 = %u (expected 1), z2 = %u (expected 4), z3 = %u (expected 10)\n\n", z1, z2, z3);

    /* Системы счисления: %Cv (нижний регистр) и %CV (верхний регистр) */
    const char *input_bases = "ff 2A 1010";
    int b1 = 0, b2 = 0, b3 = 0;
    matched = oversscanf(input_bases, "%Cv %CV %Cv", &b1, 16, &b2, 16, &b3, 2);
    printf("[%%Cv, %%CV] Matched %d items from \"%s\":\n", matched, input_bases);
    printf("  b1(hex lower) = %d (255), b2(hex upper) = %d (42), b3(bin) = %d (10)\n\n", b1, b2, b3);

    /* 2. Тестирование overfscanf через файл */
    printf("--- 2. Testing overfscanf from temporary file ---\n");
    const char *tmp_name = "test_overscan.txt";
    FILE *f_out = fopen(tmp_name, "w");
    if (f_out) {
        fprintf(f_out, "CXXVIII 1011 1a3F 77\n");
        fclose(f_out);
    }

    FILE *f_in = fopen(tmp_name, "r");
    if (f_in) {
        int f_roman = 0;
        unsigned int f_zeck = 0;
        int f_hex_lower = 0;
        int f_dec = 0;

        matched = overfscanf(f_in, "%Ro %Zr %Cv %d", &f_roman, &f_zeck, &f_hex_lower, 16, &f_dec);
        fclose(f_in);

        printf("overfscanf matched %d values:\n", matched);
        printf("  Roman CXXVIII = %d (expected 128)\n", f_roman);
        printf("  Zeckendorf 1011 = %u (expected 4)\n", f_zeck);
        printf("  Hex lower (stopped at 'F') = %d\n", f_hex_lower);
        printf("  Dec = %d\n", f_dec);
    }
    remove(tmp_name);

    /* 3. Тестирование EOF при пустом вводе до первого преобразования */
    printf("\n--- 3. Testing EOF on empty input failure ---\n");
    int empty_val = 0;
    const int eof_res = oversscanf("", "%d", &empty_val);
    printf("oversscanf on empty string returned %d (expected EOF=%d)\n", eof_res, EOF);

    /* 4. Тестирование стандартных спецификаторов: %x, %o, %f */
    printf("\n--- 4. Testing standard specifiers (%%x, %%o, %%f) ---\n");
    unsigned int hex_v = 0, oct_v = 0;
    float flt_v = 0.0f;
    matched = oversscanf("7B 17 3.14", "%x %o %f", &hex_v, &oct_v, &flt_v);
    printf("Matched %d items: hex=0x%x (%u), oct=0%o (%u), float=%.2f\n",
           matched, hex_v, hex_v, oct_v, oct_v, flt_v);

    return (int)ST_OK;
}
