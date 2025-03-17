#ifndef WINBOX_H_INCL
#define WINBOX_H_INCL

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <windows.h>
#include <stdint.h>
#include <stdio.h>  // For vsnprintf
#include <string.h> //For strlen

#ifdef __cplusplus
extern "C" {
#endif

#define WB_VERSION_STR "0.1.0"

#define WB_KEY_ESC 27
#define WB_KEY_ENTER 13
#define WB_KEY_BACKSPACE 8
#define WB_KEY_TAB 9
#define WB_KEY_ARROW_UP    0xE048
#define WB_KEY_ARROW_DOWN  0xE050
#define WB_KEY_ARROW_LEFT  0xE04B
#define WB_KEY_ARROW_RIGHT 0xE04D
#define WB_KEY_HOME        0xE047
#define WB_KEY_END         0xE04F
#define WB_KEY_INSERT      0xE052
#define WB_KEY_DELETE      0xE053
#define WB_KEY_PGUP        0xE049
#define WB_KEY_PGDN        0xE051
#define WB_KEY_F1          0x3B00
#define WB_KEY_F2          0x3C00
#define WB_KEY_F3          0x3D00
#define WB_KEY_F4          0x3E00
#define WB_KEY_F5          0x3F00
#define WB_KEY_F6          0x4000
#define WB_KEY_F7          0x4100
#define WB_KEY_F8          0x4200
#define WB_KEY_F9          0x4300
#define WB_KEY_F10         0x4400
#define WB_KEY_F11         0x8500
#define WB_KEY_F12         0x8600


#define WB_DEFAULT 0
#define WB_BLACK   0
#define WB_RED     4
#define WB_GREEN   2
#define WB_YELLOW  6
#define WB_BLUE    1
#define WB_MAGENTA 5
#define WB_CYAN    3
#define WB_WHITE   7

#define WB_BOLD        8
#define WB_UNDERLINE   0x8000
#define WB_REVERSE     0x4000

#define WB_EVENT_KEY 1
#define WB_EVENT_RESIZE 2

#define WB_MOD_ALT 1
#define WB_MOD_CTRL 2 // Not accurate, just a placeholder
#define WB_MOD_SHIFT 4


#define WB_OUTPUT_NORMAL 1

#define WB_OK 0
#define WB_ERR -1
#define WB_ERR_MEM -5
#define WB_ERR_NOT_INIT -8
#define WB_ERR_OUT_OF_BOUNDS -9
#define WB_ERR_NO_EVENT -6

typedef uint16_t uintattr_t;

struct wb_cell {
    uint32_t ch;
    uintattr_t fg;
    uintattr_t bg;
};

struct wb_event {
    uint8_t type;
    uint8_t mod;
    uint16_t key;
    uint32_t ch;
    int32_t w;
    int32_t h;
};

int wb_init(void);
int wb_shutdown(void);
int wb_width(void);
int wb_height(void);
int wb_clear(void);
int wb_set_clear_attrs(uintattr_t fg, uintattr_t bg);
int wb_present(void);
int wb_set_cursor(int cx, int cy);
int wb_hide_cursor(void);
int wb_set_cell(int x, int y, uint32_t ch, uintattr_t fg, uintattr_t bg);
int wb_poll_event(struct wb_event *event);
int wb_set_output_mode(int mode);
int wb_printf(int x, int y, uintattr_t fg, uintattr_t bg, const char *fmt, ...);
const char *wb_version(void);

#ifdef __cplusplus
}
#endif

#endif // MINIMAL_WINDOWS_WB_H

#ifdef WB_IMPL

#ifndef WB_PRINTF_BUF
#define WB_PRINTF_BUF 4096
#endif

struct wb_global {
    HANDLE hconin;
    HANDLE hconout;
    int width;
    int height;
    struct wb_cell *back;
    struct wb_cell *front;
    CONSOLE_SCREEN_BUFFER_INFO orig_csbi;
    uintattr_t fg;
    uintattr_t bg;
    int cursor_x;
    int cursor_y;
    int output_mode;
    int initialized;
};

static struct wb_global global = {0};

