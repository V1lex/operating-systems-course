#include <stdlib.h>

int PrimeCount(int A, int B) {
    if (A < 1 || B < A) {
        return -1;
    }

    size_t size = (size_t)B + 1;
    char* is_composite = calloc(size, 1);

    if (is_composite == NULL) {
        return -1;
    }

    for (int number = 2; number <= B / number; ++number) {
        if (is_composite[number] == 0) {
            for (int multiplier = number; multiplier <= B / number; ++multiplier) {
                is_composite[number * multiplier] = 1;
            }
        }
    }

    int count = 0;

    for (int number = A; number < B; ++number) {
        if (number >= 2 && is_composite[number] == 0) {
            ++count;
        }
    }

    if (B >= 2 && is_composite[B] == 0) {
        ++count;
    }

    free(is_composite);
    return count;
}

char* translation(long x) {
    char digits[sizeof(long) * 8];
    size_t length = 0;
    int negative = 0;

    if (x < 0) {
        negative = 1;
    }

    do {
        int remainder = x % 3;

        if (remainder < 0) {
            remainder = -remainder;
        }

        digits[length] = '0' + remainder;
        ++length;
        x /= 3;
    } while (x != 0);

    char* result = malloc(length + negative + 1);

    if (result == NULL) {
        return NULL;
    }

    size_t position = 0;

    if (negative == 1) {
        result[position] = '-';
        ++position;
    }

    for (size_t index = length; index > 0; --index) {
        result[position] = digits[index - 1];
        ++position;
    }

    result[position] = '\0';
    return result;
}
