// clay_term_renderer.h
#ifndef CLAY_TERM_RENDERER_H
#define CLAY_TERM_RENDERER_H

#include "C:/.lib/clay/clay.h"
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>   // For wide character support
#include <windows.h> // Needed for HANDLE, COORD, etc.

// --- API Functions ---

// Initializes the terminal renderer.  Must be called before any other
// renderer functions.
void Clay_Term_Initialize();

// Shuts down the terminal renderer and restores console settings.
void Clay_Term_Shutdown();

// Clears the terminal screen.
void Clay_Term_Clear();

// "Presents" the rendered frame (updates the console display).
void Clay_Term_Present();

// Text measurement function (to be passed to Clay_SetMeasureTextFunction)
// uhh what?
Clay_Dimensions Clay_Term_MeasureText(Clay_StringSlice text,
                                      Clay_TextElementConfig *config,
                                      void *userData);

// Get the terminal window dimensions.
Clay_Dimensions Clay_Term_GetDimensions();

// --- Internal Rendering functions, made public for Clay's use. ---
void Clay_Term_DrawRectangle(Clay_BoundingBox rect, Clay_Color color);
void Clay_Term_DrawText(Clay_BoundingBox rect, Clay_StringSlice text,
                        Clay_Color color);
void Clay_Term_DrawBorder(Clay_BoundingBox rect,
                          Clay_BorderRenderData borderData);

// --- Terminal Renderer Implementation (INSIDE the header guard) ---

typedef struct {
  HANDLE consoleOutput;
  CONSOLE_SCREEN_BUFFER_INFOEX consoleInfo;
  CHAR_INFO *screenBuffer;
  SMALL_RECT renderRegion;
  COORD bufferSize;
  COORD bufferCoord;
} Clay_TermRenderer;

static Clay_TermRenderer Clay__Term_Renderer; // Internal state

void Clay_Term_Initialize() {
  Clay__Term_Renderer.consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  if (Clay__Term_Renderer.consoleOutput == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Failed to get console output handle.\n");
    exit(1);
  }

  // Enable UTF-8 output
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8); // Set input

  // Retrieve current console information
  Clay__Term_Renderer.consoleInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
  if (!GetConsoleScreenBufferInfoEx(Clay__Term_Renderer.consoleOutput,
                                    &Clay__Term_Renderer.consoleInfo)) {
    fprintf(stderr, "GetConsoleScreenBufferInfoEx failed. Error: %lu\n",
            GetLastError());
    exit(1);
  }

  Clay__Term_Renderer.bufferSize.X = Clay__Term_Renderer.consoleInfo.dwSize.X;
  Clay__Term_Renderer.bufferSize.Y = Clay__Term_Renderer.consoleInfo.dwSize.Y;
  Clay__Term_Renderer.renderRegion =
      Clay__Term_Renderer.consoleInfo.srWindow; // Use full window initially

  // Allocate the screen buffer
  Clay__Term_Renderer.screenBuffer =
      (CHAR_INFO *)malloc(Clay__Term_Renderer.bufferSize.X *
                          Clay__Term_Renderer.bufferSize.Y * sizeof(CHAR_INFO));
  if (Clay__Term_Renderer.screenBuffer == NULL) {
    fprintf(stderr, "Failed to allocate screen buffer.\n");
    exit(1);
  }

  // Clear the buffer (initialize with spaces)
  for (int i = 0;
       i < Clay__Term_Renderer.bufferSize.X * Clay__Term_Renderer.bufferSize.Y;
       ++i) {
    Clay__Term_Renderer.screenBuffer[i].Char.UnicodeChar = L' ';
    Clay__Term_Renderer.screenBuffer[i].Attributes =
        0x00; // Black background, Black foreground
  }
}

void Clay_Term_Shutdown() {
  free(Clay__Term_Renderer.screenBuffer);
  // Restore original console settings (optional, but good practice)
  if (!SetConsoleScreenBufferInfoEx(Clay__Term_Renderer.consoleOutput,
                                    &Clay__Term_Renderer.consoleInfo)) {
    fprintf(stderr,
            "SetConsoleScreenBufferInfoEx failed during shutdown. Error: %lu\n",
            GetLastError());
    // This is not a fatal error. We're exiting.
  }
}

void Clay_Term_Clear() {
  for (int i = 0;
       i < Clay__Term_Renderer.bufferSize.X * Clay__Term_Renderer.bufferSize.Y;
       ++i) {
    Clay__Term_Renderer.screenBuffer[i].Char.UnicodeChar = L' ';
    Clay__Term_Renderer.screenBuffer[i].Attributes = 0x00;
  }
}

