CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -Iinclude
LDFLAGS = -lm

SRCS = src/main.c src/student.c src/analytics.c src/storage.c src/utils.c
TEST_SRCS = tests/test_p3.c src/student.c src/analytics.c src/storage.c src/utils.c

all: student_analytics

student_analytics: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o student_analytics $(LDFLAGS)

test: $(TEST_SRCS)
	$(CC) $(CFLAGS) $(TEST_SRCS) -o test_p3 $(LDFLAGS)
	./test_p3

clean:
	rm -f student_analytics test_p3 *.o
