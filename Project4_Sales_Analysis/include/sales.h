#ifndef P4_SALES_H
#define P4_SALES_H

#define PROD_NAME_LEN 64
#define DATE_STR_LEN  12
#define INIT_TX_CAP   4

typedef struct {
    int transaction_id;
    char product_name[PROD_NAME_LEN];
    int quantity;
    double unit_price;
    double total_amount;
    char transaction_date[DATE_STR_LEN];
} Transaction;

typedef struct {
    Transaction *items;
    int count;
    int capacity;
} SalesLedger;

int ledger_init(SalesLedger *ledger);
void ledger_free(SalesLedger *ledger);
Transaction *find_transaction_by_id(const SalesLedger *ledger, int tx_id);
int ledger_add_record(SalesLedger *ledger, const Transaction *src);

void op_add_transaction(SalesLedger *ledger);
void op_display_transactions(const SalesLedger *ledger);
void op_update_transaction(SalesLedger *ledger);
void op_delete_transaction(SalesLedger *ledger);

#endif
