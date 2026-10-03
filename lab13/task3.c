#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
    ST_ERR_NULL_PTR
};

/* кол-во корней уравнения */
enum RootCount {
    ROOT_NONE = 0,
    ROOT_ONE  = 1,
    ROOT_TWO  = 2,
    ROOT_INF  = -1
};

/* для хранения перестановки коэффициентов */
struct Permutation {
    double a, b, c;
};

/* проверка флага */
enum Status parse_flag(const char *s, char *f) {
    if (!s || !f) return ST_ERR_FLAG;
    if ((s[0] != '-' && s[0] != '/') || !s[1] || s[2]) return ST_ERR_FLAG;

    if (s[1] == 'q' || s[1] == 'm' || s[1] == 't') {
        *f = s[1];
        return ST_OK;
    }
    return ST_ERR_FLAG;
}

/* разбор вещественного числа */
enum Status parse_double(const char *s, double *res) {
    if (!s || !res || !*s || isspace((unsigned char)*s)) return ST_ERR_NUM;

    char *end;
    double val = strtod(s, &end);

    /* если конец строки не достигнут */
    if (end == s || *end != '\0') return ST_ERR_NUM;

    /* проверка на бесконечность */
    if (!isfinite(val)) return ST_ERR_OVERFLOW;

    *res = val;
    return ST_OK;
}

/* разбор эпсилон */
enum Status parse_eps(const char *s, double *res) {
    enum Status st = parse_double(s, res);
    if (st != ST_OK) return st;

    if (*res <= 0.0 || *res >= 1.0) return ST_ERR_RANGE;
    return ST_OK;
}

/* разбор целого числа */
enum Status parse_long(const char *s, long long *x) {
    int i = 0;
    int neg = 0;
    unsigned long long val = 0;
    unsigned long long limit;
    unsigned int max_digit;

    if (!s || !x || !*s || isspace((unsigned char)*s)) return ST_ERR_NUM;

    if (s[i] == '+' || s[i] == '-') {
        neg = (s[i] == '-');
        ++i;
    }

    if (s[i] == '\0') return ST_ERR_NUM;

    limit = neg ? (unsigned long long)LLONG_MAX + 1ULL : (unsigned long long)LLONG_MAX;
    max_digit = (unsigned int)(limit % 10ULL);

    for (; s[i] != '\0'; ++i) {
        unsigned int d;
        if (!isdigit((unsigned char)s[i])) return ST_ERR_NUM;
        d = (unsigned int)(s[i] - '0');

        if (val > limit / 10ULL) return ST_ERR_OVERFLOW;
        if (val == limit / 10ULL && d > max_digit) return ST_ERR_OVERFLOW;

        val = val * 10ULL + d;
    }

    if (neg) {
        *x = (val > (unsigned long long)LLONG_MAX) ? LLONG_MIN : -(long long)val;
    } else {
        *x = (long long)val;
    }
    return ST_OK;
}

/* решение уравнения */
enum Status solve_quadratic(const double a, const double b, const double c,
                            const double eps, int *root_count, double *x1, double *x2) {
    if (!root_count || !x1 || !x2) return ST_ERR_NULL_PTR;

    /* коэффициент при x^2 близок к нулю - уравнение линейное */
    if (fabs(a) < eps) {
        if (fabs(b) < eps) {
            *root_count = (fabs(c) < eps) ? ROOT_INF : ROOT_NONE;
        } else {
            *x1 = -c / b;
            *root_count = ROOT_ONE;
        }
        return ST_OK;
    }

    const double d = b * b - 4.0 * a * c;
    if (!isfinite(d)) return ST_ERR_OVERFLOW;

    if (fabs(d) < eps) {
        *x1 = -b / (2.0 * a);
        *root_count = ROOT_ONE;
    } else if (d > 0.0) {
        const double sq = sqrt(d);
        *x1 = (-b - sq) / (2.0 * a);
        *x2 = (-b + sq) / (2.0 * a);
        *root_count = ROOT_TWO;
    } else {
        *root_count = ROOT_NONE;
    }
    return ST_OK;
}

/* проверка двух перестановок на идентичность */
static int is_same_perm(const struct Permutation *p1, const struct Permutation *p2, const double eps) {
    return (fabs(p1->a - p2->a) < eps &&
            fabs(p1->b - p2->b) < eps &&
            fabs(p1->c - p2->c) < eps);
}

