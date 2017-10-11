#include <unistd.h> // for sleep()
#include <stdio.h>
#include <string.h>
#include "../src/termbox.h"

int main(void) {
  if (tb_init() != 0) {
    return 1; // couldn't initialize our screen
  }

  // set up our colors and test string
  int bg_color = TB_DEFAULT;
  int fg_color = TB_DEFAULT;

  // get the screen resolution
  int w = tb_width();
  int h = tb_height();

  tb_print((w/2)-6, h/2, bg_color, fg_color, "Click, click.");
  tb_present();

  int clicks = 0;
  tb_select_input_mode(TB_INPUT_MOUSE);

  // now, wait for keyboard or input
  struct tb_event ev;

  while (tb_poll_event(&ev) != -1) {
    switch (ev.type) {
    case TB_EVENT_RESIZE:
      tb_clear();
      w = tb_width();
      h = tb_height();
      tb_printf((w/2)-10, h/2, bg_color, fg_color, "Window resized to: %dx%d", w, h);
      break;

    case TB_EVENT_KEY:
      if (ev.key == TB_KEY_ESC || ev.key == TB_KEY_CTRL_C)
        goto done;

      break;
    case TB_EVENT_MOUSE:
      if (ev.key == TB_KEY_MOUSE_LEFT) {
        tb_printf((w/2)-10, h/2, bg_color, fg_color, "Click number %d! (%d, %d)", ++clicks, ev.x, ev.y);
      }
      break;
    }

    // flush the output
    tb_present();
  }

  done:
  // make sure to shutdown
  tb_shutdown();
  return 0;
}