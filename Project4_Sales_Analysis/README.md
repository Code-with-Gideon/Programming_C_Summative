# Project 4: Sales Transaction Processing and Analysis System

**Platform:** Linux / WSL (`gcc -Wall -Wextra -Werror -std=c99`)  
**Directory Structure:**
- `include/sales.h`, `src/sales.c`: Dynamic `SalesLedger` array and validated transaction CRUD
- `include/analytics.h`, `src/analytics.c`: `SalesMetricFn` dispatch table, `TxPredicateCb` (`void *ctx`) filters, comparator-driven Selection Sort, 6-metric summary report
- `include/storage.h`, `src/storage.c`: Binary (`STXN`) persistence, CSV import/export, 6-record sample seeder
- `include/utils.h`, `src/utils.c`: Safe `strtol`/`strtod` numeric parsing and leap-year ISO calendar validation
- `src/main.c`: 12-option interactive console loop with automatic binary save and cleanup on exit
- `tests/test_p4.c`: Unit tests covering calendar validation, empty-ledger safety, dispatch metrics, callbacks, sorting, and file round-trips

---

## Architecture & Engineering Choices

1. **Runtime Metric Dispatch (`SalesMetricFn` Function Pointer Table):**
   To avoid repeating boilerplate conditional logic inside `op_process_sales_data`, 5 metric handlers (`metric_total_revenue`, `metric_average_value`, `metric_highest_value`, `metric_lowest_value`, `metric_total_units`) share one `SalesMetricFn` signature and sit inside `static const SalesMetricFn metric_ops[5]`. User selection invokes `metric_ops[choice - 1](ledger->items, ledger->count, &hit_idx)` dynamically at runtime.
2. **Context-Aware Filtering Callbacks (`TxPredicateCb` + `void *ctx`):**
   `filter_transactions` and `count_transactions_if` separate array traversal from matching logic by accepting any `int (*)(const Transaction *, const void *)` callback. `cb_amount_above` reads a `double *` threshold, `cb_product_match` reads a substring query, and `cb_date_match` reads an ISO date string through that same uniform signature.
3. **Comparator-Driven In-Place Sort (`TxCompareCb`):**
   `manual_sort_transactions` accepts any `TxCompareCb` comparator (`cmp_tx_id`, `cmp_tx_product`, `cmp_tx_total`) alongside an ascending/descending flag and sorts elements directly in-place via pointer swaps.
4. **Calendar Bounds & Dual Binary/CSV Storage:**
   `validate_iso_date` validates `YYYY-MM-DD` format plus real month lengths including leap-year February checks (accepting `2024-02-29` while rejecting `2026-02-29` and `2026-02-30`). Line totals are always derived from `quantity * unit_price`, and records persist cleanly to both binary (`STXN` header + flat `Transaction` records) and CSV (`data/sales_export.csv`).

---

## Build, Run, and Verification Commands

```bash
cd Project4_Sales_Analysis

# Option A: Using Makefile
make clean all test
./sales_analysis

# Option B: Direct gcc compilation
gcc -Wall -Wextra -Werror -std=c99 -Iinclude src/main.c src/sales.c src/analytics.c src/storage.c src/utils.c -o sales_analysis -lm
```

### Verified WSL Compiler & Interactive Summary Excerpt
```text
[PASS] Project 4 function pointers, callbacks, manual sort, and dual-persistence tests succeeded.

Sales Summary Report
-------------------------------------------------------------
Total Transactions        : 6
Total Revenue             : 482.50
Average Transaction Value : 80.42
Highest Transaction       : 114.00 (Tx #5006, STM32 Dev Board)
Lowest Transaction        : 49.00 (Tx #5004, Arduino Uno Kit)
Total Product Units Sold  : 40
Enter a product name to break down revenue (or Enter to skip): Product 'Arduino Uno Kit' Breakdown -> 2 tx, 6 units, Revenue: 147.00
```
