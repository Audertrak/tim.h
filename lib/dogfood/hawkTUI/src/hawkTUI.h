// hawkTUI.h - Single-header TUI library

#ifndef HAWKTUI_H
#define HAWKTUI_H

#include "arena.h" // Include arena.h
#include "clay.h"  // Include Clay

// --- Error Codes ---

typedef enum {
  TUI_SUCCESS,
  TUI_ERROR_INIT_FAILED,
  TUI_ERROR_OUT_OF_MEMORY,
  // ... other error codes ...
} TUI_Result;

// --- Character Attributes ---

typedef struct {
  char ch;    // Character to display
  uint8_t fg; // Foreground color
  uint8_t bg; // Background color
} TUI_Cell;

// --- Key Codes (Simplified for Now) ---
typedef struct {
  int code;
  // bool alt;
  // bool ctrl; // Add these in when we need them
  // bool shift;
} TUI_KeyCode;

// --- Events ---
typedef struct {
  bool pressed;
  TUI_KeyCode key;
} TUI_Event;

// --- TUI Context (Opaque) ---
typedef struct TUI_Context TUI_Context;

// --- Function Prototypes (Public API) ---
TUI_Result tui_init(TUI_Context **context, Clay_Dimensions dimensions);
void tui_free(TUI_Context *context);
TUI_Result tui_clear(TUI_Context *context);
TUI_Result tui_draw_cell(TUI_Context *context, int x, int y, TUI_Cell cell);
TUI_Result tui_draw_string(TUI_Context *context, int x, int y, const char *str,
                           uint8_t fg, uint8_t bg);
Clay_Dimensions tui_get_dimensions(TUI_Context *context);
TUI_Result tui_swap_buffers(TUI_Context *context);
TUI_Result tui_get_event(TUI_Context *context, TUI_Event *event);
bool tui_running(TUI_Context *context);

// --- Implementation (Conditional Compilation) ---

#ifdef HAWKTUI_IMPLEMENTATION

// --- Platform-Agnostic Helpers ---
TUI_Result allocate_cell_buffer(TUI_Context *context,
                                Clay_Dimensions dimensions) {
  context->buffer = (TUI_Cell *)arena_alloc(
      context->arena,
      sizeof(TUI_Cell) * (int)(dimensions.width * dimensions.height));
  if (!context->buffer) {
    return TUI_ERROR_OUT_OF_MEMORY; // Or your preferred error code
  }
  return TUI_SUCCESS;
}

TUI_Result tui_clear(TUI_Context *context) {

  if (!context || !context->buffer) {
    return TUI_ERROR_INIT_FAILED;
  }

  Clay_Dimensions dimensions = tui_get_dimensions(context);
  // fill with empty cells
  for (int i = 0; i < dimensions.width * dimensions.height; i++) {
    context->buffer[i] = (TUI_Cell){' ', 7, 0}; // default empty cell
  }
  return TUI_SUCCESS;
}

TUI_Result tui_draw_cell(TUI_Context *context, int x, int y, TUI_Cell cell) {
  if (!context || !context->buffer) {
    return TUI_ERROR_INIT_FAILED; // Or a more specific error
  }

  Clay_Dimensions dimensions = tui_get_dimensions(context);
  if (x < 0 || x >= dimensions.width || y < 0 || y >= dimensions.height) {
    return TUI_ERROR_OUT_OF_MEMORY;
  }

  int index = y * (int)dimensions.width + x;
  context->buffer[index] = cell;
  return TUI_SUCCESS;
}
// --- Windows Implementation ---

#ifdef _WIN32

#include <stdio.h> //for debugging
#include <windows.h>

// Windows-specific TUI context
typedef struct {
  TUI_Context base; // Inherit from the common context (MUST BE FIRST)
  HANDLE consoleOutput;
  HANDLE consoleInput;
  CHAR_INFO *win_buffer; // Windows-specific buffer
  SMALL_RECT drawRect;
  Clay_Dimensions dimensions; // Store dimensions here
  bool running;               // store if running.
  Arena *arena;
} WindowsTUIContext;