static int wb_resize_buffers(void);
static WORD wb_attr_to_win(uintattr_t fg, uintattr_t bg);
static int wb_utf8_to_utf16(const char *str, WCHAR *out, int max_out);
static int wb_utf16_to_utf8(const WCHAR* str, char* out, int max_out);
static int wb_utf8_char_length(char c);
static int wb_utf8_char_to_unicode(uint32_t *out, const char *c);
static int wb_utf32_to_utf8(uint32_t c, char *out);

int wb_init(void) {
    if (global.initialized) return WB_OK;

    global.hconin = GetStdHandle(STD_INPUT_HANDLE);
    global.hconout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (global.hconin == INVALID_HANDLE_VALUE || global.hconout == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "GetStdHandle failed: hconin=%p, hconout=%p, error=%lu\n",
                (void*)global.hconin, (void*)global.hconout, GetLastError());
        return WB_ERR;
    }

    DWORD mode;
    GetConsoleMode(global.hconin, &mode);
    // Correct parentheses for bitwise operations:
    SetConsoleMode(global.hconin, (mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT)) | ENABLE_WINDOW_INPUT);
    GetConsoleMode(global.hconout, &mode);
    SetConsoleMode(global.hconout, mode| ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);


    if (!GetConsoleScreenBufferInfo(global.hconout, &global.orig_csbi)) {
        return WB_ERR;
    }
    global.width = global.orig_csbi.dwSize.X;
    global.height = global.orig_csbi.dwSize.Y;

    if (wb_resize_buffers() != WB_OK) return WB_ERR_MEM;


    global.fg = WB_DEFAULT;
    global.bg = WB_DEFAULT;
    global.cursor_x = -1;
    global.cursor_y = -1;
    global.output_mode = WB_OUTPUT_NORMAL;
    global.initialized = 1;

    return WB_OK;
}

int wb_shutdown(void) {
    if (!global.initialized) return WB_OK;

    SetConsoleCursorPosition(global.hconout, global.orig_csbi.dwCursorPosition);
    SetConsoleTextAttribute(global.hconout, global.orig_csbi.wAttributes);

    CONSOLE_CURSOR_INFO cci;
    cci.dwSize = 25;
    cci.bVisible = TRUE;
    SetConsoleCursorInfo(global.hconout, &cci);

    if (global.back) free(global.back);
    if (global.front) free(global.front);

    memset(&global, 0, sizeof(global));
    global.cursor_x = -1;
    global.cursor_y = -1;
    return WB_OK;
}

int wb_width(void) {
    if (!global.initialized) return -1;
    return global.width;
}

int wb_height(void) {
    if (!global.initialized) return -1;
    return global.height;
}

int wb_clear(void) {
    if (!global.initialized) return WB_ERR_NOT_INIT;
    int i;
    for (i = 0; i < global.width * global.height; ++i) {
        global.back[i].ch = ' ';
        global.back[i].fg = global.fg;
        global.back[i].bg = global.bg;
    }
    return WB_OK;
}

int wb_set_clear_attrs(uintattr_t fg, uintattr_t bg) {
    if (!global.initialized) return WB_ERR_NOT_INIT;
    global.fg = fg;
    global.bg = bg;
    return WB_OK;
}

int wb_present(void) {
    if (!global.initialized) return WB_ERR_NOT_INIT;

    int x, y;
    COORD coord = {0, 0};
    DWORD written;
    WORD attr;

    for (y = 0; y < global.height; ++y) {
        for (x = 0; x < global.width; ++x) {
            int i = y * global.width + x;

            if (global.back[i].ch != global.front[i].ch ||
                global.back[i].fg != global.front[i].fg ||
                global.back[i].bg != global.front[i].bg) {

                char utf8_char[5]; // To store UTF-8
                int utf8_len = wb_utf32_to_utf8(global.back[i].ch, utf8_char);
                utf8_char[utf8_len] = '\0'; // Null-terminate

                WCHAR wide_char[2]; // UTF-16 character
                // Use the UTF-8 string for conversion:
                int n_chars = wb_utf8_to_utf16(utf8_char, wide_char, 2);

                coord.X = (SHORT)x;
                coord.Y = (SHORT)y;
                attr = wb_attr_to_win(global.back[i].fg, global.back[i].bg);
                FillConsoleOutputAttribute(global.hconout, attr, n_chars, coord, &written);
                WriteConsoleOutputCharacterW(global.hconout, wide_char, n_chars, coord, &written);

                global.front[i] = global.back[i];
            }
        }
    }
    if (global.cursor_x != -1 && global.cursor_y != -1) {
        wb_set_cursor(global.cursor_x, global.cursor_y);
    }

    FlushFileBuffers(global.hconout);
    return WB_OK;
}


