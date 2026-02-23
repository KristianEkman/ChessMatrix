CC ?= clang
CFLAGS ?= -std=c11 -O3 -Wall -Wextra
LDFLAGS ?=

SRC_DIR := src
SOURCES := $(wildcard $(SRC_DIR)/*.c)
TARGET := chessmatrix

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS) -pthread

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
