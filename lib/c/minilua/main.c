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
  printf("\033[2J");
  printf("ANSI escape codes enabled\n");

  // enter alternate screen buffer
  printf("\033[?1049h");

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
void exitui() { printf("\033[?1049l"); }

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
