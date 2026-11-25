#define _CRT_SECURE_NO_WARNINGS
#include "../include/storage.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC_TAG "SREC"
#define BIN_VERSION 1

typedef struct {
    int student_id;
    char first_name[NAME_LEN];
    char last_name[NAME_LEN];
    char programme[PROG_LEN];
    int year_of_study;
    int num_courses;
} StudentDiskHeader;

static void sanitize_csv_field(char *dst, const char *src, size_t cap) {
    size_t i = 0;
    if (cap == 0) return;
    while (*src != '\0' && i + 1 < cap) {
        char c = *src++;
        dst[i++] = (c == ',' || c == '\n' || c == '\r') ? ' ' : c;
    }
    dst[i] = '\0';
}

int save_registry_binary(const StudentRegistry *reg, const char *path) {
    FILE *fp;
    const Student *p;
    const Student *end;
    int version = BIN_VERSION;

    if (reg == NULL || path == NULL) {
        return 0;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("Error opening binary file for writing");
        return 0;
    }

    if (fwrite(MAGIC_TAG, 1, 4, fp) != 4 ||
        fwrite(&version, sizeof(int), 1, fp) != 1 ||
        fwrite(&reg->count, sizeof(int), 1, fp) != 1) {
        perror("Error writing file header");
        fclose(fp);
        return 0;
    }

    end = reg->items + reg->count;
    for (p = reg->items; p < end; p++) {
        StudentDiskHeader hdr;
        memset(&hdr, 0, sizeof(hdr));
        hdr.student_id = p->student_id;
        strncpy(hdr.first_name, p->first_name, NAME_LEN - 1);
        strncpy(hdr.last_name, p->last_name, NAME_LEN - 1);
        strncpy(hdr.programme, p->programme, PROG_LEN - 1);
        hdr.year_of_study = p->year_of_study;
        hdr.num_courses = p->num_courses;

        if (fwrite(&hdr, sizeof(StudentDiskHeader), 1, fp) != 1) {
            perror("Error writing student record");
            fclose(fp);
            return 0;
        }
        if (hdr.num_courses > 0 && p->courses != NULL) {
            if (fwrite(p->courses, sizeof(Course), (size_t)hdr.num_courses, fp) != (size_t)hdr.num_courses) {
                perror("Error writing course records");
                fclose(fp);
                return 0;
            }
        }
    }

    fclose(fp);
    return 1;
}

int load_registry_binary(StudentRegistry *reg, const char *path) {
    FILE *fp;
    char magic[4];
    int version = 0;
    int total = 0;
    int i;
    StudentRegistry loaded;

    if (reg == NULL || path == NULL) {
        return 0;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1;
    }

    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, MAGIC_TAG, 4) != 0 ||
        fread(&version, sizeof(int), 1, fp) != 1 || version != BIN_VERSION ||
        fread(&total, sizeof(int), 1, fp) != 1 || total < 0 || total > 100000) {
        fprintf(stderr, "Error: Invalid or corrupted binary file format '%s'.\n", path);
        fclose(fp);
        return 0;
    }

    if (!registry_init(&loaded)) {
        fclose(fp);
        return 0;
    }

    for (i = 0; i < total; i++) {
        StudentDiskHeader hdr;
        Student s;
        int j;

        if (fread(&hdr, sizeof(StudentDiskHeader), 1, fp) != 1 ||
            hdr.num_courses < 0 || hdr.num_courses > 500) {
            fprintf(stderr, "Error: Corrupted student entry in '%s'.\n", path);
            registry_free(&loaded);
            fclose(fp);
            return 0;
        }

        memset(&s, 0, sizeof(s));
        s.student_id = hdr.student_id;
        strncpy(s.first_name, hdr.first_name, NAME_LEN - 1);
        strncpy(s.last_name, hdr.last_name, NAME_LEN - 1);
        strncpy(s.programme, hdr.programme, PROG_LEN - 1);
        s.year_of_study = hdr.year_of_study;

        if (registry_add_student_record(&loaded, &s) != 1) {
            registry_free(&loaded);
            fclose(fp);
            return 0;
        }

        if (hdr.num_courses > 0) {
            Student *dst_stu = loaded.items + (loaded.count - 1);
            Course *tmp_courses = (Course *)malloc((size_t)hdr.num_courses * sizeof(Course));
            if (tmp_courses == NULL) {
                registry_free(&loaded);
                fclose(fp);
                return 0;
            }
            if (fread(tmp_courses, sizeof(Course), (size_t)hdr.num_courses, fp) != (size_t)hdr.num_courses) {
                free(tmp_courses);
                registry_free(&loaded);
                fclose(fp);
                return 0;
            }
            for (j = 0; j < hdr.num_courses; j++) {
                student_add_course_record(dst_stu, tmp_courses + j);
            }
            free(tmp_courses);
        }
    }

    fclose(fp);
    registry_free(reg);
    *reg = loaded;
    return 1;
}

