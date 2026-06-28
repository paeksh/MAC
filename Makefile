# Makefile
CC = gcc
CFLAGS = -Wall -Wextra

all: mac

mac: mac.c
	$(CC) $(CFLAGS) -o mac mac.c 

clean:
	rm -f mac *.log               