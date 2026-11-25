#define _CRT_SECURE_NO_WARNINGS
#include "../include/student.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float compute_weighted_avg(const Student *stu) {
    const Course *c;
    const Course *end;
    float sum_obt = 0.0f;
    float sum_max = 0.0f;

    if (stu == NULL || stu->num_courses <= 0 || stu->courses == NULL) {
        return -1.0f;
    }
    end = stu->courses + stu->num_courses;
    for (c = stu->courses; c < end; c++) {
        sum_obt += c->marks_obtained;
        sum_max += c->max_marks;
    }
    if (sum_max <= 0.0f) {
        return -1.0f;
    }
    return (sum_obt / sum_max) * 100.0f;
}

static void print_student_courses_table(const Student *s) {
    const Course *c;
    const Course *end;
    float sum_obt = 0.0f;
    float sum_max = 0.0f;
    float avg;

    if (s->num_courses <= 0 || s->courses == NULL) {
        printf("No courses registered yet for %s %s.\n", s->first_name, s->last_name);
        return;
    }

    printf("\n%-12s %-30s %10s %10s %9s %-8s\n",
           "Code", "Course Name", "Obtained", "Max", "Percent", "Result");
    printf("-------------------------------------------------------------------------------------\n");

    end = s->courses + s->num_courses;
    for (c = s->courses; c < end; c++) {
        float pct = (c->max_marks > 0.0f) ? (c->marks_obtained / c->max_marks) * 100.0f : 0.0f;
        const char *res = (pct >= 50.0f) ? "Pass" : "Fail";
        printf("%-12.12s %-30.30s %10.2f %10.2f %8.2f%% %-8s\n",
               c->code, c->name, c->marks_obtained, c->max_marks, pct, res);
        sum_obt += c->marks_obtained;
        sum_max += c->max_marks;
    }

    avg = compute_weighted_avg(s);
    printf("-------------------------------------------------------------------------------------\n");
    printf("Total Marks: %.2f / %.2f | Overall Average: %.2f%% (%s)\n",
           sum_obt, sum_max, avg, (avg >= 50.0f) ? "PASS" : "FAIL");
}

int registry_init(StudentRegistry *reg) {
    if (reg == NULL) {
        return 0;
    }
    reg->items = (Student *)calloc(INIT_REG_CAP, sizeof(Student));
    if (reg->items == NULL) {
        reg->count = 0;
        reg->capacity = 0;
        return 0;
    }
    reg->count = 0;
    reg->capacity = INIT_REG_CAP;
    return 1;
}

void registry_free(StudentRegistry *reg) {
    Student *p;
    Student *end;

    if (reg == NULL) {
        return;
    }
    if (reg->items != NULL) {
        end = reg->items + reg->count;
        for (p = reg->items; p < end; p++) {
            free(p->courses);
            p->courses = NULL;
        }
        free(reg->items);
        reg->items = NULL;
    }
    reg->count = 0;
    reg->capacity = 0;
}

Student *find_student_by_id(const StudentRegistry *reg, int student_id) {
    Student *p;
    Student *end;

    if (reg == NULL || reg->items == NULL) {
        return NULL;
    }
    end = reg->items + reg->count;
    for (p = reg->items; p < end; p++) {
        if (p->student_id == student_id) {
            return p;
        }
    }
    return NULL;
}

Course *find_course_by_code(const Student *stu, const char *code) {
    Course *c;
    Course *end;

    if (stu == NULL || stu->courses == NULL || code == NULL) {
        return NULL;
    }
    end = stu->courses + stu->num_courses;
    for (c = stu->courses; c < end; c++) {
        if (str_casecmp_local(c->code, code) == 0) {
            return c;
        }
    }
    return NULL;
}

