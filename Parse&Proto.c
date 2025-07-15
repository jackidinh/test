#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "accounts_reader.h"  
#include "accountsProtoSchema.pb-c.h"        

int main() {
    // Open FlatBuffer file
    FILE *fb_file = fopen("account.bin", "rb");
    if (!fb_file) {
        perror("Failed to open FlatBuffer file");
        return 1;
    }
    fseek(fb_file, 0, SEEK_END);
    size_t size = ftell(fb_file);
    fseek(fb_file, 0, SEEK_SET);

    // Read FlatBuffer data into buffer
    uint8_t *buffer = malloc(size);
    if (!buffer) {
        perror("malloc failed");
        fclose(fb_file);
        return 1;
    }

    if (fread(buffer, 1, size, fb_file) != size) {
        perror("Failed to read FlatBuffer data");
        free(buffer);
        fclose(fb_file);
        return 1;
    }
    fclose(fb_file);

    // Parse root FlatBuffer object
    Trade_flatbuf_Account_table_t fb_account = Trade_flatbuf_Account_as_root(buffer);
    if (!fb_account) {
        fprintf(stderr, "Invalid FlatBuffer data\n");
        free(buffer);
        return 1;
    }

    // Initialize protobuf Account message
    Trade__Account pb_account = TRADE__ACCOUNT__INIT;

    // Copy scalar fields
    pb_account.id = Trade_flatbuf_Account_id(fb_account);
    pb_account.name = (char *)Trade_flatbuf_Account_name(fb_account);

    // Copy wallet (Balance)
    Trade_flatbuf_Balance_table_t fb_balance = Trade_flatbuf_Account_wallet(fb_account);
    if (fb_balance) {
        static Trade__Balance pb_balance = TRADE__BALANCE__INIT;
        pb_balance.currency = (char *)Trade_flatbuf_Balance_currency(fb_balance);
        pb_balance.amount = Trade_flatbuf_Balance_amount(fb_balance);
        pb_account.wallet = &pb_balance;
    }

    // Copy orders vector
    size_t orders_len = Trade_flatbuf_Account_orders_length(fb_account);
    if (orders_len > 0) {
        // Allocate arrays for protobuf repeated field
        Trade__Order **pb_orders = malloc(sizeof(Trade__Order*) * orders_len);
        Trade__Order *pb_orders_storage = malloc(sizeof(Trade__Order) * orders_len);
        if (!pb_orders || !pb_orders_storage) {
            fprintf(stderr, "Memory allocation failed\n");
            free(buffer);
            free(pb_orders);
            free(pb_orders_storage);
            return 1;
        }

        for (size_t i = 0; i < orders_len; i++) {
            Trade_flatbuf_Order_table_t fb_order = Trade_flatbuf_Account_orders(fb_account, i);
            pb_orders_storage[i] = TRADE__ORDER__INIT;

            pb_orders_storage[i].id = Trade_flatbuf_Order_id(fb_order);
            pb_orders_storage[i].symbol = (char *)Trade_flatbuf_Order_symbol(fb_order);
            pb_orders_storage[i].side = (Trade__OrderSide)Trade_flatbuf_Order_side(fb_order);
            pb_orders_storage[i].type = (Trade__OrderType)Trade_flatbuf_Order_type(fb_order);
            pb_orders_storage[i].price = Trade_flatbuf_Order_price(fb_order);
            pb_orders_storage[i].volume = Trade_flatbuf_Order_volume(fb_order);

            pb_orders[i] = &pb_orders_storage[i];
        }
        pb_account.n_orders = orders_len;
        pb_account.orders = pb_orders;

        // After serialization, free these
    }

    // Serialize protobuf message to file
    FILE *pb_file = fopen("account.pb", "wb");
    if (!pb_file) {
        perror("Failed to open protobuf output file");
        free(buffer);
        free(pb_account.orders);
        free(pb_orders_storage);
        return 1;
    }

    size_t pb_len = trade__account__get_packed_size(&pb_account);
    uint8_t *pb_buf = malloc(pb_len);
    if (!pb_buf) {
        perror("malloc failed");
        fclose(pb_file);
        free(buffer);
        free(pb_account.orders);
        free(pb_orders_storage);
        return 1;
    }

    trade__account__pack(&pb_account, pb_buf);
    fwrite(pb_buf, 1, pb_len, pb_file);
    fclose(pb_file);

    // Cleanup
    free(buffer);
    free(pb_account.orders);
    free(pb_orders_storage);
    free(pb_buf);

    printf("Successfully converted FlatBuffer data to Protobuf format.\n");

    return 0;
}
