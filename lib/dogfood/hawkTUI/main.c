#define CLAY_IMPLEMENTATION
#include "clay.h"
#include <stdbool.h>
#include <stdio.h>
#include <windows.h>

// --- Console and Buffer Handling ---

// Get the dimensions of the current console window in characters.
Clay_Dimensions GetConsoleDimensions() {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  return (Clay_Dimensions){
      (float)(csbi.srWindow.Right - csbi.srWindow.Left + 1),
      (float)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1)};
}

// Create a double buffer for flicker-free rendering.
typedef struct {
  CHAR_INFO *buffer;
  HANDLE consoleHandle;
  SMALL_RECT drawRect;
} ConsoleBuffer;

ConsoleBuffer CreateConsoleBuffer(Clay_Dimensions dimensions) {
  ConsoleBuffer cb;
  cb.consoleHandle = CreateConsoleScreenBuffer(
      GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
  SetConsoleScreenBufferSize(
      cb.consoleHandle,
      (COORD){(short)dimensions.width, (short)dimensions.height});

  cb.buffer = (CHAR_INFO *)malloc(sizeof(CHAR_INFO) *
                                  (int)(dimensions.width * dimensions.height));
  cb.drawRect = (SMALL_RECT){0, 0, (short)(dimensions.width - 1),
                             (short)(dimensions.height - 1)};
  return cb;
}

void FreeConsoleBuffer(ConsoleBuffer *cb) {
  CloseHandle(cb->consoleHandle);
  free(cb->buffer);
}
// Render Clay's output to the console buffer.
void RenderToConsoleBuffer(Clay_RenderCommandArray renderCommands,
                           ConsoleBuffer *cb, Clay_Dimensions dimensions) {
  // Clear the buffer (fill with spaces and default attributes).
  for (int i = 0; i < dimensions.width * dimensions.height; ++i) {
    cb->buffer[i].Char.AsciiChar = ' ';
    cb->buffer[i].Attributes =
        7; // White on Black is a common default.  7 = FOREGROUND_BLUE |
           // FOREGROUND_GREEN | FOREGROUND_RED
  }

  // We aren't actually rendering anything in the hello world, so no need to
  // loop through render commands.

  // --- Double Buffering (Write to Console) ---

  WriteConsoleOutput(cb->consoleHandle, cb->buffer,
                     (COORD){(short)dimensions.width, (short)dimensions.height},
                     (COORD){0, 0}, &cb->drawRect);

  SetConsoleActiveScreenBuffer(cb->consoleHandle);
}

// --- Clay Measurement Function (Required) ---

Clay_Dimensions MeasureText(Clay_StringSlice text,
                            Clay_TextElementConfig *config, void *userData) {
  // Super-basic text measurement (assumes monospaced font, and a consistent
  // size.) This is NOT production-ready; it's just for the demo.  A real
  // implementation would need to use GetTextExtentPoint32 or similar, and
  // properly handle fonts.

  // Assume a character width of 1 and height of 1 (in console character cells)
  // Note: This is for layout only, not drawing.
  return (Clay_Dimensions){(float)text.length, 1.0f};
}

// --- Main ---

int main() {
  // 1. Initialize Clay.
  Clay_Dimensions consoleDimensions = GetConsoleDimensions();
  uint32_t minMemorySize = Clay_MinMemorySize();
  void *clayMemory = malloc(minMemorySize);
  Clay_Arena arena =
      Clay_CreateArenaWithCapacityAndMemory(minMemorySize, clayMemory);
  Clay_ErrorHandler errorHandler = {.errorHandlerFunction = NULL,
                                    .userData = NULL};
  Clay_Context *context =
      Clay_Initialize(arena, consoleDimensions, errorHandler);

  if (!context) {
    fprintf(stderr, "Clay initialization failed!\n");
    free(clayMemory);
    return 1;
  }

  Clay_SetMeasureTextFunction(MeasureText, NULL);

  // 2. Create Console Buffer.
  ConsoleBuffer consoleBuffer = CreateConsoleBuffer(consoleDimensions);

  // 3. Main Loop (single iteration for this example).
  // In a real application, this would be a loop that handles input, updates the
  // UI, and renders.
  // for(;;) { // Example of how a render loop might start.

  // --- Clay Layout ---
  Clay_BeginLayout();
  // Create a single root element that fills the entire console.
  CLAY({.layout = {
            .sizing = {CLAY_SIZING_GROW(1, 1), CLAY_SIZING_GROW(1, 1)}}}) {}
  Clay_RenderCommandArray renderCommands = Clay_EndLayout();

  // --- Rendering ---
  RenderToConsoleBuffer(renderCommands, &consoleBuffer, consoleDimensions);

  // --- Input Handling (Minimal Example - Wait for a keypress) ---
  printf("Press any key to exit...\n");

  // Use ReadConsoleInput to wait for a key press.
  INPUT_RECORD inputRecord;
  DWORD eventsRead;
  HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);

  // Enable KEY_EVENT
  DWORD mode;
  GetConsoleMode(hInput, &mode);
  SetConsoleMode(
      hInput,
      mode | ENABLE_PROCESSED_INPUT); // Ensure processed input is enabled

  while (true) {
    ReadConsoleInput(hInput, &inputRecord, 1, &eventsRead);
    if (inputRecord.EventType == KEY_EVENT &&
        inputRecord.Event.KeyEvent.bKeyDown) {
      break; // Exit loop on any key press
    }
  }
  //  if (_kbhit()) break; // example of how loop may exit
  //  Sleep(16); // example of crude "framerate"
  //}

  // 4. Cleanup.
  FreeConsoleBuffer(&consoleBuffer);
  free(clayMemory);

  return 0;
}
