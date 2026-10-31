#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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

/* структура абстрактного потока вывода */
struct OutStream {
    FILE *fp;
    char *buf;
    size_t written;
};

/* вывод одного символа в абстрактный поток */
static void out_putc(struct OutStream *out, const char c) {
    if (out->fp) {
        fputc(c, out->fp);
    } else if (out->buf) {
        out->buf[out->written] = c;
    }
    ++out->written;
}

/* вывод строки в абстрактный поток */
static void out_puts(struct OutStream *out, const char *s) {
    if (!s) return;
    for (size_t i = 0; s[i] != '\0'; ++i) {
        out_putc(out, s[i]);
    }
}

/* печать целого числа в римской системе счисления (%Ro) */
static void print_roman(struct OutStream *out, const int val) {
    if (val == 0) {
        out_puts(out, "0");
        return;
    }

    unsigned int uval = 0;
    if (val < 0) {
        out_putc(out, '-');
        uval = (val == INT_MIN) ? ((unsigned int)INT_MAX + 1U) : (unsigned int)(-val);
    } else {
        uval = (unsigned int)val;
    }

    const unsigned int nums[] = { 1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1 };
    const char *romans[] = { "M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I" };

    for (size_t i = 0; i < sizeof(nums) / sizeof(nums[0]); ++i) {
        while (uval >= nums[i]) {
            out_puts(out, romans[i]);
            uval -= nums[i];
        }
    }
}

/* печать цекендорфова представления unsigned int (%Zr) */
static void print_zeckendorf(struct OutStream *out, const unsigned int val) {
    if (val == 0) {
        out_puts(out, "01");
        return;
    }

    /* генерация чисел Фибоначчи F_2 = 1, F_3 = 2, F_4 = 3... */
    unsigned int fib[64];
    fib[0] = 1;
    fib[1] = 2;
    size_t fib_count = 2;

    while (fib_count < 64) {
        unsigned int next = fib[fib_count - 1] + fib[fib_count - 2];
        if (next < fib[fib_count - 1] || next > val) {
            break;
        }
        fib[fib_count++] = next;
    }

    /* обрезка массива до наибольшего числа Фибоначчи <= val */
    while (fib_count > 1 && fib[fib_count - 1] > val) {
        --fib_count;
    }

    int coeffs[64] = { 0 };
    unsigned int rem = val;

    for (size_t i = fib_count; i > 0; --i) {
        const size_t idx = i - 1;
        if (fib[idx] <= rem) {
            coeffs[idx] = 1;
            rem -= fib[idx];
        }
    }

    for (size_t i = 0; i < fib_count; ++i) {
        out_putc(out, (char)('0' + coeffs[i]));
    }
    out_putc(out, '1');
}

/* печать числа int в системе счисления base [2..36] (%Cv, %CV) */
static void print_base_int(struct OutStream *out, const int val, int base, const int upper) {
    if (base < 2 || base > 36) {
        base = 10;
    }

    if (val == 0) {
        out_putc(out, '0');
        return;
    }

    unsigned int uval;
    if (val < 0) {
        out_putc(out, '-');
        uval = (val == INT_MIN) ? ((unsigned int)INT_MAX + 1U) : (unsigned int)(-val);
    } else {
        uval = (unsigned int)val;
    }

    char buf[64];
    size_t len = 0;
    const char *digits = upper ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               : "0123456789abcdefghijklmnopqrstuvwxyz";

    while (uval > 0) {
        buf[len++] = digits[uval % (unsigned int)base];
        uval /= (unsigned int)base;
    }

    for (size_t i = len; i > 0; --i) {
        out_putc(out, buf[i - 1]);
    }
}

/* перевод числа из строки в base [2..36] в base 10 (%to, %TO) */
static void print_to_base10(struct OutStream *out, const char *str, int base, const int upper) {
    if (base < 2 || base > 36) {
        base = 10;
    }

    if (!str || !*str) {
        out_puts(out, "0");
        return;
    }

    size_t i = 0;
    int neg = 0;
    if (str[i] == '-' || str[i] == '+') {
        neg = (str[i] == '-');
        ++i;
    }

    long long res = 0;
    for (; str[i] != '\0'; ++i) {
        const char c = str[i];
        int d = -1;

        if (c >= '0' && c <= '9') {
            d = c - '0';
        } else if (upper && (c >= 'A' && c <= 'Z')) {
            d = c - 'A' + 10;
        } else if (!upper && (c >= 'a' && c <= 'z')) {
            d = c - 'a' + 10;
        }

        if (d < 0 || d >= base) {
            break;
        }
        res = res * base + d;
    }

    if (neg) res = -res;

    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", res);
    out_puts(out, buf);
}

