SRC_DIR = src
BUILD_DIR = build
TEST_DIR = test

CC = zig cc
CFLAGS = -I$(SRC_DIR) -Wall -Wextra -g
LDFLAGS =

TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_ARTIFACTS = $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%, $(TEST_SOURCES))

$(TEST_DIR):
	mkdir -p $(TEST_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o$@ $< $(LDFLAGS)

.PHONY: test 
test: $(TEST_ARTIFACTS)

.PHONY: clean
clean:
		rm -rf $(BUILD_DIR)/*
