#!/usr/bin/env bash
# Linux System Health and Resource Monitoring Manager
# Modular bash utility for system inspection, threshold alerts, process control, and reporting

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_FILE="${SCRIPT_DIR}/system_monitor_activity.log"
DEFAULT_REPORT_DIR="${SCRIPT_DIR}/reports"

CPU_THRESHOLD=80
MEM_THRESHOLD=80
DISK_THRESHOLD=85

require_cmd() {
    local cmd="$1"
    if ! command -v "$cmd" >/dev/null 2>&1; then
        printf "Error: Required system command '%s' is not available on this host.\n" "$cmd" >&2
        log_activity "ERROR" "Missing required utility: $cmd"
        return 1
    fi
    return 0
}

log_activity() {
    local level="$1"
    local message="$2"
    local ts
    ts="$(date '+%Y-%m-%d %H:%M:%S')"
    if ! printf "[%s] [%s] %s\n" "$ts" "$level" "$message" >> "$LOG_FILE" 2>/dev/null; then
        printf "Warning: Could not append to activity log '%s'.\n" "$LOG_FILE" >&2
    fi
}

validate_threshold() {
    local val="$1"
    if [[ ! "$val" =~ ^[0-9]+$ ]] || [ "$val" -lt 1 ] || [ "$val" -gt 100 ]; then
        return 1
    fi
    return 0
}

prompt_threshold() {
    local label="$1"
    local current_val="$2"
    local raw
    printf "Current %s warning threshold is %d%%. Enter new threshold (1-100, Enter to keep): " "$label" "$current_val" >&2
    read -r raw
    if [ -z "$raw" ]; then
        printf "%d" "$current_val"
        return 0
    fi
    if ! validate_threshold "$raw"; then
        printf "Invalid threshold '%s'. Must be a whole number from 1 to 100.\n" "$raw" >&2
        return 1
    fi
    printf "%d" "$raw"
    return 0
}

confirm_action() {
    local prompt="$1"
    local reply
    printf "%s [y/N]: " "$prompt"
    read -r reply
    case "$reply" in
        [yY]|[yY][eE][sS]) return 0 ;;
        *) return 1 ;;
    esac
}

display_system_info() {
    local host os_name kernel up_info now_str
    host="$(hostname 2>/dev/null || echo 'unknown')"
    if [ -r /etc/os-release ]; then
        os_name="$(awk -F= '/^PRETTY_NAME=/{gsub(/"/, "", $2); print $2}' /etc/os-release)"
    else
        os_name="$(uname -s 2>/dev/null || echo 'Linux')"
    fi
    kernel="$(uname -r 2>/dev/null || echo 'unknown')"
    up_info="$(uptime -p 2>/dev/null || uptime 2>/dev/null || echo 'unavailable')"
    now_str="$(date '+%Y-%m-%d %H:%M:%S %Z')"

    printf "\n=== System Information ===\n"
    printf "Hostname         : %s\n" "$host"
    printf "Operating System : %s\n" "${os_name:-Linux}"
    printf "Kernel Version   : %s\n" "$kernel"
    printf "System Uptime    : %s\n" "$up_info"
    printf "Current Time     : %s\n" "$now_str"

    log_activity "INFO" "Inspected system information (host=$host, kernel=$kernel)"
}

get_cpu_usage_pct() {
    if [ -r /proc/stat ]; then
        local c1 c2
        c1="$(awk '/^cpu /{print $2+$3+$4+$5+$6+$7+$8, $5+$6}' /proc/stat)"
        sleep 0.5
        c2="$(awk '/^cpu /{print $2+$3+$4+$5+$6+$7+$8, $5+$6}' /proc/stat)"
        LC_ALL=C awk -v a="$c1" -v b="$c2" 'BEGIN{
            split(a, p, " "); split(b, q, " ");
            dt = q[1] - p[1]; di = q[2] - p[2];
            if (dt <= 0) { printf "0.0"; } else { printf "%.1f", (dt - di) * 100.0 / dt; }
        }'
        return 0
    elif command -v top >/dev/null 2>&1; then
        LC_ALL=C top -bn1 | awk '/Cpu\(s\)/{for(i=1;i<=NF;i++) if($i ~ /id,?/){gsub(/[^0-9.]/,"",$(i-1)); printf "%.1f", 100.0 - $(i-1); exit}}'
        return 0
    fi
    return 1
}

