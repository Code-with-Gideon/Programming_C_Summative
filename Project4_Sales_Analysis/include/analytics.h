#ifndef P4_ANALYTICS_H
#define P4_ANALYTICS_H

#include "sales.h"

/* Function pointer type for runtime sales-processing metrics */
typedef double (*SalesMetricFn)(const Transaction *arr, int count, int *out_index);

/* Callback predicate type for filtering and counting records */
typedef int (*TxPredicateCb)(const Transaction *tx, const void *ctx);

/* Comparator callback type for in-place sorting */
typedef int (*TxCompareCb)(const Transaction *a, const Transaction *b);

/* Runtime metric handlers */
double metric_total_revenue(const Transaction *arr, int count, int *out_index);
double metric_average_value(const Transaction *arr, int count, int *out_index);
double metric_highest_value(const Transaction *arr, int count, int *out_index);
double metric_lowest_value(const Transaction *arr, int count, int *out_index);
double metric_total_units(const Transaction *arr, int count, int *out_index);
void op_process_sales_data(const SalesLedger *ledger);

/* Callback predicates and higher-order filtering */
int cb_amount_above(const Transaction *tx, const void *ctx);
int cb_product_match(const Transaction *tx, const void *ctx);
int cb_date_match(const Transaction *tx, const void *ctx);

int count_transactions_if(const Transaction *arr, int count, TxPredicateCb cb, const void *ctx);
int filter_transactions(const Transaction *arr, int count, TxPredicateCb cb, const void *ctx,
                        const Transaction **out_matches, int max_out);
void op_filter_transactions(const SalesLedger *ledger);

/* Search and callback-driven in-place sorting */
int cmp_tx_id(const Transaction *a, const Transaction *b);
int cmp_tx_product(const Transaction *a, const Transaction *b);
int cmp_tx_total(const Transaction *a, const Transaction *b);

void manual_sort_transactions(SalesLedger *ledger, TxCompareCb cmp, int descending);
void op_search_transactions(const SalesLedger *ledger);
void op_sort_transactions(SalesLedger *ledger);

/* Summary and product revenue breakdown */
double calculate_product_revenue(const SalesLedger *ledger, const char *product_name, int *out_units);
void op_generate_sales_report(const SalesLedger *ledger);

#endif
