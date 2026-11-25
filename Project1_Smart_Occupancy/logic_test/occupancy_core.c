#define _CRT_SECURE_NO_WARNINGS
#include "occupancy_core.h"

#include <stdio.h>
#include <stdlib.h>

static void append_event(OccupancySystem *sys, unsigned long now_ms, EventAction act) {
    if (sys == NULL || sys->record == NULL || sys->batch_log == NULL) {
        return;
    }

    if (sys->batch_count >= sys->batch_cap) {
        if (!occupancy_flush_batch(sys)) {
            return;
        }
    }

    EventRecord *slot = sys->batch_log + sys->batch_count;
    slot->timestamp_ms = now_ms;
    slot->action = act;
    slot->count_after = sys->record->current_count;
    sys->batch_count++;
}

int occupancy_init(OccupancySystem *sys, int max_capacity) {
    if (sys == NULL || max_capacity <= 0) {
        return 0;
    }

    sys->record = (OccupancyRecord *)calloc(1, sizeof(OccupancyRecord));
    if (sys->record == NULL) {
        return 0;
    }

    sys->batch_log = (EventRecord *)malloc((size_t)BATCH_LOG_CAP * sizeof(EventRecord));
    if (sys->batch_log == NULL) {
        free(sys->record);
        sys->record = NULL;
        return 0;
    }

    sys->record->current_count = 0;
    sys->record->max_capacity = max_capacity;
    sys->record->lcd_dirty = 1;
    sys->batch_count = 0;
    sys->batch_cap = BATCH_LOG_CAP;
    return 1;
}

void occupancy_destroy(OccupancySystem *sys) {
    if (sys == NULL) {
        return;
    }
    free(sys->record);
    sys->record = NULL;
    free(sys->batch_log);
    sys->batch_log = NULL;
    sys->batch_count = 0;
    sys->batch_cap = 0;
}

int occupancy_flush_batch(OccupancySystem *sys) {
    EventRecord *fresh;

    if (sys == NULL) {
        return 0;
    }

    free(sys->batch_log);
    sys->batch_log = NULL;
    sys->batch_count = 0;

    fresh = (EventRecord *)malloc((size_t)BATCH_LOG_CAP * sizeof(EventRecord));
    if (fresh == NULL) {
        sys->batch_cap = 0;
        return 0;
    }

    sys->batch_log = fresh;
    sys->batch_cap = BATCH_LOG_CAP;
    return 1;
}

int occupancy_reset(OccupancySystem *sys, int max_capacity) {
    if (sys == NULL || max_capacity <= 0) {
        return 0;
    }
    occupancy_destroy(sys);
    return occupancy_init(sys, max_capacity);
}

void button_init(DebouncedButton *btn, int pin) {
    if (btn == NULL) {
        return;
    }
    btn->pin = pin;
    btn->last_raw = 0;
    btn->stable_state = 0;
    btn->last_change_ms = 0UL;
}

int button_update_debounced(DebouncedButton *btn, int raw_state, unsigned long now_ms) {
    int normalized = (raw_state != 0) ? 1 : 0;

    if (btn == NULL) {
        return 0;
    }

    if (normalized != btn->last_raw) {
        btn->last_raw = normalized;
        btn->last_change_ms = now_ms;
    }

    if ((now_ms - btn->last_change_ms) >= DEBOUNCE_INTERVAL_MS) {
        if (normalized != btn->stable_state) {
            btn->stable_state = normalized;
            if (btn->stable_state == 1) {
                return 1;
            }
        }
    }

    return 0;
}

EventAction handle_entry(OccupancySystem *sys, unsigned long now_ms) {
    OccupancyRecord *rec;

    if (sys == NULL || sys->record == NULL) {
        return ACT_NONE;
    }

    rec = sys->record;
    if (rec->current_count >= rec->max_capacity) {
        rec->rejected_entries++;
        rec->lcd_dirty = 1;
        append_event(sys, now_ms, ACT_ENTRY_REJECTED);
        return ACT_ENTRY_REJECTED;
    }

    rec->current_count++;
    rec->total_entries++;
    rec->lcd_dirty = 1;
    append_event(sys, now_ms, ACT_ENTRY_OK);
    return ACT_ENTRY_OK;
}

EventAction handle_exit(OccupancySystem *sys, unsigned long now_ms) {
    OccupancyRecord *rec;

    if (sys == NULL || sys->record == NULL) {
        return ACT_NONE;
    }

    rec = sys->record;
    if (rec->current_count <= 0) {
        rec->rejected_exits++;
        rec->lcd_dirty = 1;
        append_event(sys, now_ms, ACT_EXIT_REJECTED);
        return ACT_EXIT_REJECTED;
    }

    rec->current_count--;
    rec->total_exits++;
    rec->lcd_dirty = 1;
    append_event(sys, now_ms, ACT_EXIT_OK);
    return ACT_EXIT_OK;
}

int get_remaining_capacity(const OccupancyRecord *rec) {
    int rem;
    if (rec == NULL) {
        return 0;
    }
    rem = rec->max_capacity - rec->current_count;
    return (rem > 0) ? rem : 0;
}

int is_facility_full(const OccupancyRecord *rec) {
    if (rec == NULL) {
        return 0;
    }
    return rec->current_count >= rec->max_capacity;
}

void get_led_states(const OccupancyRecord *rec, int *green_on, int *red_on) {
    int full = is_facility_full(rec);
    if (green_on != NULL) {
        *green_on = full ? 0 : 1;
    }
    if (red_on != NULL) {
        *red_on = full ? 1 : 0;
    }
}

void format_lcd_lines(const OccupancyRecord *rec, char *line1, size_t l1_size, char *line2, size_t l2_size) {
    int rem;
    const char *tag;

    if (rec == NULL || line1 == NULL || line2 == NULL || l1_size < 17 || l2_size < 17) {
        return;
    }

    rem = get_remaining_capacity(rec);
    tag = is_facility_full(rec) ? "FULL!" : "OPEN ";

    snprintf(line1, l1_size, "In:%3d / Max:%3d", rec->current_count, rec->max_capacity);
    snprintf(line2, l2_size, "Avail:%3d  %s", rem, tag);
}
