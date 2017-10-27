include sources.mk
CC = gcc

CFLAGS = -g -Wall -O0 -I $(HDRPATH)

.PHONY: build
build : $(SOURCE)
	$(CC) $^ $(CFLAGS) -o main -pthread -lrt

clean : 
	rm -rf main log.txt
