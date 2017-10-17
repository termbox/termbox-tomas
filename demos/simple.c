#include <unistd.h> // for sleep()
#include "../src/termbox.h"

int main(void) {
  if (tb_init() != 0) {
    return 1; // couldn't initialize our screen
  }

  // set up our colors and test string
  int fg_color = tb_rgb(0xFFCC00);
  int bg_color = TB_DEFAULT;

  // get the screen resolution
  int w = tb_width();
  int h = tb_height();

  tb_string((w/2)-6, (h/2), fg_color, bg_color, "Hello there. ☺");
  tb_render();

  int clicks = 0;
  tb_enable_mouse();

  // now, wait for keyboard or input
  struct tb_event ev;

  while (tb_poll_event(&ev) != -1) {
    switch (ev.type) {
    case TB_EVENT_RESIZE:
      w = ev.w;
      h = ev.h;
      tb_clear_buffer();
      tb_stringf((w/2)-10, h/2, fg_color, bg_color, "Window resized to: %dx%d", ev.w, ev.h);
      break;

    case TB_EVENT_KEY:
      if (ev.key == TB_KEY_ESC || ev.key == TB_KEY_CTRL_C)
        goto done;

      break;
    case TB_EVENT_MOUSE:
      if (ev.key == TB_KEY_MOUSE_LEFT) {
        tb_stringf((w/2)-10, h/2, fg_color, bg_color, "Click number %d! (%d, %d)", ++clicks, ev.x, ev.y);
      }
      break;
    }

    // flush the output
    tb_render();
  }

  done:
  // make sure to shutdown
  tb_shutdown();
  return 0;
}