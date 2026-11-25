#include "../include/analyzer.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    const char *keyword;
    int num_files;
    pthread_t *threads;
    int *created;
    FileTask *tasks;
    int i;
    int any_failed = 0;

    if (argc < 3 || argv[1][0] == '\0') {
        fprintf(stderr, "Usage: %s <keyword> <file1> [file2 ...]\n",
                (argc > 0 && argv[0] != NULL) ? argv[0] : "./document_analyzer");
        return EXIT_FAILURE;
    }

    keyword = argv[1];
    num_files = argc - 2;

    threads = (pthread_t *)calloc((size_t)num_files, sizeof(pthread_t));
    created = (int *)calloc((size_t)num_files, sizeof(int));
    tasks = (FileTask *)calloc((size_t)num_files, sizeof(FileTask));

    if (threads == NULL || created == NULL || tasks == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for thread management.\n");
        free(threads);
        free(created);
        free(tasks);
        return EXIT_FAILURE;
    }

    printf("===== MULTI-THREADED DOCUMENT ANALYZER =====\n");
    printf("Student ID: 1376251039 | Name: Erioluwa Gideon Olowoyo\n");
    printf("Search Keyword: %s\n", keyword);
    printf("Number of Input Files: %d\n\n", num_files);

    for (i = 0; i < num_files; i++) {
        init_file_task(tasks + i, i + 1, argv[i + 2], keyword);
        if (pthread_create(threads + i, NULL, process_file_worker, tasks + i) != 0) {
            created[i] = 0;
            tasks[i].success = 0;
            snprintf(tasks[i].status_msg, sizeof(tasks[i].status_msg),
                     "Failed (pthread_create error)");
            fprintf(stderr, "[Main] Error: Failed to spawn thread %d for '%s'.\n",
                    i + 1, tasks[i].input_path);
        } else {
            created[i] = 1;
        }
    }

    for (i = 0; i < num_files; i++) {
        if (created[i]) {
            pthread_join(threads[i], NULL);
        }
        if (!tasks[i].success) {
            any_failed = 1;
        }
    }

    display_processing_summary(tasks, num_files);

    for (i = 0; i < num_files; i++) {
        free_file_task(tasks + i);
    }
    free(threads);
    free(created);
    free(tasks);

    return any_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
