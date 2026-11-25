#define _CRT_SECURE_NO_WARNINGS
#include "../include/analytics.h"
#include "../include/storage.h"
#include "../include/student.h"
#include "../include/utils.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void test_crud_validation_and_math(void) {
    StudentRegistry reg;
    Student s1;
    Student s2;
    Student *found;
    Course c_bad;
    Course c1;
    Course c2;

    assert(registry_init(&reg) == 1);

    /* Student 1 with 2 courses (different max_marks) */
    memset(&s1, 0, sizeof(s1));
    s1.student_id = 102;
    strcpy(s1.first_name, "Zainab");
    strcpy(s1.last_name, "Bello");
    strcpy(s1.programme, "Software Engineering");
    s1.year_of_study = 2;
    assert(registry_add_student_record(&reg, &s1) == 1);

    /* Duplicate ID rejected */
    assert(registry_add_student_record(&reg, &s1) == -1);
    assert(reg.count == 1);

    /* Student 2 with 0 courses -> div-by-zero protected */
    memset(&s2, 0, sizeof(s2));
    s2.student_id = 101;
    strcpy(s2.first_name, "Amina");
    strcpy(s2.last_name, "Uwimana");
    strcpy(s2.programme, "Computer Science");
    s2.year_of_study = 1;
    assert(registry_add_student_record(&reg, &s2) == 1);

    found = find_student_by_id(&reg, 101);
    assert(found != NULL);
    assert(calculate_student_average(found) < 0.0f);

    found = find_student_by_id(&reg, 102);
    assert(found != NULL);

    /* Invalid marks > max_marks rejected */
    memset(&c_bad, 0, sizeof(c_bad));
    strcpy(c_bad.code, "CS201");
    strcpy(c_bad.name, "Systems Programming");
    c_bad.max_marks = 50.0f;
    c_bad.marks_obtained = 60.0f;
    assert(student_add_course_record(found, &c_bad) == 0);

    memset(&c1, 0, sizeof(c1));
    strcpy(c1.code, "CS201");
    strcpy(c1.name, "Systems Programming");
    c1.max_marks = 50.0f;
    c1.marks_obtained = 40.0f;
    assert(student_add_course_record(found, &c1) == 1);

    /* Duplicate course rejected */
    assert(student_add_course_record(found, &c1) == -1);

    memset(&c2, 0, sizeof(c2));
    strcpy(c2.code, "CS202");
    strcpy(c2.name, "Algorithms");
    c2.max_marks = 100.0f;
    c2.marks_obtained = 80.0f;
    assert(student_add_course_record(found, &c2) == 1);

    /* Weighted average = (40 + 80) / (50 + 100) * 100 = 80.00% */
    assert(fabsf(calculate_student_average(found) - 80.0f) < 0.01f);

    registry_free(&reg);
}

static void test_manual_sorting_and_persistence(void) {
    StudentRegistry reg;
    StudentRegistry bin_loaded;
    StudentRegistry csv_loaded;
    int i;

    assert(registry_init(&reg) == 1);
    assert(seed_sample_students(&reg) == 5);

    /* Add a 0-course student too */
    {
        Student s0;
        memset(&s0, 0, sizeof(s0));
        s0.student_id = 1099;
        strcpy(s0.first_name, "Brian");
        strcpy(s0.last_name, "Mutua");
        strcpy(s0.programme, "Computer Engineering");
        s0.year_of_study = 1;
        assert(registry_add_student_record(&reg, &s0) == 1);
    }

    /* Manual Sort by Name */
    manual_sort_students(&reg, SORT_BY_NAME);
    for (i = 0; i < reg.count - 1; i++) {
        assert(str_casecmp_local(reg.items[i].last_name, reg.items[i + 1].last_name) <= 0);
    }

    /* Manual Sort by Average (descending, 0-course last) */
    manual_sort_students(&reg, SORT_BY_AVERAGE);
    assert(calculate_student_average(&reg.items[0]) >= calculate_student_average(&reg.items[1]));
    assert(calculate_student_average(&reg.items[reg.count - 1]) < 0.0f);

    /* Manual Sort by ID */
    manual_sort_students(&reg, SORT_BY_ID);
    for (i = 0; i < reg.count - 1; i++) {
        assert(reg.items[i].student_id < reg.items[i + 1].student_id);
    }

    /* Manual Sort by Programme */
    manual_sort_students(&reg, SORT_BY_PROGRAMME);
    for (i = 0; i < reg.count - 1; i++) {
        assert(str_casecmp_local(reg.items[i].programme, reg.items[i + 1].programme) <= 0);
    }

    /* Binary round-trip reconstructing nested dynamic courses */
    assert(save_registry_binary(&reg, "test_students.bin") == 1);
    assert(registry_init(&bin_loaded) == 1);
    assert(load_registry_binary(&bin_loaded, "test_students.bin") == 1);
    assert(bin_loaded.count == reg.count);
    for (i = 0; i < reg.count; i++) {
        assert(bin_loaded.items[i].student_id == reg.items[i].student_id);
        assert(bin_loaded.items[i].num_courses == reg.items[i].num_courses);
    }

    /* CSV round-trip preserving both graded and 0-course students */
    assert(export_registry_csv(&reg, "test_students.csv") == 1);
    assert(registry_init(&csv_loaded) == 1);
    assert(import_registry_csv(&csv_loaded, "test_students.csv") == 1);
    assert(csv_loaded.count == reg.count);

    remove("test_students.bin");
    remove("test_students.csv");

    registry_free(&reg);
    registry_free(&bin_loaded);
    registry_free(&csv_loaded);
}

int main(void) {
    test_crud_validation_and_math();
    test_manual_sorting_and_persistence();
    printf("[PASS] Project 3 modular C CRUD, manual sorting, and dual-persistence tests succeeded.\n");
    return 0;
}
