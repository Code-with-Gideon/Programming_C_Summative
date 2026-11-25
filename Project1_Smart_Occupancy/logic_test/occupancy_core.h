#ifndef OCCUPANCY_CORE_H
#define OCCUPANCY_CORE_H

#include <stddef.h>

#define DEFAULT_MAX_CAPACITY 10
#define DEBOUNCE_INTERVAL_MS 50UL
#define LCD_REFRESH_MS       250UL
#define BATCH_LOG_CAP        8

typedef enum {
    ACT_NONE = 0,
    ACT_ENTRY_OK = 1,
    ACT_EXIT_OK = 2,
    ACT_ENTRY_REJECTED = -1,
    ACT_EXIT_REJECTED = -2
} EventAction;

typedef struct {
    int current_count;
    int max_capacity;
    unsigned long total_entries;
    unsigned long total_exits;
    unsigned long rejected_entries;
    unsigned long rejected_exits;
    int lcd_dirty;
} OccupancyRecord;

typedef struct {
    int pin;
    int last_raw;
    int stable_state;
    unsigned long last_change_ms;
} DebouncedButton;

typedef struct {
    unsigned long timestamp_ms;
    EventAction action;
    int count_after;
} EventRecord;

typedef struct {
    OccupancyRecord *record;
    EventRecord *batch_log;
    int batch_count;
    int batch_cap;
} OccupancySystem;

int occupancy_init(OccupancySystem *sys, int max_capacity);
void occupancy_destroy(OccupancySystem *sys);
int occupancy_flush_batch(OccupancySystem *sys);
int occupancy_reset(OccupancySystem *sys, int max_capacity);

void button_init(DebouncedButton *btn, int pin);
int button_update_debounced(DebouncedButton *btn, int raw_state, unsigned long now_ms);

EventAction handle_entry(OccupancySystem *sys, unsigned long now_ms);
EventAction handle_exit(OccupancySystem *sys, unsigned long now_ms);

int get_remaining_capacity(const OccupancyRecord *rec);
int is_facility_full(const OccupancyRecord *rec);
void get_led_states(const OccupancyRecord *rec, int *green_on, int *red_on);
void format_lcd_lines(const OccupancyRecord *rec, char *line1, size_t l1_size, char *line2, size_t l2_size);

#endif
