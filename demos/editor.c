#include <stdio.h>
#include <string.h>
#include "../src/termbox.h"

#define MAX 10000;
static int count;
// static uint32_t buf[10000];
static char buf[10000];

void append(char c) {
  if (count > 10000)
    count = 0; // restart

  // memcpy(&buf, &c, sizeof c);
  // count += sizeof(c);
  buf[count++] = c;
}

void draw() {
  int w = tb_width();
  int c = 0;

  // print status bar
  tb_stringf(0, 0, 0, TB_BLACK, "%*c", w, ' ');
  tb_stringf(1, 0, 0, TB_BLACK, "%s", "Basic editor");
  tb_stringf(w-18, 0, 0, TB_BLACK, "%s", "Press ESC to quit");

  // print text
  c = 0;
  int y = 1;
  while (c < count) {
    tb_stringf(0, y++, 0, 0, "%.*s", w, buf + c);
    c += w;
  }

  // and flush the output
  tb_render();
}

int main(void) {
  if (tb_init() != 0) {
    return 1; // couldn't initialize our screen
  }

  tb_show_cursor();
  draw();

  struct tb_event ev;
  while (tb_poll_event(&ev) != -1) {

    switch (ev.type) {
      case TB_EVENT_RESIZE:
        tb_clear_buffer();
        tb_resize();
        break;

      case TB_EVENT_KEY:
        if (ev.key == TB_KEY_ESC || ev.key == TB_KEY_CTRL_C)
          goto done;

        if (ev.ch)
          append(ev.ch);
        else if (ev.key == TB_KEY_ENTER)
          append('!');

        break;

      case TB_EVENT_MOUSE:
        break;
    }

    // flush the output
    draw();
  }

  done:
  tb_shutdown();
  return 0;
}