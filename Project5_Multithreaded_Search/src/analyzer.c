#define _CRT_SECURE_NO_WARNINGS
#include "../include/analyzer.h"
#include "../include/utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int init_file_task(FileTask *task, int thread_index, const char *input_path, const char *keyword) {
    if (task == NULL || input_path == NULL || keyword == NULL) {
        return 0;
    }

    memset(task, 0, sizeof(*task));
    task->thread_index = thread_index;
    task->input_path = input_path;
    task->keyword = keyword;
    build_report_path(input_path, task->report_path, sizeof(task->report_path));
    snprintf(task->status_msg, sizeof(task->status_msg), "Pending");
    return 1;
}

void free_file_task(FileTask *task) {
    if (task == NULL) {
        return;
    }
    free(task->match_line_nums);
    task->match_line_nums = NULL;
    task->match_cap = 0;
}

static int record_matching_line(FileTask *task, long line_num) {
    if (task->matching_lines_count >= (long)task->match_cap) {
        size_t next_cap = (task->match_cap == 0) ? 16 : (task->match_cap * 2);
        long *tmp = (long *)realloc(task->match_line_nums, next_cap * sizeof(long));
        if (tmp == NULL) {
            return 0;
        }
        task->match_line_nums = tmp;
        task->match_cap = next_cap;
    }

    task->match_line_nums[task->matching_lines_count] = line_num;
    task->matching_lines_count++;
    return 1;
}

int search_document_task(FileTask *task) {
    FILE *fp;
    int eof_reached = 0;

    if (task == NULL || task->input_path == NULL || task->keyword == NULL) {
        return 0;
    }

    fp = fopen(task->input_path, "r");
    if (fp == NULL) {
        snprintf(task->status_msg, sizeof(task->status_msg),
                 "Failed (cannot open input file: %.64s)", strerror(errno));
        task->success = 0;
        return 0;
    }

    while (1) {
        char *line = read_line_dynamic(fp, &eof_reached);
        long hits;

        if (line == NULL) {
            if (!eof_reached) {
                snprintf(task->status_msg, sizeof(task->status_msg),
                         "Failed (memory allocation error reading line)");
                fclose(fp);
                task->success = 0;
                return 0;
            }
            break;
        }

        task->total_lines++;
        hits = count_keyword_nocase(line, task->keyword);
        if (hits > 0) {
            task->total_occurrences += hits;
            if (!record_matching_line(task, task->total_lines)) {
                free(line);
                snprintf(task->status_msg, sizeof(task->status_msg),
                         "Failed (memory allocation error storing match line)");
                fclose(fp);
                task->success = 0;
                return 0;
            }
        }
        free(line);
    }

    fclose(fp);
    task->success = 1;
    snprintf(task->status_msg, sizeof(task->status_msg), "Completed");
    return 1;
}

int generate_analysis_report(FileTask *task) {
    FILE *out;
    long i;

    if (task == NULL) {
        return 0;
    }

    out = fopen(task->report_path, "w");
    if (out == NULL) {
        snprintf(task->status_msg, sizeof(task->status_msg),
                 "Failed (cannot create report file: %.64s)", strerror(errno));
        task->success = 0;
        return 0;
    }

    fprintf(out, "Document Analysis Report\n");
    fprintf(out, "--------------------------------------------------\n");
    fprintf(out, "Input File              : %s\n", task->input_path);
    fprintf(out, "Search Keyword          : %s (case-insensitive)\n", task->keyword);
    fprintf(out, "Total Lines Processed   : %ld\n", task->total_lines);
    fprintf(out, "Total Keyword Matches   : %ld\n", task->total_occurrences);
    fprintf(out, "Lines With Match        : %ld\n", task->matching_lines_count);
    fprintf(out, "Matching Line Numbers   : ");

    if (task->matching_lines_count == 0) {
        fprintf(out, "None\n");
    } else {
        for (i = 0; i < task->matching_lines_count; i++) {
            fprintf(out, "%s%ld", (i > 0) ? ", " : "", task->match_line_nums[i]);
        }
        fprintf(out, "\n");
    }

    fprintf(out, "Processing Status       : %s\n",
            task->success ? ((task->total_occurrences > 0) ? "Completed" : "Completed (keyword not found)")
                          : task->status_msg);

    if (fclose(out) != 0) {
        snprintf(task->status_msg, sizeof(task->status_msg),
                 "Failed (error closing report file)");
        task->success = 0;
        return 0;
    }

    return 1;
}

void *process_file_worker(void *arg) {
    FileTask *task = (FileTask *)arg;

    if (task == NULL) {
        return NULL;
    }

    printf("[Thread %d] Processing %s ...\n", task->thread_index, task->input_path);
    fflush(stdout);

    if (!search_document_task(task)) {
        printf("[Thread %d] Error on %s -> %s\n",
               task->thread_index, task->input_path, task->status_msg);
        fflush(stdout);
        return NULL;
    }

    if (!generate_analysis_report(task)) {
        printf("[Thread %d] Error writing report for %s -> %s\n",
               task->thread_index, task->input_path, task->status_msg);
        fflush(stdout);
        return NULL;
    }

    printf("[Thread %d] Successfully processed %s (%ld occurrence(s) on %ld line(s) -> %s)\n",
           task->thread_index, task->input_path,
           task->total_occurrences, task->matching_lines_count, task->report_path);
    fflush(stdout);

    return NULL;
}

void display_processing_summary(const FileTask *tasks, int count) {
    int i;
    int ok = 0;
    int fail = 0;

    printf("\n===== PROCESSING SUMMARY =====\n");
    for (i = 0; i < count; i++) {
        const FileTask *t = tasks + i;
        printf("%s: %s", t->input_path, t->status_msg);
        if (t->success) {
            printf(" (Occurrences: %ld, Matching lines: %ld, Report: %s)\n",
                   t->total_occurrences, t->matching_lines_count, t->report_path);
            ok++;
        } else {
            printf("\n");
            fail++;
        }
    }

    printf("\nAll processing tasks finished. Succeeded: %d | Failed: %d.\n", ok, fail);
}
