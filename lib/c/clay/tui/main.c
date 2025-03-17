// hello_clay_tui.c

// VERY IMPORTANT: Define CLAY_IMPLEMENTATION *before* including clay.h in
// *exactly one* C file.
#define CLAY_IMPLEMENTATION
#include "main.h" // Include the renderer header
#include <stdio.h>
#include <stdlib.h>
#include <windows.h> // For input handling

void Clay_Example_ErrorHandler(Clay_ErrorData errorData) {
    fprintf(stderr, "Clay Error: %s\n", errorData.errorText.chars);
    exit(1);
}

int main() {
    // Initialize the terminal renderer
    Clay_Term_Initialize();

    // Get initial window dimensions from renderer
    Clay_Dimensions windowDims = Clay_Term_GetDimensions();

    // Clay initialization
    uint32_t minMemory = Clay_MinMemorySize();
    void *memory = malloc(minMemory);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(minMemory, memory);

    Clay_Context *context = Clay_Initialize(arena, windowDims,(Clay_ErrorHandler) {Clay_Example_ErrorHandler, NULL} );
     if(!context)
    {
         fprintf(stderr, "Failed to initialize Clay\n");
        free(memory);
        return 1;
    }

    // Set the text measurement function
    Clay_SetMeasureTextFunction(Clay_Term_MeasureText, NULL);

    bool running = true;
    Clay_Vector2 lastMousePos = {0.0f, 0.0f}; // Store last known mouse position

    while (running) {
        // Clear the terminal
        Clay_Term_Clear();


         // Handle input
        INPUT_RECORD inputBuffer[128];
        DWORD numEventsRead;

        // Check for console events without blocking
        if (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), inputBuffer, 128, &numEventsRead) && numEventsRead > 0)
        {
            ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), inputBuffer, 128, &numEventsRead);

            for (DWORD i = 0; i < numEventsRead; i++) {
                if (inputBuffer[i].EventType == KEY_EVENT) {
                    if (inputBuffer[i].Event.KeyEvent.bKeyDown) { // Key pressed
                        if (inputBuffer[i].Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) {
                             running = false; // Exit on Esc key press
                        }
                    }
                }
                else if (inputBuffer[i].EventType == MOUSE_EVENT) {
                    // Map console coordinates to Clay coordinates.
                    lastMousePos.x = (float)inputBuffer[i].Event.MouseEvent.dwMousePosition.X;
                    lastMousePos.y = (float)inputBuffer[i].Event.MouseEvent.dwMousePosition.Y;

                    bool leftButtonPressed = (inputBuffer[i].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
                    Clay_SetPointerState(lastMousePos, leftButtonPressed);
                }
            }
        }
         else
        {
            // Use last known mouse position and set button state to false
             Clay_SetPointerState(lastMousePos, false);
        }



        // Clay layout
        Clay_BeginLayout();

        CLAY({.backgroundColor = {0, 0, 0, 255}}) { // Root: Black background
            CLAY({.layout = {.padding = {2, 2, 1, 1}}, .backgroundColor = {64, 64, 64, 255}}) { // Dark Gray
                CLAY_TEXT(CLAY_STRING("Hello, Clay TUI!"), CLAY_TEXT_CONFIG({.textColor = {255, 255, 0, 255}})); // Yellow text
            }
             CLAY({ .layout = { .padding = { 2, 2, 0, 0 } }, .backgroundColor = { 192, 0, 0, 255 } }) // Red
            {
                CLAY_TEXT(CLAY_STRING("Press ESC to quit."), CLAY_TEXT_CONFIG({ .textColor = { 255, 255, 255, 255 } })); // White Text
            }
            CLAY({.layout = { .padding = { 2, 5, 0, 0 } },  .border = {.color = {0, 255, 255, 255}, .width = {.top = 1, .right = 2, .left = 1, .bottom = 2} }  }) { // Cyan border
                 CLAY_TEXT(CLAY_STRING("This is inside a border."), CLAY_TEXT_CONFIG({ .textColor = { 255, 0, 255, 255 } }));// Magenta text
            }
            CLAY({.layout = {.padding = {2, 2, 1, 1}}, .backgroundColor = { 0, 255, 0, 255}}) {
                CLAY_TEXT(CLAY_STRING("Hover over this text"), CLAY_TEXT_CONFIG({.textColor = {0, 0, 0, 255}})); // Black Text
            }

             if (Clay_Hovered()) {
                CLAY({.layout = {.padding = {2, 2, 1, 1}}, .backgroundColor = { 255, 0, 255, 128}}) {  // Semi-transparent Magenta
                     CLAY_TEXT(CLAY_STRING("Hovering!"), CLAY_TEXT_CONFIG({.textColor = {255, 255, 255, 255}})); // White text
                }
             }

        }

        Clay_RenderCommandArray commands = Clay_EndLayout();

        // Render
        for (int i = 0; i < commands.length; i++) {
            Clay_RenderCommand *command = Clay_RenderCommandArray_Get(&commands, i);
            switch (command->commandType) {
                case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
                    Clay_Term_DrawRectangle(command->boundingBox, command->renderData.rectangle.backgroundColor);
                    break;
                case CLAY_RENDER_COMMAND_TYPE_TEXT:
                    Clay_Term_DrawText(command->boundingBox, command->renderData.text.stringContents, command->renderData.text.textColor);
                    break;
                case CLAY_RENDER_COMMAND_TYPE_BORDER:
                    Clay_Term_DrawBorder(command->boundingBox, command->renderData.border);
                    break;
                default: break;
            }
        }

        Clay_Term_Present();
    }

    Clay_Term_Shutdown();
    free(memory);
    return 0;
}
