#ifndef P3_ANALYTICS_H
#define P3_ANALYTICS_H

#include "student.h"

typedef enum {
    SORT_BY_ID = 1,
    SORT_BY_NAME = 2,
    SORT_BY_AVERAGE = 3,
    SORT_BY_PROGRAMME = 4
} StudentSortField;

float calculate_student_average(const Student *stu);

void op_search_students(const StudentRegistry *reg);
void manual_sort_students(StudentRegistry *reg, StudentSortField field);
void op_sort_students(StudentRegistry *reg);
void op_generate_academic_report(const StudentRegistry *reg);

#endif
