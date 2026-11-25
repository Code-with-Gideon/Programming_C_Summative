#ifndef P5_ANALYZER_H
#define P5_ANALYZER_H

#include <stddef.h>

typedef struct {
    int thread_index;
    const char *input_path;
    const char *keyword;
    char report_path[512];
    long total_lines;
    long total_occurrences;
    long matching_lines_count;
    long *match_line_nums;
    size_t match_cap;
    int success;
    char status_msg[128];
} FileTask;

int init_file_task(FileTask *task, int thread_index, const char *input_path, const char *keyword);
void free_file_task(FileTask *task);

int search_document_task(FileTask *task);
int generate_analysis_report(FileTask *task);
void *process_file_worker(void *arg);
void display_processing_summary(const FileTask *tasks, int count);

#endif
