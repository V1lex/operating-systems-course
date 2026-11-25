#include <stdlib.h>

int is_prime(int number) {
    if (number < 2) {
        return 0;
    }

    for (int divisor = 2; divisor < number; ++divisor) {
        if (number % divisor == 0) {
            return 0;
        }
    }

    return 1;
}

int PrimeCount(int A, int B) {
    if (A < 1 || B < A) {
        return -1;
    }

    int count = 0;

    for (int number = A; number < B; ++number) {
        if (is_prime(number)) {
            ++count;
        }
    }

    if (is_prime(B)) {
        ++count;
    }

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
        int remainder = x % 2;

        if (remainder < 0) {
            remainder = -remainder;
        }

        digits[length] = '0' + remainder;
        ++length;
        x /= 2;
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
