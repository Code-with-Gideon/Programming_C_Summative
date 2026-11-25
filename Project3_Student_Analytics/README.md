# Project 3: Student Academic Records and Performance Analytics System

**Platform:** Linux / WSL (`gcc -Wall -Wextra -Werror -std=c99`)  
**Directory Structure:**
- `include/student.h`, `src/student.c`: Dynamic `StudentRegistry` and nested `Course *` CRUD operations
- `include/analytics.h`, `src/analytics.c`: Case-insensitive search, in-place Selection Sort, 8-metric summary report
- `include/storage.h`, `src/storage.c`: Binary (`SREC`) serializer, CSV exporter/importer, 5-student sample loader
- `include/utils.h`, `src/utils.c`: Range-checked `strtol`/`strtof` input parsing and string helpers
- `src/main.c`: 15-option interactive console menu (`0` saves and frees all memory before exit)
- `tests/test_p3.c`: Unit tests covering bounds, duplicate guards, weighted math, sorting, and storage round-trips

---

## Architecture & Engineering Choices

1. **Two-Level Dynamic Heap Ownership (`StudentRegistry` + Nested `Course *`):**
   Fixed arrays either waste memory or cut off records as classes grow. `StudentRegistry` manages a heap-allocated `Student *items` buffer via `calloc()` and `realloc()`, while each `Student` owns its own dynamic `Course *courses` array. Deleting a student frees `target->courses` before shifting elements, and `registry_free` releases every allocation before program exit.
2. **Weighted Course Averages & `Not Graded` Isolation:**
   Because courses carry different maximum scores (such as `50` in Data Structures vs. `100` in Operating Systems), `calculate_student_average` divides total obtained marks by total maximum marks across all enrolled courses (`(sum_obt / sum_max) * 100.0f`). Graded students pass at `>= 50%` weighted overall average (`PASS`), while `0`-course students return `-1.0f` and display separately as `N/A` / `Not Graded (0 Courses)` so they never divide by zero or skew failure counts.
3. **In-Place Selection Sort (`manual_sort_students`):**
   `manual_sort_students` walks the array with `Student *` pointers and swaps full structs using `compare_students_manual`. It supports ordering by ID, last/first name, overall weighted average (placing ungraded students cleanly at the end), and programme of study.
4. **Dual Binary (`SREC`) and CSV Persistence:**
   Writing raw `Student` structs straight to disk would serialize stale memory addresses for `courses`. To preserve full structure cleanly, `save_registry_binary` writes an `"SREC"` magic header + version, a `StudentDiskHeader` containing `num_courses`, and that student's contiguous `Course` records so `load_registry_binary` can reconstruct every nested heap array on load. Human-readable CSV export and import (`data/students_export.csv`) run alongside binary storage.

---

## Build, Run, and Verification Commands

```bash
cd Project3_Student_Analytics

# Option A: Using Makefile
make clean all test
./student_analytics

# Option B: Direct gcc compilation
gcc -Wall -Wextra -Werror -std=c99 -Iinclude src/main.c src/student.c src/analytics.c src/storage.c src/utils.c -o student_analytics -lm
```

### Verified WSL Compiler & Interactive Summary Excerpt
```text
[PASS] Project 3 modular C CRUD, manual sorting, and dual-persistence tests succeeded.

Academic Performance Summary Report
-------------------------------------------------------------
Total Registered Students   : 5
Total Registered Courses    : 10
Overall Cohort Performance  : 74.82% (weighted marks/max)
Highest Student Average     : Grace Mensah (ID 1003) - 92.00%
Lowest Student Average      : Tariq El-Amin (ID 1004) - 42.00%
Passing Students (>= 50%)   : 4 / 5 graded
Failing Students (< 50%)    : 1 / 5 graded
```
