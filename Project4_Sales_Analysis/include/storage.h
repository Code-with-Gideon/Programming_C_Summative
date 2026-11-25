#ifndef P4_STORAGE_H
#define P4_STORAGE_H

#include "sales.h"

#define SALES_BIN_PATH "data/sales.bin"
#define SALES_CSV_PATH "data/sales_export.csv"

int save_sales_binary(const SalesLedger *ledger, const char *path);
int load_sales_binary(SalesLedger *ledger, const char *path);

int export_sales_csv(const SalesLedger *ledger, const char *path);
int import_sales_csv(SalesLedger *ledger, const char *path);

int seed_sample_sales(SalesLedger *ledger);

void op_save_transactions(const SalesLedger *ledger);
void op_load_transactions(SalesLedger *ledger);

#endif
