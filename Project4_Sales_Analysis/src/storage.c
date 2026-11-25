#define _CRT_SECURE_NO_WARNINGS
#include "../include/storage.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TX_MAGIC "STXN"
#define TX_VERSION 1

static void clean_csv_str(char *dst, const char *src, size_t cap) {
    size_t i = 0;
    if (cap == 0) return;
    while (*src != '\0' && i + 1 < cap) {
        char c = *src++;
        dst[i++] = (c == ',' || c == '\n' || c == '\r') ? ' ' : c;
    }
    dst[i] = '\0';
}

int save_sales_binary(const SalesLedger *ledger, const char *path) {
    FILE *fp;
    int version = TX_VERSION;

    if (ledger == NULL || path == NULL) return 0;

    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("Error opening binary file for writing");
        return 0;
    }

    if (fwrite(TX_MAGIC, 1, 4, fp) != 4 ||
        fwrite(&version, sizeof(int), 1, fp) != 1 ||
        fwrite(&ledger->count, sizeof(int), 1, fp) != 1) {
        perror("Error writing binary header");
        fclose(fp);
        return 0;
    }

    if (ledger->count > 0 && ledger->items != NULL) {
        if (fwrite(ledger->items, sizeof(Transaction), (size_t)ledger->count, fp) != (size_t)ledger->count) {
            perror("Error writing transaction records");
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    return 1;
}

int load_sales_binary(SalesLedger *ledger, const char *path) {
    FILE *fp;
    char magic[4];
    int version = 0;
    int count = 0;
    int i;
    SalesLedger loaded;

    if (ledger == NULL || path == NULL) return 0;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1;
    }

    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, TX_MAGIC, 4) != 0 ||
        fread(&version, sizeof(int), 1, fp) != 1 || version != TX_VERSION ||
        fread(&count, sizeof(int), 1, fp) != 1 || count < 0 || count > 100000) {
        fprintf(stderr, "Error: Invalid or corrupted sales binary file '%s'.\n", path);
        fclose(fp);
        return 0;
    }

    if (!ledger_init(&loaded)) {
        fclose(fp);
        return 0;
    }

    for (i = 0; i < count; i++) {
        Transaction tx;
        if (fread(&tx, sizeof(Transaction), 1, fp) != 1) {
            fprintf(stderr, "Error: Truncated transaction record in '%s'.\n", path);
            ledger_free(&loaded);
            fclose(fp);
            return 0;
        }
        if (ledger_add_record(&loaded, &tx) != 1) {
            ledger_free(&loaded);
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    ledger_free(ledger);
    *ledger = loaded;
    return 1;
}

int export_sales_csv(const SalesLedger *ledger, const char *path) {
    FILE *fp;
    const Transaction *p;
    const Transaction *end;

    if (ledger == NULL || path == NULL) return 0;

    fp = fopen(path, "w");
    if (fp == NULL) {
        perror("Error opening CSV file for writing");
        return 0;
    }

    fprintf(fp, "transaction_id,product_name,quantity,unit_price,total_amount,transaction_date\n");
    end = ledger->items + ledger->count;
    for (p = ledger->items; p < end; p++) {
        char pname[PROD_NAME_LEN];
        clean_csv_str(pname, p->product_name, sizeof(pname));
        fprintf(fp, "%d,%s,%d,%.2f,%.2f,%s\n",
                p->transaction_id, pname, p->quantity,
                p->unit_price, p->total_amount, p->transaction_date);
    }

    fclose(fp);
    return 1;
}

static int split_csv_6(char *line, char *fields[6]) {
    int col = 0;
    char *p = line;

    fields[col++] = p;
    while (*p != '\0' && col < 6) {
        if (*p == ',') {
            *p = '\0';
            fields[col++] = p + 1;
        }
        p++;
    }
    return col == 6;
}

int import_sales_csv(SalesLedger *ledger, const char *path) {
    FILE *fp;
    char line[512];
    SalesLedger loaded;
    int line_no = 0;

    if (ledger == NULL || path == NULL) return 0;

    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Error opening CSV file for reading");
        return 0;
    }

    if (!ledger_init(&loaded)) {
        fclose(fp);
        return 0;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *f[6];
        Transaction tx;

        line_no++;
        trim_whitespace(line);
        if (line[0] == '\0') continue;
        if (line_no == 1 && strncmp(line, "transaction_id,", 15) == 0) continue;
        if (!split_csv_6(line, f)) continue;

        memset(&tx, 0, sizeof(tx));
        tx.transaction_id = atoi(f[0]);
        strncpy(tx.product_name, f[1], PROD_NAME_LEN - 1);
        tx.quantity = atoi(f[2]);
        tx.unit_price = atof(f[3]);
        strncpy(tx.transaction_date, f[5], DATE_STR_LEN - 1);

        ledger_add_record(&loaded, &tx);
    }

    fclose(fp);
    ledger_free(ledger);
    *ledger = loaded;
    return 1;
}

