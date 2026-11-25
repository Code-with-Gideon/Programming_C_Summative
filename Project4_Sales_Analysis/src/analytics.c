#define _CRT_SECURE_NO_WARNINGS
#include "../include/analytics.h"
#include "../include/utils.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double metric_total_revenue(const Transaction *arr, int count, int *out_index) {
    const Transaction *p;
    const Transaction *end;
    double sum = 0.0;

    if (out_index != NULL) *out_index = -1;
    if (arr == NULL || count <= 0) return 0.0;

    end = arr + count;
    for (p = arr; p < end; p++) {
        sum += p->total_amount;
    }
    return sum;
}

double metric_average_value(const Transaction *arr, int count, int *out_index) {
    if (out_index != NULL) *out_index = -1;
    if (arr == NULL || count <= 0) return 0.0;
    return metric_total_revenue(arr, count, NULL) / (double)count;
}

double metric_highest_value(const Transaction *arr, int count, int *out_index) {
    const Transaction *p;
    const Transaction *end;
    int best_idx = 0;

    if (arr == NULL || count <= 0) {
        if (out_index != NULL) *out_index = -1;
        return 0.0;
    }

    end = arr + count;
    for (p = arr + 1; p < end; p++) {
        if (p->total_amount > (arr + best_idx)->total_amount) {
            best_idx = (int)(p - arr);
        }
    }
    if (out_index != NULL) *out_index = best_idx;
    return (arr + best_idx)->total_amount;
}

double metric_lowest_value(const Transaction *arr, int count, int *out_index) {
    const Transaction *p;
    const Transaction *end;
    int low_idx = 0;

    if (arr == NULL || count <= 0) {
        if (out_index != NULL) *out_index = -1;
        return 0.0;
    }

    end = arr + count;
    for (p = arr + 1; p < end; p++) {
        if (p->total_amount < (arr + low_idx)->total_amount) {
            low_idx = (int)(p - arr);
        }
    }
    if (out_index != NULL) *out_index = low_idx;
    return (arr + low_idx)->total_amount;
}

double metric_total_units(const Transaction *arr, int count, int *out_index) {
    const Transaction *p;
    const Transaction *end;
    long units = 0;

    if (out_index != NULL) *out_index = -1;
    if (arr == NULL || count <= 0) return 0.0;

    end = arr + count;
    for (p = arr; p < end; p++) {
        units += (long)p->quantity;
    }
    return (double)units;
}

void op_process_sales_data(const SalesLedger *ledger) {
    static const SalesMetricFn metric_ops[5] = {
        metric_total_revenue,
        metric_average_value,
        metric_highest_value,
        metric_lowest_value,
        metric_total_units
    };
    static const char *labels[5] = {
        "Total Revenue",
        "Average Transaction Value",
        "Highest-Value Transaction",
        "Lowest-Value Transaction",
        "Total Units Sold"
    };

    int choice;
    int hit_idx = -1;
    double result;

    if (ledger == NULL || ledger->count <= 0) {
        printf("\nLedger is empty. Add transactions before running sales metrics.\n");
        return;
    }

    printf("\nProcess Sales Data (Function Pointer Dispatch):\n");
    printf("1. Calculate Total Revenue\n");
    printf("2. Calculate Average Transaction Value\n");
    printf("3. Identify Highest-Value Transaction\n");
    printf("4. Identify Lowest-Value Transaction\n");
    printf("5. Calculate Total Units Sold\n");
    if (read_int_range("Select operation (1-5): ", 1, 5, &choice) <= 0) {
        printf("Invalid operation choice.\n");
        return;
    }

    /* Invoke selected operation via function pointer table */
    result = metric_ops[choice - 1](ledger->items, ledger->count, &hit_idx);

    if (choice == 5) {
        printf("%s: %.0f unit(s) across %d transactions.\n",
               labels[choice - 1], result, ledger->count);
    } else if (hit_idx >= 0) {
        const Transaction *t = ledger->items + hit_idx;
        printf("%s: %.2f (Tx #%d - %s, x%d on %s)\n",
               labels[choice - 1], result, t->transaction_id,
               t->product_name, t->quantity, t->transaction_date);
    } else {
        printf("%s: %.2f\n", labels[choice - 1], result);
    }
}

