#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define GENERAL_ERROR_EXIT_CODE 1
#define INPUT_ERROR_EXIT_CODE 2

int PrimeCount(int A, int B);
char* translation(long x);

int read_long(char* text, char** end, long* value) {
    errno = 0;
    *value = strtol(text, end, 10);

    if (text == *end || errno == ERANGE || (**end != '\0' && !isspace((unsigned char)**end))) {
        return 0;
    }

    return 1;
}

int main(void) {
    char* line = NULL;
    size_t capacity = 0;
    int status = 0;

    while (getline(&line, &capacity, stdin) != -1) {
        char* current = line;
        char* end = NULL;
        long command;
        long first = 0;
        long second = 0;

        while (isspace((unsigned char)*current)) {
            ++current;
        }

        if (*current == '\0') {
            continue;
        }

        if (!read_long(current, &end, &command) || command < 0 || command > 2) {
            fprintf(stderr, "Ошибка: некорректная команда.\n");
            status = INPUT_ERROR_EXIT_CODE;
            break;
        }

        current = end;

        if (command == 1 || command == 2) {
            if (!read_long(current, &end, &first)) {
                fprintf(stderr, "Ошибка: некорректный аргумент.\n");
                status = INPUT_ERROR_EXIT_CODE;
                break;
            }

            current = end;
        }

        if (command == 1) {
            if (!read_long(current, &end, &second)) {
                fprintf(stderr, "Ошибка: некорректный аргумент.\n");
                status = INPUT_ERROR_EXIT_CODE;
                break;
            }

            current = end;
        }

        while (isspace((unsigned char)*current)) {
            ++current;
        }

        if (*current != '\0') {
            fprintf(stderr, "Ошибка: лишние аргументы.\n");
            status = INPUT_ERROR_EXIT_CODE;
            break;
        }

        if (command == 0) {
            fprintf(stderr, "Ошибка: переключение доступно только во второй программе.\n");
            status = INPUT_ERROR_EXIT_CODE;
            break;
        } else if (command == 1) {
            if (first < 1 || second < first || second > INT_MAX) {
                fprintf(stderr, "Ошибка: требуется диапазон натуральных чисел A <= B в пределах int.\n");
                status = INPUT_ERROR_EXIT_CODE;
                break;
            }

            int result = PrimeCount(first, second);

            if (result < 0) {
                fprintf(stderr, "Ошибка: не удалось подсчитать простые числа.\n");
                status = GENERAL_ERROR_EXIT_CODE;
                break;
            }

            printf("%d\n", result);
        } else {
            char* result = translation(first);

            if (result == NULL) {
                perror("malloc");
                status = GENERAL_ERROR_EXIT_CODE;
                break;
            }

            printf("%s\n", result);
            free(result);
        }
    }

    if (ferror(stdin)) {
        perror("getline");
        status = GENERAL_ERROR_EXIT_CODE;
    }

    free(line);
    return status;
}
