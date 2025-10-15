CC = gcc
CFLAGS = -O2 -std=c11 -Wall
TARGET = tinypvm
SOURCE = tinypvm.c

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

.PHONY: clean