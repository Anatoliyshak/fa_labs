#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
    ST_ERR_NULL_PTR,
    ST_ERR_MEMORY
};

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

/* 9.1: поиск min и max и перестановка их за один проход */
enum Status swap_min_max_single_pass(long long *arr, const int size) {
    if (!arr) return ST_ERR_NULL_PTR;
    if (size <= 0) return ST_ERR_RANGE;

    int min_idx = 0;
    int max_idx = 0;

    for (int i = 1; i < size; ++i) {
        if (arr[i] < arr[min_idx]) {
            min_idx = i;
        }
        if (arr[i] > arr[max_idx]) {
            max_idx = i;
        }
    }

    long long temp = arr[min_idx];
    arr[min_idx] = arr[max_idx];
    arr[max_idx] = temp;

    return ST_OK;
}

/* функция сравнения для qsort */
static int compare_long_long(const void *p1, const void *p2) {
    long long a = *(const long long *)p1;
    long long b = *(const long long *)p2;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

/* безопасное вычисление абсолютной разности */
static unsigned long long safe_diff(long long x, long long y) {
    if (x >= y) {
        return (unsigned long long)x - (unsigned long long)y;
    } else {
        return (unsigned long long)y - (unsigned long long)x;
    }
}

/* двоичный поиск ближайшего по значению элемента в отсортированном массиве */
static long long find_closest_in_sorted(const long long *sorted_arr, const int size, const long long target) {
    if (target <= sorted_arr[0]) return sorted_arr[0];
    if (target >= sorted_arr[size - 1]) return sorted_arr[size - 1];

    int left = 0;
    int right = size - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (sorted_arr[mid] == target) {
            return sorted_arr[mid];
        }
        if (sorted_arr[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    /* ближайший находится либо в sorted_arr[right], либо в sorted_arr[left] */
    unsigned long long diff_left = safe_diff(target, sorted_arr[right]);
    unsigned long long diff_right = safe_diff(target, sorted_arr[left]);

    if (diff_left <= diff_right) {
        return sorted_arr[right];
    } else {
        return sorted_arr[left];
    }
}

/* 9.2: формирование массива C на основе массивов A и B */
enum Status build_array_c(const long long *arr_a, const int size_a,
                          const long long *arr_b, const int size_b,
                          long long **arr_c) {
    if (!arr_a || !arr_b || !arr_c) return ST_ERR_NULL_PTR;
    if (size_a <= 0 || size_b <= 0) return ST_ERR_RANGE;

    /* создание отсортированной копии массива B для быстрого поиска ближайшего за O(log N) */
    long long *sorted_b = (long long *)malloc((size_t)size_b * sizeof(long long));
    if (!sorted_b) return ST_ERR_MEMORY;

    for (int i = 0; i < size_b; ++i) {
        sorted_b[i] = arr_b[i];
    }
    qsort(sorted_b, (size_t)size_b, sizeof(long long), compare_long_long);

    long long *res_c = (long long *)malloc((size_t)size_a * sizeof(long long));
    if (!res_c) {
        free(sorted_b);
        return ST_ERR_MEMORY;
    }

    for (int i = 0; i < size_a; ++i) {
        long long closest_b = find_closest_in_sorted(sorted_b, size_b, arr_a[i]);
        if ((closest_b > 0 && arr_a[i] > LLONG_MAX - closest_b) ||
            (closest_b < 0 && arr_a[i] < LLONG_MIN - closest_b)) {
            free(sorted_b);
            free(res_c);
            return ST_ERR_OVERFLOW;
        }
        res_c[i] = arr_a[i] + closest_b;
    }

    free(sorted_b);
    *arr_c = res_c;
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
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        print_error(ST_ERR_ARGS);
        return ST_ERR_ARGS;
    }

    long long a = 0;
    long long b = 0;

    enum Status st = parse_long(argv[1], &a);
    if (st == ST_OK) {
        st = parse_long(argv[2], &b);
    }
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    if (a > b) {
        print_error(ST_ERR_RANGE);
        return ST_ERR_RANGE;
    }

    unsigned long long span = (unsigned long long)b - (unsigned long long)a + 1ULL;
    if (span == 0ULL) {
        print_error(ST_ERR_OVERFLOW);
        return ST_ERR_OVERFLOW;
    }

    /* Для демонстрации используется rand()*/
    srand((unsigned int)time(NULL));

    /*  9.1: Фиксированный массив  */
    printf("=== Task 9.1: Fixed size array ===\n");
    const int FIXED_SIZE = 20;
    long long fixed_arr[20];

    for (int i = 0; i < FIXED_SIZE; ++i) {
        fixed_arr[i] = a + (long long)(((unsigned long long)rand() | ((unsigned long long)rand() << 15)) % span);
    }

    printf("Original array:\n");
    for (int i = 0; i < FIXED_SIZE; ++i) {
        printf("%lld%s", fixed_arr[i], (i + 1 == FIXED_SIZE) ? "\n" : " ");
    }

    st = swap_min_max_single_pass(fixed_arr, FIXED_SIZE);
    if (st != ST_OK) {
        print_error(st);
        return st;
    }

    printf("Array after swapping min and max (single pass):\n");
    for (int i = 0; i < FIXED_SIZE; ++i) {
        printf("%lld%s", fixed_arr[i], (i + 1 == FIXED_SIZE) ? "\n" : " ");
    }

    /*  9.2: Динамические массивы A, B и C  */
    printf("\n=== Task 9.2: Dynamic arrays A, B, and C ===\n");

    /* псевдослучайные размеры в диапазоне [10..10000] */
    int size_a = 10 + (rand() % (10000 - 10 + 1));
    int size_b = 10 + (rand() % (10000 - 10 + 1));

    long long *arr_a = (long long *)malloc((size_t)size_a * sizeof(long long));
    long long *arr_b = (long long *)malloc((size_t)size_b * sizeof(long long));

    if (!arr_a || !arr_b) {
        if (arr_a) free(arr_a);
        if (arr_b) free(arr_b);
        print_error(ST_ERR_MEMORY);
        return ST_ERR_MEMORY;
    }

    /* заполнение числами в диапазоне [-1000..1000] */
    for (int i = 0; i < size_a; ++i) {
        arr_a[i] = -1000 + (rand() % 2001);
    }
    for (int i = 0; i < size_b; ++i) {
        arr_b[i] = -1000 + (rand() % 2001);
    }

    long long *arr_c = NULL;
    st = build_array_c(arr_a, size_a, arr_b, size_b, &arr_c);
    if (st != ST_OK) {
        free(arr_a);
        free(arr_b);
        print_error(st);
        return st;
    }

    printf("Generated size of A: %d\n", size_a);
    printf("Generated size of B: %d\n", size_b);

    int preview_count = size_a < 10 ? size_a : 10;
    printf("\nFirst %d elements preview:\n", preview_count);
    printf("A: ");
    for (int i = 0; i < preview_count; ++i) {
        printf("%lld%s", arr_a[i], (i + 1 == preview_count) ? "\n" : " ");
    }

    int preview_b = size_b < 10 ? size_b : 10;
    printf("B: ");
    for (int i = 0; i < preview_b; ++i) {
        printf("%lld%s", arr_b[i], (i + 1 == preview_b) ? "\n" : " ");
    }

    printf("C: ");
    for (int i = 0; i < preview_count; ++i) {
        printf("%lld%s", arr_c[i], (i + 1 == preview_count) ? "\n" : " ");
    }

    free(arr_a);
    free(arr_b);
    free(arr_c);

    return ST_OK;
}
