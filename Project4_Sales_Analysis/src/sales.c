#define _CRT_SECURE_NO_WARNINGS
#include "../include/sales.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ledger_init(SalesLedger *ledger) {
    if (ledger == NULL) {
        return 0;
    }
    ledger->items = (Transaction *)calloc(INIT_TX_CAP, sizeof(Transaction));
    if (ledger->items == NULL) {
        ledger->count = 0;
        ledger->capacity = 0;
        return 0;
    }
    ledger->count = 0;
    ledger->capacity = INIT_TX_CAP;
    return 1;
}

void ledger_free(SalesLedger *ledger) {
    if (ledger == NULL) {
        return;
    }
    free(ledger->items);
    ledger->items = NULL;
    ledger->count = 0;
    ledger->capacity = 0;
}

Transaction *find_transaction_by_id(const SalesLedger *ledger, int tx_id) {
    Transaction *p;
    Transaction *end;

    if (ledger == NULL || ledger->items == NULL) {
        return NULL;
    }
    end = ledger->items + ledger->count;
    for (p = ledger->items; p < end; p++) {
        if (p->transaction_id == tx_id) {
            return p;
        }
    }
    return NULL;
}

int ledger_add_record(SalesLedger *ledger, const Transaction *src) {
    Transaction *slot;

    if (ledger == NULL || src == NULL || src->transaction_id <= 0 ||
        src->quantity <= 0 || src->unit_price < 0.0 ||
        !validate_iso_date(src->transaction_date)) {
        return 0;
    }

    if (find_transaction_by_id(ledger, src->transaction_id) != NULL) {
        return -1;
    }

    if (ledger->count >= ledger->capacity) {
        int next_cap = (ledger->capacity <= 0) ? INIT_TX_CAP : (ledger->capacity * 2);
        Transaction *tmp = (Transaction *)realloc(ledger->items, (size_t)next_cap * sizeof(Transaction));
        if (tmp == NULL) {
            return 0;
        }
        ledger->items = tmp;
        ledger->capacity = next_cap;
    }

    slot = ledger->items + ledger->count;
    *slot = *src;
    slot->total_amount = (double)slot->quantity * slot->unit_price;
    ledger->count++;
    return 1;
}

void op_add_transaction(SalesLedger *ledger) {
    Transaction tx;
    int status;

    memset(&tx, 0, sizeof(tx));
    printf("\nAdd New Sales Transaction\n");

    if (read_int_range("Transaction ID (1 - 999999): ", 1, 999999, &tx.transaction_id) <= 0) {
        printf("Invalid transaction ID.\n");
        return;
    }
    if (find_transaction_by_id(ledger, tx.transaction_id) != NULL) {
        printf("Duplicate rejected: Transaction ID %d already exists.\n", tx.transaction_id);
        return;
    }
    if (read_nonempty_string("Product Name: ", tx.product_name, sizeof(tx.product_name)) <= 0) {
        printf("Product name cannot be empty.\n");
        return;
    }
    if (read_int_range("Quantity Sold (1 - 1000000): ", 1, 1000000, &tx.quantity) <= 0) {
        printf("Invalid quantity. Must be a positive integer.\n");
        return;
    }
    if (read_double_range("Unit Price (>= 0): ", 0.0, 10000000.0, &tx.unit_price) <= 0) {
        printf("Invalid unit price. Cannot be negative.\n");
        return;
    }
    if (read_nonempty_string("Transaction Date (YYYY-MM-DD): ", tx.transaction_date, sizeof(tx.transaction_date)) <= 0 ||
        !validate_iso_date(tx.transaction_date)) {
        printf("Invalid date format or calendar value. Expected YYYY-MM-DD.\n");
        return;
    }

    status = ledger_add_record(ledger, &tx);
    if (status == 1) {
        const Transaction *added = ledger->items + (ledger->count - 1);
        printf("Recorded Tx #%d (%s x%d @ %.2f = %.2f). Active records: %d (cap %d).\n",
               added->transaction_id, added->product_name, added->quantity,
               added->unit_price, added->total_amount, ledger->count, ledger->capacity);
    } else if (status == -1) {
        printf("Duplicate transaction ID rejected.\n");
    } else {
        fprintf(stderr, "Error: Failed to allocate memory for new transaction.\n");
    }
}

