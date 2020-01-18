#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../src/termbox.h"

static int curCol = 0;
static int curRune = 4;
static struct tb_cell *backbuf;
static int bbw = 0, bbh = 0;

static const tb_chr runes[] = {
  0x20,   // ' '
  0x2591, // '░'
  0x2592, // '▒'
  0x2593, // '▓'
  0x2588  // '█'
};

#define len(a) (sizeof(a)/sizeof(a[0]))

static const tb_color colors[] = {
  TB_BLACK,
  TB_RED,
  TB_GREEN,
  TB_YELLOW,
  TB_BLUE,
  TB_MAGENTA,
  TB_CYAN,
  TB_LIGHT_RED,
  TB_LIGHT_GREEN,
  TB_LIGHT_YELLOW,
  TB_LIGHT_BLUE,
  TB_LIGHT_MAGENTA,
  TB_LIGHT_CYAN,
  TB_WHITE,
  TB_LIGHTEST_GRAY,
  TB_LIGHTER_GRAY,
  TB_LIGHT_GRAY,
  TB_GRAY,
  TB_DARK_GRAY,
  TB_DARKER_GRAY,
  TB_DARKEST_GRAY,
};

void updateAndDrawButtons(int *current, int x, int y, int mx, int my, int n, void (*attrFunc)(int, tb_chr*, tb_color*, tb_color*)) {
  int lx = x;
  int ly = y;
  for (int i = 0; i < n; i++) {
    if (lx <= mx && mx <= lx+3 && ly <= my && my <= ly+1) {
      *current = i;
    }
    tb_chr r;
    tb_color fg, bg;
    (*attrFunc)(i, &r, &fg, &bg);
    tb_char(lx+0, ly+0, fg, bg, r);
    tb_char(lx+1, ly+0, fg, bg, r);
    tb_char(lx+2, ly+0, fg, bg, r);
    tb_char(lx+3, ly+0, fg, bg, r);
    tb_char(lx+0, ly+1, fg, bg, r);
    tb_char(lx+1, ly+1, fg, bg, r);
    tb_char(lx+2, ly+1, fg, bg, r);
    tb_char(lx+3, ly+1, fg, bg, r);
    lx += 4;
  }
  lx = x;
  ly = y;
  for (int i = 0; i < n; i++) {
    if (*current == i) {
      tb_color fg = TB_RED | TB_BOLD;
      tb_color bg = TB_DEFAULT;
      tb_char(lx+0, ly+2, fg, bg, '^');
      tb_char(lx+1, ly+2, fg, bg, '^');
      tb_char(lx+2, ly+2, fg, bg, '^');
      tb_char(lx+3, ly+2, fg, bg, '^');
    }
    lx += 4;
  }
}

void runeAttrFunc(int i, tb_chr *r, tb_color *fg, tb_color *bg) {
  *r = runes[i];
  *fg = TB_DEFAULT;
  *bg = TB_DEFAULT;
}

void colorAttrFunc(int i, tb_chr *r, tb_color *fg, tb_color *bg) {
  *r = ' ';
  *fg = TB_DEFAULT;
  *bg = colors[i];
}

void updateAndRedrawAll(int mx, int my, int rune) {
  int h = tb_height();

  if (mx != -1 && my != -1) {
    backbuf[bbw*my+mx].ch = rune;
    backbuf[bbw*my+mx].fg = colors[curCol];
  }

  memcpy(tb_cell_buffer(), backbuf, sizeof(struct tb_cell)*bbw*bbh);
  updateAndDrawButtons(&curRune, 0, 0, mx, my, len(runes), runeAttrFunc);
  updateAndDrawButtons(&curCol, 0, h-3, mx, my, len(colors), colorAttrFunc);

  tb_render();
}

void reallocBackBuffer(int w, int h) {
  bbw = w;
  bbh = h;
  if (backbuf) free(backbuf);
  backbuf = calloc(sizeof(struct tb_cell), w*h);
}

int main(void) {
  int code = tb_init();
  if (code < 0) {
    fprintf(stderr, "termbox init failed, code: %d\n", code);
    return -1;
  }

  tb_enable_mouse();
  int w = tb_width();
  int h = tb_height();
  reallocBackBuffer(w, h);
  updateAndRedrawAll(-1, -1, 0);

  int mx, my;
  tb_chr rune = 0;
  struct tb_event ev;

  for (;;) {
    mx = -1; my = -1;

    int t = tb_poll_event(&ev);

    if (t == -1) {
      tb_shutdown();
      fprintf(stderr, "termbox poll event error\n");
      return -1;
    }

    switch (t) {
    case TB_EVENT_KEY:
      if (ev.key == TB_KEY_ESC || ev.key == TB_KEY_CTRL_C || ev.ch == 'q') {
        tb_shutdown();
        return 0;
      }
      if (ev.key == TB_KEY_TAB || ev.ch == ' ') {
        curCol++;
        if (curCol > 20) curCol = 0;
      }

      if (ev.ch >= '0' && ev.ch <= '5') {
        curRune = 5 - (54 - ev.ch);
      }
      break;

    case TB_EVENT_MOUSE:
      if (ev.key == TB_KEY_MOUSE_LEFT) {
        mx = ev.x; my = ev.y;
        rune = runes[curRune];
      }

      else if (ev.key == TB_KEY_MOUSE_RIGHT) {
        mx = ev.x; my = ev.y;
        rune = runes[0];
      }

      break;

    case TB_EVENT_RESIZE:
      tb_resize();
      reallocBackBuffer(ev.w, ev.h);
      break;
    }

    updateAndRedrawAll(mx, my, rune);
  }
}
