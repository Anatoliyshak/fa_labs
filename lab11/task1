#include <stdio.h>
#include <ctype.h>
#include <limits.h>

/* статусы ошибок */
enum Status {
    ST_OK = 0,
    ST_ERR_ARGS,
    ST_ERR_FLAG,
    ST_ERR_NUM,
    ST_ERR_RANGE,
    ST_ERR_OVERFLOW
};

/* результат для -p */
enum NumberKind {
    NK_NEITHER = 0,
    NK_PRIME,
    NK_COMPOSITE
};

/* проверка флага */
enum Status parse_flag(const char *s, char *f) {
    if (!s || !f) return ST_ERR_FLAG;
    if ((s[0] != '-' && s[0] != '/') || !s[1] || s[2]) return ST_ERR_FLAG;

    if (s[1] == 'h' || s[1] == 'p' || s[1] == 's' ||
        s[1] == 'e' || s[1] == 'a' || s[1] == 'f') {
        *f = s[1];
        return ST_OK;
    }

    return ST_ERR_FLAG;
}

/* проверка числа */
enum Status parse_num(const char *s, long long *x) {
    int i = 0;
    int neg = 0;
    unsigned long long val = 0;
    unsigned long long limit;
    unsigned int max_digit;

    if (!s || !x || !*s || isspace((unsigned char)*s)) return ST_ERR_NUM;

    /* можно один знак в начале */
    if (s[i] == '+' || s[i] == '-') {
        neg = (s[i] == '-');
        ++i;
    }

    /* после знака должна быть хотя бы одна цифра */
    if (s[i] == '\0') return ST_ERR_NUM;

    limit = neg ? (unsigned long long)LLONG_MAX + 1ULL
                : (unsigned long long)LLONG_MAX;

    max_digit = (unsigned int)(limit % 10ULL);

    for (; s[i] != '\0'; ++i) {
        unsigned int d;

        if (!isdigit((unsigned char)s[i])) return ST_ERR_NUM;

        d = (unsigned int)(s[i] - '0');

        /* проверяем переполнение до того, как умножить на 10 */
        if (val > limit / 10ULL) return ST_ERR_OVERFLOW;
        if (val == limit / 10ULL && d > max_digit) return ST_ERR_OVERFLOW;

        val = val * 10ULL + d;
    }

    if (neg) {
        if (val > (unsigned long long)LLONG_MAX) {
            *x = LLONG_MIN;
        } else {
            *x = -(long long)val;
        }
    } else {
        *x = (long long)val;
    }

    return ST_OK;
}

/* -h числа до 100, кратные x */
enum Status do_h(long long x, long long *a, int *n) {
    if (!a || !n) return ST_ERR_ARGS;
    if (x <= 0) return ST_ERR_RANGE;

    *n = 0;

    /* идём сразу по кратным числам */
    for (long long i = x; i <= 100; i += x) {
        a[(*n)++] = i;
    }

    return ST_OK;
}

/* -p простое/составное/ни туда, ни сюда */
enum Status do_p(long long x, enum NumberKind *k) {
    if (!k) return ST_ERR_ARGS;
    if (x < 1) return ST_ERR_RANGE;

    if (x == 1) {
        *k = NK_NEITHER;
        return ST_OK;
    }

    if (x == 2) {
        *k = NK_PRIME;
        return ST_OK;
    }

    if (x % 2 == 0) {
        *k = NK_COMPOSITE;
        return ST_OK;
    }

    for (long long d = 3; d <= x / d; d += 2) {
        if (x % d == 0) {
            *k = NK_COMPOSITE;
            return ST_OK;
        }
    }

    *k = NK_PRIME;
    return ST_OK;
}

/* -s шестадцатиричные цифры */
enum Status do_s(long long x, char *b, int cap, int *n) {
    unsigned long long t;
    int need = 0;

    if (!b || !n) return ST_ERR_ARGS;
    if (cap < 1) return ST_ERR_ARGS;
    if (x < 0) return ST_ERR_RANGE;

    if (x == 0) {
        b[0] = '0';
        *n = 1;
        return ST_OK;
    }

    t = (unsigned long long)x;

    /* считаем, сколько цифр понадобится */
    while (t) {
        ++need;
        t /= 16;
    }

    /* проверка размера буфера */
    if (cap < need) return ST_ERR_ARGS;

    t = (unsigned long long)x;

