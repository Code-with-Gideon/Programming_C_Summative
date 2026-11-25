#define _CRT_SECURE_NO_WARNINGS
#include "../include/analytics.h"
#include "../include/sales.h"
#include "../include/storage.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>

static void print_menu(void) {
    printf("\nSales Transaction Management System\n");
    printf("1.  Add New Transaction\n");
    printf("2.  Display All Transactions\n");
    printf("3.  Update Transaction\n");
    printf("4.  Delete Transaction\n");
    printf("5.  Process Sales Data\n");
    printf("6.  Filter Transactions\n");
    printf("7.  Search Transactions\n");
    printf("8.  Sort Transactions\n");
    printf("9.  Generate Sales Report\n");
    printf("10. Save Transactions (Binary or CSV)\n");
    printf("11. Load Transactions (Binary, CSV, or Samples)\n");
    printf("0.  Save and Exit\n");
}

int main(void) {
    SalesLedger ledger;
    int choice;
    int status;
    int load_res;

    printf("Student ID: 1376251039 | Name: Erioluwa Gideon Olowoyo\n");
    printf("Project 4: Sales Transaction Processing and Analysis System\n");

    if (!ledger_init(&ledger)) {
        fprintf(stderr, "Fatal: Initial transaction ledger allocation failed.\n");
        return EXIT_FAILURE;
    }

    load_res = load_sales_binary(&ledger, SALES_BIN_PATH);
    if (load_res == 1) {
        printf("Loaded %d saved transaction(s) from '%s'.\n", ledger.count, SALES_BIN_PATH);
    } else if (load_res == -1) {
        printf("No saved file at '%s'. Starting with an empty ledger.\n", SALES_BIN_PATH);
    }

    while (1) {
        print_menu();
        status = read_int_range("Choice (0-11): ", 0, 11, &choice);
        if (status == 0) {
            break;
        }
        if (status < 0) {
            printf("Invalid choice. Pick 0 to 11.\n");
            continue;
        }
        if (choice == 0) {
            break;
        }

        switch (choice) {
            case 1:  op_add_transaction(&ledger); break;
            case 2:  op_display_transactions(&ledger); break;
            case 3:  op_update_transaction(&ledger); break;
            case 4:  op_delete_transaction(&ledger); break;
            case 5:  op_process_sales_data(&ledger); break;
            case 6:  op_filter_transactions(&ledger); break;
            case 7:  op_search_transactions(&ledger); break;
            case 8:  op_sort_transactions(&ledger); break;
            case 9:  op_generate_sales_report(&ledger); break;
            case 10: op_save_transactions(&ledger); break;
            case 11: op_load_transactions(&ledger); break;
            default: break;
        }
    }

    if (save_sales_binary(&ledger, SALES_BIN_PATH)) {
        printf("\nSaved %d transaction(s) to '%s'.\n", ledger.count, SALES_BIN_PATH);
    }
    ledger_free(&ledger);
    printf("Freed transaction ledger memory. Exiting.\n");
    return EXIT_SUCCESS;
}
