#include "accounts_builder.h"
#include <stdio.h>


int main(void) {
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	flatbuffers_string_ref_t name = flatbuffers_string_create_str(&builder, "Jackie");
	flatbuffers_string_ref_t currency = flatbuffers_string_create_str(&builder, "USD");

	Trade_flatbuf_Balance_ref_t balance = Trade_flatbuf_Balance_create(&builder, currency, 1000.0);
	Trade_flatbuf_Order_ref_t order = Trade_flatbuf_Order_create(&builder, 1, flatbuffers_string_create_str(&builder, "AAPL"), Trade_flatbuf_OrderSide_buy, Trade_flatbuf_OrderType_limit, 145.0, 10.0);

	Trade_flatbuf_Order_ref_t orders[] = { order };
	Trade_flatbuf_Order_vec_ref_t order_vec = Trade_flatbuf_Account_orders_create(&builder, orders, 1);
	Trade_flatbuf_Account_ref_t account = Trade_flatbuf_Account_create(&builder, 42, name, balance, order_vec);
	Trade_flatbuf_Account_finish(&builder, account);

	size_t size;
	void* buffer = flatcc_builder_get_direct_buffer(&builder, &size);
	FILE* f = fopen("account.bin", "wb");
	fwrite(buffer, 1, size, f);
	fclose(f);

	flatcc_builder_clear(&builder);
	return 0;
}
