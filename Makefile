CC=gcc
CFLAGS=-g -Wall -Wextra

INDEX_OBJ= index_main.o index.o merge.o  naive_hash_table.o bin_heap.o md5.o util.o
SEARCH_OBJ= search_main.o search.o naive_hash_table.o md5.o util.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

all: index search

index: $(INDEX_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
search: $(SEARCH_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o  index search
