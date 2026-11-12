#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define GENERAL_ERROR_EXIT_CODE 1
#define DIVISION_BY_ZERO_EXIT_CODE 2
#define INPUT_ERROR_EXIT_CODE 3

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Ошибка: неверное количество аргументов.\n");
        return GENERAL_ERROR_EXIT_CODE;
    }

    int shared_fd = (int)strtol(argv[1], NULL, 10);
    pid_t parent_pid = getppid();
    sigset_t signals;

    if (sigemptyset(&signals) == -1 || sigaddset(&signals, SIGUSR2) == -1) {
        perror("sigset");
        return GENERAL_ERROR_EXIT_CODE;
    }

    if (sigprocmask(SIG_BLOCK, &signals, NULL) == -1) {
        perror("sigprocmask");
        return GENERAL_ERROR_EXIT_CODE;
    }

    float* shared = mmap(NULL, sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);

    if (shared == MAP_FAILED) {
        perror("mmap");
        return GENERAL_ERROR_EXIT_CODE;
    }

    if (close(shared_fd) == -1) {
        perror("close");

        if (munmap(shared, sizeof(float)) == -1) {
            perror("munmap");
        }

        return GENERAL_ERROR_EXIT_CODE;
    }

    char* line = NULL;
    size_t capacity = 0;
    size_t line_number = 0;
    int status = 0;

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
        errno = 0;
        float result = strtof(current, &end);

        if (current == end || errno == ERANGE || !isfinite(result) || (*end != '\0' && !isspace((unsigned char)*end))) {
            fprintf(stderr, "Ошибка в строке %zu: некорректное число.\n", line_number);
            status = INPUT_ERROR_EXIT_CODE;
            break;
        }

        current = end;

        while (1) {
            while (isspace((unsigned char)*current)) {
                current++;
            }

            if (*current == '\0') {
                break;
            }

            errno = 0;
            float divisor = strtof(current, &end);

            if (current == end || errno == ERANGE || !isfinite(divisor) || (*end != '\0' && !isspace((unsigned char)*end))) {
                fprintf(stderr, "Ошибка в строке %zu: некорректное число.\n", line_number);
                status = INPUT_ERROR_EXIT_CODE;
                break;
            }

            if (divisor == 0.0f) {
                fprintf(stderr, "Ошибка в строке %zu: деление на ноль.\n", line_number);
                status = DIVISION_BY_ZERO_EXIT_CODE;
                break;
            }

            result /= divisor;

            if (!isfinite(result)) {
                fprintf(stderr, "Ошибка в строке %zu: результат вычисления вышел за диапазон float.\n", line_number);
                status = INPUT_ERROR_EXIT_CODE;
                break;
            }

            current = end;
        }

        if (status != 0) {
            break;
        }

        *shared = result;

        if (kill(parent_pid, SIGUSR1) == -1) {
            perror("kill");
            status = GENERAL_ERROR_EXIT_CODE;
            break;
        }

        int received;
        int error = sigwait(&signals, &received);

        if (error != 0) {
            fprintf(stderr, "Ошибка sigwait: %d\n", error);
            status = GENERAL_ERROR_EXIT_CODE;
            break;
        }
    }

    if (ferror(stdin)) {
        perror("getline");
        status = GENERAL_ERROR_EXIT_CODE;
    }

    free(line);

    if (munmap(shared, sizeof(float)) == -1) {
        perror("munmap");
        status = GENERAL_ERROR_EXIT_CODE;
    }

    return status;
}