/* вывод дампа памяти в двоичной системе счисления через пробел */
static void print_memory_dump(struct OutStream *out, const unsigned char *bytes, const size_t size) {
    for (size_t b = 0; b < size; ++b) {
        if (b > 0) {
            out_putc(out, ' ');
        }
        for (int bit = 7; bit >= 0; --bit) {
            out_putc(out, ((bytes[b] >> bit) & 1) ? '1' : '0');
        }
    }
}

/* общий движок форматирования для overfprintf и oversprintf */
static int voverprintf(struct OutStream *out, const char *format, va_list ap) {
    if (!out || !format) return -1;

    for (size_t i = 0; format[i] != '\0'; ++i) {
        if (format[i] != '%') {
            out_putc(out, format[i]);
            continue;
        }

        ++i;
        if (format[i] == '\0') break;

        if (format[i] == 'R' && format[i + 1] == 'o') {
            ++i;
            const int v = va_arg(ap, int);
            print_roman(out, v);
        } else if (format[i] == 'Z' && format[i + 1] == 'r') {
            ++i;
            const unsigned int v = va_arg(ap, unsigned int);
            print_zeckendorf(out, v);
        } else if (format[i] == 'C' && format[i + 1] == 'v') {
            ++i;
            const int v = va_arg(ap, int);
            const int base = va_arg(ap, int);
            print_base_int(out, v, base, 0);
        } else if (format[i] == 'C' && format[i + 1] == 'V') {
            ++i;
            const int v = va_arg(ap, int);
            const int base = va_arg(ap, int);
            print_base_int(out, v, base, 1);
        } else if (format[i] == 't' && format[i + 1] == 'o') {
            ++i;
            const char *s = va_arg(ap, const char *);
            const int base = va_arg(ap, int);
            print_to_base10(out, s, base, 0);
        } else if (format[i] == 'T' && format[i + 1] == 'O') {
            ++i;
            const char *s = va_arg(ap, const char *);
            const int base = va_arg(ap, int);
            print_to_base10(out, s, base, 1);
        } else if (format[i] == 'm' && format[i + 1] == 'i') {
            ++i;
            const int v = va_arg(ap, int);
            print_memory_dump(out, (const unsigned char *)&v, sizeof(int));
        } else if (format[i] == 'm' && format[i + 1] == 'u') {
            ++i;
            const unsigned int v = va_arg(ap, unsigned int);
            print_memory_dump(out, (const unsigned char *)&v, sizeof(unsigned int));
        } else if (format[i] == 'm' && format[i + 1] == 'd') {
            ++i;
            const double v = va_arg(ap, double);
            print_memory_dump(out, (const unsigned char *)&v, sizeof(double));
        } else if (format[i] == 'm' && format[i + 1] == 'f') {
            ++i;
            const float v = (float)va_arg(ap, double);
            print_memory_dump(out, (const unsigned char *)&v, sizeof(float));
        } else if (format[i] == 'd' || format[i] == 'i') {
            const int v = va_arg(ap, int);
            char b[32];
            snprintf(b, sizeof(b), "%d", v);
            out_puts(out, b);
        } else if (format[i] == 'u') {
            const unsigned int v = va_arg(ap, unsigned int);
            char b[32];
            snprintf(b, sizeof(b), "%u", v);
            out_puts(out, b);
        } else if (format[i] == 'x') {
            const unsigned int v = va_arg(ap, unsigned int);
            char b[32];
            snprintf(b, sizeof(b), "%x", v);
            out_puts(out, b);
        } else if (format[i] == 'X') {
            const unsigned int v = va_arg(ap, unsigned int);
            char b[32];
            snprintf(b, sizeof(b), "%X", v);
            out_puts(out, b);
        } else if (format[i] == 'o') {
            const unsigned int v = va_arg(ap, unsigned int);
            char b[32];
            snprintf(b, sizeof(b), "%o", v);
            out_puts(out, b);
        } else if (format[i] == 'p') {
            void *p = va_arg(ap, void *);
            char b[32];
            snprintf(b, sizeof(b), "%p", p);
            out_puts(out, b);
        } else if (format[i] == 'e' || format[i] == 'E') {
            const double v = va_arg(ap, double);
            char b[64];
            snprintf(b, sizeof(b), (format[i] == 'e') ? "%e" : "%E", v);
            out_puts(out, b);
        } else if (format[i] == 'g' || format[i] == 'G') {
            const double v = va_arg(ap, double);
            char b[64];
            snprintf(b, sizeof(b), (format[i] == 'g') ? "%g" : "%G", v);
            out_puts(out, b);
        } else if (format[i] == 's') {
            const char *s = va_arg(ap, const char *);
            out_puts(out, s ? s : "(null)");
        } else if (format[i] == 'c') {
            const char c = (char)va_arg(ap, int);
            out_putc(out, c);
        } else if (format[i] == 'f') {
            const double v = va_arg(ap, double);
            char b[64];
            snprintf(b, sizeof(b), "%f", v);
            out_puts(out, b);
        } else if (format[i] == '%') {
            out_putc(out, '%');
        } else {
            out_putc(out, '%');
            out_putc(out, format[i]);
        }
    }

    if (out->buf) {
        out->buf[out->written] = '\0';
    }

    return (int)out->written;
}

