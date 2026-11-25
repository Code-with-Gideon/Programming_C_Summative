#define _CRT_SECURE_NO_WARNINGS
#include "../include/analytics.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float calculate_student_average(const Student *stu) {
    const Course *c;
    const Course *end;
    float total_obt = 0.0f;
    float total_max = 0.0f;

    if (stu == NULL || stu->num_courses <= 0 || stu->courses == NULL) {
        return -1.0f;
    }

    end = stu->courses + stu->num_courses;
    for (c = stu->courses; c < end; c++) {
        total_obt += c->marks_obtained;
        total_max += c->max_marks;
    }

    if (total_max <= 0.0f) {
        return -1.0f;
    }
    return (total_obt / total_max) * 100.0f;
}

static void print_search_match_row(const Student *s) {
    float avg = calculate_student_average(s);
    if (avg < 0.0f) {
        printf("  ID: %-6d | %-15s %-15s | %-24s | Yr %d | Courses: %d | Avg: N/A\n",
               s->student_id, s->first_name, s->last_name, s->programme, s->year_of_study, s->num_courses);
    } else {
        printf("  ID: %-6d | %-15s %-15s | %-24s | Yr %d | Courses: %d | Avg: %.2f%%\n",
               s->student_id, s->first_name, s->last_name, s->programme, s->year_of_study, s->num_courses, avg);
    }
}

void op_search_students(const StudentRegistry *reg) {
    int mode;
    int matches = 0;
    const Student *p;
    const Student *end;

    if (reg == NULL || reg->count <= 0) {
        printf("\nNo student records available to search.\n");
        return;
    }

    printf("\nSearch Students By:\n");
    printf("1. Student ID\n");
    printf("2. First or Last Name\n");
    printf("3. Programme of Study\n");
    if (read_int_range("Option (1-3): ", 1, 3, &mode) <= 0) {
        printf("Invalid search option.\n");
        return;
    }

    end = reg->items + reg->count;

    if (mode == 1) {
        int id;
        const Student *hit;
        if (read_int_range("Enter Student ID: ", 1, 999999, &id) <= 0) {
            printf("Invalid student ID.\n");
            return;
        }
        hit = find_student_by_id(reg, id);
        if (hit != NULL) {
            printf("Match found:\n");
            print_search_match_row(hit);
        } else {
            printf("No student found with ID %d.\n", id);
        }
        return;
    }

    if (mode == 2) {
        char query[NAME_LEN];
        if (read_nonempty_string("Enter First or Last Name: ", query, sizeof(query)) <= 0) {
            printf("Search name cannot be empty.\n");
            return;
        }
        printf("Search Results:\n");
        for (p = reg->items; p < end; p++) {
            if (str_contains_nocase(p->first_name, query) ||
                str_contains_nocase(p->last_name, query)) {
                print_search_match_row(p);
                matches++;
            }
        }
    } else {
        char prog[PROG_LEN];
        if (read_nonempty_string("Enter Programme Name: ", prog, sizeof(prog)) <= 0) {
            printf("Programme search query cannot be empty.\n");
            return;
        }
        printf("Students matching programme '%s':\n", prog);
        for (p = reg->items; p < end; p++) {
            if (str_contains_nocase(p->programme, prog)) {
                print_search_match_row(p);
                matches++;
            }
        }
    }

    if (matches == 0) {
        printf("  No matching student records found.\n");
    } else {
        printf("Total matches: %d\n", matches);
    }
}

static int compare_students_manual(const Student *a, const Student *b, StudentSortField field) {
    if (field == SORT_BY_ID) {
        return (a->student_id > b->student_id) - (a->student_id < b->student_id);
    }
    if (field == SORT_BY_NAME) {
        int c = str_casecmp_local(a->last_name, b->last_name);
        if (c != 0) return c;
        c = str_casecmp_local(a->first_name, b->first_name);
        if (c != 0) return c;
        return (a->student_id > b->student_id) - (a->student_id < b->student_id);
    }
    if (field == SORT_BY_AVERAGE) {
        float avg_a = calculate_student_average(a);
        float avg_b = calculate_student_average(b);
        if (avg_a < 0.0f && avg_b < 0.0f) {
            return (a->student_id > b->student_id) - (a->student_id < b->student_id);
        }
        if (avg_a < 0.0f) return 1;
        if (avg_b < 0.0f) return -1;
        if (avg_b > avg_a) return 1;
        if (avg_b < avg_a) return -1;
        return (a->student_id > b->student_id) - (a->student_id < b->student_id);
    }
    /* SORT_BY_PROGRAMME */
    {
        int c = str_casecmp_local(a->programme, b->programme);
        if (c != 0) return c;
        c = str_casecmp_local(a->last_name, b->last_name);
        if (c != 0) return c;
        return (a->student_id > b->student_id) - (a->student_id < b->student_id);
    }
}

