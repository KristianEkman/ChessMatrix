CC ?= clang
LDFLAGS ?=

ARCH := $(shell uname -m)
ifeq ($(ARCH), arm64)
    CFLAGS ?= -std=c11 -O3 -Wall -Wextra
else
    CFLAGS ?= -std=c11 -O3 -Wall -Wextra -mavx2 -mfma
endif

SRC_DIR := src
CORE_SOURCES := $(filter-out $(SRC_DIR)/tests.c,$(wildcard $(SRC_DIR)/*.c))
TEST_SOURCES := $(wildcard $(SRC_DIR)/tests/*.c)
SOURCES := $(CORE_SOURCES) $(TEST_SOURCES)
TARGET := chessmatrix

all: $(TARGET)

strict: clean
	$(MAKE) CFLAGS='$(STRICT_CFLAGS)'

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SOURCES) -o $(TARGET) $(LDFLAGS) -pthread

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@tmp=$$(mktemp); \
	printf "i\nt\n\nq\nquit\n" | ./$(TARGET) | tee "$$tmp"; \
	if grep -q "There are failed tests\." "$$tmp"; then \
		rm -f "$$tmp"; \
		exit 1; \
	fi; \
	grep -q "Success! Tests are good!" "$$tmp"; \
	status=$$?; \
	rm -f "$$tmp"; \
	exit $$status

clean:
	rm -f $(TARGET)

.PHONY: all strict run test clean
