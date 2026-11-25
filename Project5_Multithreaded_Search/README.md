# Project 5: Multi-Threaded Text Search and Analysis Engine

**Platform:** Linux / WSL POSIX Threads (`gcc -Wall -Wextra -Werror -std=c99 -pthread`)  
**Directory Structure:**
- `include/analyzer.h`, `src/analyzer.c`: Per-thread `FileTask` state, search scanner, report generator, processing summary
- `include/utils.h`, `src/utils.c`: Dynamic `malloc`/`realloc` line reader, case-insensitive substring matcher, report path builder
- `src/main.c`: Argument validation, worker spawning via `pthread_create`, guarded `pthread_join`, heap cleanup
- `samples/`: Test inputs covering multi-match lines, long lines (`>256` bytes), and zero-match files
- `sample_reports/`: Output reports captured during WSL verification
- `tests/test_p5.sh`: Automated CLI and fault-isolation verification script

---

## Architecture & Engineering Choices

1. **Lock-Free Per-Thread `FileTask` Isolation:**
   Giving each worker thread its own independent workspace eliminates contention before it starts. In `main.c`, each input file gets a dedicated `FileTask` slot from a `calloc()` array, writes solely to its own `<input>_report.txt` file, and stores match line numbers inside its own `match_line_nums` heap buffer. Since worker threads never modify shared state during file scanning, there are zero data races and no mutex bottleneck.
2. **Arbitrary-Length Dynamic Line Reader (`read_line_dynamic`):**
   Fixed-size buffers such as `char buf[256]` chop long lines in two and can miss keywords straddling the split. `read_line_dynamic` streams characters from `FILE *fp` starting at 128 bytes and doubles capacity via `realloc()` whenever needed, verified on `samples/document2.txt` where matches appear both early and at the end of lines past 256 bytes.
3. **Guarded Thread Joins & Sibling Fault Isolation (`created[]`):**
   `main.c` tracks thread creation inside a `created[i]` array so `pthread_join` only joins workers that actually launched. If one input file is missing or unreadable (tested in `tests/test_p5.sh` step 3 using `missing_input.txt`), that worker captures `strerror(errno)`, sets `success = 0`, and exits while sibling threads complete their reports safely. All file I/O uses standard C streams with zero `system()` invocations.

---

## Build, Run, and Verification Commands

```bash
cd Project5_Multithreaded_Search

# Option A: Using Makefile
make clean all
./document_analyzer "IoT" samples/document1.txt samples/document2.txt samples/document3.txt samples/zero_matches.txt

# Option B: Direct gcc compilation
gcc -Wall -Wextra -Werror -std=c99 -Iinclude -pthread src/main.c src/analyzer.c src/utils.c -o document_analyzer -pthread

# Run automated fault-isolation & report test suite
bash tests/test_p5.sh
```

### Verified WSL Run Output & Sample Report Excerpt
```text
===== PROCESSING SUMMARY =====
samples/document1.txt: Completed (Occurrences: 5, Matching lines: 5, Report: samples/document1_report.txt)
samples/document2.txt: Completed (Occurrences: 4, Matching lines: 2, Report: samples/document2_report.txt)
samples/document3.txt: Completed (Occurrences: 3, Matching lines: 2, Report: samples/document3_report.txt)
samples/zero_matches.txt: Completed (Occurrences: 0, Matching lines: 0, Report: samples/zero_matches_report.txt)

All processing tasks finished. Succeeded: 4 | Failed: 0.
[PASS] Project 5 multi-threaded tests all passed.
```

Sample report (`sample_reports/document1_report.txt`):
```text
Document Analysis Report
--------------------------------------------------
Input File              : samples/document1.txt
Search Keyword          : IoT (case-insensitive)
Total Lines Processed   : 7
Total Keyword Matches   : 5
Lines With Match        : 5
Matching Line Numbers   : 1, 2, 4, 6, 7
Processing Status       : Completed
```