monitor_cpu() {
    local new_t cpu_pct cpu_int
    if new_t="$(prompt_threshold "CPU" "$CPU_THRESHOLD")"; then
        CPU_THRESHOLD="$new_t"
    else
        return 1
    fi

    if ! cpu_pct="$(get_cpu_usage_pct)"; then
        printf "Error: Unable to read CPU telemetry on this host.\n" >&2
        log_activity "ERROR" "CPU monitoring failed: telemetry unavailable"
        return 1
    fi

    printf "\n=== CPU Utilization ===\n"
    printf "Current CPU Usage : %s%% (Warning Threshold: %d%%)\n" "$cpu_pct" "$CPU_THRESHOLD"

    cpu_int="${cpu_pct%%.*}"
    if [ -n "$cpu_int" ] && [ "$cpu_int" -ge "$CPU_THRESHOLD" ]; then
        printf "[WARNING] CPU utilization (%s%%) reached/exceeded threshold (%d%%)!\n" "$cpu_pct" "$CPU_THRESHOLD"
        log_activity "WARNING" "CPU threshold exceeded: ${cpu_pct}% >= ${CPU_THRESHOLD}%"
    else
        log_activity "INFO" "CPU check normal: ${cpu_pct}% (threshold ${CPU_THRESHOLD}%)"
    fi

    if require_cmd ps; then
        printf "\nTop 5 CPU-Consuming Processes:\n"
        ps -eo pid,user,%cpu,%mem,comm --sort=-%cpu 2>/dev/null | awk 'NR<=6'
    fi
}

get_memory_stats() {
    if [ -r /proc/meminfo ]; then
        LC_ALL=C awk '
            /^MemTotal:/     { total = $2 / 1024.0 }
            /^MemAvailable:/ { avail = $2 / 1024.0 }
            /^MemFree:/      { mfree = $2 / 1024.0 }
            END {
                if (total <= 0) exit 1;
                if (avail <= 0) avail = mfree;
                used = total - avail;
                pct = (used * 100.0) / total;
                printf "%.0f %.0f %.0f %.1f", total, used, avail, pct;
            }' /proc/meminfo
        return 0
    elif command -v free >/dev/null 2>&1; then
        LC_ALL=C free -m | awk '/^Mem:/ { printf "%d %d %d %.1f", $2, $3, $7, ($3 * 100.0) / $2 }'
        return 0
    fi
    return 1
}

monitor_memory() {
    local new_t stats total_mb used_mb avail_mb mem_pct mem_int
    if new_t="$(prompt_threshold "Memory" "$MEM_THRESHOLD")"; then
        MEM_THRESHOLD="$new_t"
    else
        return 1
    fi

    if ! stats="$(get_memory_stats)"; then
        printf "Error: Unable to collect memory metrics on this host.\n" >&2
        log_activity "ERROR" "Memory monitoring failed: telemetry unavailable"
        return 1
    fi

    read -r total_mb used_mb avail_mb mem_pct <<< "$stats"
    printf "\n=== Memory Utilization ===\n"
    printf "Total Memory      : %s MB\n" "$total_mb"
    printf "Used Memory       : %s MB\n" "$used_mb"
    printf "Available Memory  : %s MB\n" "$avail_mb"
    printf "Utilization       : %s%% (Warning Threshold: %d%%)\n" "$mem_pct" "$MEM_THRESHOLD"

    mem_int="${mem_pct%%.*}"
    if [ -n "$mem_int" ] && [ "$mem_int" -ge "$MEM_THRESHOLD" ]; then
        printf "[WARNING] Memory utilization (%s%%) exceeded threshold (%d%%)!\n" "$mem_pct" "$MEM_THRESHOLD"
        log_activity "WARNING" "Memory threshold exceeded: ${mem_pct}% >= ${MEM_THRESHOLD}%"
    else
        log_activity "INFO" "Memory check normal: ${mem_pct}% (${used_mb}/${total_mb} MB)"
    fi

    if require_cmd ps; then
        printf "\nTop 5 Memory-Consuming Processes:\n"
        ps -eo pid,user,%mem,%cpu,comm --sort=-%mem 2>/dev/null | awk 'NR<=6'
    fi
}