void op_display_transactions(const SalesLedger *ledger) {
    const Transaction *p;
    const Transaction *end;

    if (ledger == NULL || ledger->count <= 0 || ledger->items == NULL) {
        printf("\nNo sales transactions recorded.\n");
        return;
    }

    end = ledger->items + ledger->count;
    printf("\n%-8s %-24s %8s %11s %12s %-12s\n",
           "Tx ID", "Product Name", "Qty", "Unit Price", "Total", "Date");
    printf("--------------------------------------------------------------------------------\n");
    for (p = ledger->items; p < end; p++) {
        printf("%-8d %-24.24s %8d %11.2f %12.2f %-12s\n",
               p->transaction_id, p->product_name, p->quantity,
               p->unit_price, p->total_amount, p->transaction_date);
    }
    printf("Total transactions: %d\n", ledger->count);
}

void op_update_transaction(SalesLedger *ledger) {
    int id;
    Transaction *tx;
    char buf[PROD_NAME_LEN];
    int qty;
    double price;

    if (ledger == NULL || ledger->count <= 0) {
        printf("\nNo transactions available to update.\n");
        return;
    }
    if (read_int_range("Enter Transaction ID to update: ", 1, 999999, &id) <= 0) {
        printf("Invalid transaction ID.\n");
        return;
    }

    tx = find_transaction_by_id(ledger, id);
    if (tx == NULL) {
        printf("Transaction ID %d not found.\n", id);
        return;
    }

    printf("Updating Tx #%d (Product: %s, Qty: %d, Unit Price: %.2f, Date: %s)\n",
           tx->transaction_id, tx->product_name, tx->quantity, tx->unit_price, tx->transaction_date);

    printf("New Product Name (Enter to keep '%s'): ", tx->product_name);
    if (read_line_trimmed(buf, PROD_NAME_LEN) && buf[0] != '\0') {
        strncpy(tx->product_name, buf, PROD_NAME_LEN - 1);
        tx->product_name[PROD_NAME_LEN - 1] = '\0';
    }

    if (read_int_range("New Quantity Sold (1 - 1000000): ", 1, 1000000, &qty) <= 0) {
        printf("Invalid quantity. Update aborted.\n");
        return;
    }
    if (read_double_range("New Unit Price (>= 0): ", 0.0, 10000000.0, &price) <= 0) {
        printf("Invalid price. Update aborted.\n");
        return;
    }

    tx->quantity = qty;
    tx->unit_price = price;
    tx->total_amount = (double)qty * price;

    printf("New Date YYYY-MM-DD (Enter to keep '%s'): ", tx->transaction_date);
    if (read_line_trimmed(buf, DATE_STR_LEN) && buf[0] != '\0') {
        if (validate_iso_date(buf)) {
            strncpy(tx->transaction_date, buf, DATE_STR_LEN - 1);
            tx->transaction_date[DATE_STR_LEN - 1] = '\0';
        } else {
            printf("Invalid date entered; keeping existing date '%s'.\n", tx->transaction_date);
        }
    }

    printf("Updated Tx #%d. New total: %.2f.\n", tx->transaction_id, tx->total_amount);
}

void op_delete_transaction(SalesLedger *ledger) {
    int id;
    Transaction *target;
    Transaction *p;
    Transaction *end;

    if (ledger == NULL || ledger->count <= 0) {
        printf("\nNo transactions available to delete.\n");
        return;
    }
    if (read_int_range("Enter Transaction ID to delete: ", 1, 999999, &id) <= 0) {
        printf("Invalid transaction ID.\n");
        return;
    }

    target = find_transaction_by_id(ledger, id);
    if (target == NULL) {
        printf("Transaction ID %d not found.\n", id);
        return;
    }

    end = ledger->items + ledger->count;
    for (p = target; p < end - 1; p++) {
        *p = *(p + 1);
    }
    ledger->count--;

    if (ledger->count > INIT_TX_CAP && ledger->count <= ledger->capacity / 2) {
        int next_cap = ledger->capacity / 2;
        Transaction *shrunk = (Transaction *)realloc(ledger->items, (size_t)next_cap * sizeof(Transaction));
        if (shrunk != NULL) {
            ledger->items = shrunk;
            ledger->capacity = next_cap;
        }
    }

    printf("Deleted Transaction ID %d. Remaining records: %d.\n", id, ledger->count);
}