/* In-place Selection Sort over StudentRegistry */
void manual_sort_students(StudentRegistry *reg, StudentSortField field) {
    Student *i;
    Student *j;
    Student *end;

    if (reg == NULL || reg->count <= 1 || reg->items == NULL) {
        return;
    }

    end = reg->items + reg->count;
    for (i = reg->items; i < end - 1; i++) {
        Student *best = i;
        for (j = i + 1; j < end; j++) {
            if (compare_students_manual(j, best, field) < 0) {
                best = j;
            }
        }
        if (best != i) {
            Student tmp = *i;
            *i = *best;
            *best = tmp;
        }
    }
}

void op_sort_students(StudentRegistry *reg) {
    int choice;
    static const char *labels[4] = {
        "Student ID (ascending)",
        "Student Name (alphabetical)",
        "Overall Academic Average (highest first)",
        "Programme of Study (alphabetical)"
    };

    if (reg == NULL || reg->count <= 1) {
        printf("\nNeed at least 2 students to sort.\n");
        return;
    }

    printf("\nSort Students By:\n");
    printf("1. Student ID\n");
    printf("2. Student Name (alphabetically)\n");
    printf("3. Overall Academic Average\n");
    printf("4. Programme of Study\n");
    if (read_int_range("Option (1-4): ", 1, 4, &choice) <= 0) {
        printf("Invalid sort option.\n");
        return;
    }

    manual_sort_students(reg, (StudentSortField)choice);
    printf("Sorted %d students by %s.\n", reg->count, labels[choice - 1]);
    op_display_all_students(reg);
}

typedef struct {
    char code[CODE_LEN];
    char name[COURSE_LEN];
    int enrollments;
    float sum_obtained;
    float sum_max;
} CourseStat;

typedef struct {
    char programme[PROG_LEN];
    int students;
} ProgrammeStat;