/* функция overfprintf */
int overfprintf(FILE *stream, const char *format, ...) {
    if (!stream || !format) return -1;
    struct OutStream out = { stream, NULL, 0 };
    va_list ap;
    va_start(ap, format);
    const int res = voverprintf(&out, format, ap);
    va_end(ap);
    return res;
}

/*
 * Сигнатура oversprintf идентична стандартному sprintf (без параметра размера),
 * как требует задание: «поведение схоже с поведением стандартных функций
 * fprintf и sprintf, то есть эти функции имеют одинаковый прототип».
 * Ответственность за достаточный размер буфера — на вызывающем коде.
 */
int oversprintf(char *str, const char *format, ...) {
    if (!str || !format) return -1;
    struct OutStream out = { NULL, str, 0 };
    va_list ap;
    va_start(ap, format);
    const int res = voverprintf(&out, format, ap);
    va_end(ap);
    return res;
}

int main(void) {
    printf("=== Demonstration of overfprintf and oversprintf ===\n\n");

    overfprintf(stdout, "[%%Ro] 2026: %Ro, 49: %Ro, 9: %Ro, -15: %Ro\n", 2026, 49, 9, -15);

    /* проверка цекендорфова представления, включая 1 */
    overfprintf(stdout, "[%%Zr] Zeckendorf for 1: %Zr (expected 11)\n", 1U);
    overfprintf(stdout, "[%%Zr] Zeckendorf for 4: %Zr (expected 1011)\n", 4U);
    overfprintf(stdout, "[%%Zr] Zeckendorf for 10: %Zr (expected 010011)\n", 10U);

    overfprintf(stdout, "[%%Cv] 255 in base 16 (lower): %Cv\n", 255, 16);
    overfprintf(stdout, "[%%CV] 255 in base 16 (upper): %CV\n", 255, 16);
    overfprintf(stdout, "[%%Cv] -42 in base 2: %Cv\n", -42, 2);

    overfprintf(stdout, "[%%to] 'ff' base 16 -> base 10: %to\n", "ff", 16);
    overfprintf(stdout, "[%%TO] 'FF' base 16 -> base 10: %TO\n", "FF", 16);

    const int sample_int = 42;
    overfprintf(stdout, "[%%mi] Dump of int 42: %mi\n", sample_int);

    const unsigned int sample_uint = 255;
    overfprintf(stdout, "[%%mu] Dump of uint 255: %mu\n", sample_uint);

    const float sample_float = 1.0f;
    overfprintf(stdout, "[%%mf] Dump of float 1.0: %mf\n", (double)sample_float);

    const double sample_double = 1.0;
    overfprintf(stdout, "[%%md] Dump of double 1.0: %md\n", sample_double);

    char buf[256];
    oversprintf(buf, "Formatted string: Roman=%Ro, Zeck=%Zr, Hex=%CV", 99, 13U, 31, 16);
    printf("\noversprintf result: \"%s\"\n", buf);

    /* демонстрация стандартных спецификаторов: %x, %X, %o, %e, %g */
    printf("\n--- Standard specifiers demonstration ---\n");
    overfprintf(stdout, "[%%x, %%X, %%o] hex=%x, HEX=%X, oct=%o\n", 255, 255, 64);
    overfprintf(stdout, "[%%e, %%g] sci=%e, compact=%g\n", 123456.78, 0.0000456);

    return (int)ST_OK;
}
