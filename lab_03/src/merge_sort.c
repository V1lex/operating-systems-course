
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define GENERAL_ERROR_EXIT_CODE 1
#define INPUT_ERROR_EXIT_CODE 2

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    size_t arrived;
    size_t generation;
    size_t participants;
    int started;
    int cancelled;
} Barrier;

typedef struct {
    int* source;
    int* destination;
    size_t length;
    size_t thread_count;
    Barrier barrier;
} SortState;

typedef struct {
    SortState* state;
    size_t id;
} Worker;

int read_numbers(const char* filename, int** numbers, size_t* length) {
    FILE* input = fopen(filename, "r");

    if (input == NULL) {
        perror("fopen");
        return GENERAL_ERROR_EXIT_CODE;
    }

    char* line = NULL;
    size_t line_capacity = 0;
    size_t capacity = 0;
    *numbers = NULL;
    *length = 0;

    while (getline(&line, &line_capacity, input) != -1) {
        char* current = line;

        while (*current != '\0') {
            while (isspace((unsigned char)*current)) {
                current++;
            }

            if (*current == '\0') {
                break;
            }

            char* end = NULL;
            errno = 0;
            long value = strtol(current, &end, 10);

            if (current == end || errno == ERANGE || value < INT_MIN || value > INT_MAX) {
                fprintf(stderr, "Ошибка: входной файл содержит некорректное целое число.\n");
                free(line);
                fclose(input);
                free(*numbers);
                *numbers = NULL;
                return INPUT_ERROR_EXIT_CODE;
            }

            if (*length == capacity) {
                size_t next_capacity = capacity == 0 ? 1024 : capacity * 2;

                if (next_capacity < capacity || next_capacity > SIZE_MAX / sizeof(int)) {
                    fprintf(stderr, "Ошибка: входной файл слишком велик.\n");
                    free(line);
                    fclose(input);
                    free(*numbers);
                    *numbers = NULL;
                    return GENERAL_ERROR_EXIT_CODE;
                }

                int* resized = realloc(*numbers, next_capacity * sizeof(int));

                if (resized == NULL) {
                    perror("realloc");
                    free(line);
                    fclose(input);
                    free(*numbers);
                    *numbers = NULL;
                    return GENERAL_ERROR_EXIT_CODE;
                }

                *numbers = resized;
                capacity = next_capacity;
            }

            (*numbers)[(*length)++] = (int)value;
            current = end;
        }
    }

    int read_error = ferror(input);
    free(line);

    int close_error = fclose(input);

    if (read_error || close_error == EOF) {
        perror(read_error ? "getline" : "fclose");
        free(*numbers);
        *numbers = NULL;
        return GENERAL_ERROR_EXIT_CODE;
    }

    return 0;
}

void check_pthread(int error, const char* function) {
    if (error != 0) {
        fprintf(stderr, "%s: %s\n", function, strerror(error));
        exit(GENERAL_ERROR_EXIT_CODE);
    }
}

int init_barrier(Barrier* barrier, size_t participants) {
    int error = pthread_mutex_init(&barrier->mutex, NULL);

    if (error != 0) {
        fprintf(stderr, "pthread_mutex_init: %s\n", strerror(error));
        return -1;
    }

    error = pthread_cond_init(&barrier->condition, NULL);

    if (error != 0) {
        fprintf(stderr, "pthread_cond_init: %s\n", strerror(error));
        check_pthread(pthread_mutex_destroy(&barrier->mutex), "pthread_mutex_destroy");
        return -1;
    }

    barrier->arrived = 0;
    barrier->generation = 0;
    barrier->participants = participants;
    barrier->started = 0;
    barrier->cancelled = 0;
    return 0;
}

void wait_for_start(Barrier* barrier) {
    check_pthread(pthread_mutex_lock(&barrier->mutex), "pthread_mutex_lock");

    while (!barrier->started) {
        check_pthread(pthread_cond_wait(&barrier->condition, &barrier->mutex), "pthread_cond_wait");
    }

    check_pthread(pthread_mutex_unlock(&barrier->mutex), "pthread_mutex_unlock");
}