TUI_Result windows_init(TUI_Context **context, Clay_Dimensions dimensions) {
  WindowsTUIContext *winContext =
      (WindowsTUIContext *)malloc(sizeof(WindowsTUIContext));
  if (!winContext) {
    return TUI_ERROR_OUT_OF_MEMORY;
  }

  // Initialize Arena.
  winContext->arena = (Arena *)malloc(sizeof(Arena));
  if (!winContext->arena) {
    free(winContext);
    return TUI_ERROR_OUT_OF_MEMORY;
  }
  *winContext->arena = (Arena){0}; // zero out

  // Initialize common part of context
  *context = (TUI_Context *)winContext; // IMPORTANT:  Set the OUT parameter.
  (*context)->running = true;

  winContext->consoleOutput = CreateConsoleScreenBuffer(
      GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
  if (winContext->consoleOutput == INVALID_HANDLE_VALUE) {
    free(winContext->arena);
    free(winContext);
    return TUI_ERROR_INIT_FAILED;
  }

  winContext->consoleInput = GetStdHandle(STD_INPUT_HANDLE);
  if (winContext->consoleInput == INVALID_HANDLE_VALUE) {
    CloseHandle(winContext->consoleOutput);
    free(winContext->arena);
    free(winContext);
    return TUI_ERROR_INIT_FAILED;
  }

  // Enable KEY_EVENT
  DWORD mode;
  GetConsoleMode(winContext->consoleInput, &mode);
  SetConsoleMode(winContext->consoleInput,
                 mode | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);

  winContext->dimensions = dimensions;
  SetConsoleScreenBufferSize(
      winContext->consoleOutput,
      (COORD){(short)dimensions.width, (short)dimensions.height});

  winContext->win_buffer = (CHAR_INFO *)arena_alloc(
      winContext->arena,
      sizeof(CHAR_INFO) * (int)(dimensions.width * dimensions.height));

  if (!winContext->win_buffer) {
    CloseHandle(winContext->consoleOutput);
    free(winContext->arena);
    free(winContext);
    return TUI_ERROR_OUT_OF_MEMORY;
  }

  winContext->drawRect = (SMALL_RECT){0, 0, (short)(dimensions.width - 1),
                                      (short)(dimensions.height - 1)};

  // allocate cell buffer in tui.c
  if (allocate_cell_buffer(*context, dimensions) != TUI_SUCCESS) {
    CloseHandle(winContext->consoleOutput);
    arena_free(winContext->arena);
    free(winContext->arena);
    free(winContext);

    return TUI_ERROR_OUT_OF_MEMORY;
  }
  return TUI_SUCCESS;
}

void windows_free(TUI_Context *context) {
  if (context) {
    WindowsTUIContext *winContext = (WindowsTUIContext *)context;
    CloseHandle(winContext->consoleOutput);
    CloseHandle(winContext->consoleInput);
    arena_free(winContext->arena); // free the arena
    free(winContext->arena);
    free(winContext); // free context
  }
}

Clay_Dimensions windows_get_dimensions(TUI_Context *context) {
  WindowsTUIContext *winContext = (WindowsTUIContext *)context;
  return winContext->dimensions;
}

TUI_Result windows_swap_buffers(TUI_Context *context) {
  WindowsTUIContext *winContext = (WindowsTUIContext *)context;

  // copy our buffer to the windows buffer
  for (int i = 0;
       i < winContext->dimensions.width * winContext->dimensions.height; i++) {
    winContext->win_buffer[i].Char.AsciiChar = context->buffer[i].ch;
    winContext->win_buffer[i].Attributes =
        context->buffer[i].fg +
        (context->buffer[i].bg << 4); // calculate attributes.
  }

  if (!WriteConsoleOutput(winContext->consoleOutput, winContext->win_buffer,
                          (COORD){(short)winContext->dimensions.width,
                                  (short)winContext->dimensions.height},
                          (COORD){0, 0}, &winContext->drawRect)) {
    return TUI_ERROR_INIT_FAILED; // Replace with a proper error code
  }

  if (!SetConsoleActiveScreenBuffer(winContext->consoleOutput)) {
    return TUI_ERROR_INIT_FAILED; // Replace with a proper error code.
  }

  return TUI_SUCCESS;
}
TUI_Result windows_get_event(TUI_Context *context, TUI_Event *event) {
  WindowsTUIContext *winContext = (WindowsTUIContext *)context;
  INPUT_RECORD inputRecord;
  DWORD eventsRead;
  // Initialize the event to a default state (important!)
  event->pressed = false;
  event->key.code = 0;

  if (!ReadConsoleInput(winContext->consoleInput, &inputRecord, 1,
                        &eventsRead)) {
    return TUI_ERROR_INIT_FAILED;
  }

  if (inputRecord.EventType == KEY_EVENT &&
      inputRecord.Event.KeyEvent.bKeyDown) {
    // Translate windows keycodes into TUI keycodes (place inside windows.c)
    int key_code = inputRecord.Event.KeyEvent.wVirtualKeyCode;
    event->pressed = true;
    event->key.code = key_code;
    return TUI_SUCCESS;
  }
  // not handled.
  return TUI_ERROR_INIT_FAILED;
}

// check to ensure running.
bool windows_running(TUI_Context *context) {
  WindowsTUIContext *win_context = (WindowsTUIContext *)context;
  return win_context->running;
}
// --- Linux Implementation (Placeholder) ---

#elif defined(__linux__)

// Linux-specific TUI context (you'll need to define this)
typedef struct {
  TUI_Context base; // inherit base.
  // ... ncurses or terminfo related members ...
  bool running;
  Arena *arena;
} LinuxTUIContext;

TUI_Result linux_init(TUI_Context **context, Clay_Dimensions dimensions) {
  LinuxTUIContext *linuxContext =
      (LinuxTUIContext *)malloc(sizeof(LinuxTUIContext));
  if (!linuxContext) {
    return TUI_ERROR_OUT_OF_MEMORY;
  }

  // init arena.
  linuxContext->arena = (Arena *)malloc(sizeof(Arena));
  if (!linuxContext->arena) {
    free(linuxContext);
    return TUI_ERROR_OUT_OF_MEMORY;
  }
  *linuxContext->arena = (Arena){0};

  *context = (TUI_Context *)linuxContext;
  // set default to be running.
  (*context)->running = true;
  if (allocate_cell_buffer(*context, dimensions) !=
      TUI_SUCCESS) { // allocate cell buffer.
    free(linuxContext->arena);
    free(linuxContext);
    return TUI_ERROR_OUT_OF_MEMORY;
  }

  // TODO: Initialize ncurses or terminfo here
  fprintf(stderr, "linux_init not implemented yet!\n");

  return TUI_SUCCESS; // Or an appropriate error code
}

void linux_free(TUI_Context *context) {
  if (context) {
    LinuxTUIContext *linux_context = (LinuxTUIContext *)context;
    arena_free(linux_context->arena);
    free(linux_context->arena); // free the arena
    free(linux_context);        // free context
  }
}
Clay_Dimensions linux_get_dimensions(TUI_Context *context) {
  // TODO: Implement using ncurses or terminfo
  LinuxTUIContext *linuxContext = (LinuxTUIContext *)context;

  // For now, return a dummy value
  fprintf(stderr, "linux_get_dimensions not implemented yet!\n");

  // Clay_Dimensions dimensions = {80,24};
  return (Clay_Dimensions){80, 24}; // Dummy dimensions
}

TUI_Result linux_swap_buffers(TUI_Context *context) {
  // TODO: Implement using ncurses or terminfo
  // fprintf(stderr, "linux_swap_buffers not implemented yet!\n");
  return TUI_SUCCESS;
}
TUI_Result linux_get_event(TUI_Context *context, TUI_Event *event) {
  // TODO: Implement using ncurses or terminfo

  // fprintf(stderr, "linux_get_input not implemented.\n");
  return TUI_ERROR_INIT_FAILED; // no input available.
}
// check if the tui is running
bool linux_running(TUI_Context *context) {
  LinuxTUIContext *linux_context = (LinuxTUIContext *)context;
  return linux_context->running; // return if running or not
}

#else
#error "Unsupported platform"
#endif

// --- Platform-Agnostic Dispatch ---
// These functions call the appropriate platform-specific implementation.

TUI_Result tui_init(TUI_Context **context, Clay_Dimensions dimensions) {
#ifdef _WIN32
  return windows_init(context, dimensions);
#elif defined(__linux__)
  return linux_init(context, dimensions);
#else
#error "Unsupported platform"
#endif
}

void tui_free(TUI_Context *context) {
#ifdef _WIN32
  windows_free(context);
#elif defined(__linux__)
  linux_free(context);
#else
#error "Unsupported platform"
#endif
}

Clay_Dimensions tui_get_dimensions(TUI_Context *context) {
#ifdef _WIN32
  return windows_get_dimensions(context);
#elif defined(__linux__)
  return linux_get_dimensions(context);
#else
#error "Unsupported platform"
#endif
}

TUI_Result tui_swap_buffers(TUI_Context *context) {
#ifdef _WIN32
  return windows_swap_buffers(context);
#elif defined(__linux__)
  return linux_swap_buffers(context);
#else
#error "Unsupported platform"
#endif
}
TUI_Result tui_get_event(TUI_Context *context, TUI_Event *event) {
#ifdef _WIN32
  return windows_get_event(context, event);
#elif defined(__linux__)
  return linux_get_event(context, event);
#else
#error "Unsupported Platform"
#endif
}

// draw a string of cells.
TUI_Result tui_draw_string(TUI_Context *context, int x, int y, const char *str,
                           uint8_t fg, uint8_t bg) {
  if (!context || !str) {
    return TUI_ERROR_INIT_FAILED;
  }

  Clay_Dimensions dimensions = tui_get_dimensions(context);
  int len = strlen(str);

  for (int i = 0; i < len; i++) {
    if (x + i >= dimensions.width) {
      break; // prevent drawing out of bounds.
    }
    TUI_Result result =
        tui_draw_cell(context, x + i, y, (TUI_Cell){str[i], fg, bg});
    if (result != TUI_SUCCESS) {
      return result; // Propagate the error
    }
  }
  return TUI_SUCCESS;
}
bool tui_running(TUI_Context *context) {
#ifdef _WIN32
  return windows_running(context);
#elif defined(__linux__)
  return linux_running(context);
#else
#error "Unsupported Platform"
#endif
}
#endif // HAWKTUI_IMPLEMENTATION

#endif // HAWKTUI_H
