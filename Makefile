CC ?= clang
LDFLAGS ?=

ARCH := $(shell uname -m)
ifeq ($(ARCH), arm64)
    CFLAGS ?= -std=c11 -O3 -Wall -Wextra
else
    CFLAGS ?= -std=c11 -O3 -Wall -Wextra -mavx2 -mfma
endif

SRC_DIR := src
ANN_DIR := ../ANN
ANN_BUILD_DIR := $(ANN_DIR)/build
ANN_LIB := $(ANN_BUILD_DIR)/libANN_lib.a
ANN_INCLUDE_DIR := $(ANN_DIR)/ANN
CORE_SOURCES := $(filter-out $(SRC_DIR)/tests.c,$(wildcard $(SRC_DIR)/*.c))
TEST_SOURCES := $(wildcard $(SRC_DIR)/tests/*.c)
SOURCES := $(CORE_SOURCES) $(TEST_SOURCES)
TARGET := chessmatrix
TEST_NAME_ARG := $(strip $(testName))
BENCH_DEPTH_ARG := $(if $(strip $(depth)),$(strip $(depth)),7)

ifeq ($(TEST_NAME_ARG),)
TEST_NAME_ARG := $(firstword $(filter-out test,$(MAKECMDGOALS)))
endif

ifneq ($(filter test,$(MAKECMDGOALS)),)
$(foreach goal,$(filter-out test,$(MAKECMDGOALS)),$(eval .PHONY: $(goal))$(eval $(goal): ; @:))
endif

all: $(TARGET)

strict: clean
	$(MAKE) CFLAGS='$(STRICT_CFLAGS)'

ann-lib:
	@if [ ! -f "$(ANN_BUILD_DIR)/CMakeCache.txt" ]; then $(MAKE) -C $(ANN_DIR) configure; fi
	@$(MAKE) -C $(ANN_DIR) build

$(TARGET): $(SOURCES) ann-lib
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(ANN_INCLUDE_DIR) $(SOURCES) $(ANN_LIB) -o $(TARGET) $(LDFLAGS) -pthread -lm

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@if [ -n "$(TEST_NAME_ARG)" ]; then \
		./$(TARGET) test "$(TEST_NAME_ARG)"; \
	else \
		./$(TARGET) test; \
	fi

bench: $(TARGET)
	./$(TARGET) bench $(BENCH_DEPTH_ARG)

clean:
	rm -f $(TARGET)

.PHONY: all strict run test bench clean ann-lib
