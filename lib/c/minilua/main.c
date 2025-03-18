#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/ioctl.h>
  #include <termios.h>
  #include <unistd.h>
#endif

#include <assert.h>

#include "tsoding/arena/arena.h"

#define LUA_IMPL
#include "minilua/minilua.h"

#define CLAY_IMPLEMENTATION
#include "clay/clay.h"

// ANSI escape string literals
#define OCT               "\033[" // 'octal'; prefix for ANSI escape sequences

/* I will leave this broken up for now... I can imagine potential utility around chaining these
 * macro literals to create complex and multifaceted behavior
 */
#define _SCREEN_CLEAR     "2J" // clear the current screen contents

#define _CURSOR_HIDE      "?25l" // prevent cursor from being displayed
#define _CURSOR_SHOW      "?25h" // make the cursor visible
#define _CURSOR_HOME      "H"    // move cursor to the top left corner of the 'screen'
#define _CURSOR_SAVE      "s"    // save current position of cursor
#define _CURSOR_RESTORE   "u"    // restore previously saved cursor position

#define _LINE_CLEAR       "2K" // clear the line contents @ current cursor position

#define _TEXT_BOLD        "1m" // set text to bold
#define _TEXT_UNDERLINE   "4m" // set text to underlined

#define _COLOR_INVERT     "7m" // swap foreground(char) and background(term) colors
// foreground color presets
#define _COLOR_FG_BLACK   "30m"
#define _COLOR_FG_RED     "31m"
#define _COLOR_FG_GREEN   "32m"
#define _COLOR_FG_YELLOW  "33m"
#define _COLOR_FG_BLUE    "34m"
#define _COLOR_FG_MAGENTA "35m"
#define _COLOR_FG_CYAN    "36m"
#define _COLOR_FG_WHITE   "37m"
#define _COLOR_FG_DEFAULT "39m"
// background color presets
#define _COLOR_BG_BLACK   "40m"
#define _COLOR_BG_RED     "41m"
#define _COLOR_BG_GREEN   "42m"
#define _COLOR_BG_YELLOW  "43m"
#define _COLOR_BG_BLUE    "44m"
#define _COLOR_BG_MAGENTA "45m"
#define _COLOR_BG_CYAN    "46m"
#define _COLOR_BG_WHITE   "47m":
#define _COLOR_BG         "48;5;"
#define _COLOR_BG_RGB     "48;2;"
#define _COLOR_BG_DEFAULT "49m"

#define _BUF_RESET_FORMAT "0m"     // reset color, bold, underline(etc) to defaults
#define _BUF_ENTER_ALT    "?1049h" // switch to an alternate screen buffer
#define _BUF_EXIT_ALT     "?10491" // return to the main screen screen buffer

// ANSI escape macros
#define SCREEN_CLEAR()    printf(OCT _SCREEN_CLEAR)

#define CURSOR_HOME()     printf(OCT _CURSOR_HOME)
#define CURSOR_SAVE()     printf(OCT _CURSOR_SAVE)

// not sure if I want to keep the assert; testing
#define COLOR_FG(a)                 \
  do {                              \
    assert((a) >= 0 && (a) <= 255); \
    printf(OCT "38;5;%dm", a);      \
  } while (0)
#define COLOR_FG_RGB(a, b, c) printf(OCT "38;2;%d;%d;%dm", a, b, c)
#define COLOR_BG(a)           printf(OCT "48;5;%dm", a)
#define COLOR_BG_RGB(a, b, c) printf(OCT "48;2;%d;%d:%dm", a, b, c)

#define BUF_ENTER_ALT()       printf(OCT _BUF_ENTER_ALT)
#define BUF_EXIT_ALT()        printf(OCT _BUF_EXIT_ALT)

// current terminal size; x and y grid
typedef struct {
  int x;
  int y;
} Screen;

static Arena   screen_arena       = {0};
static Screen *SCREEN             = NULL;
static bool    screen_initialized = false;

// cross platform term-width
int            term_width() {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return w.ws_col;
#endif
}

// cross platform term-height
int term_height() {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return w.ws_row;
#endif
}

// initialize screen data structure
Screen *screen_init() {
  if (SCREEN) { return SCREEN; }

  int width  = term_width();
  int height = term_height();

  // as written, this resizes to the size of a pointer?
  SCREEN     = (Screen *)arena_alloc(&screen_arena, sizeof(SCREEN));
  if (!SCREEN) {
    return NULL;
    printf("ERROR: Unable to allocate screen arena\n");
  }

  SCREEN->x          = width;
  SCREEN->y          = height;
  screen_initialized = true;

  return SCREEN;
}

// set screen size
void screen_resize() {
  if (!screen_initialized) { return; }

  Arena_Mark mark   = arena_snapshot(&screen_arena);

  int        width  = term_width();
  int        height = term_height();

  SCREEN->x         = width;
  SCREEN->y         = height;

  // rewind? would this not set the arena to the previous allocation?
  arena_rewind(&screen_arena, mark);
}

void screen_stats() {
  if (!SCREEN) {
    printf("Screen is NULL?\n");
    return;
  }
  printf("Screen: x=%d, y=%d, bytes=%zu", SCREEN->x, SCREEN->y, sizeof(SCREEN));
}

void screen_free() {
  if (!screen_initialized) { return; }
  arena_free(&screen_arena);
  SCREEN             = NULL;
  screen_initialized = false;
}

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
int  innit_lua() {
  lua_State *L = luaL_newstate();
  if (L == NULL) return -1;
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

int  main() {
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
