#define _CRT_SECURE_NO_WARNINGS
#include "../include/utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *read_line_dynamic(FILE *fp, int *out_eof) {
    size_t cap = 128;
    size_t len = 0;
    int ch;
    char *buf;

    if (out_eof != NULL) {
        *out_eof = 0;
    }
    if (fp == NULL) {
        return NULL;
    }

    buf = (char *)malloc(cap);
    if (buf == NULL) {
        return NULL;
    }

    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            break;
        }
        if (len + 1 >= cap) {
            size_t next_cap = cap * 2;
            char *tmp = (char *)realloc(buf, next_cap);
            if (tmp == NULL) {
                free(buf);
                return NULL;
            }
            buf = tmp;
            cap = next_cap;
        }
        buf[len++] = (char)ch;
    }

    if (ch == EOF && len == 0) {
        free(buf);
        if (out_eof != NULL) {
            *out_eof = 1;
        }
        return NULL;
    }

    buf[len] = '\0';
    return buf;
}

long count_keyword_nocase(const char *line, const char *keyword) {
    size_t kw_len;
    const char *p;
    long hits = 0;

    if (line == NULL || keyword == NULL || *keyword == '\0') {
        return 0;
    }

    kw_len = strlen(keyword);
    p = line;

    while (*p != '\0') {
        size_t i;
        int match = 1;
        for (i = 0; i < kw_len; i++) {
            if (p[i] == '\0' ||
                tolower((unsigned char)p[i]) != tolower((unsigned char)keyword[i])) {
                match = 0;
                break;
            }
        }
        if (match) {
            hits++;
            p += kw_len;
        } else {
            p++;
        }
    }

    return hits;
}

void build_report_path(const char *input_path, char *out_buf, size_t out_size) {
    const char *slash_fwd;
    const char *slash_bwd;
    const char *base;
    const char *dot;

    if (input_path == NULL || out_buf == NULL || out_size == 0) {
        return;
    }

    slash_fwd = strrchr(input_path, '/');
    slash_bwd = strrchr(input_path, '\\');
    base = input_path;
    if (slash_fwd != NULL && slash_fwd + 1 > base) base = slash_fwd + 1;
    if (slash_bwd != NULL && slash_bwd + 1 > base) base = slash_bwd + 1;

    dot = strrchr(base, '.');
    if (dot != NULL && dot > base) {
        size_t prefix_len = (size_t)(dot - input_path);
        if (prefix_len + sizeof("_report") + strlen(dot) < out_size) {
            snprintf(out_buf, out_size, "%.*s_report%s", (int)prefix_len, input_path, dot);
            return;
        }
    }

    snprintf(out_buf, out_size, "%s_report.txt", input_path);
}