monitor_disk() {
    local new_t alerts=0
    if ! require_cmd df; then
        return 1
    fi

    if new_t="$(prompt_threshold "Disk" "$DISK_THRESHOLD")"; then
        DISK_THRESHOLD="$new_t"
    else
        return 1
    fi

    printf "\n=== Disk Space Utilization (Threshold: %d%%) ===\n" "$DISK_THRESHOLD"
    df -P -h -x tmpfs -x devtmpfs -x squashfs -x efivarfs 2>/dev/null || df -P -h

    while read -r fs size used avail pct mount; do
        local pct_num="${pct%%%}"
        if [[ "$pct_num" =~ ^[0-9]+$ ]] && [ "$pct_num" -ge "$DISK_THRESHOLD" ]; then
            printf "[WARNING] Filesystem '%s' mounted at '%s' is at %s%% (>= %d%%)!\n" \
                   "$fs" "$mount" "$pct_num" "$DISK_THRESHOLD"
            log_activity "WARNING" "Disk threshold exceeded on $mount ($fs): ${pct_num}% >= ${DISK_THRESHOLD}%"
            alerts=$((alerts + 1))
        fi
    done < <( (df -P -h -x tmpfs -x devtmpfs -x squashfs -x efivarfs 2>/dev/null || df -P -h) | awk 'NR>1 {print $1, $2, $3, $4, $5, $6}' )

    if [ "$alerts" -eq 0 ]; then
        log_activity "INFO" "Disk check normal across mounted filesystems (threshold ${DISK_THRESHOLD}%)"
    fi
}

view_and_search_processes() {
    local query
    if ! require_cmd ps; then
        return 1
    fi

    printf "\nTop 15 Active Processes (Sorted by CPU):\n"
    ps -eo pid,ppid,user,%cpu,%mem,stat,comm --sort=-%cpu 2>/dev/null | awk 'NR<=16'

    printf "\nEnter PID or process name keyword to search (or press Enter to skip): "
    read -r query
    if [ -z "$query" ]; then
        return 0
    fi

    local matches
    printf "\n=== Search Results for '%s' ===\n" "$query"
    printf "%-8s %-8s %-14s %-7s %-7s %-6s %s\n" "PID" "PPID" "USER" "%CPU" "%MEM" "STAT" "COMMAND"
    if [[ "$query" =~ ^[0-9]+$ ]]; then
        matches="$(ps -p "$query" -o pid=,ppid=,user=,%cpu=,%mem=,stat=,comm= 2>/dev/null | awk '{printf "%-8s %-8s %-14s %-7s %-7s %-6s %s\n", $1,$2,$3,$4,$5,$6,$7}' || true)"
    else
        matches="$(ps -eo pid=,ppid=,user=,%cpu=,%mem=,stat=,comm= 2>/dev/null | grep -iF -- "$query" | awk '{printf "%-8s %-8s %-14s %-7s %-7s %-6s %s\n", $1,$2,$3,$4,$5,$6,$7}' || true)"
    fi

    if [ -z "$matches" ]; then
        printf "No running processes matched '%s'.\n" "$query"
        log_activity "INFO" "Process search returned no matches for query='$query'"
        return 0
    fi
    printf "%s\n" "$matches"

    log_activity "INFO" "Searched running processes for '$query'"
}