int wb_set_cursor(int cx, int cy) {
    if (!global.initialized) return WB_ERR_NOT_INIT;

    if (cx < 0 || cx >= global.width || cy < 0 || cy >= global.height) {

        CONSOLE_CURSOR_INFO cci;
        cci.dwSize = 25;
        cci.bVisible = FALSE;
        SetConsoleCursorInfo(global.hconout, &cci);
        global.cursor_x = -1;
        global.cursor_y = -1;
    } else {

        CONSOLE_CURSOR_INFO cci;
        cci.dwSize = 25;
        cci.bVisible = TRUE;
        SetConsoleCursorInfo(global.hconout, &cci);

        COORD coord = {(SHORT)cx, (SHORT)cy};
        SetConsoleCursorPosition(global.hconout, coord);
        global.cursor_x = cx;
        global.cursor_y = cy;
    }

    return WB_OK;
}

int wb_hide_cursor(void) {
      return wb_set_cursor(-1, -1);
}

int wb_set_cell(int x, int y, uint32_t ch, uintattr_t fg, uintattr_t bg) {
    if (!global.initialized) return WB_ERR_NOT_INIT;
    if (x < 0 || x >= global.width || y < 0 || y >= global.height) {
        return WB_ERR_OUT_OF_BOUNDS;
    }
    int i = y * global.width + x;
    global.back[i].ch = ch;
    global.back[i].fg = fg;
    global.back[i].bg = bg;
    return WB_OK;
}

int wb_poll_event(struct wb_event *event) {
    if (!global.initialized) return WB_ERR_NOT_INIT;
    INPUT_RECORD record;
    DWORD read;

    if (!GetNumberOfConsoleInputEvents(global.hconin, &read)) return WB_ERR;
    if (read == 0) return WB_ERR_NO_EVENT;

    memset(event, 0, sizeof(*event));

    if (!ReadConsoleInputW(global.hconin, &record, 1, &read)) return WB_ERR;
    if (read == 0) return WB_ERR_NO_EVENT;

    if (record.EventType == KEY_EVENT) {
        if (record.Event.KeyEvent.bKeyDown) {
            event->type = WB_EVENT_KEY;
            event->ch = record.Event.KeyEvent.uChar.UnicodeChar;
            event->key = record.Event.KeyEvent.wVirtualKeyCode;
            if((record.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) !=0){
                event->mod |= WB_MOD_ALT;
            }
            if((record.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) !=0){
                event->mod |= WB_MOD_CTRL;
            }

            if((record.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) !=0){
                event->mod |= WB_MOD_SHIFT;
            }

            if(record.Event.KeyEvent.wVirtualKeyCode >= VK_F1 && record.Event.KeyEvent.wVirtualKeyCode <= VK_F12){
              event->key = WB_KEY_F1 + (record.Event.KeyEvent.wVirtualKeyCode - VK_F1);
            } else if (record.Event.KeyEvent.wVirtualKeyCode == VK_UP){
                event->key = WB_KEY_ARROW_UP;
            } else if(record.Event.KeyEvent.wVirtualKeyCode == VK_DOWN){
                event->key = WB_KEY_ARROW_DOWN;
            } else if(record.Event.KeyEvent.wVirtualKeyCode == VK_LEFT){
                event->key = WB_KEY_ARROW_LEFT;
            }else if(record.Event.KeyEvent.wVirtualKeyCode == VK_RIGHT){
                event->key = WB_KEY_ARROW_RIGHT;
            } else if(record.Event.KeyEvent.wVirtualKeyCode == VK_HOME){
                event->key = WB_KEY_HOME;
            } else if(record.Event.KeyEvent.wVirtualKeyCode == VK_END){
                event->key = WB_KEY_END;
            }else if(record.Event.KeyEvent.wVirtualKeyCode == VK_INSERT){
                event->key = WB_KEY_INSERT;
            }else if(record.Event.KeyEvent.wVirtualKeyCode == VK_DELETE){
                event->key = WB_KEY_DELETE;
            }else if(record.Event.KeyEvent.wVirtualKeyCode == VK_PRIOR){
                event->key = WB_KEY_PGUP;
            }else if(record.Event.KeyEvent.wVirtualKeyCode == VK_NEXT){
                event->key = WB_KEY_PGDN;
            }
            else if(event->ch == 0 && event->key != 0){
                event->key = 0xE000 | record.Event.KeyEvent.wVirtualScanCode;

             }

            if(event->ch <= 31){
               event->key = event->ch;
               event->mod |= WB_MOD_CTRL;
            }


            if (event->ch == 0 && event->key >= 0xE000) { // Extended keys
                 // Correctly map extended keys
            }

            // Convert UTF-16 back to UTF-8 for 'ch'
            char utf8_char[5]; // Buffer for UTF-8 conversion
            int utf8_len = wb_utf16_to_utf8(&record.Event.KeyEvent.uChar.UnicodeChar, utf8_char, sizeof(utf8_char) -1);

            if(utf8_len > 0){
              utf8_char[utf8_len] = '\0';
              wb_utf8_to_utf16(utf8_char, (WCHAR*)&event->ch, 1); // Store as UTF-32 (uint32_t)
            } else {
                event->ch = 0;
            }

        }
    } else if (record.EventType == WINDOW_BUFFER_SIZE_EVENT) {
        event->type = WB_EVENT_RESIZE;
        event->w = record.Event.WindowBufferSizeEvent.dwSize.X;
        event->h = record.Event.WindowBufferSizeEvent.dwSize.Y;
        global.width = event->w;
        global.height = event->h;
        wb_resize_buffers();

    }

    return WB_OK;
}