int export_registry_csv(const StudentRegistry *reg, const char *path) {
    FILE *fp;
    const Student *p;
    const Student *end;

    if (reg == NULL || path == NULL) {
        return 0;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        perror("Error opening CSV file for writing");
        return 0;
    }

    fprintf(fp, "student_id,first_name,last_name,programme,year_of_study,course_code,course_name,marks_obtained,max_marks\n");

    end = reg->items + reg->count;
    for (p = reg->items; p < end; p++) {
        char fn[NAME_LEN];
        char ln[NAME_LEN];
        char pr[PROG_LEN];

        sanitize_csv_field(fn, p->first_name, sizeof(fn));
        sanitize_csv_field(ln, p->last_name, sizeof(ln));
        sanitize_csv_field(pr, p->programme, sizeof(pr));

        if (p->num_courses <= 0 || p->courses == NULL) {
            fprintf(fp, "%d,%s,%s,%s,%d,,,,\n",
                    p->student_id, fn, ln, pr, p->year_of_study);
        } else {
            const Course *c;
            const Course *c_end = p->courses + p->num_courses;
            for (c = p->courses; c < c_end; c++) {
                char code[CODE_LEN];
                char cname[COURSE_LEN];
                sanitize_csv_field(code, c->code, sizeof(code));
                sanitize_csv_field(cname, c->name, sizeof(cname));
                fprintf(fp, "%d,%s,%s,%s,%d,%s,%s,%.2f,%.2f\n",
                        p->student_id, fn, ln, pr, p->year_of_study,
                        code, cname, c->marks_obtained, c->max_marks);
            }
        }
    }

    fclose(fp);
    return 1;
}

static int split_csv_9(char *line, char *fields[9]) {
    int col = 0;
    char *p = line;

    fields[col++] = p;
    while (*p != '\0' && col < 9) {
        if (*p == ',') {
            *p = '\0';
            fields[col++] = p + 1;
        }
        p++;
    }
    return col == 9;
}

int import_registry_csv(StudentRegistry *reg, const char *path) {
    FILE *fp;
    char line[512];
    StudentRegistry loaded;
    int line_no = 0;

    if (reg == NULL || path == NULL) {
        return 0;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Error opening CSV file for reading");
        return 0;
    }

    if (!registry_init(&loaded)) {
        fclose(fp);
        return 0;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *f[9];
        int id;
        int yr;
        Student *existing;

        line_no++;
        trim_whitespace(line);
        if (line[0] == '\0') continue;
        if (line_no == 1 && strncmp(line, "student_id,", 11) == 0) continue;

        if (!split_csv_9(line, f)) {
            continue;
        }

        id = atoi(f[0]);
        yr = atoi(f[4]);
        if (id <= 0 || yr < 1 || yr > 6 || f[1][0] == '\0' || f[2][0] == '\0') {
            continue;
        }

        existing = find_student_by_id(&loaded, id);
        if (existing == NULL) {
            Student s;
            memset(&s, 0, sizeof(s));
            s.student_id = id;
            strncpy(s.first_name, f[1], NAME_LEN - 1);
            strncpy(s.last_name, f[2], NAME_LEN - 1);
            strncpy(s.programme, f[3], PROG_LEN - 1);
            s.year_of_study = yr;
            if (registry_add_student_record(&loaded, &s) != 1) {
                registry_free(&loaded);
                fclose(fp);
                return 0;
            }
            existing = loaded.items + (loaded.count - 1);
        }

        if (f[5][0] != '\0') {
            Course c;
            memset(&c, 0, sizeof(c));
            strncpy(c.code, f[5], CODE_LEN - 1);
            strncpy(c.name, f[6], COURSE_LEN - 1);
            c.marks_obtained = (float)atof(f[7]);
            c.max_marks = (float)atof(f[8]);
            student_add_course_record(existing, &c);
        }
    }

    fclose(fp);
    registry_free(reg);
    *reg = loaded;
    return 1;
}

