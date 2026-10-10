#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

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

/* структура для двумерной точки */
struct Point {
    double x;
    double y;
};

/* 6.1: проверка многоугольника на выпуклость */
enum Status is_polygon_convex(int *is_convex, const double eps, const int num_vertices, ...) {
    if (!is_convex) return ST_ERR_NULL_PTR;
    if (num_vertices < 3) return ST_ERR_RANGE;
    if (eps <= 0.0) return ST_ERR_RANGE;

    struct Point *pts = (struct Point *)malloc((size_t)num_vertices * sizeof(struct Point));
    if (!pts) return ST_ERR_MEMORY;

    va_list args;
    va_start(args, num_vertices);
    for (int i = 0; i < num_vertices; ++i) {
        pts[i].x = va_arg(args, double);
        pts[i].y = va_arg(args, double);
    }
    va_end(args);

    int pos = 0;
    int neg = 0;

    for (int i = 0; i < num_vertices; ++i) {
        int i_next = (i + 1) % num_vertices;
        int i_nnext = (i + 2) % num_vertices;

        double dx1 = pts[i_next].x - pts[i].x;
        double dy1 = pts[i_next].y - pts[i].y;
        double dx2 = pts[i_nnext].x - pts[i_next].x;
        double dy2 = pts[i_nnext].y - pts[i_next].y;

        /* косое произведение векторов */
        double cross_product = dx1 * dy2 - dy1 * dx2;

        if (cross_product > eps) {
            pos = 1;
        } else if (cross_product < -eps) {
            neg = 1;
        }

        if (pos && neg) {
            *is_convex = 0;
            free(pts);
            return ST_OK;
        }
    }

    *is_convex = 1;
    free(pts);
    return ST_OK;
}

/* 6.2: вычисление значения многочлена степени n по схеме Горнера */
enum Status eval_polynomial(double *res, const double x, const int n, ...) {
    if (!res) return ST_ERR_NULL_PTR;
    if (n < 0) return ST_ERR_RANGE;

    va_list args;
    va_start(args, n);

    /* старший коэффициент an */
    double cur = va_arg(args, double);

    for (int i = 0; i < n; ++i) {
        double coeff = va_arg(args, double);
        cur = cur * x + coeff;
        if (!isfinite(cur)) {
            va_end(args);
            return ST_ERR_OVERFLOW;
        }
    }
    va_end(args);

    *res = cur;
    return ST_OK;
}

/* перевод символа в числовое значение цифры */
static int char_to_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}

/* перевод цифры в символ */
static char digit_to_char(int d) {
    if (d >= 0 && d <= 9) return (char)('0' + d);
    if (d >= 10 && d <= 35) return (char)('A' + d - 10);
    return '?';
}

/* строгое математическое определение числа Капрекара: правая часть имеет ровно столько же цифр, сколько x */
static int is_kaprekar_number(unsigned long long x, int base) {
    if (x == 1ULL) return 1;
    if (x == 0ULL) return 0;

    /* подсчёт количества цифр d в исходном числе x в системе base */
    int d_count = 0;
    unsigned long long temp = x;
    while (temp > 0ULL) {
        ++d_count;
        temp /= (unsigned long long)base;
    }

    unsigned long long sq = x * x;
    /* получение представления квадрата в системе base */
    char buf[128];
    int len = 0;

    temp = sq;
    while (temp > 0ULL) {
        buf[len++] = digit_to_char((int)(temp % (unsigned long long)base));
        temp /= (unsigned long long)base;
    }
    /* реверс строки */
    for (int i = 0; i < len / 2; ++i) {
        char t = buf[i];
        buf[i] = buf[len - 1 - i];
        buf[len - 1 - i] = t;
    }
    buf[len] = '\0';

    /* правая часть обязана иметь ровно d_count цифр */
    if (len <= d_count) return 0;

    int k = len - d_count;
    unsigned long long left = 0ULL;
    unsigned long long right = 0ULL;

    for (int i = 0; i < k; ++i) {
        left = left * (unsigned long long)base + (unsigned long long)char_to_digit(buf[i]);
    }
    for (int i = k; i < len; ++i) {
        right = right * (unsigned long long)base + (unsigned long long)char_to_digit(buf[i]);
    }

    if (right > 0ULL && left + right == x) {
        return 1;
    }

    return 0;
}

/* 6.3: поиск чисел Капрекара в переданном наборе строк */
enum Status find_kaprekar(const int base, int *count_found, char ***result_strings, const int total_strings, ...) {
    if (!count_found || !result_strings) return ST_ERR_NULL_PTR;
    if (base < 2 || base > 36) return ST_ERR_RANGE;
    if (total_strings < 0) return ST_ERR_RANGE;

    char **arr = (char **)malloc((size_t)(total_strings > 0 ? total_strings : 1) * sizeof(char *));
    if (!arr) return ST_ERR_MEMORY;