int cb_amount_above(const Transaction *tx, const void *ctx) {
    const double *threshold = (const double *)ctx;
    if (tx == NULL || threshold == NULL) return 0;
    return tx->total_amount > *threshold;
}

int cb_product_match(const Transaction *tx, const void *ctx) {
    const char *target = (const char *)ctx;
    if (tx == NULL || target == NULL) return 0;
    return str_contains_nocase(tx->product_name, target);
}

int cb_date_match(const Transaction *tx, const void *ctx) {
    const char *date = (const char *)ctx;
    if (tx == NULL || date == NULL) return 0;
    return strcmp(tx->transaction_date, date) == 0;
}

int count_transactions_if(const Transaction *arr, int count, TxPredicateCb cb, const void *ctx) {
    const Transaction *p;
    const Transaction *end;
    int hits = 0;

    if (arr == NULL || count <= 0 || cb == NULL) return 0;

    end = arr + count;
    for (p = arr; p < end; p++) {
        if (cb(p, ctx)) {
            hits++;
        }
    }
    return hits;
}

int filter_transactions(const Transaction *arr, int count, TxPredicateCb cb, const void *ctx,
                        const Transaction **out_matches, int max_out) {
    const Transaction *p;
    const Transaction *end;
    int hits = 0;

    if (arr == NULL || count <= 0 || cb == NULL || out_matches == NULL || max_out <= 0) {
        return 0;
    }

    end = arr + count;
    for (p = arr; p < end; p++) {
        if (cb(p, ctx)) {
            if (hits < max_out) {
                out_matches[hits] = p;
            }
            hits++;
        }
    }
    return hits;
}

void op_filter_transactions(const SalesLedger *ledger) {
    int mode;
    const Transaction **matches;
    int count_hits = 0;
    int i;
    int limit;

    if (ledger == NULL || ledger->count <= 0) {
        printf("\nLedger is empty. Nothing to filter.\n");
        return;
    }

    printf("\nFilter Transactions (Callback Predicates):\n");
    printf("1. Above a specified sales amount\n");
    printf("2. Matching a product name\n");
    printf("3. Matching a transaction date (YYYY-MM-DD)\n");
    if (read_int_range("Option (1-3): ", 1, 3, &mode) <= 0) {
        printf("Invalid filter option.\n");
        return;
    }

    matches = (const Transaction **)calloc((size_t)ledger->count, sizeof(const Transaction *));
    if (matches == NULL) {
        fprintf(stderr, "Memory allocation failed for filter results.\n");
        return;
    }

    if (mode == 1) {
        double floor_val;
        if (read_double_range("Show transactions above amount: ", 0.0, 1000000000.0, &floor_val) <= 0) {
            printf("Invalid threshold amount.\n");
            free((void *)matches);
            return;
        }
        count_hits = filter_transactions(ledger->items, ledger->count, cb_amount_above, &floor_val, matches, ledger->count);
    } else if (mode == 2) {
        char prod[PROD_NAME_LEN];
        if (read_nonempty_string("Enter product name keyword: ", prod, sizeof(prod)) <= 0) {
            printf("Product keyword cannot be empty.\n");
            free((void *)matches);
            return;
        }
        count_hits = filter_transactions(ledger->items, ledger->count, cb_product_match, prod, matches, ledger->count);
    } else {
        char dt[DATE_STR_LEN];
        if (read_nonempty_string("Enter date (YYYY-MM-DD): ", dt, sizeof(dt)) <= 0 || !validate_iso_date(dt)) {
            printf("Invalid date.\n");
            free((void *)matches);
            return;
        }
        count_hits = filter_transactions(ledger->items, ledger->count, cb_date_match, dt, matches, ledger->count);
    }

    limit = (count_hits < ledger->count) ? count_hits : ledger->count;
    if (limit == 0) {
        printf("No transactions satisfied the callback predicate.\n");
    } else {
        printf("\nMatched %d transaction(s):\n", count_hits);
        for (i = 0; i < limit; i++) {
            const Transaction *t = matches[i];
            printf("  Tx #%d | %-20.20s | Qty: %4d | Unit: %8.2f | Total: %10.2f | %s\n",
                   t->transaction_id, t->product_name, t->quantity,
                   t->unit_price, t->total_amount, t->transaction_date);
        }
    }

    free((void *)matches);
}

