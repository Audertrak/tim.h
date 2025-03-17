#define WB_IMPL
#include "lib/winbox.h"
#include <stdio.h>

int main(void) {
  struct wb_event ev;
  int y = 0;

  if (wb_init() != WB_OK) {
    fprintf(stderr, "wb_init() failed\n");
    return 1;
  }

  wb_printf(0, y++, WB_GREEN, WB_BLACK, "hello from winbox");
  wb_printf(0, y++, WB_WHITE, WB_BLACK, "width=%d height=%d", wb_width(),
            wb_height());
  wb_printf(0, y++, WB_WHITE, WB_BLACK, "press any key...");
  wb_present();
  wb_poll_event(&ev);
  y++;
  wb_printf(0, y++, WB_WHITE, WB_BLACK, "event type=%d key=%d ch=%X", ev.type,
            ev.key, ev.ch);
  wb_printf(0, y++, WB_WHITE, WB_BLACK, "press any key to quit...");

  wb_present();

  wb_poll_event(&ev);
  wb_shutdown();

  return 0;
}
