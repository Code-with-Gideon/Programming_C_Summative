#define _CRT_SECURE_NO_WARNINGS
#include "../include/analytics.h"
#include "../include/storage.h"
#include "../include/student.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>

static void print_main_menu(void) {
    printf("\nStudent Academic Records and Analytics Menu\n");
    printf("1.  Register a new student\n");
    printf("2.  Display all registered students\n");
    printf("3.  View detailed student information\n");
    printf("4.  Update student information\n");
    printf("5.  Delete a student record\n");
    printf("6.  Add a course to a student\n");
    printf("7.  Update marks for a course\n");
    printf("8.  Display a student's academic performance\n");
    printf("9.  Search students (ID, name, or programme)\n");
    printf("10. Sort students\n");
    printf("11. Generate academic performance summary report\n");
    printf("12. Save / export records (Binary or CSV)\n");
    printf("13. Load / import records (Binary or CSV)\n");
    printf("14. Load sample academic dataset (5 students)\n");
    printf("0.  Save and Exit\n");
}

int main(void) {
    StudentRegistry reg;
    int choice;
    int status;
    int load_status;

    printf("Student ID: 1376251039 | Name: Erioluwa Gideon Olowoyo\n");
    printf("Project 3: Student Academic Records and Performance Analytics System\n");

    if (!registry_init(&reg)) {
        fprintf(stderr, "Fatal: Failed to allocate initial student registry.\n");
        return EXIT_FAILURE;
    }

    load_status = load_registry_binary(&reg, DEFAULT_BIN_PATH);
    if (load_status == 1) {
        printf("Loaded %d saved record(s) from '%s'.\n", reg.count, DEFAULT_BIN_PATH);
    } else if (load_status == -1) {
        printf("No saved binary file at '%s'. Starting with an empty registry.\n", DEFAULT_BIN_PATH);
    }

    while (1) {
        print_main_menu();
        status = read_int_range("Choice (0-14): ", 0, 14, &choice);
        if (status == 0) {
            break;
        }
        if (status < 0) {
            printf("Invalid choice. Enter a number from 0 to 14.\n");
            continue;
        }
        if (choice == 0) {
            break;
        }

        switch (choice) {
            case 1:  op_register_student(&reg); break;
            case 2:  op_display_all_students(&reg); break;
            case 3:  op_view_student_detail(&reg); break;
            case 4:  op_update_student(&reg); break;
            case 5:  op_delete_student(&reg); break;
            case 6:  op_add_course_to_student(&reg); break;
            case 7:  op_update_course_marks(&reg); break;
            case 8:  op_display_student_performance(&reg); break;
            case 9:  op_search_students(&reg); break;
            case 10: op_sort_students(&reg); break;
            case 11: op_generate_academic_report(&reg); break;
            case 12: op_save_records(&reg); break;
            case 13: op_load_records(&reg); break;
            case 14: {
                int added = seed_sample_students(&reg);
                printf("Seeded %d sample student(s). Total students: %d.\n", added, reg.count);
                break;
            }
            default: break;
        }
    }

    if (save_registry_binary(&reg, DEFAULT_BIN_PATH)) {
        printf("\nSaved %d record(s) to '%s'.\n", reg.count, DEFAULT_BIN_PATH);
    }
    registry_free(&reg);
    printf("Freed all student and course allocations. Exiting.\n");
    return EXIT_SUCCESS;
}
