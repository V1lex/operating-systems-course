#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define GENERAL_ERROR_EXIT_CODE 1
#define DIVISION_BY_ZERO_EXIT_CODE 2
#define INPUT_ERROR_EXIT_CODE 3
#define EXEC_ERROR_EXIT_CODE 4

int main(void) {
    char* filename = NULL;
    size_t capacity = 0;

    printf("Введите имя файла в папке data: ");

    if (fflush(stdout) == EOF) {
        perror("fflush");
        return GENERAL_ERROR_EXIT_CODE;
    }

    ssize_t length = getline(&filename, &capacity, stdin);

    if (length == -1) {
        if (ferror(stdin)) {
            perror("getline");
        } else {
            fprintf(stderr, "\nОшибка: имя файла не введено.\n");
        }

        free(filename);
        return GENERAL_ERROR_EXIT_CODE;
    }

    while (length > 0 && filename[length - 1] == '\n') {
        filename[--length] = '\0';
    }

    if (length == 0) {
        fprintf(stderr, "Ошибка: имя файла не может быть пустым.\n");
        free(filename);
        return GENERAL_ERROR_EXIT_CODE;
    }

    char filepath[4096] = "../../lab_02/data/";
    strcat(filepath, filename);

    free(filename);

    int input_fd = open(filepath, O_RDONLY);

    if (input_fd == -1) {
        perror("open");
        return GENERAL_ERROR_EXIT_CODE;
    }

    int pipe_fd[2];

    if (pipe(pipe_fd) == -1) {
        perror("pipe");

        if (close(input_fd) == -1) {
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

        if (close(pipe_fd[0]) == -1) {
            perror("close");
        }

        if (close(pipe_fd[1]) == -1) {
            perror("close");
        }

        return GENERAL_ERROR_EXIT_CODE;
    }

    if (pid == 0) {
        if (close(pipe_fd[0]) == -1) {
            perror("close");
            exit(GENERAL_ERROR_EXIT_CODE);
        }

        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2");
            exit(GENERAL_ERROR_EXIT_CODE);
        }

        if (dup2(pipe_fd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(GENERAL_ERROR_EXIT_CODE);
        }

        if (input_fd != STDIN_FILENO && close(input_fd) == -1) {
            perror("close");
            exit(GENERAL_ERROR_EXIT_CODE);
        }

        if (pipe_fd[1] != STDOUT_FILENO && close(pipe_fd[1]) == -1) {
            perror("close");
            exit(GENERAL_ERROR_EXIT_CODE);
        }

        execl("./lab2_child", "lab2_child", (char*)NULL);

        perror("execl");
        exit(EXEC_ERROR_EXIT_CODE);
    }

    int parent_error = 0;

    if (close(input_fd) == -1) {
        perror("close");
        parent_error = 1;
    }

    if (close(pipe_fd[1]) == -1) {
        perror("close");
        parent_error = 1;
    }

    char buffer[4096];

    while (!parent_error) {
        ssize_t bytes_read = read(pipe_fd[0], buffer, sizeof(buffer));

        if (bytes_read > 0) {
            ssize_t bytes_written = write(STDOUT_FILENO, buffer, (size_t)bytes_read);

            if (bytes_written == -1) {
                if (errno == EINTR) {
                    continue;
                }

                perror("write");
                parent_error = 1;
                break;
            }

            if (bytes_written != bytes_read) {
                fprintf(stderr, "Ошибка: данные выведены не полностью.\n");
                parent_error = 1;
                break;
            }

            continue;
        }

        if (bytes_read == 0) {
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        perror("read");
        parent_error = 1;
        break;
    }

    if (close(pipe_fd[0]) == -1) {
        perror("close");
        parent_error = 1;
    }

    int status;

    while (waitpid(pid, &status, 0) == -1) {
        if (errno == EINTR) {
            continue;
        }

        perror("waitpid");
        return GENERAL_ERROR_EXIT_CODE;
    }

    if (parent_error) {
        return GENERAL_ERROR_EXIT_CODE;
    }

    if (WIFEXITED(status)) {
        int child_status = WEXITSTATUS(status);

        if (child_status == 0) {
            return 0;
        }

        if (child_status == DIVISION_BY_ZERO_EXIT_CODE) {
            return DIVISION_BY_ZERO_EXIT_CODE;
        }

        if (child_status == INPUT_ERROR_EXIT_CODE) {
            return INPUT_ERROR_EXIT_CODE;
        }

        if (child_status == EXEC_ERROR_EXIT_CODE) {
            fprintf(stderr, "Ошибка: не удалось запустить дочерний процесс.\n");
            return EXEC_ERROR_EXIT_CODE;
        }

        fprintf(stderr, "Дочерний процесс завершился с кодом %d.\n", child_status);
        return child_status;
    }

    if (WIFSIGNALED(status)) {
        fprintf(stderr, "Дочерний процесс завершён сигналом %d.\n", WTERMSIG(status));
        return GENERAL_ERROR_EXIT_CODE;
    }

    fprintf(stderr, "Ошибка: дочерний процесс завершился в неизвестном состоянии.\n");
    return GENERAL_ERROR_EXIT_CODE;
}