    int found = 0;
    va_list args;
    va_start(args, total_strings);

    for (int i = 0; i < total_strings; ++i) {
        const char *s = va_arg(args, const char *);
        if (!s || !*s) continue;

        unsigned long long val = 0ULL;
        int valid = 1;

        for (int j = 0; s[j] != '\0'; ++j) {
            int d = char_to_digit(s[j]);
            if (d < 0 || d >= base) {
                valid = 0;
                break;
            }
            if (val > (ULLONG_MAX - (unsigned long long)d) / (unsigned long long)base) {
                valid = 0;
                break;
            }
            val = val * (unsigned long long)base + (unsigned long long)d;
        }

        if (valid && is_kaprekar_number(val, base)) {
            size_t slen = strlen(s);
            arr[found] = (char *)malloc(slen + 1);
            if (!arr[found]) {
                va_end(args);
                for (int k = 0; k < found; ++k) free(arr[k]);
                free(arr);
                return ST_ERR_MEMORY;
            }
            strcpy(arr[found], s);
            ++found;
        }
    }
    va_end(args);

    *count_found = found;
    *result_strings = arr;
    return ST_OK;
}

/* 6.4: среднее геометрическое вещественных чисел */
enum Status calc_geometric_mean(double *res, const int count, ...) {
    if (!res) return ST_ERR_NULL_PTR;
    if (count <= 0) return ST_ERR_RANGE;

    va_list args;
    va_start(args, count);

    double log_sum = 0.0;
    int has_zero = 0;

    for (int i = 0; i < count; ++i) {
        double val = va_arg(args, double);
        if (val < -1e-15) {
            va_end(args);
            return ST_ERR_RANGE;
        }
        if (fabs(val) <= 1e-15) {
            has_zero = 1;
        } else {
            log_sum += log(val);
        }
    }
    va_end(args);

    if (has_zero) {
        *res = 0.0;
    } else {
        *res = exp(log_sum / (double)count);
    }
    return ST_OK;
}

/* 6.5: рекурсивное быстрое возведение вещественного числа в степень */
enum Status fast_pow_rec(const double base, const long long exp, const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0) return ST_ERR_RANGE;

    if (exp == 0) {
        *res = 1.0;
        return ST_OK;
    }

    if (fabs(base) < eps) {
        if (exp < 0) return ST_ERR_RANGE;
        *res = 0.0;
        return ST_OK;
    }

    if (exp < 0) {
        double pos_pow = 0.0;
        enum Status st = fast_pow_rec(base, -exp, eps, &pos_pow);
        if (st != ST_OK) return st;
        if (fabs(pos_pow) < eps) return ST_ERR_OVERFLOW;
        *res = 1.0 / pos_pow;
        return ST_OK;
    }

    if (exp % 2 == 0) {
        double half = 0.0;
        enum Status st = fast_pow_rec(base, exp / 2, eps, &half);
        if (st != ST_OK) return st;
        *res = half * half;
        return ST_OK;
    } else {
        double prev = 0.0;
        enum Status st = fast_pow_rec(base, exp - 1, eps, &prev);
        if (st != ST_OK) return st;
        *res = base * prev;
        return ST_OK;
    }
}

/* 6.6: поиск корня методом дихотомии */
enum Status find_root_bisection(double a, double b, const double eps, double (*f)(double), double *root) {
    if (!f || !root) return ST_ERR_NULL_PTR;
    if (eps <= 0.0) return ST_ERR_RANGE;

    if (a > b) {
        double tmp = a;
        a = b;
        b = tmp;
    }

    double fa = f(a);
    double fb = f(b);

    if (fabs(fa) < eps) {
        *root = a;
        return ST_OK;
    }
    if (fabs(fb) < eps) {
        *root = b;
        return ST_OK;
    }

    if (fa * fb > 0.0) return ST_ERR_RANGE;

    while ((b - a) > eps) {
        double mid = a + (b - a) / 2.0;
        double f_mid = f(mid);

        if (fabs(f_mid) < eps) {
            *root = mid;
            return ST_OK;
        }

        if (fa * f_mid < 0.0) {
            b = mid;
            fb = f_mid;
        } else {
            a = mid;
            fa = f_mid;
        }
    }

    *root = a + (b - a) / 2.0;
    return ST_OK;
}

/* тестовые функции для метода дихотомии */
static double eq1(double x) {
    return x * x - 2.0; /* корень sqrt(2) ~ 1.41421 */
}

static double eq2(double x) {
    return sin(x); /* корень pi ~ 3.14159 на [3, 4] */
}

