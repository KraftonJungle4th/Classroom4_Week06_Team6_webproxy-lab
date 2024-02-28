CC = gcc
CFLAGS = -O2 -Wall -I ..

all: adder

adder: adder4Web adder_POST
	$(CC) $(CFLAGS) -o adder4Web adder4Web.c adder_POST

clean:
	rm -f adder *~
