#!/usr/bin/env bash
set -euo pipefail

P5_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$P5_DIR"

echo "[1/4] Testing invalid CLI usage rejection..."
if ./document_analyzer >/dev/null 2>&1; then
    echo "FAIL: Expected non-zero exit when called with missing arguments" >&2
    exit 1
fi
if ./document_analyzer "" samples/document1.txt >/dev/null 2>&1; then
    echo "FAIL: Expected non-zero exit for empty search keyword" >&2
    exit 1
fi

echo "[2/4] Testing multi-threaded search across 4 sample files (including 0-match file)..."
./document_analyzer "IoT" \
    samples/document1.txt \
    samples/document2.txt \
    samples/document3.txt \
    samples/zero_matches.txt

mkdir -p sample_reports
cp -f samples/document1_report.txt sample_reports/document1_report.txt
cp -f samples/document2_report.txt sample_reports/document2_report.txt
cp -f samples/document3_report.txt sample_reports/document3_report.txt
cp -f samples/zero_matches_report.txt sample_reports/zero_matches_report.txt

grep -q "Total Keyword Matches   : 5" sample_reports/document1_report.txt
grep -q "Matching Line Numbers   : 1, 2, 4, 6, 7" sample_reports/document1_report.txt
grep -q "Total Keyword Matches   : 4" sample_reports/document2_report.txt
grep -q "Matching Line Numbers   : 2, 5" sample_reports/document2_report.txt
grep -q "Total Keyword Matches   : 3" sample_reports/document3_report.txt
grep -q "Total Keyword Matches   : 0" sample_reports/zero_matches_report.txt
grep -q "Completed (keyword not found)" sample_reports/zero_matches_report.txt

echo "[3/4] Testing graceful handling of missing input file alongside valid sibling threads..."
set +e
./document_analyzer "IoT" samples/document1.txt samples/missing_input.txt samples/document3.txt > /tmp/p5_err_test.log 2>&1
rc=$?
set -e
if [ "$rc" -eq 0 ]; then
    echo "FAIL: Expected non-zero exit when an input file is missing" >&2
    exit 1
fi
grep -q "document1.txt: Completed" /tmp/p5_err_test.log
grep -q "missing_input.txt: Failed" /tmp/p5_err_test.log
grep -q "document3.txt: Completed" /tmp/p5_err_test.log

echo "[4/4] Cleaning temporary generated reports inside samples/..."
rm -f samples/*_report.txt /tmp/p5_err_test.log

echo "[PASS] Project 5 multi-threaded tests all passed."