WORD Clay_Term_ColorToAttribute(Clay_Color color) {
  WORD attribute = 0;

  // Handle transparency. If alpha is low, don't set any color bits.
  if (color.a < 64) {
    return attribute; // Return 0 (default attributes - usually black/white)
  }

  // Foreground colors
  if (color.r > 127)
    attribute |= FOREGROUND_RED;
  if (color.g > 127)
    attribute |= FOREGROUND_GREEN;
  if (color.b > 127)
    attribute |= FOREGROUND_BLUE;

  // Calculate a simple "brightness" for intensity.
  // This is a better heuristic than just checking if any channel is > 192.
  int brightness = (int)color.r + (int)color.g + (int)color.b;
  if (brightness > 384) { // (255 * 3) / 2 = 382.5.  Use a threshold.
    attribute |= FOREGROUND_INTENSITY;
  }

  // Background colors:  Use a *lower* threshold than for foreground,
  // as background colors tend to be too bright.  Also, only set
  // background if the color is reasonably opaque (a > 192).
  if (color.a > 192) {
    if (color.r > 63)
      attribute |= BACKGROUND_RED;
    if (color.g > 63)
      attribute |= BACKGROUND_GREEN;
    if (color.b > 63)
      attribute |= BACKGROUND_BLUE;

    // Background intensity:  Make it even harder to trigger.
    if (brightness > 512) { // Higher threshold than foreground.
      attribute |= BACKGROUND_INTENSITY;
    }
  }

  return attribute;
}

void Clay_Term_DrawRectangle(Clay_BoundingBox rect, Clay_Color color) {
  WORD attribute = Clay_Term_ColorToAttribute(color);

  // Clip rectangle to rendering area
  int startX = (int)max(0, rect.x);
  int startY = (int)max(0, rect.y);
  int endX = (int)min(Clay__Term_Renderer.bufferSize.X, rect.x + rect.width);
  int endY = (int)min(Clay__Term_Renderer.bufferSize.Y, rect.y + rect.height);

  for (int y = startY; y < endY; y++) {
    for (int x = startX; x < endX; x++) {
      int index = y * Clay__Term_Renderer.bufferSize.X + x;
      Clay__Term_Renderer.screenBuffer[index].Char.UnicodeChar =
          L' '; // Use space for solid fill
      Clay__Term_Renderer.screenBuffer[index].Attributes = attribute;
    }
  }
}

void Clay_Term_DrawText(Clay_BoundingBox rect, Clay_StringSlice text,
                        Clay_Color color) {
  WORD attribute = Clay_Term_ColorToAttribute(color);

  int startX = (int)max(0, rect.x);
  int startY = (int)max(0, rect.y);
  int endX = (int)min(Clay__Term_Renderer.bufferSize.X, rect.x + rect.width);
  int endY = (int)min(Clay__Term_Renderer.bufferSize.Y, rect.y + rect.height);

  int textIndex = 0;
  for (int y = startY; y < endY && textIndex < text.length; y++) {
    for (int x = startX; x < endX && textIndex < text.length;
         x++, textIndex++) {
      int index = y * Clay__Term_Renderer.bufferSize.X + x;
      // Convert char to WCHAR (wide character). This assumes your strings
      // are either ASCII or UTF-8 (which is what Clay uses).
      wchar_t wideChar;

      // Simple check for ASCII/UTF-8 (handles common case efficiently)
      if ((text.chars[textIndex] & 0x80) ==
          0) { // Check if the highest bit is 0 (ASCII)
        wideChar = (wchar_t)text.chars[textIndex];
      } else {
        // Multi-byte UTF-8 character, need MultiByteToWideChar
        int result = MultiByteToWideChar(CP_UTF8, 0, &text.chars[textIndex],
                                         text.length - textIndex, &wideChar, 1);
        if (result == 0) {
          wideChar = L'?'; // Invalid character, replace with ?

        } else {
          int charSize = MultiByteToWideChar(CP_UTF8, 0, &text.chars[textIndex],
                                             text.length - textIndex, NULL, 0);
          if (charSize > 0) {
            textIndex += charSize - 1;
          }
        }
      }

      Clay__Term_Renderer.screenBuffer[index].Char.UnicodeChar = wideChar;
      Clay__Term_Renderer.screenBuffer[index].Attributes = attribute;
    }
  }
}

