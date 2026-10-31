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

/* структура вектора в n-мерном пространстве */
struct Vector {
    double *coords;
    size_t dim;
    size_t id;          /* идентификатор вектора для наглядности */
};

/* универсальный тип указателя на функцию вычисления нормы */
typedef double (*NormFunc)(const struct Vector *v, const void *params);

/* результат поиска самых длинных векторов по одной норме */
struct NormMaxResult {
    const struct Vector **longest_vectors;
    size_t count;
    double max_norm_val;
};

/* 1. Норма бесконечность: ||x||_inf = max_j |x_j| */
double norm_inf(const struct Vector *v, const void *params) {
    (void)params;
    if (!v || !v->coords || v->dim == 0) return 0.0;

    double max_val = fabs(v->coords[0]);
    for (size_t j = 1; j < v->dim; ++j) {
        const double cur = fabs(v->coords[j]);
        if (cur > max_val) {
            max_val = cur;
        }
    }
    return max_val;
}

/* 2. p-норма: ||x||_p = (sum |x_j|^p)^(1/p), p >= 1 */
double norm_p(const struct Vector *v, const void *params) {
    if (!v || !v->coords || v->dim == 0 || !params) return 0.0;

    const double p = *(const double *)params;
    if (p < 1.0) return 0.0;

    double sum = 0.0;
    for (size_t j = 0; j < v->dim; ++j) {
        sum += pow(fabs(v->coords[j]), p);
    }
    return pow(sum, 1.0 / p);
}

/* 3. Матричная норма: ||x||_A = sqrt((Ax, x)), A - матрица n x n */
double norm_matrix(const struct Vector *v, const void *params) {
    if (!v || !v->coords || v->dim == 0 || !params) return 0.0;

    const double *a = (const double *)params;
    const size_t n = v->dim;

    double quad_form = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double ax_i = 0.0;
        for (size_t j = 0; j < n; ++j) {
            ax_i += a[i * n + j] * v->coords[j];
        }
        quad_form += ax_i * v->coords[i];
    }

    if (quad_form < 0.0) return 0.0;
    return sqrt(quad_form);
}

/* освобождение результата */
void free_norm_results(struct NormMaxResult *results, const size_t count) {
    if (!results) return;
    for (size_t i = 0; i < count; ++i) {
        if (results[i].longest_vectors) {
            free(results[i].longest_vectors);
            results[i].longest_vectors = NULL;
        }
    }
    free(results);
}

/* чистая функция: вычисление длиннейших векторов для списка норм */
enum Status find_longest_vectors(const size_t dim,
                                 const size_t vec_count,
                                 const struct Vector *vectors,
                                 const double eps,
                                 struct NormMaxResult **out_results,
                                 const size_t norm_count, ...) {
    if (dim == 0 || vec_count == 0 || !vectors || !out_results) {
        return ST_ERR_NULL_PTR;
    }
    if (eps <= 0.0 || eps >= 1.0) return ST_ERR_RANGE;

    *out_results = NULL;
    if (norm_count == 0) return ST_OK;

    struct NormMaxResult *results = (struct NormMaxResult *)malloc(norm_count * sizeof(struct NormMaxResult));
    if (!results) return ST_ERR_MEMORY;

    for (size_t i = 0; i < norm_count; ++i) {
        results[i].longest_vectors = NULL;
        results[i].count = 0;
        results[i].max_norm_val = 0.0;
    }

    va_list ap;
    va_start(ap, norm_count);

    for (size_t n_idx = 0; n_idx < norm_count; ++n_idx) {
        NormFunc func = va_arg(ap, NormFunc);
        const void *params = va_arg(ap, const void *);

        if (!func) {
            va_end(ap);
            free_norm_results(results, norm_count);
            return ST_ERR_NULL_PTR;
        }

        double *norms = (double *)malloc(vec_count * sizeof(double));
        if (!norms) {
            va_end(ap);
            free_norm_results(results, norm_count);
            return ST_ERR_MEMORY;
        }

        double max_val = -1.0;
        for (size_t v = 0; v < vec_count; ++v) {
            norms[v] = func(&vectors[v], params);
            if (norms[v] > max_val) {
                max_val = norms[v];
            }
        }

        /* подсчет количества векторов с максимумом нормы через параметр eps */
        size_t max_count = 0;
        for (size_t v = 0; v < vec_count; ++v) {
            if (fabs(norms[v] - max_val) < eps) {
                ++max_count;
            }
        }

        const struct Vector **longest = (const struct Vector **)malloc(max_count * sizeof(struct Vector *));
        if (!longest) {
            free(norms);
            va_end(ap);
            free_norm_results(results, norm_count);
            return ST_ERR_MEMORY;
        }

        size_t k = 0;
        for (size_t v = 0; v < vec_count; ++v) {
            if (fabs(norms[v] - max_val) < eps) {
                longest[k++] = &vectors[v];
            }
        }

        free(norms);

        results[n_idx].longest_vectors = longest;
        results[n_idx].count = max_count;
        results[n_idx].max_norm_val = max_val;
    }

    va_end(ap);
    *out_results = results;
    return ST_OK;
}

