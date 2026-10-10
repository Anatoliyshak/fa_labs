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

/* разбор вещественного числа */
enum Status parse_double(const char *s, double *res) {
    if (!s || !res || !*s || isspace((unsigned char)*s)) return ST_ERR_NUM;

    char *end;
    double val = strtod(s, &end);

    if (end == s || *end != '\0') return ST_ERR_NUM;
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

/* вычисление суммы a: sum x^n / n! */
enum Status calc_sum_a(const double eps, const double x, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double sum = 0.0;
    double term = 1.0;
    long long n = 0;

    while (fabs(term) >= eps && n < 100000) {
        if (!isfinite(term) || !isfinite(sum)) return ST_ERR_OVERFLOW;
        sum += term;
        ++n;
        term = term * x / (double)n;
    }

    if (!isfinite(term) || !isfinite(sum)) return ST_ERR_OVERFLOW;
    if (fabs(term) >= eps) return ST_ERR_RANGE;

    sum += term;
    *res = sum;
    return ST_OK;
}

/* вычисление суммы b: sum (-1)^n * x^(2n) / (2n)! */
enum Status calc_sum_b(const double eps, const double x, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    double sum = 0.0;
    double term = 1.0;
    long long n = 0;
    const double x2 = x * x;

    while (fabs(term) >= eps && n < 100000) {
        if (!isfinite(term) || !isfinite(sum)) return ST_ERR_OVERFLOW;
        sum += term;
        ++n;
        term = -term * x2 / ((2.0 * (double)n - 1.0) * (2.0 * (double)n));
    }

    if (!isfinite(term) || !isfinite(sum)) return ST_ERR_OVERFLOW;
    if (fabs(term) >= eps) return ST_ERR_RANGE;

    sum += term;
    *res = sum;
    return ST_OK;
}

/* вычисление суммы c: sum 3^(3n) * (n!)^3 * x^(2n) / (3n)! */
enum Status calc_sum_c(const double eps, const double x, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    /* радиус сходимости данного ряда |x| < 1 */
    if (fabs(x) >= 1.0 - eps) return ST_ERR_RANGE;

    double sum = 0.0;
    double term = 1.0;
    long long n = 0;
    const double x2 = x * x;

    while (fabs(term) >= eps && n < 1000000) {
        if (!isfinite(term) || !isfinite(sum)) return ST_ERR_OVERFLOW;
        sum += term;
        double numerator = 9.0 * ((double)n + 1.0) * ((double)n + 1.0) * x2;
        double denominator = (3.0 * (double)n + 1.0) * (3.0 * (double)n + 2.0);
        term = term * (numerator / denominator);
        ++n;
    }

    if (!isfinite(term) || !isfinite(sum)) return ST_ERR_OVERFLOW;
    if (fabs(term) >= eps) return ST_ERR_RANGE;

    sum += term;
    *res = sum;
    return ST_OK;
}

/* вычисление суммы d: sum_{n=1}^inf (-1)^n * (2n-1)!! * x^(2n) / (2n)!! */
enum Status calc_sum_d(const double eps, const double x, double *res) {
    if (!res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    /* радиус сходимости данного ряда |x| < 1 */
    if (fabs(x) >= 1.0 - eps) return ST_ERR_RANGE;

    double sum = 0.0;
    const double x2 = x * x;
    double term = - x2 / 2.0;
    long long n = 1;

    while (fabs(term) >= eps && n < 1000000) {
        if (!isfinite(term) || !isfinite(sum)) return ST_ERR_OVERFLOW;
        sum += term;
        double mult = - ((2.0 * (double)n + 1.0) / (2.0 * (double)n + 2.0)) * x2;
        term = term * mult;
        ++n;
    }

    if (!isfinite(term) || !isfinite(sum)) return ST_ERR_OVERFLOW;
    if (fabs(term) >= eps) return ST_ERR_RANGE;

    sum += term;
    *res = sum;
    return ST_OK;
}

/* подынтегральная функция a: ln(1+x) / x */
static double integrand_a(double t) {
    if (t < 1e-15) return 1.0;
    return log1p(t) / t;
}

/* подынтегральная функция b: exp(-x^2 / 2) */
static double integrand_b(double t) {
    return exp(- (t * t) / 2.0);
}

/* подынтегральная функция c с заменой x = 1 - u^2: -4 * u * ln(u) */
static double integrand_c_regularized(double u) {
    if (u < 1e-15) return 0.0;
    return -4.0 * u * log(u);
}

/* подынтегральная функция d: x^x */
static double integrand_d(double t) {
    if (t < 1e-15) return 1.0;
    return pow(t, t);
}

/* составная формула Симпсона на отрезке [a, b] с числом разбиений n */
static double simpson_rule(double (*f)(double), double a, double b, int n) {
    double h = (b - a) / (double)n;
    double sum = f(a) + f(b);

    for (int i = 1; i < n; ++i) {
        double x = a + (double)i * h;
        if (i % 2 == 1) {
            sum += 4.0 * f(x);
        } else {
            sum += 2.0 * f(x);
        }
    }

    return sum * h / 3.0;
}

/* адаптивное интегрирование методом Симпсона с оценкой Рунге */
enum Status integrate_simpson(double (*f)(double), double a, double b, const double eps, double *res) {
    if (!f || !res) return ST_ERR_NULL_PTR;
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    int n = 8;
    double prev = simpson_rule(f, a, b, n);
    double cur = prev;
    int converged = 0;

