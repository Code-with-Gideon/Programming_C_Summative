#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "occupancy_core.h"

static void test_initial_state_and_underflow(void) {
    OccupancySystem sys;
    int green = -1;
    int red = -1;
    char l1[17];
    char l2[17];

    assert(occupancy_init(&sys, DEFAULT_MAX_CAPACITY) == 1);
    assert(sys.record != NULL);
    assert(sys.batch_log != NULL);
    assert(sys.record->current_count == 0);
    assert(get_remaining_capacity(sys.record) == 10);
    assert(is_facility_full(sys.record) == 0);

    get_led_states(sys.record, &green, &red);
    assert(green == 1 && red == 0);

    format_lcd_lines(sys.record, l1, sizeof(l1), l2, sizeof(l2));
    assert(strlen(l1) == 16);
    assert(strlen(l2) == 16);
    assert(strcmp(l1, "In:  0 / Max: 10") == 0);
    assert(strcmp(l2, "Avail: 10  OPEN ") == 0);

    assert(handle_exit(&sys, 100UL) == ACT_EXIT_REJECTED);
    assert(sys.record->current_count == 0);
    assert(sys.record->rejected_exits == 1UL);

    occupancy_destroy(&sys);
    assert(sys.record == NULL);
    assert(sys.batch_log == NULL);
}

static void test_millis_debounce_and_single_edge(void) {
    DebouncedButton entry_btn;
    button_init(&entry_btn, 8);

    /* Switch bounce shorter than 50 ms must be ignored */
    assert(button_update_debounced(&entry_btn, 1, 100UL) == 0);
    assert(button_update_debounced(&entry_btn, 0, 120UL) == 0);
    assert(button_update_debounced(&entry_btn, 1, 140UL) == 0);
    assert(button_update_debounced(&entry_btn, 1, 189UL) == 0);

    /* Triggers once when stable high crosses 50 ms */
    assert(button_update_debounced(&entry_btn, 1, 190UL) == 1);

    /* Holding the button longer must not double-count */
    assert(button_update_debounced(&entry_btn, 1, 300UL) == 0);
    assert(button_update_debounced(&entry_btn, 1, 900UL) == 0);

    /* Release and debounce low before next press */
    assert(button_update_debounced(&entry_btn, 0, 950UL) == 0);
    assert(button_update_debounced(&entry_btn, 0, 1000UL) == 0);
    assert(button_update_debounced(&entry_btn, 1, 1010UL) == 0);
    assert(button_update_debounced(&entry_btn, 1, 1060UL) == 1);
}

static void test_capacity_ceiling_leds_and_batch_free(void) {
    OccupancySystem sys;
    int green = 0;
    int red = 0;
    int i;
    char l1[17];
    char l2[17];

    assert(occupancy_init(&sys, 10) == 1);

    /* Fill up to 10; crosses BATCH_LOG_CAP=8 so automatic free+realloc runs */
    for (i = 0; i < 10; i++) {
        assert(handle_entry(&sys, (unsigned long)(1000 + i * 100)) == ACT_ENTRY_OK);
    }

    assert(sys.record->current_count == 10);
    assert(sys.record->total_entries == 10UL);
    assert(get_remaining_capacity(sys.record) == 0);
    assert(is_facility_full(sys.record) == 1);

    get_led_states(sys.record, &green, &red);
    assert(green == 0 && red == 1);

    format_lcd_lines(sys.record, l1, sizeof(l1), l2, sizeof(l2));
    assert(strcmp(l1, "In: 10 / Max: 10") == 0);
    assert(strcmp(l2, "Avail:  0  FULL!") == 0);

    /* Attempt entry while full */
    assert(handle_entry(&sys, 2500UL) == ACT_ENTRY_REJECTED);
    assert(sys.record->current_count == 10);
    assert(sys.record->rejected_entries == 1UL);
    get_led_states(sys.record, &green, &red);
    assert(green == 0 && red == 1);

    /* Exit once -> back to OPEN */
    assert(handle_exit(&sys, 2600UL) == ACT_EXIT_OK);
    assert(sys.record->current_count == 9);
    get_led_states(sys.record, &green, &red);
    assert(green == 1 && red == 0);

    /* Manual flush + dynamic reset */
    assert(occupancy_flush_batch(&sys) == 1);
    assert(sys.batch_count == 0);
    assert(occupancy_reset(&sys, 5) == 1);
    assert(sys.record->max_capacity == 5);
    assert(sys.record->current_count == 0);

    occupancy_destroy(&sys);
}

int main(void) {
    test_initial_state_and_underflow();
    test_millis_debounce_and_single_edge();
    test_capacity_ceiling_leds_and_batch_free();
    printf("[PASS] Project 1 occupancy core & debounce unit tests succeeded.\n");
    return 0;
}
