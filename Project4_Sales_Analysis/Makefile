CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -Iinclude
LDFLAGS = -lm

SRCS = src/main.c src/sales.c src/analytics.c src/storage.c src/utils.c
TEST_SRCS = tests/test_p4.c src/sales.c src/analytics.c src/storage.c src/utils.c

all: sales_analysis

sales_analysis: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o sales_analysis $(LDFLAGS)

test: $(TEST_SRCS)
	$(CC) $(CFLAGS) $(TEST_SRCS) -o test_p4 $(LDFLAGS)
	./test_p4

clean:
	rm -f sales_analysis test_p4 *.o