int wb_set_output_mode(int mode)
{
    if (!global.initialized) return WB_ERR_NOT_INIT;
    if(mode == WB_OUTPUT_NORMAL){
        global.output_mode = mode;
        return WB_OK;
    }
    return WB_OK;
}

int wb_printf(int x, int y, uintattr_t fg, uintattr_t bg, const char *fmt, ...) {
    if (!global.initialized) return WB_ERR_NOT_INIT;

    char buf[WB_PRINTF_BUF];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len < 0 || len >= (int)sizeof(buf)) {
        return WB_ERR;
    }

    const char *p = buf;
    uint32_t codepoint;
    int char_len;

    while (*p) {
        char_len = wb_utf8_char_to_unicode(&codepoint, p);
        if (char_len < 0) {
          codepoint = 0xFFFD; // Replacement character
          char_len = 1;       // Skip one byte for invalid UTF-8
        }

        if (x < global.width && y < global.height) { // Check bounds
            wb_set_cell(x, y, codepoint, fg, bg);
        }

        x++;
        p += char_len;
    }

    return WB_OK;
}

const char *wb_version(void) {
    return WB_VERSION_STR;
}

static int wb_resize_buffers(void) {

    if (global.back) free(global.back);
    if (global.front) free(global.front);

    // Get updated buffer size
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(global.hconout, &csbi)) {
        return WB_ERR;
    }
    global.width = csbi.dwSize.X;
    global.height = csbi.dwSize.Y;

    global.back = (struct wb_cell *)malloc(sizeof(struct wb_cell) * global.width * global.height);
    global.front = (struct wb_cell *)malloc(sizeof(struct wb_cell) * global.width * global.height);

    if (!global.back || !global.front) {
        return WB_ERR_MEM;
    }

    // Initialize BOTH buffers!
    int i;
    for (i = 0; i < global.width * global.height; ++i) {
        global.front[i].ch = ' ';
        global.front[i].fg = global.fg;  // Use default attributes
        global.front[i].bg = global.bg;
        global.back[i].ch = ' ';
        global.back[i].fg = global.fg;
        global.back[i].bg = global.bg;
    }
    return WB_OK;
}