int seed_sample_sales(SalesLedger *ledger) {
    static const Transaction samples[6] = {
        {5001, "Arduino Uno Kit", 4, 24.50, 98.00, "2026-11-18"},
        {5002, "DHT22 Sensor Pack", 10, 8.20, 82.00, "2026-11-19"},
        {5003, "16x2 LCD Module", 6, 12.00, 72.00, "2026-11-20"},
        {5004, "Arduino Uno Kit", 2, 24.50, 49.00, "2026-11-21"},
        {5005, "Breadboard Jumper Kit", 15, 4.50, 67.50, "2026-11-22"},
        {5006, "STM32 Dev Board", 3, 38.00, 114.00, "2026-11-23"}
    };

    int i;
    int added = 0;
    for (i = 0; i < 6; i++) {
        if (ledger_add_record(ledger, &samples[i]) == 1) {
            added++;
        }
    }
    return added;
}

void op_save_transactions(const SalesLedger *ledger) {
    int fmt;
    char path[160];

    printf("\nSave / Export Sales Format:\n");
    printf("1. Binary File (default: %s)\n", SALES_BIN_PATH);
    printf("2. CSV File (default: %s)\n", SALES_CSV_PATH);
    if (read_int_range("Format (1-2): ", 1, 2, &fmt) <= 0) {
        printf("Invalid format.\n");
        return;
    }

    if (fmt == 1) {
        printf("Enter path (Enter for %s): ", SALES_BIN_PATH);
        if (!read_line_trimmed(path, sizeof(path))) return;
        if (path[0] == '\0') strcpy(path, SALES_BIN_PATH);
        if (save_sales_binary(ledger, path)) {
            printf("Saved %d transaction(s) to binary '%s'.\n", ledger->count, path);
        }
    } else {
        printf("Enter path (Enter for %s): ", SALES_CSV_PATH);
        if (!read_line_trimmed(path, sizeof(path))) return;
        if (path[0] == '\0') strcpy(path, SALES_CSV_PATH);
        if (export_sales_csv(ledger, path)) {
            printf("Exported %d transaction(s) to CSV '%s'.\n", ledger->count, path);
        }
    }
}

void op_load_transactions(SalesLedger *ledger) {
    int opt;
    char path[160];

    printf("\nLoad Transactions From:\n");
    printf("1. Binary File (default: %s)\n", SALES_BIN_PATH);
    printf("2. CSV File (default: %s)\n", SALES_CSV_PATH);
    printf("3. Load Built-in Sample Transactions (6 records)\n");
    if (read_int_range("Option (1-3): ", 1, 3, &opt) <= 0) {
        printf("Invalid load option.\n");
        return;
    }

    if (opt == 1) {
        int res;
        printf("Enter path (Enter for %s): ", SALES_BIN_PATH);
        if (!read_line_trimmed(path, sizeof(path))) return;
        if (path[0] == '\0') strcpy(path, SALES_BIN_PATH);
        res = load_sales_binary(ledger, path);
        if (res == 1) {
            printf("Loaded %d transaction(s) from binary '%s'.\n", ledger->count, path);
        } else if (res == -1) {
            printf("File '%s' does not exist.\n", path);
        }
    } else if (opt == 2) {
        printf("Enter path (Enter for %s): ", SALES_CSV_PATH);
        if (!read_line_trimmed(path, sizeof(path))) return;
        if (path[0] == '\0') strcpy(path, SALES_CSV_PATH);
        if (import_sales_csv(ledger, path)) {
            printf("Imported %d transaction(s) from CSV '%s'.\n", ledger->count, path);
        }
    } else {
        int added = seed_sample_sales(ledger);
        printf("Added %d sample transaction(s). Active records: %d.\n", added, ledger->count);
    }
}