void wait_at_barrier(Barrier* barrier) {
    check_pthread(pthread_mutex_lock(&barrier->mutex), "pthread_mutex_lock");
    size_t generation = barrier->generation;
    barrier->arrived++;

    if (barrier->arrived == barrier->participants) {
        barrier->arrived = 0;
        barrier->generation++;
        check_pthread(pthread_cond_broadcast(&barrier->condition), "pthread_cond_broadcast");
    } else {
        while (generation == barrier->generation) {
            check_pthread(pthread_cond_wait(&barrier->condition, &barrier->mutex), "pthread_cond_wait");
        }
    }

    check_pthread(pthread_mutex_unlock(&barrier->mutex), "pthread_mutex_unlock");
}

void merge_block(SortState* state, size_t left, size_t width) {
    size_t span = width * 2;
    size_t middle = left + width < state->length ? left + width : state->length;
    size_t right = left + span < state->length ? left + span : state->length;
    size_t first = left;
    size_t second = middle;

    for (size_t output = left; output < right; output++) {
        if (second == right || (first < middle && state->source[first] <= state->source[second])) {
            state->destination[output] = state->source[first++];
        } else {
            state->destination[output] = state->source[second++];
        }
    }
}

void* sort_worker(void* argument) {
    Worker* worker = argument;
    SortState* state = worker->state;
    wait_for_start(&state->barrier);

    if (state->barrier.cancelled) {
        return NULL;
    }

    for (size_t width = 1; width < state->length; width *= 2) {
        size_t span = width * 2;
        size_t blocks = state->length / span + (state->length % span != 0);

        for (size_t block = worker->id; block < blocks; block += state->thread_count) {
            merge_block(state, block * span, width);
        }

        wait_at_barrier(&state->barrier);

        if (worker->id == 0) {
            int* previous = state->source;
            state->source = state->destination;
            state->destination = previous;
        }

        wait_at_barrier(&state->barrier);
    }

    return NULL;
}

int sort_numbers(int* numbers, size_t length, size_t limit, int observe, int** result, size_t* used_threads) {
    if (length == 0) {
        *result = numbers;
        *used_threads = 1;
        return 0;
    }

    *used_threads = length < limit ? length : limit;

    int* buffer = malloc(length * sizeof(int));
    pthread_t* threads = NULL;

    if (*used_threads > 1) {
        threads = calloc(*used_threads - 1, sizeof(pthread_t));
    }

    Worker* workers = calloc(*used_threads, sizeof(Worker));

    if (buffer == NULL || workers == NULL || (*used_threads > 1 && threads == NULL)) {
        perror("malloc/calloc");
        free(buffer);
        free(threads);
        free(workers);
        return -1;
    }

    SortState state;
    state.source = numbers;
    state.destination = buffer;
    state.length = length;
    state.thread_count = *used_threads;

    if (init_barrier(&state.barrier, *used_threads) != 0) {
        free(buffer);
        free(threads);
        free(workers);
        return -1;
    }

    workers[0].state = &state;
    workers[0].id = 0;
    size_t created = 0;

    for (size_t id = 1; id < *used_threads; id++) {
        workers[id].state = &state;
        workers[id].id = id;

        int error = pthread_create(&threads[id - 1], NULL, sort_worker, &workers[id]);

        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));

            check_pthread(pthread_mutex_lock(&state.barrier.mutex), "pthread_mutex_lock");
            state.barrier.cancelled = 1;
            state.barrier.started = 1;
            check_pthread(pthread_cond_broadcast(&state.barrier.condition), "pthread_cond_broadcast");
            check_pthread(pthread_mutex_unlock(&state.barrier.mutex), "pthread_mutex_unlock");

            for (size_t index = 0; index < created; index++) {
                check_pthread(pthread_join(threads[index], NULL), "pthread_join");
            }

            check_pthread(pthread_cond_destroy(&state.barrier.condition), "pthread_cond_destroy");
            check_pthread(pthread_mutex_destroy(&state.barrier.mutex), "pthread_mutex_destroy");
            free(buffer);
            free(threads);
            free(workers);
            return -1;
        }

        created++;
    }

    if (observe) {
        fprintf(stderr, "PID %ld: всего %zu потоков (включая главный). Нажмите Enter для сортировки...\n", (long)getpid(), *used_threads);
        getchar();
    }

    check_pthread(pthread_mutex_lock(&state.barrier.mutex), "pthread_mutex_lock");
    state.barrier.started = 1;
    check_pthread(pthread_cond_broadcast(&state.barrier.condition), "pthread_cond_broadcast");
    check_pthread(pthread_mutex_unlock(&state.barrier.mutex), "pthread_mutex_unlock");

    sort_worker(&workers[0]);

    for (size_t index = 0; index < created; index++) {
        check_pthread(pthread_join(threads[index], NULL), "pthread_join");
    }

    check_pthread(pthread_cond_destroy(&state.barrier.condition), "pthread_cond_destroy");
    check_pthread(pthread_mutex_destroy(&state.barrier.mutex), "pthread_mutex_destroy");
    free(threads);
    free(workers);

    if (state.source == numbers) {
        free(buffer);
    } else {
        free(numbers);
    }

    *result = state.source;
    return 0;
}

