#include <stdio.h>
#include "../src/termbox.h"

// set up our colors and test string
tb_color fg_color = TB_YELLOW | TB_BOLD;
tb_color bg_color = TB_DEFAULT;

#define MAX_COLOR_VALUE 255

void increase(tb_color * col) {
  *col = (*col & 0xFF) + 1;
  if (*col > MAX_COLOR_VALUE) {
    *col = 0;
  }
}

void decrease(tb_color * col) {
  if ((*col & 0xFF) == 0) {
    *col = MAX_COLOR_VALUE;
  } else {
    *col = (*col & 0xFF) - 1;
  }
}

void toggle_attr(int attr, tb_color * col) {
  if (*col & attr) { // has attr
    *col ^= attr;
  } else {
    *col |= attr;
  }
}

int main(void) {
  if (tb_init() != 0) {
    return 1; // couldn't initialize our screen
  }

  // get the screen resolution
  int w = tb_width();
  int h = tb_height();

  tb_string((w/2)-6, (h/2), fg_color, bg_color, "Hello there. ☺");
  tb_render();

  // int clicks = 0;
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

      if (ev.ch == 'b') {
        // toggle_attr(TB_BOLD, ev.meta & TB_META_SHIFT ? &bg_color : &fg_color);
        toggle_attr(TB_BOLD, &fg_color);
      } else if (ev.ch == 'u') {
        toggle_attr(TB_UNDERLINE, &fg_color);
      } else if (ev.ch == 'r') {
        toggle_attr(TB_REVERSE, &fg_color);
      } else if (ev.key == TB_KEY_ARROW_RIGHT) {
        increase(ev.meta & TB_META_SHIFT ? &bg_color : &fg_color);
      } else if (ev.key == TB_KEY_ARROW_LEFT) {
        decrease(ev.meta & TB_META_SHIFT ? &bg_color : &fg_color);
      }

      tb_stringf((w/2)-14, h/2, fg_color, bg_color, "Foreground: %03d, background: %03d", fg_color & 0xFF, bg_color & 0xFF);

      break;
    case TB_EVENT_MOUSE:
      if (ev.key == TB_KEY_MOUSE_LEFT) {
        tb_stringf((w/2)-10, h/2, fg_color, bg_color, "Click! Count: %d! (%d, %d)", ev.h, ev.x, ev.y);
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