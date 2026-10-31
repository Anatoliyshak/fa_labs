#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

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

/* чистая функция: переразложение многочлена по степеням (x - a) */
/* f(x) = f_0 + f_1*x + ... + f_n*x^n = g_0 + g_1*(x - a) + ... + g_n*(x - a)^n */
enum Status reexpand_polynomial(const double eps, const double a,
                                double **out_g, const size_t degree, ...) {
    if (!out_g) return ST_ERR_NULL_PTR;
    if (!isfinite(eps) || eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;
    if (!isfinite(a)) return ST_ERR_OVERFLOW;

    *out_g = NULL;

    const size_t count = degree + 1;

    /* считывание исходных коэффициентов из va_list */
    double *f = (double *)malloc(count * sizeof(double));
    if (!f) return ST_ERR_MEMORY;

    va_list ap;
    va_start(ap, degree);
    for (size_t i = 0; i < count; ++i) {
        f[i] = va_arg(ap, double);
        if (!isfinite(f[i])) {
            va_end(ap);
            free(f);
            return ST_ERR_OVERFLOW;
        }
    }
    va_end(ap);

    /* рабочий массив для обобщенной схемы Горнера (деление столбиком) */
    /* размещаем коэффициенты в порядке убывания степеней x: c[0] = f_n, ..., c[n] = f_0 */
    double *c = (double *)malloc(count * sizeof(double));
    if (!c) {
        free(f);
        return ST_ERR_MEMORY;
    }

    for (size_t i = 0; i < count; ++i) {
        c[i] = f[degree - i];
    }
    free(f);

    /* массив результирующих коэффициентов g_0, g_1, ..., g_n */
    double *g = (double *)malloc(count * sizeof(double));
    if (!g) {
        free(c);
        return ST_ERR_MEMORY;
    }

    /* алгоритм Руффини-Горнера: последовательное деление на (x - a) */
    for (size_t step = 0; step < count; ++step) {
        const size_t cur_len = count - step;
        for (size_t k = 1; k < cur_len; ++k) {
            c[k] += a * c[k - 1];
            if (!isfinite(c[k])) {
                free(c);
                free(g);
                return ST_ERR_OVERFLOW;
            }
        }
        /* остаток от деления равен соответствующему коэффициенту Тейлора g_step */
        double rem = c[cur_len - 1];
        if (fabs(rem) < eps) {
            rem = 0.0;
        }
        g[step] = rem;
    }

    free(c);

    *out_g = g;
    return ST_OK;
}

/* вывод многочлена в читаемом виде */
static void print_polynomial_x(const double *f, const size_t degree, const double eps) {
    printf("f(x) = ");
    for (size_t i = 0; i <= degree; ++i) {
        if (i == 0) {
            printf("%.2f", f[i]);
        } else {
            const char *sign_str = (f[i] >= -eps) ? " + " : " - ";
            if (i == 1) {
                printf("%s%.2f*x", sign_str, fabs(f[i]));
            } else {
                printf("%s%.2f*x^%zu", sign_str, fabs(f[i]), i);
            }
        }
    }
    printf("\n");
}

/* вывод разложения по степеням (x - a) */
static void print_polynomial_g(const double *g, const size_t degree, const double a, const double eps) {
    printf("g(x-a) = ");
    for (size_t i = 0; i <= degree; ++i) {
        if (i == 0) {
            printf("%.2f", g[i]);
        } else {
            const char *sign_str = (g[i] >= -eps) ? " + " : " - ";
            char term_buf[64];
            if (fabs(a) < eps) {
                snprintf(term_buf, sizeof(term_buf), "*x");
            } else if (a > 0.0) {
                snprintf(term_buf, sizeof(term_buf), "*(x - %.2f)", a);
            } else {
                snprintf(term_buf, sizeof(term_buf), "*(x + %.2f)", fabs(a));
            }

            if (i == 1) {
                printf("%s%.2f%s", sign_str, fabs(g[i]), term_buf);
            } else {
                printf("%s%.2f%s^%zu", sign_str, fabs(g[i]), term_buf, i);
            }
        }
    }
    printf("\n");
}

int main(void) {
    printf("=== Demonstration of Polynomial Re-expansion in Powers of (x - a) ===\n\n");

    const double eps = 1e-9;

    /* Пример 1: f(x) = 1 + 2x + 3x^2, a = 1 */
    /* Ожидается: g_0 = 6, g_1 = 8, g_2 = 3 */
    {
        const double a1 = 1.0;
        const size_t deg1 = 2;
        const double f1[] = { 1.0, 2.0, 3.0 };

        double *g1 = NULL;
        enum Status st = reexpand_polynomial(eps, a1, &g1, deg1, f1[0], f1[1], f1[2]);
        if (st == ST_OK) {
            printf("--- Example 1 (degree 2, a = %.2f) ---\n", a1);
            print_polynomial_x(f1, deg1, eps);
            print_polynomial_g(g1, deg1, a1, eps);
            printf("Coefficients g: g_0 = %.4f, g_1 = %.4f, g_2 = %.4f\n\n",
                   g1[0], g1[1], g1[2]);
            free(g1);
        }
    }

    /* Пример 2: f(x) = -2 + 0*x + 0*x^2 + 1*x^3 (x^3 - 2), a = 2 */
    /* Ожидается: g_0 = 6, g_1 = 12, g_2 = 6, g_3 = 1 */
    {
        const double a2 = 2.0;
        const size_t deg2 = 3;
        const double f2[] = { -2.0, 0.0, 0.0, 1.0 };

        double *g2 = NULL;
        enum Status st = reexpand_polynomial(eps, a2, &g2, deg2, f2[0], f2[1], f2[2], f2[3]);
        if (st == ST_OK) {
            printf("--- Example 2 (degree 3, a = %.2f) ---\n", a2);
            print_polynomial_x(f2, deg2, eps);
            print_polynomial_g(g2, deg2, a2, eps);
            printf("Coefficients g: g_0 = %.4f, g_1 = %.4f, g_2 = %.4f, g_3 = %.4f\n\n",
                   g2[0], g2[1], g2[2], g2[3]);
            free(g2);
        }
    }

    /* Пример 3: f(x) = 5 - 4x + 3x^2 - 2x^3 + x^4, a = -1 */
    {
        const double a3 = -1.0;
        const size_t deg3 = 4;
        const double f3[] = { 5.0, -4.0, 3.0, -2.0, 1.0 };

        double *g3 = NULL;
        enum Status st = reexpand_polynomial(eps, a3, &g3, deg3,
                                             f3[0], f3[1], f3[2], f3[3], f3[4]);
        if (st == ST_OK) {
            printf("--- Example 3 (degree 4, a = %.2f) ---\n", a3);
            print_polynomial_x(f3, deg3, eps);
            print_polynomial_g(g3, deg3, a3, eps);
            printf("Coefficients g: ");
            for (size_t i = 0; i <= deg3; ++i) {
                printf("g_%zu = %.4f%s", i, g3[i], i < deg3 ? ", " : "\n\n");
            }
            free(g3);
        }
    }

    return (int)ST_OK;
}