int registry_add_student_record(StudentRegistry *reg, const Student *src) {
    Student *slot;

    if (reg == NULL || src == NULL) {
        return 0;
    }
    if (find_student_by_id(reg, src->student_id) != NULL) {
        return -1;
    }

    if (reg->count >= reg->capacity) {
        int next_cap = (reg->capacity <= 0) ? INIT_REG_CAP : (reg->capacity * 2);
        Student *tmp = (Student *)realloc(reg->items, (size_t)next_cap * sizeof(Student));
        if (tmp == NULL) {
            return 0;
        }
        reg->items = tmp;
        reg->capacity = next_cap;
    }

    slot = reg->items + reg->count;
    *slot = *src;
    slot->num_courses = 0;
    slot->course_cap = 0;
    slot->courses = NULL;

    if (src->num_courses > 0 && src->courses != NULL) {
        int cap = (src->num_courses > INIT_CRS_CAP) ? src->num_courses : INIT_CRS_CAP;
        slot->courses = (Course *)calloc((size_t)cap, sizeof(Course));
        if (slot->courses == NULL) {
            return 0;
        }
        memcpy(slot->courses, src->courses, (size_t)src->num_courses * sizeof(Course));
        slot->num_courses = src->num_courses;
        slot->course_cap = cap;
    }

    reg->count++;
    return 1;
}

int student_add_course_record(Student *stu, const Course *crs) {
    Course *slot;

    if (stu == NULL || crs == NULL || crs->max_marks <= 0.0f ||
        crs->marks_obtained < 0.0f || crs->marks_obtained > crs->max_marks) {
        return 0;
    }
    if (find_course_by_code(stu, crs->code) != NULL) {
        return -1;
    }

    if (stu->num_courses >= stu->course_cap) {
        int next_cap = (stu->course_cap <= 0) ? INIT_CRS_CAP : (stu->course_cap * 2);
        Course *tmp = (Course *)realloc(stu->courses, (size_t)next_cap * sizeof(Course));
        if (tmp == NULL) {
            return 0;
        }
        stu->courses = tmp;
        stu->course_cap = next_cap;
    }

    slot = stu->courses + stu->num_courses;
    *slot = *crs;
    stu->num_courses++;
    return 1;
}

void op_register_student(StudentRegistry *reg) {
    Student s;
    int status;

    memset(&s, 0, sizeof(s));
    printf("\nRegister New Student\n");
    if (read_int_range("Student ID (1 - 999999): ", 1, 999999, &s.student_id) <= 0) {
        printf("Invalid student ID.\n");
        return;
    }
    if (find_student_by_id(reg, s.student_id) != NULL) {
        printf("Duplicate rejected: Student ID %d already exists.\n", s.student_id);
        return;
    }
    if (read_nonempty_string("First Name: ", s.first_name, sizeof(s.first_name)) <= 0) {
        printf("First name cannot be empty.\n");
        return;
    }
    if (read_nonempty_string("Last Name: ", s.last_name, sizeof(s.last_name)) <= 0) {
        printf("Last name cannot be empty.\n");
        return;
    }
    if (read_nonempty_string("Programme of Study: ", s.programme, sizeof(s.programme)) <= 0) {
        printf("Programme cannot be empty.\n");
        return;
    }
    if (read_int_range("Year of Study (1 - 6): ", 1, 6, &s.year_of_study) <= 0) {
        printf("Invalid year of study.\n");
        return;
    }

    status = registry_add_student_record(reg, &s);
    if (status == 1) {
        printf("Registered %s %s (ID %d). Total students: %d.\n",
               s.first_name, s.last_name, s.student_id, reg->count);
    } else if (status == -1) {
        printf("Duplicate rejected: Student ID %d already exists.\n", s.student_id);
    } else {
        fprintf(stderr, "Error: Memory allocation failed while adding student.\n");
    }
}

void op_display_all_students(const StudentRegistry *reg) {
    const Student *p;
    const Student *end;

    if (reg == NULL || reg->count <= 0) {
        printf("\nNo student records found.\n");
        return;
    }

    end = reg->items + reg->count;
    printf("\n%-8s %-24s %-28s %-5s %-8s %-8s\n",
           "ID", "Full Name", "Programme", "Year", "Courses", "Avg %");
    printf("-------------------------------------------------------------------------------------\n");
    for (p = reg->items; p < end; p++) {
        char full[84];
        float avg = compute_weighted_avg(p);
        snprintf(full, sizeof(full), "%s %s", p->first_name, p->last_name);
        if (avg < 0.0f) {
            printf("%-8d %-24.24s %-28.28s %-5d %-8d %-8s\n",
                   p->student_id, full, p->programme, p->year_of_study, p->num_courses, "N/A");
        } else {
            printf("%-8d %-24.24s %-28.28s %-5d %-8d %6.2f%%\n",
                   p->student_id, full, p->programme, p->year_of_study, p->num_courses, avg);
        }
    }
    printf("Total registered students: %d\n", reg->count);
}