is_protected_process() {
    local pid="$1"
    local ppid="$2"
    local comm="$3"

    if [ "$pid" -le 1 ] || [ "$pid" -eq "$$" ] || [ "$pid" -eq "$PPID" ]; then
        return 0
    fi
    if [ "$ppid" -eq 2 ] || [[ "$comm" == \[*\] ]]; then
        return 0
    fi
    case "$comm" in
        init|systemd|kthreadd|sshd|dbus-daemon|systemd-journald|systemd-udevd|login|agetty|wsl|wslinit)
            return 0 ;;
    esac
    return 1
}

terminate_process() {
    local target_pid info ppid owner comm current_user current_uid
    if ! require_cmd ps || ! require_cmd kill; then
        return 1
    fi

    printf "Enter PID of process to terminate: "
    read -r target_pid

    if [[ ! "$target_pid" =~ ^[0-9]+$ ]] || [ "$target_pid" -le 0 ]; then
        printf "Error: Invalid PID '%s'. Must be a positive integer.\n" "$target_pid" >&2
        log_activity "WARNING" "Rejected invalid termination PID: '$target_pid'"
        return 1
    fi

    info="$(ps -p "$target_pid" -o ppid=,user=,comm= 2>/dev/null || true)"
    if [ -z "$info" ]; then
        printf "Error: Process PID %s does not exist.\n" "$target_pid" >&2
        log_activity "WARNING" "Termination failed: nonexistent PID $target_pid"
        return 1
    fi

    read -r ppid owner comm <<< "$info"
    if is_protected_process "$target_pid" "${ppid:-0}" "${comm:-unknown}"; then
        printf "Blocked: Refusing to terminate protected system/session process PID %s (%s).\n" "$target_pid" "$comm" >&2
        log_activity "WARNING" "Blocked termination of protected process PID $target_pid ($comm)"
        return 1
    fi

    current_user="$(id -un 2>/dev/null || whoami)"
    current_uid="$(id -u 2>/dev/null || echo 1000)"
    if [ "$current_uid" -ne 0 ] && [ "$owner" != "$current_user" ]; then
        printf "Error: Insufficient permissions to terminate PID %s (%s), owned by '%s'.\n" "$target_pid" "$comm" "$owner" >&2
        log_activity "WARNING" "Permission denied terminating PID $target_pid owned by $owner"
        return 1
    fi

    printf "Target -> PID: %s | Owner: %s | Command: %s\n" "$target_pid" "$owner" "$comm"
    if ! confirm_action "Are you sure you want to terminate PID $target_pid ($comm)?"; then
        printf "Termination cancelled by user.\n"
        log_activity "INFO" "User cancelled termination of PID $target_pid ($comm)"
        return 0
    fi

    if kill -TERM "$target_pid" 2>/dev/null; then
        printf "Successfully sent SIGTERM to PID %s (%s).\n" "$target_pid" "$comm"
        log_activity "INFO" "Terminated process PID $target_pid ($comm)"
        return 0
    else
        printf "Error: Failed to terminate PID %s (permission denied or process exited).\n" "$target_pid" >&2
        log_activity "ERROR" "Failed to kill PID $target_pid ($comm)"
        return 1
    fi
}