    /* заполняем с конца */
    for (int i = need - 1; i >= 0; --i) {
        int d = (int)(t % 16);
        b[i] = (char)(d < 10 ? '0' + d : 'A' + d - 10);
        t /= 16;
    }

    *n = need;
    return ST_OK;
}

/* -e таблица степеней */
enum Status do_e(long long x, long long t[10][10], int *cols) {
    if (!t || !cols) return ST_ERR_ARGS;
    if (x < 1 || x > 10) return ST_ERR_RANGE;

    *cols = (int)x;

    for (int base = 1; base <= 10; ++base) {
        long long v = 1;

        for (int exp = 1; exp <= x; ++exp) {
            if (v > LLONG_MAX / base) return ST_ERR_OVERFLOW;

            v *= base;
            t[base - 1][exp - 1] = v;
        }
    }

    return ST_OK;
}

/* -a сумма натуральных чисел */
enum Status do_a(long long x, unsigned long long *s) {
    unsigned long long n, a, b;

    if (!s) return ST_ERR_ARGS;
    if (x < 1) return ST_ERR_RANGE;

    n = (unsigned long long)x;

    /* n * (n + 1) / 2 */
    if (n % 2 == 0) {
        a = n / 2;
        b = n + 1;
    } else {
        a = n;
        b = (n + 1) / 2;
    }

    if (b != 0 && a > ULLONG_MAX / b) return ST_ERR_OVERFLOW;

    *s = a * b;
    return ST_OK;
}

/* -f факториал */
enum Status do_f(long long x, unsigned long long *f) {
    if (!f) return ST_ERR_ARGS;
    if (x < 0) return ST_ERR_RANGE;

    *f = 1;

    for (long long i = 2; i <= x; ++i) {
        unsigned long long m = (unsigned long long)i;

        if (*f > ULLONG_MAX / m) return ST_ERR_OVERFLOW;

        *f *= m;
    }

    return ST_OK;
}

/* вывод ошибоxчек */
void print_error(enum Status st) {
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
    }
}

int main(int argc, char **argv) {
    enum Status st = ST_OK;
    enum NumberKind kind;
    char flag;
    long long x;
    const char *num = NULL;

    if (argc != 3) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    /* flag может быть первым или вторым */
    if (parse_flag(argv[1], &flag) == ST_OK) {
        num = argv[2];
    } else if (parse_flag(argv[2], &flag) == ST_OK) {
        num = argv[1];
    } else {
        print_error(ST_ERR_FLAG);
        return ST_ERR_FLAG;
    }

    st = parse_num(num, &x);

    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    if (flag == 'h') {
        long long a[100];
        int n;

        st = do_h(x, a, &n);

        if (st == ST_OK) {
            if (n == 0) {
                printf("No such nums.\n");
            } else {
                for (int i = 0; i < n; ++i) {
                    printf("%s%lld", i ? " " : "", a[i]);
                }
                printf("\n");
            }
        }
    } else if (flag == 'p') {
        st = do_p(x, &kind);

        if (st == ST_OK) {
            if (kind == NK_PRIME) {
                printf("num %lld is prime.\n", x);
            } else if (kind == NK_COMPOSITE) {
                printf("num %lld is composite.\n", x);
            } else {
                printf("num %lld is ni tuda, ni s'uda\n", x);
            }
        }
    } else if (flag == 's') {
        char b[32];
        int n;

        st = do_s(x, b, (int)sizeof(b), &n);

        if (st == ST_OK) {
            for (int i = 0; i < n; ++i) {
                printf("%s%c", i ? " " : "", b[i]);
            }
            printf("\n");
        }
    } else if (flag == 'e') {
        long long t[10][10];
        int cols;

        st = do_e(x, t, &cols);

        if (st == ST_OK) {
            for (int base = 0; base < 10; ++base) {
                for (int exp = 0; exp < cols; ++exp) {
                    printf("%s%lld", exp ? " " : "", t[base][exp]);
                }
                printf("\n");
            }
        }
    } else if (flag == 'a') {
        unsigned long long s;

        st = do_a(x, &s);

        if (st == ST_OK) {
            printf("Sum = %llu\n", s);
        }
    } else if (flag == 'f') {
        unsigned long long f;

        st = do_f(x, &f);

        if (st == ST_OK) {
            printf("factorial = %llu\n", f);
        }
    } else {
        st = ST_ERR_FLAG;
    }

    if (st != ST_OK) {
        print_error(st);
    }

    return st;
}
