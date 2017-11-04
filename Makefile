include sources.mk
CC = arm-linux-gnueabihf-gcc

CFLAGS = -g -Wall -O0 -I $(HDRPATH)

.PHONY: build
build : $(SOURCE)
	$(CC) $^ $(CFLAGS) -o main -pthread -lrt -lm

clean : 
	rm -rf main log.txt