void op_view_student_detail(const StudentRegistry *reg) {
    int id;
    const Student *s;

    if (reg->count <= 0) {
        printf("\nNo student records available.\n");
        return;
    }
    if (read_int_range("Enter Student ID: ", 1, 999999, &id) <= 0) {
        printf("Invalid student ID.\n");
        return;
    }
    s = find_student_by_id(reg, id);
    if (s == NULL) {
        printf("No student found with ID %d.\n", id);
        return;
    }

    printf("\nStudent Profile\n");
    printf("------------------------------\n");
    printf("ID        : %d\n", s->student_id);
    printf("Name      : %s %s\n", s->first_name, s->last_name);
    printf("Programme : %s\n", s->programme);
    printf("Year      : %d\n", s->year_of_study);
    printf("Courses   : %d\n", s->num_courses);
    print_student_courses_table(s);
}

void op_update_student(StudentRegistry *reg) {
    int id;
    Student *s;
    char buf[PROG_LEN];
    int yr;

    if (reg->count <= 0) {
        printf("\nNo student records available to update.\n");
        return;
    }
    if (read_int_range("Enter Student ID to update: ", 1, 999999, &id) <= 0) {
        printf("Invalid student ID.\n");
        return;
    }
    s = find_student_by_id(reg, id);
    if (s == NULL) {
        printf("Student ID %d not found.\n", id);
        return;
    }

    printf("Updating %s %s (press Enter to keep current value)\n", s->first_name, s->last_name);

    printf("First Name [%s]: ", s->first_name);
    if (read_line_trimmed(buf, NAME_LEN) && buf[0] != '\0') {
        strncpy(s->first_name, buf, NAME_LEN - 1);
        s->first_name[NAME_LEN - 1] = '\0';
    }

    printf("Last Name [%s]: ", s->last_name);
    if (read_line_trimmed(buf, NAME_LEN) && buf[0] != '\0') {
        strncpy(s->last_name, buf, NAME_LEN - 1);
        s->last_name[NAME_LEN - 1] = '\0';
    }

    printf("Programme [%s]: ", s->programme);
    if (read_line_trimmed(buf, PROG_LEN) && buf[0] != '\0') {
        strncpy(s->programme, buf, PROG_LEN - 1);
        s->programme[PROG_LEN - 1] = '\0';
    }

    printf("Current Year [%d]. ", s->year_of_study);
    if (read_int_range("New Year (1 - 6): ", 1, 6, &yr) > 0) {
        s->year_of_study = yr;
    }

    printf("Updated student ID %d.\n", s->student_id);
}

void op_delete_student(StudentRegistry *reg) {
    int id;
    Student *target;
    Student *p;
    Student *end;
    char full_name[90];

    if (reg->count <= 0) {
        printf("\nNo student records available to delete.\n");
        return;
    }
    if (read_int_range("Enter Student ID to delete: ", 1, 999999, &id) <= 0) {
        printf("Invalid student ID.\n");
        return;
    }

    target = find_student_by_id(reg, id);
    if (target == NULL) {
        printf("Student ID %d not found.\n", id);
        return;
    }

    snprintf(full_name, sizeof(full_name), "%s %s", target->first_name, target->last_name);
    free(target->courses);
    target->courses = NULL;

    end = reg->items + reg->count;
    for (p = target; p < end - 1; p++) {
        *p = *(p + 1);
    }
    reg->count--;

    if (reg->count > INIT_REG_CAP && reg->count <= reg->capacity / 2) {
        int next_cap = reg->capacity / 2;
        Student *shrunk = (Student *)realloc(reg->items, (size_t)next_cap * sizeof(Student));
        if (shrunk != NULL) {
            reg->items = shrunk;
            reg->capacity = next_cap;
        }
    }

    printf("Deleted %s (ID %d). Remaining students: %d.\n", full_name, id, reg->count);
}