/* вывод вектора */
static void print_vector(const struct Vector *v) {
    printf("Vector #%zu: (", v->id);
    for (size_t i = 0; i < v->dim; ++i) {
        printf("%s%.2f", i ? ", " : "", v->coords[i]);
    }
    printf(")\n");
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
    printf("=== Demonstration of Vector Norms Calculation ===\n\n");

    const double eps = 1e-9;
    const size_t dim = 3;
    const size_t vec_count = 4;

    double c1[] = { 1.0, 2.0, 3.0 };
    double c2[] = { -3.0, 0.0, 1.0 };
    double c3[] = { 2.0, 2.0, 2.0 };
    double c4[] = { 0.0, 3.0, -1.0 };

    struct Vector vectors[4];
    vectors[0].coords = c1; vectors[0].dim = dim; vectors[0].id = 1;
    vectors[1].coords = c2; vectors[1].dim = dim; vectors[1].id = 2;
    vectors[2].coords = c3; vectors[2].dim = dim; vectors[2].id = 3;
    vectors[3].coords = c4; vectors[3].dim = dim; vectors[3].id = 4;

    printf("Input vectors:\n");
    for (size_t i = 0; i < vec_count; ++i) {
        print_vector(&vectors[i]);
    }
    printf("\n");

    const double p_val = 2.0;

    const double matrix_a[9] = {
        2.0, 0.0, 0.0,
        0.0, 3.0, 0.0,
        0.0, 0.0, 1.0
    };

    const size_t norm_count = 3;
    struct NormMaxResult *results = NULL;

    const enum Status st = find_longest_vectors(dim, vec_count, vectors, eps, &results, norm_count,
                                                norm_inf, (const void *)NULL,
                                                norm_p, (const void *)&p_val,
                                                norm_matrix, (const void *)matrix_a);

    if (st != ST_OK) {
        print_error(st);
        return (int)st;
    }

    const char *norm_names[] = {
        "1. Norm Infinity (max |x_j|)",
        "2. Norm p (p = 2.0, Euclidean)",
        "3. Norm Matrix A (sqrt(Ax, x))"
    };

    for (size_t i = 0; i < norm_count; ++i) {
        printf("--- %s ---\n", norm_names[i]);
        printf("Max norm value: %.6f\n", results[i].max_norm_val);
        printf("Longest vectors (%zu found):\n", results[i].count);
        for (size_t k = 0; k < results[i].count; ++k) {
            printf("  ");
            print_vector(results[i].longest_vectors[k]);
        }
        printf("\n");
    }

    free_norm_results(results, norm_count);
    return (int)ST_OK;
}