/* получение уникальных перестановок */
enum Status get_unique_perms(const double coeffs[3], const double eps,
                             struct Permutation *perms, int *count) {
    if (!coeffs || !perms || !count) return ST_ERR_NULL_PTR;

    const int indices[6][3] = {
        {0, 1, 2}, {0, 2, 1}, {1, 0, 2},
        {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
    };

    *count = 0;
    for (int i = 0; i < 6; ++i) {
        const struct Permutation p = {
            coeffs[indices[i][0]],
            coeffs[indices[i][1]],
            coeffs[indices[i][2]]
        };

        int is_unique = 1;
        for (int j = 0; j < *count; ++j) {
            if (is_same_perm(&p, &perms[j], eps)) {
                is_unique = 0;
                break;
            }
        }

        if (is_unique) {
            perms[*count] = p;
            (*count)++;
        }
    }
    return ST_OK;
}

/* проверка кратности двух целых чисел */
enum Status do_m(const long long a, const long long b, int *is_multiple) {
    if (!is_multiple) return ST_ERR_NULL_PTR;
    if (a == 0 || b == 0) return ST_ERR_RANGE;

    *is_multiple = (a % b == 0);
    return ST_OK;
}

/* являются ли стороны сторонами прямоугольного треугольника */
enum Status do_t(const double s1, const double s2, const double s3,
                 const double eps, int *is_right) {
    if (!is_right) return ST_ERR_NULL_PTR;

    /* сортировка сторон по возрастанию */
    double sides[3] = {s1, s2, s3};
    for (int i = 0; i < 2; ++i) {
        for (int j = i + 1; j < 3; ++j) {
            if (sides[i] > sides[j]) {
                const double tmp = sides[i];
                sides[i] = sides[j];
                sides[j] = tmp;
            }
        }
    }

    /* стороны должны быть > нуля */
    if (sides[0] <= eps) {
        *is_right = 0;
        return ST_OK;
    }

    /* проверка теоремы Пифагора */
    const double sum_sq = sides[0] * sides[0] + sides[1] * sides[1];
    const double hyp_sq = sides[2] * sides[2];

    *is_right = (fabs(sum_sq - hyp_sq) < eps);
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
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    char flag = '\0';
    enum Status st = parse_flag(argv[1], &flag);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    if (flag == 'q') {
        if (argc != 6) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        double eps, coeffs[3];
        st = parse_eps(argv[2], &eps);
        for (int i = 0; st == ST_OK && i < 3; ++i) {
            st = parse_double(argv[3 + i], &coeffs[i]);
        }

        if (st != ST_OK) {
            print_error(st);
            return st;
        }

        struct Permutation perms[6];
        int perm_count = 0;
        st = get_unique_perms(coeffs, eps, perms, &perm_count);
        if (st != ST_OK) {
            print_error(st);
            return st;
        }

        for (int i = 0; i < perm_count; ++i) {
            const double a = perms[i].a;
            const double b = perms[i].b;
            const double c = perms[i].c;

            int root_count = 0;
            double x1 = 0.0, x2 = 0.0;

            st = solve_quadratic(a, b, c, eps, &root_count, &x1, &x2);
            if (st != ST_OK) {
                print_error(st);
                return st;
            }

            printf("%.6g*x^2 + %.6g*x + %.6g = 0  ->  ", a, b, c);
            if (root_count == ROOT_INF) {
                printf("Infinite roots\n");
            } else if (root_count == ROOT_NONE) {
                printf("No real roots\n");
            } else if (root_count == ROOT_ONE) {
                printf("x = %.10f\n", x1);
            } else {
                printf("x1 = %.10f, x2 = %.10f\n", x1, x2);
            }
        }

    } else if (flag == 'm') {
        if (argc != 4) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        long long a, b;
        st = parse_long(argv[2], &a);
        if (st == ST_OK) {
            st = parse_long(argv[3], &b);
        }

        if (st != ST_OK) {
            print_error(st);
            return st;
        }

        int is_multiple = 0;
        st = do_m(a, b, &is_multiple);
        if (st != ST_OK) {
            print_error(st);
            return st;
        }

        if (is_multiple) {
            printf("%lld is a multiple of %lld\n", a, b);
        } else {
            printf("%lld is not a multiple of %lld\n", a, b);
        }

    } else if (flag == 't') {
        if (argc != 6) {
            print_error(ST_ERR_ARGS);
            return ST_ERR_ARGS;
        }

        double eps, sides[3];
        st = parse_eps(argv[2], &eps);
        for (int i = 0; st == ST_OK && i < 3; ++i) {
            st = parse_double(argv[3 + i], &sides[i]);
        }

        if (st != ST_OK) {
            print_error(st);
            return st;
        }

        int is_right = 0;
        st = do_t(sides[0], sides[1], sides[2], eps, &is_right);
        if (st != ST_OK) {
            print_error(st);
            return st;
        }

        if (is_right) {
            printf("Sides %.6g, %.6g, %.6g can make triangle\n", sides[0], sides[1], sides[2]);
        } else {
            printf("Sides %.6g, %.6g, %.6g dont can make triangle\n", sides[0], sides[1], sides[2]);
        }

    } else {
        print_error(ST_ERR_FLAG);
        return ST_ERR_FLAG;
    }

    return ST_OK;
}