static WORD wb_attr_to_win(uintattr_t fg, uintattr_t bg) {
    WORD out = 0;

    if (global.output_mode == WB_OUTPUT_NORMAL) {
        switch (fg & 0x0F) {
            case WB_BLACK:   out |= 0; break;
            case WB_RED:     out |= FOREGROUND_RED; break;
            case WB_GREEN:   out |= FOREGROUND_GREEN; break;
            case WB_YELLOW:  out |= FOREGROUND_RED | FOREGROUND_GREEN; break;
            case WB_BLUE:    out |= FOREGROUND_BLUE; break;
            case WB_MAGENTA: out |= FOREGROUND_RED | FOREGROUND_BLUE; break;
            case WB_CYAN:    out |= FOREGROUND_GREEN | FOREGROUND_BLUE; break;
            case WB_WHITE:   out |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        }

        switch (bg & 0x0F) {
            case WB_BLACK:   out |= 0; break;
            case WB_RED:     out |= BACKGROUND_RED; break;
            case WB_GREEN:   out |= BACKGROUND_GREEN; break;
            case WB_YELLOW:  out |= BACKGROUND_RED | BACKGROUND_GREEN; break;
            case WB_BLUE:    out |= BACKGROUND_BLUE; break;
            case WB_MAGENTA: out |= BACKGROUND_RED | BACKGROUND_BLUE; break;
            case WB_CYAN:    out |= BACKGROUND_GREEN | BACKGROUND_BLUE; break;
            case WB_WHITE:   out |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; break;
        }

        if (fg & WB_BOLD) out |= FOREGROUND_INTENSITY;
        if (bg & WB_BOLD) out |= BACKGROUND_INTENSITY;

    }

    return out;
}

static int wb_utf8_to_utf16(const char *str, WCHAR *out, int max_out) {
   int result = MultiByteToWideChar(CP_UTF8, 0, str, -1, out, max_out); // -1 for null termination
    if (result == 0) {
        fprintf(stderr, "MultiByteToWideChar failed: error = %lu\n", GetLastError()); //Add error print
    }
    return result;
}

static int wb_utf16_to_utf8(const WCHAR* str, char* out, int max_out) {
    return WideCharToMultiByte(CP_UTF8, 0, str, -1, out, max_out, NULL, NULL);
}

static int wb_utf8_char_length(char c) {
    // Check the first byte to determine the length
    if ((c & 0x80) == 0) return 1;        // 0xxxxxxx - 1 byte
    if ((c & 0xE0) == 0xC0) return 2;     // 110xxxxx - 2 bytes
    if ((c & 0xF0) == 0xE0) return 3;     // 1110xxxx - 3 bytes
    if ((c & 0xF8) == 0xF0) return 4;     // 11110xxx - 4 bytes
    return 1; // Default to 1 in case of invalid byte.
}

static int wb_utf8_char_to_unicode(uint32_t *out, const char *c) {
    if (*c == '\0') return 0;

    int len = wb_utf8_char_length(*c);
    uint32_t result = 0;

    switch (len) {
        case 1:
            *out = (uint32_t)*c;
            return 1;
        case 2:
            result = ((uint32_t)(c[0] & 0x1F) << 6) |
                     ((uint32_t)(c[1] & 0x3F));
            break;
        case 3:
            result = ((uint32_t)(c[0] & 0x0F) << 12) |
                     ((uint32_t)(c[1] & 0x3F) << 6)  |
                     ((uint32_t)(c[2] & 0x3F));
            break;
        case 4:
            result = ((uint32_t)(c[0] & 0x07) << 18) |
                     ((uint32_t)(c[1] & 0x3F) << 12) |
                     ((uint32_t)(c[2] & 0x3F) << 6)  |
                     ((uint32_t)(c[3] & 0x3F));
            break;
        default:
            return -1; // Invalid UTF-8
    }
     if (result > 0x10FFFF) {
        return -1;
    }
    *out = result;
    return len;
}

static int wb_utf32_to_utf8(uint32_t c, char *out) {
    if (c < 0x80) {
        out[0] = (char)c;
        return 1;
    } else if (c < 0x800) {
        out[0] = (char)(0xC0 | (c >> 6));
        out[1] = (char)(0x80 | (c & 0x3F));
        return 2;
    } else if (c < 0x10000) {
        out[0] = (char)(0xE0 | (c >> 12));
        out[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        out[2] = (char)(0x80 | (c & 0x3F));
        return 3;
    } else if (c <= 0x10FFFF) {
        out[0] = (char)(0xF0 | (c >> 18));
        out[1] = (char)(0x80 | ((c >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((c >> 6) & 0x3F));
        out[3] = (char)(0x80 | (c & 0x3F));
        return 4;
    } else {
        // Invalid code point
        return 0;
    }
}
#endif // WB_IMPL