static double eq3(double x) {
    return x * x * x - x - 2.0; /* корень ~ 1.52138 на [1, 2] */
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

int main(void) {
    const double eps = 1e-7;

    printf("=== Demonstration of Task 6 ===\n\n");

    /* 6.1: выпуклость многоугольника */
    printf("1. Polygon convexity:\n");
    int conv_square = 0;
    /* квадрат (0,0), (2,0), (2,2), (0,2) */
    enum Status st = is_polygon_convex(&conv_square, eps, 4,
                                       0.0, 0.0,
                                       2.0, 0.0,
                                       2.0, 2.0,
                                       0.0, 2.0);
    if (st == ST_OK) {
        printf("   Square (0,0)-(2,0)-(2,2)-(0,2): %s\n", conv_square ? "Convex" : "Non-convex");
    } else {
        print_error(st);
    }

    int conv_star = 0;
    /* вогнутый четырехугольник (0,0), (2,0), (1,1), (0,2) */
    st = is_polygon_convex(&conv_star, eps, 4,
                           0.0, 0.0,
                           2.0, 0.0,
                           1.0, 1.0,
                           0.0, 2.0);
    if (st == ST_OK) {
        printf("   Arrowhead (0,0)-(2,0)-(1,1)-(0,2): %s\n\n", conv_star ? "Convex" : "Non-convex");
    } else {
        print_error(st);
    }

    /* 6.2: многочлен в точке */
    printf("2. Polynomial evaluation (Horner's scheme):\n");
    double poly_res = 0.0;
    /* P(x) = 2*x^3 - 4*x^2 + 3*x - 5 в точке x = 2: 2*8 - 4*4 + 3*2 - 5 = 16 - 16 + 6 - 5 = 1 */
    st = eval_polynomial(&poly_res, 2.0, 3, 2.0, -4.0, 3.0, -5.0);
    if (st == ST_OK) {
        printf("   P(2) for 2x^3 - 4x^2 + 3x - 5 = %.6f\n\n", poly_res);
    } else {
        print_error(st);
    }

    /* 6.3: числа Капрекара */
    printf("3. Kaprekar numbers detection:\n");
    int kap_count = 0;
    char **kap_arr = NULL;
    /* проверка в системе base 10: 9, 45, 55, 12, 99 */
    st = find_kaprekar(10, &kap_count, &kap_arr, 5, "9", "45", "55", "12", "99");
    if (st == ST_OK) {
        printf("   Base 10 found %d Kaprekar numbers:", kap_count);
        for (int i = 0; i < kap_count; ++i) {
            printf(" %s", kap_arr[i]);
            free(kap_arr[i]);
        }
        printf("\n");
        free(kap_arr);
    } else {
        print_error(st);
    }

    /* проверка в системе base 16: A, F, 1B */
    st = find_kaprekar(16, &kap_count, &kap_arr, 3, "A", "F", "1B");
    if (st == ST_OK) {
        printf("   Base 16 found %d Kaprekar numbers:", kap_count);
        for (int i = 0; i < kap_count; ++i) {
            printf(" %s", kap_arr[i]);
            free(kap_arr[i]);
        }
        printf("\n\n");
        free(kap_arr);
    } else {
        print_error(st);
    }

    /* 6.4: среднее геометрическое */
    printf("4. Geometric mean:\n");
    double gm = 0.0;
    st = calc_geometric_mean(&gm, 4, 2.0, 4.0, 8.0, 16.0);
    if (st == ST_OK) {
        printf("   Geometric mean of [2, 4, 8, 16] = %.6f\n\n", gm);
    } else {
        print_error(st);
    }

    /* 6.5: рекурсивное быстрое возведение в степень */
    printf("5. Fast recursive power:\n");
    double pow_res = 0.0;
    st = fast_pow_rec(2.0, 10, eps, &pow_res);
    if (st == ST_OK) {
        printf("   2.0 ^ 10 = %.6f\n", pow_res);
    } else {
        print_error(st);
    }

    st = fast_pow_rec(2.0, -3, eps, &pow_res);
    if (st == ST_OK) {
        printf("   2.0 ^ (-3) = %.6f\n\n", pow_res);
    } else {
        print_error(st);
    }

    /* 6.6: дихотомия */
    printf("6. Bisection root finder:\n");
    double root = 0.0;
    st = find_root_bisection(1.0, 2.0, eps, eq1, &root);
    if (st == ST_OK) {
        printf("   Root of x^2 - 2 = 0 on [1, 2]: %.8f\n", root);
    } else {
        print_error(st);
    }

    st = find_root_bisection(3.0, 4.0, eps, eq2, &root);
    if (st == ST_OK) {
        printf("   Root of sin(x) = 0 on [3, 4]: %.8f\n", root);
    } else {
        print_error(st);
    }

    st = find_root_bisection(1.0, 2.0, eps, eq3, &root);
    if (st == ST_OK) {
        printf("   Root of x^3 - x - 2 = 0 on [1, 2]: %.8f\n", root);
    } else {
        print_error(st);
    }

    return ST_OK;
}