void Clay_Term_DrawBorder(Clay_BoundingBox rect,
                          Clay_BorderRenderData borderData) {
  WORD attribute = Clay_Term_ColorToAttribute(borderData.color);

  // Clip borders to the rendering area.
  int startX = (int)max(0, rect.x);
  int startY = (int)max(0, rect.y);
  int endX = (int)min(Clay__Term_Renderer.bufferSize.X, rect.x + rect.width);
  int endY = (int)min(Clay__Term_Renderer.bufferSize.Y, rect.y + rect.height);

  // Top border
  if (borderData.width.top > 0) {
    for (int x = startX; x < endX; x++) {
      int index = startY * Clay__Term_Renderer.bufferSize.X + x;
      Clay__Term_Renderer.screenBuffer[index].Char.UnicodeChar =
          L'─'; // Horizontal line
      Clay__Term_Renderer.screenBuffer[index].Attributes = attribute;
    }
  }

  // Bottom border
  if (borderData.width.bottom > 0) {
    for (int x = startX; x < endX; x++) {
      int index = (endY - 1) * Clay__Term_Renderer.bufferSize.X +
                  x; // endY-1 since endY is exclusive
      Clay__Term_Renderer.screenBuffer[index].Char.UnicodeChar =
          L'─'; // Horizontal line
      Clay__Term_Renderer.screenBuffer[index].Attributes = attribute;
    }
  }
  // Left border
  if (borderData.width.left > 0) {
    for (int y = startY; y < endY; y++) {
      int index = y * Clay__Term_Renderer.bufferSize.X + startX;
      Clay__Term_Renderer.screenBuffer[index].Char.UnicodeChar =
          L'│'; // Vertical line
      Clay__Term_Renderer.screenBuffer[index].Attributes = attribute;
    }
  }
  // Right border
  if (borderData.width.right > 0) {
    for (int y = startY; y < endY; y++) {
      int index = y * Clay__Term_Renderer.bufferSize.X +
                  (endX - 1); // endX-1 since endX is exclusive
      Clay__Term_Renderer.screenBuffer[index].Char.UnicodeChar =
          L'│'; // Vertical line
      Clay__Term_Renderer.screenBuffer[index].Attributes = attribute;
    }
  }

  // Corners (if multiple sides are drawn, corners are drawn)
  if (borderData.width.top > 0 && borderData.width.left > 0) {
    Clay__Term_Renderer
        .screenBuffer[startY * Clay__Term_Renderer.bufferSize.X + startX]
        .Char.UnicodeChar = L'┌'; // Top-left
    Clay__Term_Renderer
        .screenBuffer[startY * Clay__Term_Renderer.bufferSize.X + startX]
        .Attributes = attribute;
  }
  if (borderData.width.top > 0 && borderData.width.right > 0) {
    Clay__Term_Renderer
        .screenBuffer[startY * Clay__Term_Renderer.bufferSize.X + (endX - 1)]
        .Char.UnicodeChar = L'┐'; // Top-right
    Clay__Term_Renderer
        .screenBuffer[startY * Clay__Term_Renderer.bufferSize.X + (endX - 1)]
        .Attributes = attribute;
  }
  if (borderData.width.bottom > 0 && borderData.width.left > 0) {
    Clay__Term_Renderer
        .screenBuffer[(endY - 1) * Clay__Term_Renderer.bufferSize.X + startX]
        .Char.UnicodeChar = L'└'; // Bottom-left
    Clay__Term_Renderer
        .screenBuffer[(endY - 1) * Clay__Term_Renderer.bufferSize.X + startX]
        .Attributes = attribute;
  }
  if (borderData.width.bottom > 0 && borderData.width.right > 0) {
    Clay__Term_Renderer
        .screenBuffer[(endY - 1) * Clay__Term_Renderer.bufferSize.X +
                      (endX - 1)]
        .Char.UnicodeChar = L'┘'; // Bottom-right
    Clay__Term_Renderer
        .screenBuffer[(endY - 1) * Clay__Term_Renderer.bufferSize.X +
                      (endX - 1)]
        .Attributes = attribute;
  }
}

void Clay_Term_Present() {
  Clay__Term_Renderer.bufferCoord.X = 0;
  Clay__Term_Renderer.bufferCoord.Y = 0;
  if (!WriteConsoleOutputW(
          Clay__Term_Renderer.consoleOutput, Clay__Term_Renderer.screenBuffer,
          Clay__Term_Renderer.bufferSize, Clay__Term_Renderer.bufferCoord,
          &Clay__Term_Renderer.renderRegion)) {
    fprintf(stderr, "WriteConsoleOutput failed - Error: %lu\n", GetLastError());
    exit(1);
  }
}

Clay_Dimensions Clay_Term_MeasureText(Clay_StringSlice text,
                                      Clay_TextElementConfig *config,
                                      void *userData) {
  (void)config;   // Suppress unused parameter warning
  (void)userData; // Suppress unused parameter warning
  Clay_Dimensions dimensions = {(float)text.length, 1.0f};

  // Count newlines for basic height calculation.  Still 1 char high per line.
  for (int i = 0; i < text.length; i++) {
    if (text.chars[i] == '\n') {
      dimensions.height += 1;
    }
  }

  return dimensions;
}

Clay_Dimensions Clay_Term_GetDimensions() {
  return (Clay_Dimensions){(float)Clay__Term_Renderer.bufferSize.X,
                           (float)Clay__Term_Renderer.bufferSize.Y};
}

#endif // CLAY_TERM_RENDERER_H
