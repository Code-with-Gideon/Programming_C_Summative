#ifndef P3_STUDENT_H
#define P3_STUDENT_H

#define CODE_LEN      16
#define COURSE_LEN    64
#define NAME_LEN      40
#define PROG_LEN      64
#define INIT_REG_CAP  4
#define INIT_CRS_CAP  4

typedef struct {
    char code[CODE_LEN];
    char name[COURSE_LEN];
    float marks_obtained;
    float max_marks;
} Course;

typedef struct {
    int student_id;
    char first_name[NAME_LEN];
    char last_name[NAME_LEN];
    char programme[PROG_LEN];
    int year_of_study;
    int num_courses;
    int course_cap;
    Course *courses;
} Student;

typedef struct {
    Student *items;
    int count;
    int capacity;
} StudentRegistry;

int registry_init(StudentRegistry *reg);
void registry_free(StudentRegistry *reg);

Student *find_student_by_id(const StudentRegistry *reg, int student_id);
Course *find_course_by_code(const Student *stu, const char *code);

int registry_add_student_record(StudentRegistry *reg, const Student *src);
int student_add_course_record(Student *stu, const Course *crs);

void op_register_student(StudentRegistry *reg);
void op_display_all_students(const StudentRegistry *reg);
void op_view_student_detail(const StudentRegistry *reg);
void op_update_student(StudentRegistry *reg);
void op_delete_student(StudentRegistry *reg);
void op_add_course_to_student(StudentRegistry *reg);
void op_update_course_marks(StudentRegistry *reg);
void op_display_student_performance(const StudentRegistry *reg);

#endif
