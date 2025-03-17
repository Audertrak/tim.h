// #if WIN32
// #include <windows.h>
// #elif POSIX
// #include <termios.h>
// #endif

#include <windows.h>

#define LUA_IMPL
#include "minilua/minilua.h"

#define CLAY_IMPLEMENTATION
#include "clay/clay.h"

// ANSI escape string literals
#define OCT "\033[" // 'octal'; prefix for ANSI escape sequences

#define _SCREEN_CLEAR "2J" // clear the current screen contents

#define _CURSOR_HOME "H" // move cursor to the top left corner of the 'screen'
#define _CURSOR_SAVE "s" // save current position of cursor (to restore later)

#define _LINE_CLEAR "2K" // clear the line contents @ current cursor position

#define _BUF_ENTER_ALT "?1049h" // switch to an alternate screen buffer
#define _BUF_EXIT_ALT "?10491"  // return to the main screen screen buffer

// ANSI escape macros
#define SCREEN_CLEAR() printf(OCT _SCREEN_CLEAR)

#define CURSOR_HOME() printf(OCT _CURSOR_HOME)
#define CURSOR_SAVE() printf(OCT _CURSOR_SAVE)

#define BUF_ENTER_ALT() printf(OCT _BUF_ENTER_ALT)
#define BUF_EXIT_ALT() printf(OCT _BUF_EXIT_ALT)

int innit() {
  // get the 'handle' to stdout (the console)
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "ERROR: cannot get the standard output handle\n");
    return 1;
  }

  // retrieve the current 'console mode'
  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode)) {
    fprintf(stderr, "ERROR: cannot get the console mode\n");
    return 1;
  }

  // enable virtual terminal processing
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(hOut, dwMode)) {
    fprintf(stderr, "ERROR: cannot change to ANSI mode\n");
    return 1;
  }

  // use ANSI escape sequences to clear the screen
  SCREEN_CLEAR();
  CURSOR_HOME();
  printf("ANSI escape codes enabled\n");

  // enter alternate screen buffer
  BUF_ENTER_ALT();

  return 0;
}

// create an empty buffer
void buf();

// contain contents of a window
void cont(void *buf);

// create and open a 'window'
void win(void *pane);

// split the current window into two panes
void split(void *buf, int orientation);

// draw 'floating' window
void _float(void *buf);

// create tab
void tab(void *buf);

// resize container
void resize(int direction, float size);

// TODO figure out how to return lua_State
// initialize lua
int innit_lua() {
  lua_State *L = luaL_newstate();
  if (L == NULL)
    return -1;
  luaL_openlibs(L);
  luaL_loadstring(L, "Pick a number");
  lua_call(L, 0, 0);
  lua_close(L);
  return 0;
}

// TODO: exit lua
int exit_lua() {
  // lua_close(L);
  return 0;
}

// lua print statement for shits and giggles
// void lprint(char *str) {}

// exit
void exitui() { BUF_EXIT_ALT(); }

int main() {
  int choice;

  innit();

  // innit_lua();
  printf("Hello, world!\n");
  scanf("%d", &choice);

  if (choice != 1) {
    printf("Hello, world!\n");
    scanf("%d", &choice);
  }

  // exit_lua(*L);

  exitui();

  return 0;
}
