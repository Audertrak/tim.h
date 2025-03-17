CC = zig cc
CFLAGS = -Wall -Wextra -g
LDFLAGS =

SOURCE_DIR = src
BUILD_DIR = build
TEST_DIR = test


# Platform-specific sources and flags
ifeq ($(OS),Windows_NT)
	TARGET = hawkTUI.exe
    SOURCES = $(SRC_DIR)/hawkTUI.h
    LDFLAGS += -luser32  # Link with user32.lib (for Windows API)
else
	TARGET = hawkTUI
    SOURCES = $(SRC_DIR)/hawkTUI.h
    # Add Linux-specific libraries if needed (e.g., -lncurses)
endif

TEST_TARGET = $(TEST_DIR)/test_init.exe

BUILD_TARGET = $(BUILD_DIR)/hawkTUI.exe

all: $(TARGET)

$(TEST_DIR):
	mkdir -p $(TEST_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS) -DHAWKTUI_IMPLEMENTATION

test: $(TARGET)
	$(CC) $(CFLAGS) -o $(TEST_DIR)/$(TEST_TARGET) $(TEST_DIR)/test_init.c $(LDFLAGS) -I$(SRC_DIR)

clean:
	rm -rf $(TARGET) $(TEST_DIR)/*.o $(TEST_DIR)/*.exe $(BUILD_DIR)/*.exe

.PHONY: all clean test