int cmp_tx_id(const Transaction *a, const Transaction *b) {
    return (a->transaction_id > b->transaction_id) - (a->transaction_id < b->transaction_id);
}

int cmp_tx_product(const Transaction *a, const Transaction *b) {
    int r = str_casecmp_local(a->product_name, b->product_name);
    if (r != 0) return r;
    return cmp_tx_id(a, b);
}

int cmp_tx_total(const Transaction *a, const Transaction *b) {
    if (a->total_amount < b->total_amount) return -1;
    if (a->total_amount > b->total_amount) return 1;
    return cmp_tx_id(a, b);
}

/* In-place Selection Sort driven by comparator callback */
void manual_sort_transactions(SalesLedger *ledger, TxCompareCb cmp, int descending) {
    Transaction *i;
    Transaction *j;
    Transaction *end;

    if (ledger == NULL || ledger->count <= 1 || cmp == NULL || ledger->items == NULL) {
        return;
    }

    end = ledger->items + ledger->count;
    for (i = ledger->items; i < end - 1; i++) {
        Transaction *chosen = i;
        for (j = i + 1; j < end; j++) {
            int order = cmp(j, chosen);
            if ((!descending && order < 0) || (descending && order > 0)) {
                chosen = j;
            }
        }
        if (chosen != i) {
            Transaction tmp = *i;
            *i = *chosen;
            *chosen = tmp;
        }
    }
}

void op_search_transactions(const SalesLedger *ledger) {
    int mode;
    const Transaction *p;
    const Transaction *end;
    int hits = 0;

    if (ledger == NULL || ledger->count <= 0) {
        printf("\nLedger is empty. Nothing to search.\n");
        return;
    }

    printf("\nSearch Transactions By:\n");
    printf("1. Transaction ID\n");
    printf("2. Product Name\n");
    printf("3. Transaction Amount\n");
    if (read_int_range("Option (1-3): ", 1, 3, &mode) <= 0) {
        printf("Invalid search option.\n");
        return;
    }

    end = ledger->items + ledger->count;

    if (mode == 1) {
        int id;
        const Transaction *t;
        if (read_int_range("Enter Transaction ID: ", 1, 999999, &id) <= 0) {
            printf("Invalid transaction ID.\n");
            return;
        }
        t = find_transaction_by_id(ledger, id);
        if (t != NULL) {
            printf("Found -> Tx #%d | %s | Qty: %d | Unit: %.2f | Total: %.2f | %s\n",
                   t->transaction_id, t->product_name, t->quantity,
                   t->unit_price, t->total_amount, t->transaction_date);
        } else {
            printf("No transaction found with ID %d.\n", id);
        }
        return;
    }

    if (mode == 2) {
        char prod[PROD_NAME_LEN];
        if (read_nonempty_string("Enter Product Name: ", prod, sizeof(prod)) <= 0) {
            printf("Product query cannot be empty.\n");
            return;
        }
        for (p = ledger->items; p < end; p++) {
            if (str_contains_nocase(p->product_name, prod)) {
                printf("  Tx #%d | %-20s | Qty: %d | Total: %.2f | %s\n",
                       p->transaction_id, p->product_name, p->quantity,
                       p->total_amount, p->transaction_date);
                hits++;
            }
        }
    } else {
        double amt;
        if (read_double_range("Enter Total Transaction Amount: ", 0.0, 1000000000.0, &amt) <= 0) {
            printf("Invalid transaction amount.\n");
            return;
        }
        for (p = ledger->items; p < end; p++) {
            if (fabs(p->total_amount - amt) < 0.01) {
                printf("  Tx #%d | %-20s | Qty: %d | Total: %.2f | %s\n",
                       p->transaction_id, p->product_name, p->quantity,
                       p->total_amount, p->transaction_date);
                hits++;
            }
        }
    }

    if (hits == 0) {
        printf("No matching transactions found.\n");
    } else {
        printf("Total matches: %d\n", hits);
    }
}

