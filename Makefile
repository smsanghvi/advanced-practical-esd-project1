include sources.mk
CC = gcc
CFLAGS = -Wall -std=c99 -g -O0 -I$(HDRPATH)

.PHONY: build
build : $(SOURCE)
	$(CC) $(CFLAGS) -o main $^
