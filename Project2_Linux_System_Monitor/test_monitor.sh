#!/usr/bin/env bash
set -euo pipefail

P2_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT="$P2_DIR/system_monitor.sh"

echo "[1/5] Checking Bash syntax..."
bash -n "$SCRIPT"

source "$SCRIPT"

TMP_WORK="$(mktemp -d)"
trap 'rm -rf "$TMP_WORK"' EXIT
LOG_FILE="$P2_DIR/sample_logs/sample_activity.log"
mkdir -p "$P2_DIR/sample_logs" "$P2_DIR/sample_reports"
: > "$LOG_FILE"

echo "[2/5] Testing threshold validation and protected process guards..."
if validate_threshold "abc" || validate_threshold "-5" || validate_threshold "101"; then
    echo "FAIL: Invalid threshold accepted" >&2
    exit 1
fi
validate_threshold "80"

if terminate_process <<< "1" >/dev/null 2>&1; then
    echo "FAIL: Allowed terminating PID 1" >&2
    exit 1
fi

if terminate_process <<< "999999" >/dev/null 2>&1; then
    echo "FAIL: Allowed nonexistent PID" >&2
    exit 1
fi

echo "[3/5] Testing system telemetry functions and sample health report..."
display_system_info
monitor_cpu <<< "1"
monitor_memory <<< "50"
monitor_disk <<< "50"

REPORT_SUB="$TMP_WORK/rep"
mkdir -p "$REPORT_SUB"
generate_health_report <<< "$REPORT_SUB"
REPORT_FILE="$(ls -1 "$REPORT_SUB"/system_health_report_*.txt | awk 'NR==1')"
cp -f "$REPORT_FILE" "$P2_DIR/sample_reports/sample_health_report.txt"

echo "[4/5] Testing unwritable directory handling..."
mkdir -p "$TMP_WORK/locked"
chmod 555 "$TMP_WORK/locked"
if [ "$(id -u)" -ne 0 ]; then
    if generate_health_report <<< "$TMP_WORK/locked" >/dev/null 2>&1; then
        echo "FAIL: Allowed writing report to read-only folder" >&2
        chmod 755 "$TMP_WORK/locked"
        exit 1
    fi
fi
chmod 755 "$TMP_WORK/locked"

echo "[5/5] Testing process search and safe child process termination..."
sleep 25 &
SLEEP_PID=$!
sleep 0.2
view_and_search_processes <<< "$SLEEP_PID"
terminate_process <<< "$(printf '%s\ny\n' "$SLEEP_PID")"
wait "$SLEEP_PID" 2>/dev/null || true

view_activity_log

echo "[PASS] Project 2 Linux System Health Monitor tests succeeded."
