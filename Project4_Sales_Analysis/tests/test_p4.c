#define _CRT_SECURE_NO_WARNINGS
#include "../include/analytics.h"
#include "../include/sales.h"
#include "../include/storage.h"
#include "../include/utils.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void test_validation_and_empty_guards(void) {
    SalesLedger ledger;
    Transaction bad;

    assert(ledger_init(&ledger) == 1);

    /* Empty ledger metric guards must return 0.0 safely without div-by-zero */
    assert(fabs(metric_average_value(ledger.items, ledger.count, NULL)) < 0.001);
    assert(fabs(metric_total_revenue(ledger.items, ledger.count, NULL)) < 0.001);

    /* Reject bad date (Feb 30) */
    assert(validate_iso_date("2026-02-30") == 0);
    assert(validate_iso_date("2024-02-29") == 1);
    assert(validate_iso_date("2026-02-29") == 0);

    memset(&bad, 0, sizeof(bad));
    bad.transaction_id = 1;
    strcpy(bad.product_name, "Sensor");
    bad.quantity = 2;
    bad.unit_price = 10.0;
    strcpy(bad.transaction_date, "2026-02-30");
    assert(ledger_add_record(&ledger, &bad) == 0);

    /* Negative price rejected */
    strcpy(bad.transaction_date, "2026-11-20");
    bad.unit_price = -5.0;
    assert(ledger_add_record(&ledger, &bad) == 0);

    ledger_free(&ledger);
}

static void test_fn_pointers_callbacks_manual_sort_and_files(void) {
    SalesLedger ledger;
    SalesLedger bin_loaded;
    SalesLedger csv_loaded;
    int idx = -1;
    double threshold = 80.0;
    const Transaction *matches[16];
    int n_hits;
    int units = 0;

    assert(ledger_init(&ledger) == 1);
    assert(seed_sample_sales(&ledger) == 6);
    /* Duplicate sample re-seed adds 0 */
    assert(seed_sample_sales(&ledger) == 0);

    /* 5 Function Pointer Metrics */
    assert(fabs(metric_total_revenue(ledger.items, ledger.count, &idx) - 482.50) < 0.01);
    assert(fabs(metric_average_value(ledger.items, ledger.count, &idx) - (482.50 / 6.0)) < 0.01);
    assert(fabs(metric_highest_value(ledger.items, ledger.count, &idx) - 114.00) < 0.01);
    assert(ledger.items[idx].transaction_id == 5006);
    assert(fabs(metric_lowest_value(ledger.items, ledger.count, &idx) - 49.00) < 0.01);
    assert(ledger.items[idx].transaction_id == 5004);
    assert((int)metric_total_units(ledger.items, ledger.count, NULL) == 40);

    /* Callback filtering with void *ctx */
    n_hits = count_transactions_if(ledger.items, ledger.count, cb_amount_above, &threshold);
    assert(n_hits == 3); /* 98.00, 82.00, 114.00 */
    n_hits = filter_transactions(ledger.items, ledger.count, cb_product_match, "Arduino Uno Kit", matches, 16);
    assert(n_hits == 2);

    /* Product revenue breakdown */
    assert(fabs(calculate_product_revenue(&ledger, "Arduino Uno Kit", &units) - 147.00) < 0.01);
    assert(units == 6);

    /* In-place sort via comparator callbacks */
    manual_sort_transactions(&ledger, cmp_tx_total, 1);
    assert(ledger.items[0].transaction_id == 5006);
    assert(ledger.items[ledger.count - 1].transaction_id == 5004);

    manual_sort_transactions(&ledger, cmp_tx_product, 0);
    assert(str_casecmp_local(ledger.items[0].product_name, "16x2 LCD Module") == 0);

    manual_sort_transactions(&ledger, cmp_tx_id, 0);
    assert(ledger.items[0].transaction_id == 5001);

    /* Binary + CSV round-trip */
    assert(save_sales_binary(&ledger, "test_sales.bin") == 1);
    assert(ledger_init(&bin_loaded) == 1);
    assert(load_sales_binary(&bin_loaded, "test_sales.bin") == 1);
    assert(bin_loaded.count == ledger.count);

    assert(export_sales_csv(&ledger, "test_sales.csv") == 1);
    assert(ledger_init(&csv_loaded) == 1);
    assert(import_sales_csv(&csv_loaded, "test_sales.csv") == 1);
    assert(csv_loaded.count == ledger.count);

    remove("test_sales.bin");
    remove("test_sales.csv");

    ledger_free(&ledger);
    ledger_free(&bin_loaded);
    ledger_free(&csv_loaded);
}

int main(void) {
    test_validation_and_empty_guards();
    test_fn_pointers_callbacks_manual_sort_and_files();
    printf("[PASS] Project 4 function pointers, callbacks, manual sort, and dual-persistence tests succeeded.\n");
    return 0;
}
