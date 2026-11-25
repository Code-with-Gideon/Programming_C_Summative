CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -Iinclude -pthread
LDFLAGS = -pthread

SRCS = src/main.c src/analyzer.c src/utils.c

all: document_analyzer

document_analyzer: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o document_analyzer $(LDFLAGS)

clean:
	rm -f document_analyzer *.o