int seed_sample_students(StudentRegistry *reg) {
    struct SeedRow {
        int id;
        const char *fn;
        const char *ln;
        const char *prog;
        int yr;
        const char *c1_code;
        const char *c1_name;
        float c1_obt;
        float c1_max;
        const char *c2_code;
        const char *c2_name;
        float c2_obt;
        float c2_max;
    };

    static const struct SeedRow rows[5] = {
        {1001, "Amina", "Uwimana", "Software Engineering", 2,
         "CS201", "Low-Level C Programming", 88.0f, 100.0f,
         "CS204", "Data Structures", 43.0f, 50.0f},
        {1002, "Chinedu", "Okafor", "Computer Engineering", 3,
         "CS201", "Low-Level C Programming", 72.0f, 100.0f,
         "EE301", "Embedded Systems", 69.0f, 100.0f},
        {1003, "Grace", "Mensah", "Software Engineering", 2,
         "CS204", "Data Structures", 47.0f, 50.0f,
         "CS210", "Operating Systems", 91.0f, 100.0f},
        {1004, "Tariq", "El-Amin", "Computer Science", 1,
         "CS201", "Low-Level C Programming", 41.0f, 100.0f,
         "MT102", "Discrete Mathematics", 22.0f, 50.0f},
        {1005, "Nadia", "Kamau", "Computer Engineering", 2,
         "EE301", "Embedded Systems", 79.0f, 100.0f,
         "CS210", "Operating Systems", 84.0f, 100.0f}
    };

    int i;
    int added = 0;

    for (i = 0; i < 5; i++) {
        Student s;
        Course c1;
        Course c2;

        if (find_student_by_id(reg, rows[i].id) != NULL) {
            continue;
        }
        memset(&s, 0, sizeof(s));
        s.student_id = rows[i].id;
        strncpy(s.first_name, rows[i].fn, NAME_LEN - 1);
        strncpy(s.last_name, rows[i].ln, NAME_LEN - 1);
        strncpy(s.programme, rows[i].prog, PROG_LEN - 1);
        s.year_of_study = rows[i].yr;

        if (registry_add_student_record(reg, &s) == 1) {
            Student *dst = reg->items + (reg->count - 1);
            memset(&c1, 0, sizeof(c1));
            strncpy(c1.code, rows[i].c1_code, CODE_LEN - 1);
            strncpy(c1.name, rows[i].c1_name, COURSE_LEN - 1);
            c1.marks_obtained = rows[i].c1_obt;
            c1.max_marks = rows[i].c1_max;
            student_add_course_record(dst, &c1);

            memset(&c2, 0, sizeof(c2));
            strncpy(c2.code, rows[i].c2_code, CODE_LEN - 1);
            strncpy(c2.name, rows[i].c2_name, COURSE_LEN - 1);
            c2.marks_obtained = rows[i].c2_obt;
            c2.max_marks = rows[i].c2_max;
            student_add_course_record(dst, &c2);
            added++;
        }
    }

    return added;
}

void op_save_records(const StudentRegistry *reg) {
    int fmt;
    char path[160];

    printf("\nSave / Export Format:\n");
    printf("1. Binary File (default: %s)\n", DEFAULT_BIN_PATH);
    printf("2. CSV File (default: %s)\n", DEFAULT_CSV_PATH);
    if (read_int_range("Select format (1-2): ", 1, 2, &fmt) <= 0) {
        printf("Invalid format selection.\n");
        return;
    }

    if (fmt == 1) {
        printf("Enter path (press Enter for %s): ", DEFAULT_BIN_PATH);
        if (!read_line_trimmed(path, sizeof(path))) return;
        if (path[0] == '\0') strcpy(path, DEFAULT_BIN_PATH);
        if (save_registry_binary(reg, path)) {
            printf("Saved %d student record(s) cleanly to binary '%s'.\n", reg->count, path);
        }
    } else {
        printf("Enter path (press Enter for %s): ", DEFAULT_CSV_PATH);
        if (!read_line_trimmed(path, sizeof(path))) return;
        if (path[0] == '\0') strcpy(path, DEFAULT_CSV_PATH);
        if (export_registry_csv(reg, path)) {
            printf("Exported %d student record(s) cleanly to CSV '%s'.\n", reg->count, path);
        }
    }
}

void op_load_records(StudentRegistry *reg) {
    int fmt;
    char path[160];

    printf("\nLoad / Import Format:\n");
    printf("1. Binary File (default: %s)\n", DEFAULT_BIN_PATH);
    printf("2. CSV File (default: %s)\n", DEFAULT_CSV_PATH);
    if (read_int_range("Select format (1-2): ", 1, 2, &fmt) <= 0) {
        printf("Invalid format selection.\n");
        return;
    }

    if (fmt == 1) {
        int res;
        printf("Enter path (press Enter for %s): ", DEFAULT_BIN_PATH);
        if (!read_line_trimmed(path, sizeof(path))) return;
        if (path[0] == '\0') strcpy(path, DEFAULT_BIN_PATH);
        res = load_registry_binary(reg, path);
        if (res == 1) {
            printf("Loaded %d student record(s) from binary '%s'.\n", reg->count, path);
        } else if (res == -1) {
            printf("File '%s' does not exist.\n", path);
        }
    } else {
        printf("Enter path (press Enter for %s): ", DEFAULT_CSV_PATH);
        if (!read_line_trimmed(path, sizeof(path))) return;
        if (path[0] == '\0') strcpy(path, DEFAULT_CSV_PATH);
        if (import_registry_csv(reg, path)) {
            printf("Imported %d student record(s) from CSV '%s'.\n", reg->count, path);
        }
    }
}
