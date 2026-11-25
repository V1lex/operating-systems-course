#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define GENERAL_ERROR_EXIT_CODE 1
#define INPUT_ERROR_EXIT_CODE 2

typedef int (*PrimeCountFunc)(int, int);
typedef char* (*TranslationFunc)(long);

typedef struct {
    PrimeCountFunc prime_count;
    TranslationFunc translate;
    void* handle;
    int implementation;
} Library;

int load_library(Library* library, int implementation) {
    const char* paths[] = {"./liblab5_first.dylib", "./liblab5_second.dylib"};
    void* handle = dlopen(paths[implementation], RTLD_NOW | RTLD_LOCAL);

    if (handle == NULL) {
        fprintf(stderr, "Ошибка dlopen: %s\n", dlerror());
        return -1;
    }

    dlerror();
    PrimeCountFunc prime_count = (PrimeCountFunc)dlsym(handle, "PrimeCount");
    const char* error = dlerror();

    if (error != NULL) {
        fprintf(stderr, "Ошибка dlsym: %s\n", error);
        dlclose(handle);
        return -1;
    }

    dlerror();
    TranslationFunc translate = (TranslationFunc)dlsym(handle, "translation");
    error = dlerror();

    if (error != NULL) {
        fprintf(stderr, "Ошибка dlsym: %s\n", error);
        dlclose(handle);
        return -1;
    }

    library->prime_count = prime_count;
    library->translate = translate;
    library->handle = handle;
    library->implementation = implementation;
    return 0;
}

int switch_library(Library* library) {
    int implementation = 1 - library->implementation;
    Library next = {0};

    if (load_library(&next, implementation) != 0) {
        return -1;
    }

    if (dlclose(library->handle) != 0) {
        fprintf(stderr, "Ошибка dlclose: %s\n", dlerror());
        dlclose(next.handle);
        return -1;
    }

    *library = next;
    return 0;
}

int read_long(char* text, char** end, long* value) {
    errno = 0;
    *value = strtol(text, end, 10);

    if (text == *end || errno == ERANGE ||
        (**end != '\0' && !isspace((unsigned char)**end))) {
        return 0;
    }

    return 1;
}

int main(void) {
    Library library = {0};

    if (load_library(&library, 0) != 0) {
        return GENERAL_ERROR_EXIT_CODE;
    }

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
            if (switch_library(&library) != 0) {
                status = GENERAL_ERROR_EXIT_CODE;
                break;
            }
        } else if (command == 1) {
            if (first < 1 || second < first || second > INT_MAX) {
                fprintf(stderr, "Ошибка: требуется диапазон натуральных чисел A <= B в пределах int.\n");
                status = INPUT_ERROR_EXIT_CODE;
                break;
            }

            int result = library.prime_count(first, second);

            if (result < 0) {
                fprintf(stderr, "Ошибка: не удалось подсчитать простые числа.\n");
                status = GENERAL_ERROR_EXIT_CODE;
                break;
            }

            printf("%d\n", result);
        } else {
            char* result = library.translate(first);

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

    if (dlclose(library.handle) != 0) {
        fprintf(stderr, "Ошибка dlclose: %s\n", dlerror());
        status = GENERAL_ERROR_EXIT_CODE;
    }

    return status;
}
