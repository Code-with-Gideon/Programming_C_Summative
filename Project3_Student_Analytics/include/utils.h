#ifndef P3_UTILS_H
#define P3_UTILS_H

#include <stddef.h>

void trim_whitespace(char *s);
int read_line_trimmed(char *buf, size_t size);
int read_int_range(const char *prompt, int min_val, int max_val, int *out);
int read_float_range(const char *prompt, float min_val, float max_val, float *out);
int read_nonempty_string(const char *prompt, char *buf, size_t size);
int str_casecmp_local(const char *a, const char *b);
int str_contains_nocase(const char *haystack, const char *needle);

#endif
