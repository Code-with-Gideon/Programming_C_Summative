#define _CRT_SECURE_NO_WARNINGS
#include "../include/utils.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void trim_whitespace(char *s) {
    char *start;
    char *end;
    size_t len;

    if (s == NULL || *s == '\0') {
        return;
    }

    start = s;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    len = strlen(start);
    if (start != s) {
        memmove(s, start, len + 1);
    }

    if (len == 0) {
        return;
    }

    end = s + len - 1;
    while (end >= s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

int read_line_trimmed(char *buf, size_t size) {
    size_t len;
    int ch;

    if (buf == NULL || size == 0) {
        return 0;
    }

    if (fgets(buf, (int)size, stdin) == NULL) {
        return 0;
    }

    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }

    trim_whitespace(buf);
    return 1;
}

int read_int_range(const char *prompt, int min_val, int max_val, int *out) {
    char line[128];
    char *endptr = NULL;
    long val;

    if (out == NULL) {
        return -1;
    }

    printf("%s", prompt);
    if (!read_line_trimmed(line, sizeof(line))) {
        return 0;
    }
    if (line[0] == '\0') {
        return -1;
    }

    errno = 0;
    val = strtol(line, &endptr, 10);
    if (errno == ERANGE || endptr == line || (endptr != NULL && *endptr != '\0') ||
        val < (long)INT_MIN || val > (long)INT_MAX) {
        return -1;
    }

    if ((int)val < min_val || (int)val > max_val) {
        return -1;
    }

    *out = (int)val;
    return 1;
}

int read_float_range(const char *prompt, float min_val, float max_val, float *out) {
    char line[128];
    char *endptr = NULL;
    float val;

    if (out == NULL) {
        return -1;
    }

    printf("%s", prompt);
    if (!read_line_trimmed(line, sizeof(line))) {
        return 0;
    }
    if (line[0] == '\0') {
        return -1;
    }

    errno = 0;
    val = strtof(line, &endptr);
    if (errno == ERANGE || endptr == line || (endptr != NULL && *endptr != '\0') ||
        isnan(val) || isinf(val)) {
        return -1;
    }

    if (val < min_val || val > max_val) {
        return -1;
    }

    *out = val;
    return 1;
}

int read_nonempty_string(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    if (!read_line_trimmed(buf, size)) {
        return 0;
    }
    return (buf[0] != '\0') ? 1 : -1;
}

int str_casecmp_local(const char *a, const char *b) {
    unsigned char ca;
    unsigned char cb;

    if (a == NULL && b == NULL) return 0;
    if (a == NULL) return -1;
    if (b == NULL) return 1;

    while (*a != '\0' && *b != '\0') {
        ca = (unsigned char)tolower((unsigned char)*a);
        cb = (unsigned char)tolower((unsigned char)*b);
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        a++;
        b++;
    }
    return (int)tolower((unsigned char)*a) - (int)tolower((unsigned char)*b);
}

int str_contains_nocase(const char *haystack, const char *needle) {
    size_t n_len;
    size_t h_len;
    size_t i;
    size_t j;

    if (haystack == NULL || needle == NULL || *needle == '\0') {
        return 0;
    }

    h_len = strlen(haystack);
    n_len = strlen(needle);
    if (n_len > h_len) {
        return 0;
    }

    for (i = 0; i <= h_len - n_len; i++) {
        int matched = 1;
        for (j = 0; j < n_len; j++) {
            if (tolower((unsigned char)haystack[i + j]) !=
                tolower((unsigned char)needle[j])) {
                matched = 0;
                break;
            }
        }
        if (matched) {
            return 1;
        }
    }
    return 0;
}
