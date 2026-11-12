#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define GENERAL_ERROR_EXIT_CODE 1
#define DIVISION_BY_ZERO_EXIT_CODE 2
#define INPUT_ERROR_EXIT_CODE 3
#define EXEC_ERROR_EXIT_CODE 4

sig_atomic_t result_ready = 0;
sig_atomic_t child_finished = 0;

void handle_signal(int signal_number) {
    if (signal_number == SIGUSR1) {
        result_ready = 1;
    } else if (signal_number == SIGCHLD) {
        child_finished = 1;
    }
}

int main(void) {
    struct sigaction action = {0};
    action.sa_handler = handle_signal;
    action.sa_flags = SA_NOCLDSTOP;

    if (sigaction(SIGUSR1, &action, NULL) == -1 || sigaction(SIGCHLD, &action, NULL) == -1) {
        perror("sigaction");
        return GENERAL_ERROR_EXIT_CODE;
    }

    sigset_t signals;
    sigset_t wait_mask;

    if (sigemptyset(&signals) == -1 || sigaddset(&signals, SIGUSR1) == -1 || sigaddset(&signals, SIGCHLD) == -1) {
        perror("sigset");
        return GENERAL_ERROR_EXIT_CODE;
    }

    if (sigprocmask(SIG_BLOCK, &signals, &wait_mask) == -1) {
        perror("sigprocmask");
        return GENERAL_ERROR_EXIT_CODE;
    }

    if (sigdelset(&wait_mask, SIGUSR1) == -1 || sigdelset(&wait_mask, SIGCHLD) == -1) {
        perror("sigdelset");
        return GENERAL_ERROR_EXIT_CODE;
    }

    char* filename = NULL;
    size_t capacity = 0;

    printf("Введите путь к файлу: ");

    if (fflush(stdout) == EOF) {
        perror("fflush");
        return GENERAL_ERROR_EXIT_CODE;
    }

    ssize_t length = getline(&filename, &capacity, stdin);

    if (length == -1) {
        if (ferror(stdin)) {
            perror("getline");
        } else {
            fprintf(stderr, "\nОшибка: путь к файлу не введен.\n");
        }

        free(filename);
        return GENERAL_ERROR_EXIT_CODE;
    }

    while (length > 0 && filename[length - 1] == '\n') {
        filename[--length] = '\0';
    }

    if (length == 0) {
        fprintf(stderr, "Ошибка: путь к файлу не может быть пустым.\n");
        free(filename);
        return GENERAL_ERROR_EXIT_CODE;
    }

    int input_fd = open(filename, O_RDONLY);
    free(filename);

    if (input_fd == -1) {
        perror("open");
        return GENERAL_ERROR_EXIT_CODE;
    }

    char temporary_path[] = "/tmp/lab4-XXXXXX";
    int shared_fd = mkstemp(temporary_path);

    if (shared_fd == -1) {
        perror("mkstemp");

        if (close(input_fd) == -1) {
            perror("close");
        }

        return GENERAL_ERROR_EXIT_CODE;
    }

    if (unlink(temporary_path) == -1) {
        perror("unlink");

        if (close(input_fd) == -1) {
            perror("close");
        }

        if (close(shared_fd) == -1) {
            perror("close");
        }

        return GENERAL_ERROR_EXIT_CODE;
    }

    if (ftruncate(shared_fd, sizeof(float)) == -1) {
        perror("ftruncate");

        if (close(input_fd) == -1) {
            perror("close");
        }

        if (close(shared_fd) == -1) {
            perror("close");
        }

        return GENERAL_ERROR_EXIT_CODE;
    }

    char descriptor[32];
    snprintf(descriptor, sizeof(descriptor), "%d", shared_fd);

    float* shared = mmap(NULL, sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);

    if (shared == MAP_FAILED) {
        perror("mmap");

        if (close(input_fd) == -1) {
            perror("close");
        }

        if (close(shared_fd) == -1) {
            perror("close");
        }

        return GENERAL_ERROR_EXIT_CODE;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");

        if (close(input_fd) == -1) {
            perror("close");
        }

        if (close(shared_fd) == -1) {
            perror("close");
        }

        if (munmap(shared, sizeof(float)) == -1) {
            perror("munmap");
        }

        return GENERAL_ERROR_EXIT_CODE;
    }

    if (pid == 0) {
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2");
            exit(GENERAL_ERROR_EXIT_CODE);
        }

        if (input_fd != STDIN_FILENO && close(input_fd) == -1) {
            perror("close");
            exit(GENERAL_ERROR_EXIT_CODE);
        }

        execl("./lab4_child", "lab4_child", descriptor, (char*)NULL);

        perror("execl");
        exit(EXEC_ERROR_EXIT_CODE);
    }

    int parent_error = 0;

    if (close(input_fd) == -1) {
        perror("close");
        parent_error = 1;
    }

    if (close(shared_fd) == -1) {
        perror("close");
        parent_error = 1;
    }

    while (!parent_error) {
        while (!result_ready && !child_finished) {
            if (sigsuspend(&wait_mask) == -1 && errno != EINTR) {
                perror("sigsuspend");
                parent_error = 1;
                break;
            }
        }

        if (parent_error) {
            break;
        }

        if (result_ready) {
            result_ready = 0;

            printf("%g\n", *shared);

            if (fflush(stdout) == EOF) {
                perror("fflush");
                parent_error = 1;
                break;
            }

            if (kill(pid, SIGUSR2) == -1) {
                perror("kill");
                parent_error = 1;
                break;
            }
        }

        if (child_finished) {
            break;
        }
    }

    if (parent_error && kill(pid, SIGKILL) == -1 && errno != ESRCH) {
        perror("kill");
    }

    int status = 0;

    while (waitpid(pid, &status, 0) == -1) {
        if (errno == EINTR) {
            continue;
        }

        perror("waitpid");
        parent_error = 1;
        break;
    }

    int result = GENERAL_ERROR_EXIT_CODE;

    if (!parent_error) {
        if (WIFEXITED(status)) {
            result = WEXITSTATUS(status);

            if (result == EXEC_ERROR_EXIT_CODE) {
                fprintf(stderr, "Ошибка: не удалось запустить дочерний процесс.\n");
            } else if (result != 0 && result != DIVISION_BY_ZERO_EXIT_CODE && result != INPUT_ERROR_EXIT_CODE) {
                fprintf(stderr, "Дочерний процесс завершился с кодом %d.\n", result);
            }
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Дочерний процесс завершён сигналом %d.\n", WTERMSIG(status));
        } else {
            fprintf(stderr, "Ошибка: дочерний процесс завершился в неизвестном состоянии.\n");
        }
    }

    if (munmap(shared, sizeof(float)) == -1) {
        perror("munmap");
        result = GENERAL_ERROR_EXIT_CODE;
    }

    return result;
}