int main(int argc, char** argv) {
    const char* filename = NULL;
    size_t thread_limit = 0;
    int observe = 0;

    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--threads") == 0 && index + 1 < argc && thread_limit == 0) {
            char* end = NULL;
            errno = 0;
            unsigned long value = strtoul(argv[++index], &end, 10);

            if (errno != 0 || end == argv[index] || *end != '\0' || value == 0 || value > SIZE_MAX || argv[index][0] == '-') {
                fprintf(stderr, "Ошибка: количество потоков должно быть положительным целым числом.\n");
                return INPUT_ERROR_EXIT_CODE;
            }

            thread_limit = (size_t)value;
        } else if (strcmp(argv[index], "--input") == 0 && index + 1 < argc && filename == NULL) {
            filename = argv[++index];
        } else if (strcmp(argv[index], "--observe") == 0 && !observe) {
            observe = 1;
        } else {
            fprintf(stderr, "Ошибка: неизвестный или повторный аргумент.\n");
            return INPUT_ERROR_EXIT_CODE;
        }
    }

    if (thread_limit == 0 || filename == NULL) {
        fprintf(stderr, "Использование: %s --threads N --input FILE [--observe]\n", argv[0]);
        return INPUT_ERROR_EXIT_CODE;
    }

    int* numbers = NULL;
    size_t length = 0;
    int read_result = read_numbers(filename, &numbers, &length);

    if (read_result != 0) {
        return read_result;
    }

    if (length > SIZE_MAX / 2 / sizeof(int)) {
        fprintf(stderr, "Ошибка: массив слишком велик.\n");
        free(numbers);
        return GENERAL_ERROR_EXIT_CODE;
    }

    int* sorted = NULL;
    size_t used_threads = 0;

    struct timespec start;
    struct timespec finish;

    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("clock_gettime");
        free(numbers);
        return GENERAL_ERROR_EXIT_CODE;
    }

    if (sort_numbers(numbers, length, thread_limit, observe, &sorted, &used_threads) != 0) {
        free(numbers);
        return GENERAL_ERROR_EXIT_CODE;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &finish) == -1) {
        perror("clock_gettime");
        free(sorted);
        return GENERAL_ERROR_EXIT_CODE;
    }

    double milliseconds = (double)(finish.tv_sec - start.tv_sec) * 1000.0 +
                          (double)(finish.tv_nsec - start.tv_nsec) / 1000000.0;
    fprintf(stderr, "Время сортировки: %.3f мс; потоков: %zu\n", milliseconds, used_threads);

    for (size_t index = 0; index < length; index++) {
        printf("%d ", sorted[index]);
    }
    printf("\n");

    free(sorted);
    return 0;
}
