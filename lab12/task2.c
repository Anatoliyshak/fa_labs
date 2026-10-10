#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

/* разбор вещественного числа */
enum Status parse_double(const char *s, double *res) {
    if (!s || !res || !*s || isspace((unsigned char)*s)) return ST_ERR_NUM;

    char *end;
    double val = strtod(s, &end);

    /* если конец строки не достигнут */
    if (end == s || *end != '\0') return ST_ERR_NUM;

    /* проверка на конечность */
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

/* вычисление e через предел */
enum Status calc_e_lim(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double prev = 0.0;
    double cur = 0.0;
    double n = 1.0;
    int converged = 0;

    for (int iter = 0; iter < 1000000; ++iter) {
        cur = pow(1.0 + 1.0 / n, n);
        if (iter > 0 && fabs(cur - prev) < eps) {
            converged = 1;
            break;
        }
        prev = cur;
        n *= 2.0;
    }

    if (!converged) return ST_ERR_RANGE;

    *res = cur;
    return ST_OK;
}

/* вычисление e через ряд */
enum Status calc_e_series(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double sum = 0.0;
    double term = 1.0;
    int n = 0;
    int converged = 0;

    while (n < 100000) {
        if (term < eps) {
            converged = 1;
            break;
        }
        sum += term;
        ++n;
        term /= (double)n;
    }

    if (!converged) return ST_ERR_RANGE;
    sum += term;

    *res = sum;
    return ST_OK;
}

/* вычисление e через уравнение ln(x) = 1 */
enum Status calc_e_eq(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double a = 2.0;
    double b = 3.0;
    int converged = 0;

    for (int iter = 0; iter < 100000; ++iter) {
        if ((b - a) <= eps) {
            converged = 1;
            break;
        }
        double mid = a + (b - a) / 2.0;
        double f_mid = log(mid) - 1.0;

        if (f_mid < 0.0) {
            a = mid;
        } else {
            b = mid;
        }
    }

    if (!converged) return ST_ERR_RANGE;

    *res = a + (b - a) / 2.0;
    return ST_OK;
}

/* вычисление pi через предел (формула Валлиса) */
enum Status calc_pi_lim(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double cur = 4.0;
    long long n = 2;
    int converged = 0;

    for (int iter = 0; iter < 10000000; ++iter) {
        double mult = (4.0 * (double)n * ((double)n - 1.0)) /
                      ((2.0 * (double)n - 1.0) * (2.0 * (double)n - 1.0));
        double next_val = cur * mult;

        if (fabs(next_val - cur) < eps) {
            cur = next_val;
            converged = 1;
            break;
        }
        cur = next_val;
        ++n;
    }

    if (!converged) return ST_ERR_RANGE;

    *res = cur;
    return ST_OK;
}

/* вычисление pi через ряд Лейбница */
enum Status calc_pi_series(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double sum = 0.0;
    long long n = 1;
    int converged = 0;

    for (int iter = 0; iter < 10000000; ++iter) {
        double term = 4.0 / (2.0 * (double)n - 1.0);
        if (term < eps) {
            converged = 1;
            break;
        }

        if (n % 2 == 1) {
            sum += term;
        } else {
            sum -= term;
        }
        ++n;
    }

    if (!converged) return ST_ERR_RANGE;

    *res = sum;
    return ST_OK;
}

/* вычисление pi через уравнение cos(x) = -1 методом Ньютона */
enum Status calc_pi_eq(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double x = 3.0;
    int converged = 0;

    for (int iter = 0; iter < 1000; ++iter) {
        double s = sin(x);
        if (fabs(s) < eps) {
            converged = 1;
            break;
        }
        double next_x = x + (1.0 + cos(x)) / s;
        if (fabs(next_x - x) < eps) {
            x = next_x;
            converged = 1;
            break;
        }
        x = next_x;
    }

    if (!converged) return ST_ERR_RANGE;

    *res = x;
    return ST_OK;
}

/* вычисление ln(2) через предел */
enum Status calc_ln2_lim(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double prev = 0.0;
    double cur = 0.0;
    double n = 1.0;
    int converged = 0;

    for (int iter = 0; iter < 1000000; ++iter) {
        cur = n * (pow(2.0, 1.0 / n) - 1.0);
        if (iter > 0 && fabs(cur - prev) < eps) {
            converged = 1;
            break;
        }
        prev = cur;
        n *= 2.0;
    }

    if (!converged) return ST_ERR_RANGE;

    *res = cur;
    return ST_OK;
}

/* вычисление ln(2) через знакочередующийся ряд */
enum Status calc_ln2_series(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double sum = 0.0;
    long long n = 1;
    int converged = 0;

    for (int iter = 0; iter < 10000000; ++iter) {
        double term = 1.0 / (double)n;
        if (term < eps) {
            converged = 1;
            break;
        }

        if (n % 2 == 1) {
            sum += term;
        } else {
            sum -= term;
        }
        ++n;
    }

    if (!converged) return ST_ERR_RANGE;

    *res = sum;
    return ST_OK;
}

/* вычисление ln(2) через уравнение e^x = 2 методом бисекции */
enum Status calc_ln2_eq(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double a = 0.0;
    double b = 1.0;
    int converged = 0;

    for (int iter = 0; iter < 100000; ++iter) {
        if ((b - a) <= eps) {
            converged = 1;
            break;
        }
        double mid = a + (b - a) / 2.0;
        double f_mid = exp(mid) - 2.0;

        if (f_mid < 0.0) {
            a = mid;
        } else {
            b = mid;
        }
    }

    if (!converged) return ST_ERR_RANGE;

    *res = a + (b - a) / 2.0;
    return ST_OK;
}

/* вычисление sqrt(2) через заданный предел */
enum Status calc_sqrt2_lim(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double x = -0.5;
    int converged = 0;

    for (int iter = 0; iter < 1000000; ++iter) {
        double next_x = x - (x * x) / 2.0 + 1.0;
        if (fabs(next_x - x) < eps) {
            x = next_x;
            converged = 1;
            break;
        }
        x = next_x;
    }

    if (!converged) return ST_ERR_RANGE;

    *res = x;
    return ST_OK;
}

/* вычисление sqrt(2) через бесконечное произведение */
enum Status calc_sqrt2_prod(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double prod = 1.0;
    int k = 2;
    int converged = 0;

    for (int iter = 0; iter < 10000; ++iter) {
        double factor = pow(2.0, pow(2.0, -k));
        double next_p = prod * factor;

        if (fabs(next_p - prod) < eps) {
            prod = next_p;
            converged = 1;
            break;
        }
        prod = next_p;
        ++k;
    }

    if (!converged) return ST_ERR_RANGE;

    *res = prod;
    return ST_OK;
}

/* вычисление sqrt(2) через уравнение x^2 = 2 методом бисекции */
enum Status calc_sqrt2_eq(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double a = 1.0;
    double b = 2.0;
    int converged = 0;

    for (int iter = 0; iter < 100000; ++iter) {
        if ((b - a) <= eps) {
            converged = 1;
            break;
        }
        double mid = a + (b - a) / 2.0;
        double f_mid = mid * mid - 2.0;

        if (f_mid < 0.0) {
            a = mid;
        } else {
            b = mid;
        }
    }

    if (!converged) return ST_ERR_RANGE;

    *res = a + (b - a) / 2.0;
    return ST_OK;
}

/* вычисление gamma через предел формулы с биномиальными коэффициентами */
enum Status calc_gamma_lim(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double prev_sum = 0.0;
    double cur_sum = 0.0;
    int converged = 0;

    for (int m = 1; m <= 30; ++m) {
        double sum = 0.0;
        double c_m_k = (double)m;
        double log_fact_k = 0.0;

        for (int k = 1; k <= m; ++k) {
            log_fact_k += log((double)k);
            double sign = (k % 2 == 1) ? -1.0 : 1.0;
            double term = c_m_k * (sign / (double)k) * log_fact_k;
            sum += term;

            if (k < m) {
                c_m_k = c_m_k * (double)(m - k) / (double)(k + 1);
            }
        }

        cur_sum = sum;
        if (m > 1 && fabs(cur_sum - prev_sum) < eps) {
            converged = 1;
            break;
        }
        prev_sum = cur_sum;
    }

    if (!converged) return ST_ERR_RANGE;

    *res = cur_sum;
    return ST_OK;
}

/* вычисление gamma через ряд */
enum Status calc_gamma_series(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double sum = - (M_PI * M_PI) / 6.0;
    double prev_sum = sum;
    int converged = 0;

    for (long long k = 2; k < 5000000; ++k) {
        long long root = (long long)sqrt((double)k);
        double term = (1.0 / (double)(root * root)) - (1.0 / (double)k);
        sum += term;

        if (k % 10000 == 0) {
            if (fabs(sum - prev_sum) < eps) {
                converged = 1;
                break;
            }
            prev_sum = sum;
        }
    }

    if (!converged) return ST_ERR_RANGE;

    *res = sum;
    return ST_OK;
}

/* проверка простоты числа для формулы Мертенса */
static int is_prime_number(long long n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (long long d = 3; d <= n / d; d += 2) {
        if (n % d == 0) return 0;
    }
    return 1;
}

/* вычисление gamma через уравнение e^(-x) = lim (ln t * prod (p-1)/p) */
enum Status calc_gamma_eq(const double eps, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    /* вычисление предела произведения Мертенса для t */
    long long t = 20000;
    double prod = 1.0;

    for (long long p = 2; p <= t; ++p) {
        if (is_prime_number(p)) {
            prod *= ((double)(p - 1)) / (double)p;
        }
    }

    double c_val = log((double)t) * prod;
    if (c_val <= 0.0 || c_val >= 1.0) return ST_ERR_RANGE;

    /* решение уравнения e^(-x) - c_val = 0 методом бисекции на [0, 1] */
    double a = 0.0;
    double b = 1.0;
    int converged = 0;

    for (int iter = 0; iter < 100000; ++iter) {
        if ((b - a) <= eps) {
            converged = 1;
            break;
        }
        double mid = a + (b - a) / 2.0;
        double f_mid = exp(-mid) - c_val;

        /* функция exp(-mid) монотонно убывает */
        if (f_mid > 0.0) {
            a = mid;
        } else {
            b = mid;
        }
    }

    if (!converged) return ST_ERR_RANGE;

    *res = a + (b - a) / 2.0;
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
    if (argc != 2) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    double eps = 0.0;
    enum Status st = parse_eps(argv[1], &eps);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    double e_lim = 0.0, e_ser = 0.0, e_eq = 0.0;
    double pi_lim = 0.0, pi_ser = 0.0, pi_eq = 0.0;
    double ln2_lim = 0.0, ln2_ser = 0.0, ln2_eq = 0.0;
    double sqrt2_lim = 0.0, sqrt2_prod = 0.0, sqrt2_eq = 0.0;
    double gamma_lim = 0.0, gamma_ser = 0.0, gamma_eq = 0.0;

    /* e */
    st = calc_e_lim(eps, &e_lim);
    if (st == ST_OK) st = calc_e_series(eps, &e_ser);
    if (st == ST_OK) st = calc_e_eq(eps, &e_eq);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    /* pi */
    st = calc_pi_lim(eps, &pi_lim);
    if (st == ST_OK) st = calc_pi_series(eps, &pi_ser);
    if (st == ST_OK) st = calc_pi_eq(eps, &pi_eq);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    /* ln(2) */
    st = calc_ln2_lim(eps, &ln2_lim);
    if (st == ST_OK) st = calc_ln2_series(eps, &ln2_ser);
    if (st == ST_OK) st = calc_ln2_eq(eps, &ln2_eq);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    /* sqrt(2) */
    st = calc_sqrt2_lim(eps, &sqrt2_lim);
    if (st == ST_OK) st = calc_sqrt2_prod(eps, &sqrt2_prod);
    if (st == ST_OK) st = calc_sqrt2_eq(eps, &sqrt2_eq);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    /* gamma */
    st = calc_gamma_lim(eps, &gamma_lim);
    if (st == ST_OK) st = calc_gamma_series(eps, &gamma_ser);
    if (st == ST_OK) st = calc_gamma_eq(eps, &gamma_eq);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    /* вывод результатов */
    printf("Constant calculation results (eps = %.10g):\n\n", eps);

    printf("e:\n");
    printf("  Limit:    %.10f\n", e_lim);
    printf("  Series:   %.10f\n", e_ser);
    printf("  Equation: %.10f\n\n", e_eq);

    printf("pi:\n");
    printf("  Limit:    %.10f\n", pi_lim);
    printf("  Series:   %.10f\n", pi_ser);
    printf("  Equation: %.10f\n\n", pi_eq);

    printf("ln(2):\n");
    printf("  Limit:    %.10f\n", ln2_lim);
    printf("  Series:   %.10f\n", ln2_ser);
    printf("  Equation: %.10f\n\n", ln2_eq);

    printf("sqrt(2):\n");
    printf("  Limit:    %.10f\n", sqrt2_lim);
    printf("  Product:  %.10f\n", sqrt2_prod);
    printf("  Equation: %.10f\n\n", sqrt2_eq);

    printf("gamma:\n");
    printf("  Limit:    %.10f\n", gamma_lim);
    printf("  Series:   %.10f\n", gamma_ser);
    printf("  Equation: %.10f\n", gamma_eq);

    return ST_OK;
}
