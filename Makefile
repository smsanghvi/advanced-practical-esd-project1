include sources.mk
CC = gcc

CFLAGS = -g -Wall -std=c99 -O0 -I $(HDRPATH)

.PHONY: build
build : $(SOURCE)
	$(CC) $^ $(CFLAGS) -o main -pthread -lrt
