#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <float.h>

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

/* наибольший общий делитель двух целых чисел */
static long long gcd(long long a, long long b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        long long rem = a % b;
        a = b;
        b = rem;
    }
    return a;
}

/* нахождение несократимой дроби через непрерывные дроби */
static enum Status float_to_fraction(const double x, const double eps,
                                     long long *out_p, long long *out_q) {
    if (out_p == NULL || out_q == NULL) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;
    if (!isfinite(x)) return ST_ERR_RANGE;

    /* проверка вхождения в интервал 0, 1*/
    if (x <= eps || x >= 1.0 - eps) return ST_ERR_RANGE;

    /* машинная епс для защиты от деления на ноль при исчезновении остатка */
    const double machine_eps = DBL_EPSILON * 8.0;

    double rem = x;
    long long p0 = 0, q0 = 1;
    long long p1 = 1, q1 = 0;

    for (int iter = 0; iter < 64; ++iter) {
        const double a_floor = floor(rem);

        /* защита от переполнения при приведении к целому */
        if (!isfinite(a_floor) || a_floor >= (double)LLONG_MAX) {
            return ST_ERR_OVERFLOW;
        }

        const long long a = (long long)a_floor;

        /* контроль переполнения до выполнения умножения и сложения */
        if (p1 != 0 && a > (LLONG_MAX - p0) / p1) return ST_ERR_OVERFLOW;
        if (q1 != 0 && a > (LLONG_MAX - q0) / q1) return ST_ERR_OVERFLOW;

        const long long p2 = a * p1 + p0;
        const long long q2 = a * q1 + q0;

        if (q2 <= 0) return ST_ERR_OVERFLOW;

        const double approx = (double)p2 / (double)q2;
        if (fabs(x - approx) < eps) {
            *out_p = p2;
            *out_q = q2;
            return ST_OK;
        }

        p0 = p1; q0 = q1;
        p1 = p2; q1 = q2;

        const double diff = rem - a_floor;
        if (diff < machine_eps) break;

        rem = 1.0 / diff;
    }

    /* проверка достижения точности после завершения цикла */
    const double final_approx = (double)p1 / (double)q1;
    if (fabs(x - final_approx) >= eps) return ST_ERR_NUM;

    *out_p = p1;
    *out_q = q1;
    return ST_OK;
}

/* имеет ли дробь p/q конечное представление в сс */
/* все простые делители знаменателя q обязаны делить основание */
static int is_fraction_finite_in_base(long long q, const int base) {
    if (q <= 0 || base < 2) return 0;

    long long g;
    while ((g = gcd(q, (long long)base)) > 1) {
        q /= g;
    }

    return (q == 1);
}

/* проверка конечного представления одного числа */
enum Status is_finite_in_base(const double x, const int base,
                              const double eps, int *result) {
    if (result == NULL) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;
    if (base < 2) return ST_ERR_RANGE;
    if (!isfinite(x)) return ST_ERR_RANGE;

    if (x <= eps || x >= 1.0 - eps) return ST_ERR_RANGE;

    long long p = 0, q = 1;
    enum Status st = float_to_fraction(x, eps, &p, &q);
    if (st != ST_OK) return st;

    *result = is_fraction_finite_in_base(q, base);
    return ST_OK;
}

/* основная функция */
enum Status check_finite_fractions(const int base, const double eps,
                                   int **out_results, const size_t count, ...) {
    if (out_results == NULL) return ST_ERR_NULL_PTR;
    if (base < 2) return ST_ERR_RANGE;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    *out_results = NULL;
    if (count == 0) return ST_OK;

    int *results = (int *)malloc(count * sizeof(int));
    if (!results) return ST_ERR_MEMORY;

    va_list ap;
    va_start(ap, count);

    enum Status st = ST_OK;
    for (size_t i = 0; i < count; ++i) {
        const double frac = va_arg(ap, double);
        int r = 0;
        enum Status local_st = is_finite_in_base(frac, base, eps, &r);
        if (local_st == ST_OK) {
            results[i] = r;
        } else if (local_st == ST_ERR_RANGE) {
            /* ошибки для чисел вне диапазона (0, 1) */
            results[i] = -1;
        } else {
            /* переполнение, расходимость */
            st = local_st;
            break;
        }
    }

    va_end(ap);

    if (st != ST_OK) {
        free(results);
        return st;
    }

    *out_results = results;
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

int main(void) {
    const double eps = 1e-10;

    const size_t count = 7;
    const double fractions[] = {
        0.5,
        0.2,
        0.125,
        1.0 / 3.0,
        0.1,
        -0.25,
        1.5
    };

    const int bases[] = { 10, 2, 3 };
    const size_t nbases = sizeof(bases) / sizeof(bases[0]);

    for (size_t b = 0; b < nbases; ++b) {
        int *res = NULL;
        enum Status st = check_finite_fractions(bases[b], eps, &res, count,
                                                fractions[0], fractions[1],
                                                fractions[2], fractions[3],
                                                fractions[4], fractions[5],
                                                fractions[6]);
        if (st == ST_OK) {
            printf("base %d results\n", bases[b]);
            for (size_t i = 0; i < count; ++i) {
                if (res[i] < 0) {
                    printf("  fraction %+8.6f not in range (0, 1)\n", fractions[i]);
                } else if (res[i] == 1) {
                    printf("  fraction %+8.6f y\n", fractions[i]);
                } else {
                    printf("  fraction %+8.6f n\n", fractions[i]);
                }
            }
            free(res);
        } else {
            printf("base %d operation fail: ", bases[b]);
            print_error(st);
        }
        printf("\n");
    }

    return (int)ST_OK;
}