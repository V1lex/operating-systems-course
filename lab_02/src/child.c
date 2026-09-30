#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define GENERAL_ERROR_EXIT_CODE 1
#define DIVISION_BY_ZERO_EXIT_CODE 2
#define INPUT_ERROR_EXIT_CODE 3

int read_float(char* text, char** end, float* value) {
    errno = 0;
    *value = strtof(text, end);

    if (text == *end) {
        return 0;
    }

    if (errno == ERANGE || !isfinite(*value)) {
        return 0;
    }

    return 1;
}

int main(void) {
    char* line = NULL;
    size_t capacity = 0;
    size_t line_number = 0;

    while (getline(&line, &capacity, stdin) != -1) {
        line_number++;

        char* current = line;

        while (isspace((unsigned char)*current)) {
            current++;
        }

        if (*current == '\0') {
            continue;
        }

        char* end = NULL;
        float result;

        if (!read_float(current, &end, &result)) {
            fprintf(stderr, "Ошибка в строке %zu: некорректное число.\n", line_number);
            free(line);
            return INPUT_ERROR_EXIT_CODE;
        }

        current = end;

        while (1) {
            while (isspace((unsigned char)*current)) {
                current++;
            }

            if (*current == '\0') {
                break;
            }

            float divisor;

            if (!read_float(current, &end, &divisor)) {
                fprintf(stderr, "Ошибка в строке %zu: некорректное число.\n", line_number);
                free(line);
                return INPUT_ERROR_EXIT_CODE;
            }

            if (divisor == 0.0f) {
                fprintf(stderr, "Ошибка в строке %zu: деление на ноль.\n", line_number);
                free(line);
                return DIVISION_BY_ZERO_EXIT_CODE;
            }

            result /= divisor;

            if (!isfinite(result)) {
                fprintf(stderr, "Ошибка в строке %zu: результат вычисления вышел за диапазон float.\n", line_number);
                free(line);
                return INPUT_ERROR_EXIT_CODE;
            }

            current = end;
        }

        if (printf("%g\n", (double)result) < 0) {
            perror("printf");
            free(line);
            return GENERAL_ERROR_EXIT_CODE;
        }

        if (fflush(stdout) == EOF) {
            perror("fflush");
            free(line);
            return GENERAL_ERROR_EXIT_CODE;
        }
    }

    if (ferror(stdin)) {
        perror("getline");
        free(line);
        return GENERAL_ERROR_EXIT_CODE;
    }

    free(line);
    return 0;
}