generate_health_report() {
    local target_dir ts_file report_path cpu_pct mem_stats total_mb used_mb avail_mb mem_pct warnings_found=0

    printf "Enter destination directory for health report (Enter for %s): " "$DEFAULT_REPORT_DIR"
    read -r target_dir
    if [ -z "$target_dir" ]; then
        target_dir="$DEFAULT_REPORT_DIR"
    fi
    target_dir="${target_dir/#\~/$HOME}"

    if [ ! -d "$target_dir" ]; then
        if [ -e "$target_dir" ]; then
            printf "Error: Path '%s' exists and is not a directory.\n" "$target_dir" >&2
            log_activity "ERROR" "Report failed: '$target_dir' is not a directory"
            return 1
        fi
        if ! mkdir -p "$target_dir" 2>/dev/null; then
            printf "Error: Cannot create report directory '%s' (permission denied).\n" "$target_dir" >&2
            log_activity "ERROR" "Report failed: cannot create '$target_dir'"
            return 1
        fi
    fi

    if [ ! -w "$target_dir" ]; then
        printf "Error: Directory '%s' is not writable (insufficient permissions).\n" "$target_dir" >&2
        log_activity "ERROR" "Report failed: unwritable directory '$target_dir'"
        return 1
    fi

    ts_file="$(date '+%Y-%m-%d_%H-%M-%S')"
    report_path="${target_dir}/system_health_report_${ts_file}.txt"

    cpu_pct="$(get_cpu_usage_pct 2>/dev/null || echo 'N/A')"
    mem_stats="$(get_memory_stats 2>/dev/null || echo '0 0 0 0.0')"
    read -r total_mb used_mb avail_mb mem_pct <<< "$mem_stats"

    if ! {
        printf "Linux System Health and Resource Report\n"
        printf "==============================================================\n"
        printf "Generated On     : %s\n" "$(date '+%Y-%m-%d %H:%M:%S %Z')"
        printf "Student ID       : 1376251039 (Erioluwa Gideon Olowoyo)\n"
        printf "Hostname         : %s\n" "$(hostname 2>/dev/null || echo 'unknown')"
        printf "Kernel           : %s\n" "$(uname -sr 2>/dev/null || echo 'Linux')"
        printf "Uptime           : %s\n\n" "$(uptime -p 2>/dev/null || uptime)"

        printf -- "--- CPU Summary (Threshold: %d%%) ---\n" "$CPU_THRESHOLD"
        printf "Current CPU Usage: %s%%\n" "$cpu_pct"
        if [ "$cpu_pct" != "N/A" ] && [ "${cpu_pct%%.*}" -ge "$CPU_THRESHOLD" ]; then
            printf "Alert Status     : WARNING (exceeds %d%% threshold)\n" "$CPU_THRESHOLD"
            warnings_found=$((warnings_found + 1))
        else
            printf "Alert Status     : OK\n"
        fi
        printf "\nTop 5 CPU Processes:\n"
        ps -eo pid,user,%cpu,%mem,comm --sort=-%cpu 2>/dev/null | awk 'NR<=6'
        printf "\n"

        printf -- "--- Memory Summary (Threshold: %d%%) ---\n" "$MEM_THRESHOLD"
        printf "Total / Used / Avail: %s MB / %s MB / %s MB (%s%%)\n" "$total_mb" "$used_mb" "$avail_mb" "$mem_pct"
        if [ "${mem_pct%%.*}" -ge "$MEM_THRESHOLD" ]; then
            printf "Alert Status     : WARNING (exceeds %d%% threshold)\n" "$MEM_THRESHOLD"
            warnings_found=$((warnings_found + 1))
        else
            printf "Alert Status     : OK\n"
        fi
        printf "\nTop 5 Memory Processes:\n"
        ps -eo pid,user,%mem,%cpu,comm --sort=-%mem 2>/dev/null | awk 'NR<=6'
        printf "\n"

        printf -- "--- Disk Utilization Summary (Threshold: %d%%) ---\n" "$DISK_THRESHOLD"
        df -P -h -x tmpfs -x devtmpfs -x squashfs -x efivarfs 2>/dev/null || df -P -h

        local disk_warnings=0 fs size used avail pct mount pct_num
        while read -r fs size used avail pct mount; do
            pct_num="${pct%%%}"
            if [[ "$pct_num" =~ ^[0-9]+$ ]] && [ "$pct_num" -ge "$DISK_THRESHOLD" ]; then
                printf "[WARNING] Filesystem '%s' mounted at '%s' is at %s%% (>= %d%%)\n" \
                       "$fs" "$mount" "$pct_num" "$DISK_THRESHOLD"
                warnings_found=$((warnings_found + 1))
                disk_warnings=$((disk_warnings + 1))
            fi
        done < <( (df -P -h -x tmpfs -x devtmpfs -x squashfs -x efivarfs 2>/dev/null || df -P -h) | awk 'NR>1 {print $1, $2, $3, $4, $5, $6}' )
        if [ "$disk_warnings" -eq 0 ]; then
            printf "Disk Alert Status: OK (all filesystems below %d%%)\n" "$DISK_THRESHOLD"
        fi

        printf "\nReport Threshold Warnings Detected: %d\n" "$warnings_found"
    } > "$report_path"; then
        printf "Error: Failed writing report to '%s'.\n" "$report_path" >&2
        log_activity "ERROR" "Failed to write health report '$report_path'"
        return 1
    fi

    printf "Health report saved to: %s\n" "$report_path"
    log_activity "INFO" "Generated system health report at '$report_path' (warnings=$warnings_found)"
    return 0
}

