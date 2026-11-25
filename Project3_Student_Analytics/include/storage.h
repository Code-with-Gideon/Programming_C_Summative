#ifndef P3_STORAGE_H
#define P3_STORAGE_H

#include "student.h"

#define DEFAULT_BIN_PATH "data/students.bin"
#define DEFAULT_CSV_PATH "data/students_export.csv"

int save_registry_binary(const StudentRegistry *reg, const char *path);
int load_registry_binary(StudentRegistry *reg, const char *path);

int export_registry_csv(const StudentRegistry *reg, const char *path);
int import_registry_csv(StudentRegistry *reg, const char *path);

int seed_sample_students(StudentRegistry *reg);

void op_save_records(const StudentRegistry *reg);
void op_load_records(StudentRegistry *reg);

#endif
