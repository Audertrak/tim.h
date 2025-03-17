BUILD_DIR = build
TARGET = $(BUILD_DIR)/minilua.exe
DEBUG = $(BUILD_DIR)/debug.exe

# Optimization flags
CC = zig cc
RELEASE_FLAGS = -Wall -Wextra -O3 -flto
DEBUG_FLAGS = -Wall -Wextra -g -DDEBUG

all: release debug

release: $(TARGET)

debug: $(DEBUG)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): main.c | $(BUILD_DIR)
	$(CC) $(RELEASE_FLAGS) -o $@ $<

$(DEBUG): main.c | $(BUILD_DIR)
	$(CC) $(DEBUG_FLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean release debug