view_activity_log() {
    printf "\n=== Activity Log History (%s) ===\n" "$LOG_FILE"
    if [ ! -f "$LOG_FILE" ] || [ ! -s "$LOG_FILE" ]; then
        printf "Activity log is currently empty.\n"
        return 0
    fi
    cat "$LOG_FILE"
    log_activity "INFO" "Viewed activity log"
}

clear_activity_log() {
    if [ ! -f "$LOG_FILE" ] || [ ! -s "$LOG_FILE" ]; then
        printf "\nActivity log is already empty.\n"
        return 0
    fi
    if ! confirm_action "Are you sure you want to clear all activity log entries?"; then
        printf "Clear operation cancelled.\n"
        log_activity "INFO" "User cancelled clearing activity log"
        return 0
    fi
    if : > "$LOG_FILE" 2>/dev/null; then
        printf "Activity log cleared.\n"
        log_activity "INFO" "Activity log cleared by user"
    else
        printf "Error: Unable to clear '%s' (permission denied).\n" "$LOG_FILE" >&2
        return 1
    fi
}

print_menu() {
    printf "\nLinux System Health and Resource Monitoring Manager\n"
    printf "1.  Display system information\n"
    printf "2.  Monitor CPU utilization\n"
    printf "3.  Monitor memory utilization\n"
    printf "4.  Monitor disk utilization\n"
    printf "5.  View and search running processes\n"
    printf "6.  Terminate a process\n"
    printf "7.  Generate a system health report\n"
    printf "8.  View activity history\n"
    printf "9.  Clear activity log\n"
    printf "10. Exit application\n"
}

main_menu() {
    local choice
    log_activity "INFO" "Session started by $(id -un 2>/dev/null || echo 'user')"

    while true; do
        print_menu
        printf "Select option (1-10): "
        if ! read -r choice; then
            printf "\nInput closed. Exiting.\n"
            break
        fi

        case "$choice" in
            1)  display_system_info ;;
            2)  monitor_cpu ;;
            3)  monitor_memory ;;
            4)  monitor_disk ;;
            5)  view_and_search_processes ;;
            6)  terminate_process ;;
            7)  generate_health_report ;;
            8)  view_activity_log ;;
            9)  clear_activity_log ;;
            10)
                log_activity "INFO" "Session ended normally"
                printf "Exiting System Health Manager.\n"
                break
                ;;
            *)
                printf "Invalid option '%s'. Please choose 1 to 10.\n" "$choice"
                log_activity "WARNING" "Invalid menu input: '$choice'"
                ;;
        esac
    done
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main_menu
fi
