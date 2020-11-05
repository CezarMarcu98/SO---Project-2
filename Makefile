CC = gcc
LDFLAGS = -Wall -Werror -shared -fPIC

build:
	$(CC) $(LDFLAGS) *.c -o libso_stdio.so


.PHONY: clean
clean:
	rm libso_stdio.so
