#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODE="${1:-standard}"

if [ "$MODE" != "standard" ] && [ "$MODE" != "--asan" ]; then
    echo "Usage: $0 [--asan]" >&2
    exit 1
fi

ASAN_FLAGS=""
RUN_PREFIX=""
if [ "$MODE" = "--asan" ]; then
    ASAN_FLAGS="-fsanitize=address,undefined -g -fno-omit-frame-pointer"
    if command -v setarch >/dev/null 2>&1; then
        RUN_PREFIX="setarch x86_64 -R"
    fi
    echo "=== Running Full Summative Verification (AddressSanitizer + UBSan Mode) ==="
else
    echo "=== Running Full Summative Verification (Standard C99 + Bash Suite) ==="
fi

echo ""
echo "[1/5] Project 1: Smart Occupancy Core Logic Unit Tests..."
P1_BIN="/tmp/test_p1_summative"
gcc -Wall -Wextra -Werror -std=c99 $ASAN_FLAGS \
    "$ROOT_DIR/Project1_Smart_Occupancy/logic_test/occupancy_core.c" \
    "$ROOT_DIR/Project1_Smart_Occupancy/logic_test/test_occupancy.c" \
    -o "$P1_BIN"
$RUN_PREFIX "$P1_BIN"
rm -f "$P1_BIN"

echo ""
echo "[2/5] Project 2: Linux System Health Monitor Suite..."
bash "$ROOT_DIR/Project2_Linux_System_Monitor/test_monitor.sh"

echo ""
echo "[3/5] Project 3: Student Academic Records & Analytics..."
if [ "$MODE" = "--asan" ]; then
    P3_BIN="/tmp/test_p3_asan"
    gcc -Wall -Wextra -Werror -std=c99 $ASAN_FLAGS -I"$ROOT_DIR/Project3_Student_Analytics/include" \
        "$ROOT_DIR/Project3_Student_Analytics/tests/test_p3.c" \
        "$ROOT_DIR/Project3_Student_Analytics/src/student.c" \
        "$ROOT_DIR/Project3_Student_Analytics/src/analytics.c" \
        "$ROOT_DIR/Project3_Student_Analytics/src/storage.c" \
        "$ROOT_DIR/Project3_Student_Analytics/src/utils.c" \
        -o "$P3_BIN" -lm
    (cd "$ROOT_DIR/Project3_Student_Analytics" && $RUN_PREFIX "$P3_BIN")
    rm -f "$P3_BIN"
else
    make -C "$ROOT_DIR/Project3_Student_Analytics" clean all test
fi

echo ""
echo "[4/5] Project 4: Sales Transaction Processing & Callbacks..."
if [ "$MODE" = "--asan" ]; then
    P4_BIN="/tmp/test_p4_asan"
    gcc -Wall -Wextra -Werror -std=c99 $ASAN_FLAGS -I"$ROOT_DIR/Project4_Sales_Analysis/include" \
        "$ROOT_DIR/Project4_Sales_Analysis/tests/test_p4.c" \
        "$ROOT_DIR/Project4_Sales_Analysis/src/sales.c" \
        "$ROOT_DIR/Project4_Sales_Analysis/src/analytics.c" \
        "$ROOT_DIR/Project4_Sales_Analysis/src/storage.c" \
        "$ROOT_DIR/Project4_Sales_Analysis/src/utils.c" \
        -o "$P4_BIN" -lm
    (cd "$ROOT_DIR/Project4_Sales_Analysis" && $RUN_PREFIX "$P4_BIN")
    rm -f "$P4_BIN"
else
    make -C "$ROOT_DIR/Project4_Sales_Analysis" clean all test
fi

echo ""
echo "[5/5] Project 5: Multi-Threaded Document Search Engine..."
if [ "$MODE" = "--asan" ]; then
    P5_BIN="/tmp/document_analyzer_asan"
    gcc -Wall -Wextra -Werror -std=c99 $ASAN_FLAGS -pthread -I"$ROOT_DIR/Project5_Multithreaded_Search/include" \
        "$ROOT_DIR/Project5_Multithreaded_Search/src/main.c" \
        "$ROOT_DIR/Project5_Multithreaded_Search/src/analyzer.c" \
        "$ROOT_DIR/Project5_Multithreaded_Search/src/utils.c" \
        -o "$P5_BIN" -pthread
    (
        cd "$ROOT_DIR/Project5_Multithreaded_Search"
        $RUN_PREFIX "$P5_BIN" "IoT" samples/document1.txt samples/document2.txt samples/document3.txt samples/zero_matches.txt
        set +e
        $RUN_PREFIX "$P5_BIN" "IoT" samples/document1.txt samples/missing_input.txt samples/document3.txt >/dev/null 2>&1
        rc=$?
        set -e
        if [ "$rc" -eq 0 ]; then
            echo "FAIL: Expected non-zero exit on missing_input.txt" >&2
            exit 1
        fi
        rm -f samples/*_report.txt
    )
    rm -f "$P5_BIN"
    echo "[PASS] Project 5 multi-threaded ASan + UBSan checks passed."
else
    make -C "$ROOT_DIR/Project5_Multithreaded_Search" clean all
    bash "$ROOT_DIR/Project5_Multithreaded_Search/tests/test_p5.sh"
fi

echo ""
echo "============================================================="
echo " [PASS] Project 1 - Host C99 Occupancy Core & Debounce Tests"
echo " [PASS] Project 2 - Linux System Health & Process Guard Suite"
echo " [PASS] Project 3 - Modular Student Analytics & Persistence"
echo " [PASS] Project 4 - Sales Function Pointers & Callback Suite"
echo " [PASS] Project 5 - POSIX Pthread Concurrent Search Engine"
echo "============================================================="
echo "[SCOREBOARD] All 5 project checks passed ($MODE mode)."
