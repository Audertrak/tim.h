#include <conio.h>
#include <ctype.h>
#include <processenv.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef DEBUG
#define DEBUG_LOG(fmt, ...) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...)                                                    \
  do {                                                                         \
  } while (0)
#endif

#define MIN_2(a, b) ((a) < (b) ? (a) : (b))
#define MAX_2(a, b) ((a) > (b) ? (a) : (b))
#define MIN_3(a, b, c)                                                         \
  (((a) < (b)) ? (((a) < (c)) ? (a) : (c)) : (((b) < (c)) ? (b) : (c)))

#define MAX_3(a, b, c)                                                         \
  (((a) > (b)) ? (((a) > (c)) ? (a) : (c)) : (((b) > (c)) ? (b) : (c)))

#define PART_ERROR ((Part){.name = ""})

// trim whitespace from strings
char *trim(char *str) {
  char *end;

  while (isspace((unsigned char)*str))
    str++;

  if (*str == 0)
    return str;

  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end))
    end--;

  *(end + 1) = '\0';

  return str;
}

// Windows-specific input handling with ESC detection
char *read_t() {
  size_t buf_size = 256;
  size_t pos = 0;
  char *buf = (char *)malloc(buf_size);
  if (!buf)
    return NULL;

  printf("Enter selection (or press ESC to exit): ");

  // Read characters until Enter or ESC
  int ch;
  while (1) {
    // Use _getch() to read a character without echoing
    ch = _getch();

    // Check for ESC key (ASCII 27)
    if (ch == 27) {
      printf("\nESC pressed. Exiting program...\n");
      free(buf);
      exit(0);
    }

    // Enter key - finish input
    if (ch == 13) { // '\r' for Windows
      printf("\n");
      buf[pos] = '\0';
      break;
    }

    // Backspace handling
    if (ch == 8) {
      if (pos > 0) {
        pos--;
        printf("\b \b"); // Erase character on screen
      }
      continue;
    }

    // Regular character input
    if (pos < buf_size - 1 && ch >= 32 && ch <= 126) { // Printable ASCII
      buf[pos++] = (char)ch;
      printf("%c", ch); // Echo character
    }
  }

  buf[pos] = '\0';

  // Apply trim
  trim(buf);

  if (buf[0] == '\0') {
    printf("A little premature aren't we. Why don't you try actually typing "
           "something this time?\n");
    free(buf);
    return read_t();
  }

  return buf;
}

// type aware scanf replacement with error handling that trims user input
int scan_t(const char *format, ...) {
  va_list args;
  int result;

  while (1) {
    char *userInput = read_t();
    if (!userInput)
      return EOF;

    if (format[0] == '%' && format[1] == 'd') {
      char *ptr = userInput;
      while (isspace((unsigned char)*ptr))
        ptr++;

      if (*ptr == '-' || *ptr == '+')
        ptr++;

      if (!isdigit((unsigned char)*ptr)) {
        printf("You were supposed to enter a number dingus.\n");
        free(userInput);
        continue;
      }

      while (isdigit((unsigned char)*ptr))
        ptr++;

      while (*ptr != '\0') {
        if (!isspace((unsigned char)*ptr)) {
          printf("Does '%s' look like a number to you?\n", userInput);
          free(userInput);
          userInput = NULL;
          break;
        }
        ptr++;
      }

      if (userInput == NULL)
        continue;
    }

    va_start(args, format);
    result = vsscanf(userInput, format, args);
    va_end(args);

    free(userInput);

    if (result != EOF && result > 0) {
      break;
    } else {
      printf("Speak up, I can't hear you\n");
    }
  }
  return result;
}
