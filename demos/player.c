#include <stdio.h>
#include "../src/termbox.h"

tb_color fg_color = TB_WHITE;
tb_color selected_fg_color = TB_YELLOW;
tb_color bg_color = TB_DEFAULT;

char * items[30] = {
  "Option 1",
  "Option 2",
  "Option 3",
  "Option 4",
  "Option 5",
  "Option 6",
  "Option 7",
  "Option 8",
  "Option 9",
  "Option 10",
  "Option 1",
  "Option 2",
  "Option 3",
  "Option 4",
  "Option 5",
  "Option 6",
  "Option 7",
  "Option 8",
  "Option 9",
  "Option 10",
  "Option 1",
  "Option 2",
  "Option 3",
  "Option 4",
  "Option 5",
  "Option 6",
  "Option 7",
  "Option 8",
  "Option 9",
  "Option 10",
};

int w = 0, h = 0;
int selected = -1;
int offset = 0;
int num_items = 30;

int margin_left = 1;
int margin_top = 2;
int margin_bottom = 2;

/* drawing functions */

int draw_options(void) {
  int i, line;

  for (i = 0; i < (h - (margin_top + margin_bottom)); i++) {
    tb_empty(margin_left, i + margin_top, TB_DEFAULT, w - margin_left);
  }

  for (i = 0; i < (h - (margin_top + margin_bottom)); i++) {
    line = i + offset;
    if (line >= num_items) break;
    tb_stringf(margin_left, i + margin_top, line == selected ? selected_fg_color : fg_color, bg_color, "%s", items[line]);
  }
  return 0;
}

void draw_title(void) {
  tb_string(margin_left, 0, TB_RED, bg_color, "A music player.");
}

void draw_status(void) {
  tb_empty(0, h-1, TB_CYAN, w - margin_left);
  tb_stringf(margin_left, h-1, TB_BLACK, TB_CYAN, "Playing song: %s", selected >= 0 ? items[selected] : "None");
}

void draw_window(void) {
  draw_title();
  draw_options();
  draw_status();
}

/* movement, select */

int move_up(int lines) {
  if (lines <= 0) return 0;
  selected -= lines;
  if (selected <= 0) selected = 0;
  if (selected < offset) offset -= lines;
  if (offset < 0) offset = 0;
  return 0;
}

int move_down(int lines) {
  if (lines <= 0) return 0;
  int menu_h = (h - (margin_top + margin_bottom));

  selected += lines;
  if (selected >= num_items) selected = num_items-1;

  // TODO: simplify this
  if ((selected + lines) >= menu_h) {
    // if (lines > 0 && num_items > 0 && (result > (num_items - menu_h + lines))) {
    if ((offset + lines) > (num_items - menu_h + lines))
      return 0;

    offset += lines;
  }

  return 0;
}

int set_selected(int number) {
  selected = number;
  return 0;
}

/* playback */

int play_song(int number) {
  tb_stringf(w - 12, 0, fg_color, bg_color, "Playing song: %d", number);
  return 0;
}

/* main */

int main(void) {
  if (tb_init() != 0) {
    return 1; // couldn't initialize our screen
  }

  // get the screen resolution
  w = tb_width();
  h = tb_height();

  draw_window();
  tb_render();
  tb_enable_mouse();

  // now, wait for keyboard or input
  struct tb_event ev;
  while (tb_poll_event(&ev) != -1) {
    switch (ev.type) {
    case TB_EVENT_RESIZE:
      w = ev.w;
      h = ev.h;
      tb_clear_buffer();
      // tb_stringf((w/2)-10, h/2, fg_color, bg_color, "Window resized to: %dx%d", ev.w, ev.h);
      break;

    case TB_EVENT_KEY:
      if (ev.key == TB_KEY_ESC || ev.key == TB_KEY_CTRL_C)
        goto done;

      if (ev.key == TB_KEY_ENTER)
        play_song(selected);

      if (ev.key == TB_KEY_ARROW_UP)
        move_up(1);

      if (ev.key == TB_KEY_ARROW_DOWN)
        move_down(1);

      if (ev.key == TB_KEY_PAGE_UP)
        move_up(h/2);

      if (ev.key == TB_KEY_PAGE_DOWN)
        move_down(h/2);

      break;
    case TB_EVENT_MOUSE:
      if (ev.key == TB_KEY_MOUSE_LEFT) {
        set_selected(offset + (ev.y - margin_top));
        if (ev.h == 2) play_song(selected);
      }

      if (ev.key == TB_KEY_MOUSE_WHEEL_UP) 
        move_up(5);

      if (ev.key == TB_KEY_MOUSE_WHEEL_DOWN) 
        move_down(5);

      break;
    }

    draw_window();
    tb_render();
  }

  done:
  // make sure to shutdown
  tb_shutdown();
  return 0;
}
