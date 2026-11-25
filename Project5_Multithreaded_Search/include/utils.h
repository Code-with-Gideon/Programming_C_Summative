#ifndef P5_UTILS_H
#define P5_UTILS_H

#include <stdio.h>
#include <stddef.h>

char *read_line_dynamic(FILE *fp, int *out_eof);
long count_keyword_nocase(const char *line, const char *keyword);
void build_report_path(const char *input_path, char *out_buf, size_t out_size);

#endif