void op_sort_transactions(SalesLedger *ledger) {
    static const TxCompareCb comparators[3] = {
        cmp_tx_id,
        cmp_tx_product,
        cmp_tx_total
    };
    int field;
    int dir = 1;

    if (ledger == NULL || ledger->count <= 1) {
        printf("\nNeed at least 2 transactions to sort.\n");
        return;
    }

    printf("\nSort Transactions Manually (Callback Comparators):\n");
    printf("1. Transaction ID (ascending)\n");
    printf("2. Product Name (A to Z)\n");
    printf("3. Total Transaction Amount\n");
    if (read_int_range("Sort field (1-3): ", 1, 3, &field) <= 0) {
        printf("Invalid sort field.\n");
        return;
    }

    if (field == 3) {
        if (read_int_range("Order (1 = Ascending, 2 = Descending): ", 1, 2, &dir) <= 0) {
            printf("Invalid sort order.\n");
            return;
        }
    }

    manual_sort_transactions(ledger, comparators[field - 1], (dir == 2) ? 1 : 0);
    printf("Sorted %d transactions successfully.\n", ledger->count);
    op_display_transactions(ledger);
}

double calculate_product_revenue(const SalesLedger *ledger, const char *product_name, int *out_units) {
    const Transaction *p;
    const Transaction *end;
    double rev = 0.0;
    int units = 0;

    if (out_units != NULL) *out_units = 0;
    if (ledger == NULL || ledger->count <= 0 || product_name == NULL) {
        return 0.0;
    }

    end = ledger->items + ledger->count;
    for (p = ledger->items; p < end; p++) {
        if (str_contains_nocase(p->product_name, product_name)) {
            rev += p->total_amount;
            units += p->quantity;
        }
    }
    if (out_units != NULL) *out_units = units;
    return rev;
}

void op_generate_sales_report(const SalesLedger *ledger) {
    double rev;
    double avg;
    double high_val;
    double low_val;
    double units;
    int high_idx = -1;
    int low_idx = -1;
    char prod_query[PROD_NAME_LEN];

    if (ledger == NULL || ledger->count <= 0) {
        printf("\nLedger is empty. Add transactions before generating a sales report.\n");
        return;
    }

    rev = metric_total_revenue(ledger->items, ledger->count, NULL);
    avg = metric_average_value(ledger->items, ledger->count, NULL);
    high_val = metric_highest_value(ledger->items, ledger->count, &high_idx);
    low_val = metric_lowest_value(ledger->items, ledger->count, &low_idx);
    units = metric_total_units(ledger->items, ledger->count, NULL);

    printf("\nSales Summary Report\n");
    printf("-------------------------------------------------------------\n");
    printf("Total Transactions        : %d\n", ledger->count);
    printf("Total Revenue             : %.2f\n", rev);
    printf("Average Transaction Value : %.2f\n", avg);
    if (high_idx >= 0 && low_idx >= 0) {
        const Transaction *hi = ledger->items + high_idx;
        const Transaction *lo = ledger->items + low_idx;
        printf("Highest Transaction       : %.2f (Tx #%d, %s)\n", high_val, hi->transaction_id, hi->product_name);
        printf("Lowest Transaction        : %.2f (Tx #%d, %s)\n", low_val, lo->transaction_id, lo->product_name);
    }
    printf("Total Product Units Sold  : %.0f\n", units);

    printf("Enter a product name to break down revenue (or Enter to skip): ");
    if (read_line_trimmed(prod_query, sizeof(prod_query)) && prod_query[0] != '\0') {
        int matched_units = 0;
        int matched_tx = count_transactions_if(ledger->items, ledger->count, cb_product_match, prod_query);
        double prod_rev = calculate_product_revenue(ledger, prod_query, &matched_units);
        printf("Product '%s' Breakdown -> %d tx, %d units, Revenue: %.2f\n",
               prod_query, matched_tx, matched_units, prod_rev);
    }
}
