#ifndef P4_UTILS_H
#define P4_UTILS_H

#include <stddef.h>

void trim_whitespace(char *s);
int read_line_trimmed(char *buf, size_t size);
int read_int_range(const char *prompt, int min_val, int max_val, int *out);
int read_double_range(const char *prompt, double min_val, double max_val, double *out);
int read_nonempty_string(const char *prompt, char *buf, size_t size);
int validate_iso_date(const char *date_str);
int str_casecmp_local(const char *a, const char *b);
int str_contains_nocase(const char *haystack, const char *needle);

#endif