void op_generate_academic_report(const StudentRegistry *reg) {
    const Student *p;
    const Student *end;
    const Student *best = NULL;
    const Student *worst = NULL;
    float best_avg = -1.0f;
    float worst_avg = 101.0f;
    float total_obt_all = 0.0f;
    float total_max_all = 0.0f;
    int total_courses = 0;
    int graded_students = 0;
    int passing_students = 0;
    int failing_students = 0;
    int ungraded_students = 0;

    CourseStat *c_stats = NULL;
    int c_count = 0;
    int c_cap = 0;

    ProgrammeStat *p_stats = NULL;
    int p_count = 0;
    int p_cap = 0;
    int i;

    if (reg == NULL || reg->count <= 0) {
        printf("\nNo student records available for report generation.\n");
        return;
    }

    end = reg->items + reg->count;
    for (p = reg->items; p < end; p++) {
        float avg = calculate_student_average(p);
        const Course *c;
        const Course *c_end;
        int found_prog = 0;

        total_courses += p->num_courses;

        if (avg < 0.0f) {
            ungraded_students++;
        } else {
            graded_students++;
            if (avg >= 50.0f) passing_students++;
            else failing_students++;

            if (best == NULL || avg > best_avg) {
                best_avg = avg;
                best = p;
            }
            if (worst == NULL || avg < worst_avg) {
                worst_avg = avg;
                worst = p;
            }
        }

        if (p->courses != NULL) {
            c_end = p->courses + p->num_courses;
            for (c = p->courses; c < c_end; c++) {
                int found_crs = 0;
                total_obt_all += c->marks_obtained;
                total_max_all += c->max_marks;

                for (i = 0; i < c_count; i++) {
                    if (str_casecmp_local(c_stats[i].code, c->code) == 0) {
                        c_stats[i].enrollments++;
                        c_stats[i].sum_obtained += c->marks_obtained;
                        c_stats[i].sum_max += c->max_marks;
                        found_crs = 1;
                        break;
                    }
                }
                if (!found_crs) {
                    if (c_count >= c_cap) {
                        int ncap = (c_cap == 0) ? 8 : (c_cap * 2);
                        CourseStat *tmp = (CourseStat *)realloc(c_stats, (size_t)ncap * sizeof(CourseStat));
                        if (tmp != NULL) {
                            c_stats = tmp;
                            c_cap = ncap;
                        }
                    }
                    if (c_count < c_cap) {
                        strncpy(c_stats[c_count].code, c->code, CODE_LEN - 1);
                        c_stats[c_count].code[CODE_LEN - 1] = '\0';
                        strncpy(c_stats[c_count].name, c->name, COURSE_LEN - 1);
                        c_stats[c_count].name[COURSE_LEN - 1] = '\0';
                        c_stats[c_count].enrollments = 1;
                        c_stats[c_count].sum_obtained = c->marks_obtained;
                        c_stats[c_count].sum_max = c->max_marks;
                        c_count++;
                    }
                }
            }
        }

        for (i = 0; i < p_count; i++) {
            if (str_casecmp_local(p_stats[i].programme, p->programme) == 0) {
                p_stats[i].students++;
                found_prog = 1;
                break;
            }
        }
        if (!found_prog) {
            if (p_count >= p_cap) {
                int ncap = (p_cap == 0) ? 8 : (p_cap * 2);
                ProgrammeStat *tmp = (ProgrammeStat *)realloc(p_stats, (size_t)ncap * sizeof(ProgrammeStat));
                if (tmp != NULL) {
                    p_stats = tmp;
                    p_cap = ncap;
                }
            }
            if (p_count < p_cap) {
                strncpy(p_stats[p_count].programme, p->programme, PROG_LEN - 1);
                p_stats[p_count].programme[PROG_LEN - 1] = '\0';
                p_stats[p_count].students = 1;
                p_count++;
            }
        }
    }

    printf("\nAcademic Performance Summary Report\n");
    printf("-------------------------------------------------------------\n");
    printf("Total Registered Students   : %d\n", reg->count);
    printf("Total Registered Courses    : %d\n", total_courses);
    if (total_max_all > 0.0f) {
        printf("Overall Cohort Performance  : %.2f%% (weighted marks/max)\n",
               (total_obt_all / total_max_all) * 100.0f);
    } else {
        printf("Overall Cohort Performance  : N/A (no graded courses yet)\n");
    }

    if (best != NULL && worst != NULL) {
        printf("Highest Student Average     : %s %s (ID %d) - %.2f%%\n",
               best->first_name, best->last_name, best->student_id, best_avg);
        printf("Lowest Student Average      : %s %s (ID %d) - %.2f%%\n",
               worst->first_name, worst->last_name, worst->student_id, worst_avg);
    } else {
        printf("Highest / Lowest Average    : N/A (no graded students)\n");
    }

    printf("Passing Students (>= 50%%)   : %d / %d graded\n", passing_students, graded_students);
    printf("Failing Students (< 50%%)    : %d / %d graded\n", failing_students, graded_students);
    if (ungraded_students > 0) {
        printf("Not Graded (0 Courses)      : %d\n", ungraded_students);
    }

    printf("\nAverage Performance Per Course:\n");
    if (c_count == 0) {
        printf("  No course enrollments recorded.\n");
    } else {
        printf("  %-12s %-28s %10s %10s\n", "Code", "Course Name", "Students", "Average");
        for (i = 0; i < c_count; i++) {
            float c_avg = (c_stats[i].sum_max > 0.0f)
                              ? (c_stats[i].sum_obtained / c_stats[i].sum_max) * 100.0f
                              : 0.0f;
            printf("  %-12.12s %-28.28s %10d %9.2f%%\n",
                   c_stats[i].code, c_stats[i].name, c_stats[i].enrollments, c_avg);
        }
    }

    printf("\nStudents by Programme of Study:\n");
    for (i = 0; i < p_count; i++) {
        printf("  %-36s : %d student(s)\n", p_stats[i].programme, p_stats[i].students);
    }

    free(c_stats);
    free(p_stats);
}
