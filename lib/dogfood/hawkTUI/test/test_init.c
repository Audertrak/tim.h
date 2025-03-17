// test/test_init.c - Test the tui_init and tui_free functions
#define HAWKTUI_IMPLEMENTATION // IMPORTANT
#include "../src/hawkTUI.h"    // Include the TUI header
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// basic test function.
int run_test(Clay_Dimensions dimensions) {
  // Test tui_init
  TUI_Context *context = NULL;
  TUI_Result result = tui_init(&context, dimensions);
  if (result != TUI_SUCCESS) {
    fprintf(stderr, "tui_init failed with error code: %d\n", result);
    return 1; // Indicate failure
  }
  if (!context) {
    fprintf(stderr, "Context is null\n");
    return 1;
  }

  // basic clear buffer test
  if (tui_clear(context) != TUI_SUCCESS) {
    fprintf(stderr, "tui_clear failed\n");
    return 1;
  }
  assert(context->buffer[0].ch == ' '); // check to ensure that it is empty

  // draw string test
  if (tui_draw_string(context, 0, 0, "Hello", 7, 0) != TUI_SUCCESS) {
    fprintf(stderr, "tui_draw_string test failed\n");
  }
  assert(context->buffer[0].ch == 'H');
  assert(context->buffer[1].ch == 'e');
  assert(context->buffer[2].ch == 'l');
  assert(context->buffer[3].ch == 'l');
  assert(context->buffer[4].ch == 'o');

  // draw string out of bounds test
  if (tui_draw_string(context, dimensions.width - 2, 0, "Hello", 7, 0) ==
      TUI_SUCCESS) {
    assert(context->buffer[(int)dimensions.width - 2].ch ==
           'H'); // assert first two chars exist
    assert(context->buffer[(int)dimensions.width - 1].ch == 'e');
    assert(context->buffer[(int)dimensions.width].ch !=
           'l'); // ensure that it does not write out of bounds
  }

  // draw string to second line
  if (tui_draw_string(context, 0, 1, "World", 7, 0) == TUI_SUCCESS) {
    assert(context->buffer[(int)dimensions.width].ch == 'W');
    assert(context->buffer[(int)dimensions.width + 1].ch == 'o');
    assert(context->buffer[(int)dimensions.width + 2].ch == 'r');
    assert(context->buffer[(int)dimensions.width + 3].ch == 'l');
    assert(context->buffer[(int)dimensions.width + 4].ch == 'd');
  }

  // draw single char test.
  if (tui_draw_cell(context, 0, 5, (TUI_Cell){'!', 7, 0}) != TUI_SUCCESS) {
    fprintf(stderr, "tui_draw_cell test failed.\n");
    return 1;
  }
  assert(context->buffer[5 * (int)dimensions.width].ch == '!');

  // Test tui_free
  tui_free(context);
  printf("All tests passed!\n");
  return 0;
}

// main
int main() {
  // test small window
  int small_result = run_test((Clay_Dimensions){80, 24});
  int large_result = run_test((Clay_Dimensions){500, 500});
  return small_result || large_result;
}