    for (int iter = 0; iter < 24; ++iter) {
        n *= 2;
        cur = simpson_rule(f, a, b, n);
        if (!isfinite(cur)) return ST_ERR_OVERFLOW;

        double err = fabs(cur - prev) / 15.0;

        if (err < eps) {
            converged = 1;
            *res = cur;
            return ST_OK;
        }
        prev = cur;
    }

    if (!converged) return ST_ERR_OVERFLOW;

    *res = cur;
    return ST_OK;
}

/* вычисление интеграла a */
enum Status calc_integral_a(const double eps, double *res) {
    return integrate_simpson(integrand_a, 0.0, 1.0, eps, res);
}

/* вычисление интеграла b */
enum Status calc_integral_b(const double eps, double *res) {
    return integrate_simpson(integrand_b, 0.0, 1.0, eps, res);
}

/* вычисление интеграла c */
enum Status calc_integral_c(const double eps, double *res) {
    return integrate_simpson(integrand_c_regularized, 0.0, 1.0, eps, res);
}

/* вычисление интеграла d */
enum Status calc_integral_d(const double eps, double *res) {
    return integrate_simpson(integrand_d, 0.0, 1.0, eps, res);
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
    if (argc < 2 || argc > 3) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    double eps = 0.0;
    enum Status st = parse_eps(argv[1], &eps);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    double x = 0.5;
    if (argc == 3) {
        st = parse_double(argv[2], &x);
        if (st != ST_OK) {
            print_error(st);
            return st;
        }
    }

    double sum_a = 0.0, sum_b = 0.0, sum_c = 0.0, sum_d = 0.0;
    double int_a = 0.0, int_b = 0.0, int_c = 0.0, int_d = 0.0;

    printf("=== Task 5.1: Series sums (eps = %.10g, x = %.6g) ===\n", eps, x);

    st = calc_sum_a(eps, x, &sum_a);
    if (st == ST_OK) {
        printf("Sum (a): %.10f\n", sum_a);
    } else {
        printf("Sum (a): ");
        print_error(st);
    }

    st = calc_sum_b(eps, x, &sum_b);
    if (st == ST_OK) {
        printf("Sum (b): %.10f\n", sum_b);
    } else {
        printf("Sum (b): ");
        print_error(st);
    }

    st = calc_sum_c(eps, x, &sum_c);
    if (st == ST_OK) {
        printf("Sum (c): %.10f\n", sum_c);
    } else {
        printf("Sum (c): ");
        print_error(st);
    }

    st = calc_sum_d(eps, x, &sum_d);
    if (st == ST_OK) {
        printf("Sum (d): %.10f\n", sum_d);
    } else {
        printf("Sum (d): ");
        print_error(st);
    }

    printf("\n=== Task 5.2: Definite integrals (eps = %.10g) ===\n", eps);

    st = calc_integral_a(eps, &int_a);
    if (st == ST_OK) {
        printf("Integral (a): %.10f\n", int_a);
    } else {
        printf("Integral (a): ");
        print_error(st);
    }

    st = calc_integral_b(eps, &int_b);
    if (st == ST_OK) {
        printf("Integral (b): %.10f\n", int_b);
    } else {
        printf("Integral (b): ");
        print_error(st);
    }

    st = calc_integral_c(eps, &int_c);
    if (st == ST_OK) {
        printf("Integral (c): %.10f\n", int_c);
    } else {
        printf("Integral (c): ");
        print_error(st);
    }

    st = calc_integral_d(eps, &int_d);
    if (st == ST_OK) {
        printf("Integral (d): %.10f\n", int_d);
    } else {
        printf("Integral (d): ");
        print_error(st);
    }

    return ST_OK;
}