void op_add_course_to_student(StudentRegistry *reg) {
    int id;
    Student *s;
    Course c;
    int status;

    if (reg->count <= 0) {
        printf("\nNo student records available.\n");
        return;
    }
    if (read_int_range("Enter Student ID: ", 1, 999999, &id) <= 0) {
        printf("Invalid student ID.\n");
        return;
    }
    s = find_student_by_id(reg, id);
    if (s == NULL) {
        printf("Student ID %d not found.\n", id);
        return;
    }

    memset(&c, 0, sizeof(c));
    if (read_nonempty_string("Course Code (e.g. CS201): ", c.code, sizeof(c.code)) <= 0) {
        printf("Course code cannot be empty.\n");
        return;
    }
    if (find_course_by_code(s, c.code) != NULL) {
        printf("Duplicate rejected: course '%s' is already registered for ID %d.\n", c.code, id);
        return;
    }
    if (read_nonempty_string("Course Name: ", c.name, sizeof(c.name)) <= 0) {
        printf("Course name cannot be empty.\n");
        return;
    }
    if (read_float_range("Maximum Marks (1 - 1000): ", 1.0f, 1000.0f, &c.max_marks) <= 0) {
        printf("Invalid maximum marks.\n");
        return;
    }
    if (read_float_range("Marks Obtained: ", 0.0f, c.max_marks, &c.marks_obtained) <= 0) {
        printf("Invalid marks: must be between 0.00 and %.2f.\n", c.max_marks);
        return;
    }

    status = student_add_course_record(s, &c);
    if (status == 1) {
        printf("Added course '%s' to %s %s.\n", c.code, s->first_name, s->last_name);
    } else if (status == -1) {
        printf("Duplicate course code rejected.\n");
    } else {
        fprintf(stderr, "Error adding course record.\n");
    }
}

void op_update_course_marks(StudentRegistry *reg) {
    int id;
    Student *s;
    char code[CODE_LEN];
    Course *c;
    float new_marks;

    if (reg->count <= 0) {
        printf("\nNo student records available.\n");
        return;
    }
    if (read_int_range("Enter Student ID: ", 1, 999999, &id) <= 0) {
        printf("Invalid student ID.\n");
        return;
    }
    s = find_student_by_id(reg, id);
    if (s == NULL) {
        printf("Student ID %d not found.\n", id);
        return;
    }
    if (s->num_courses <= 0) {
        printf("Student %d has no courses to update.\n", id);
        return;
    }

    if (read_nonempty_string("Course Code to update: ", code, sizeof(code)) <= 0) {
        printf("Course code cannot be empty.\n");
        return;
    }
    c = find_course_by_code(s, code);
    if (c == NULL) {
        printf("Course '%s' not found for student ID %d.\n", code, id);
        return;
    }

    printf("Current mark for %s (%s): %.2f / %.2f\n",
           c->code, c->name, c->marks_obtained, c->max_marks);
    if (read_float_range("New Marks Obtained: ", 0.0f, c->max_marks, &new_marks) <= 0) {
        printf("Invalid marks: must stay between 0.00 and %.2f.\n", c->max_marks);
        return;
    }

    c->marks_obtained = new_marks;
    printf("Updated %s marks to %.2f / %.2f.\n", c->code, c->marks_obtained, c->max_marks);
}

void op_display_student_performance(const StudentRegistry *reg) {
    int id;
    const Student *s;

    if (reg->count <= 0) {
        printf("\nNo student records available.\n");
        return;
    }
    if (read_int_range("Enter Student ID: ", 1, 999999, &id) <= 0) {
        printf("Invalid student ID.\n");
        return;
    }
    s = find_student_by_id(reg, id);
    if (s == NULL) {
        printf("Student ID %d not found.\n", id);
        return;
    }

    printf("\nAcademic Performance for %s %s (ID %d)\n", s->first_name, s->last_name, s->student_id);
    print_student_courses_table(s);
}
