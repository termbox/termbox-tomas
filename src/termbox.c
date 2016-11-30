#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#include "termbox.h"

#include "bytebuffer.inl"
#include "term.inl"
#include "input.inl"

struct cellbuf {
	int width;
	int height;
	struct tb_cell *cells;
};

#define CELL(buf, x, y) (buf)->cells[(y) * (buf)->width + (x)]
#define IS_CURSOR_HIDDEN(cx, cy) (cx == -1 || cy == -1)
#define LAST_COORD_INIT -1

static struct termios orig_tios;

static struct cellbuf back_buffer;
static struct cellbuf front_buffer;
static struct bytebuffer output_buffer;
static struct bytebuffer input_buffer;

static int termw = -1;
static int termh = -1;

static bool title_set = false;
static int inputmode  = TB_INPUT_ESC;
static int outputmode = TB_OUTPUT_NORMAL;

static int inout;
static int winch_fds[2];

static int lastx = LAST_COORD_INIT;
static int lasty = LAST_COORD_INIT;
static int cursor_x = -1;
static int cursor_y = -1;

static uint16_t background = TB_DEFAULT;
static uint16_t foreground = TB_DEFAULT;

static void write_cursor(int x, int y);
static void write_sgr(uint16_t fg, uint16_t bg);
static void write_title(const char * title);

static void cellbuf_init(struct cellbuf *buf, int width, int height);
static void cellbuf_resize(struct cellbuf *buf, int width, int height);
static void cellbuf_clear(struct cellbuf *buf);
static void cellbuf_free(struct cellbuf *buf);

static void update_size(void);
static void update_term_size(void);
static void send_attr(uint16_t fg, uint16_t bg);
static void send_char(int x, int y, uint32_t c);
static void send_clear(void);
static void sigwinch_handler(int xxx);
static int wait_fill_event(struct tb_event *event, struct timeval *timeout);

/* may happen in a different thread */
static volatile int buffer_size_change_request;

/* -------------------------------------------------------- */

int tb_init_fd(int inout_)
{
	inout = inout_;
	if (inout == -1) {
		return TB_EFAILED_TO_OPEN_TTY;
	}

	if (init_term() < 0) {
		close(inout);
		return TB_EUNSUPPORTED_TERMINAL;
	}

	if (pipe(winch_fds) < 0) {
		close(inout);
		return TB_EPIPE_TRAP_ERROR;
	}

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigwinch_handler;
	sa.sa_flags = 0;
	sigaction(SIGWINCH, &sa, 0);

	tcgetattr(inout, &orig_tios);

	struct termios tios;
	memcpy(&tios, &orig_tios, sizeof(tios));

/*
  tios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  tios.c_oflag &= ~(OPOST);
  tios.c_cflag |= (CS8);
  tios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  tios.c_cc[VMIN] = 0;  // Return each byte, or zero for timeout.
  tios.c_cc[VTIME] = 1; // 100 ms timeout (unit is tens of second).
*/

	tios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                           | INLCR | IGNCR | ICRNL | IXON);
	tios.c_oflag &= ~OPOST;
	tios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tios.c_cflag &= ~(CSIZE | PARENB);
	tios.c_cflag |= CS8;
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 0;

	tcsetattr(inout, TCSAFLUSH, &tios);

	bytebuffer_init(&input_buffer, 128);
	bytebuffer_init(&output_buffer, 32 * 1024);

	bytebuffer_puts(&output_buffer, funcs[T_ENTER_CA]);
	bytebuffer_puts(&output_buffer, funcs[T_ENTER_KEYPAD]);
	bytebuffer_puts(&output_buffer, funcs[T_HIDE_CURSOR]);
	send_clear();

	update_term_size();
	cellbuf_init(&back_buffer, termw, termh);
	cellbuf_init(&front_buffer, termw, termh);
	cellbuf_clear(&back_buffer);
	cellbuf_clear(&front_buffer);

	return 0;
}

int tb_init_file(const char* name){
	return tb_init_fd(open(name, O_RDWR));
}

int tb_init(void)
{
	return tb_init_file("/dev/tty");
}

void tb_shutdown(void)
{
	if (termw == -1) {
		fputs("tb_shutdown() should not be called twice.", stderr);
		abort();
	}

	if (title_set) write_title("");
	bytebuffer_puts(&output_buffer, funcs[T_SHOW_CURSOR]);
	bytebuffer_puts(&output_buffer, funcs[T_SGR0]);
	bytebuffer_puts(&output_buffer, funcs[T_CLEAR_SCREEN]);
	bytebuffer_puts(&output_buffer, funcs[T_EXIT_CA]);
	bytebuffer_puts(&output_buffer, funcs[T_EXIT_KEYPAD]);
	bytebuffer_puts(&output_buffer, funcs[T_EXIT_MOUSE]);
	bytebuffer_flush(&output_buffer, inout);
	tcsetattr(inout, TCSAFLUSH, &orig_tios);

	shutdown_term();
	close(inout);
	close(winch_fds[0]);
	close(winch_fds[1]);

	cellbuf_free(&back_buffer);
	cellbuf_free(&front_buffer);
	bytebuffer_free(&output_buffer);
	bytebuffer_free(&input_buffer);
	termw = termh = -1;
}

void tb_present(void)
{
	int x,y,w,i;
	struct tb_cell *back, *front;

	/* invalidate cursor position */
	lastx = LAST_COORD_INIT;
	lasty = LAST_COORD_INIT;

	if (buffer_size_change_request) {
		update_size();
		buffer_size_change_request = 0;
	}

	for (y = 0; y < front_buffer.height; ++y) {
		for (x = 0; x < front_buffer.width; ) {
			back = &CELL(&back_buffer, x, y);
			front = &CELL(&front_buffer, x, y);
			w = wcwidth(back->ch);
			if (w < 1) w = 1;
			if (memcmp(back, front, sizeof(struct tb_cell)) == 0) {
				x += w;
				continue;
			}
			memcpy(front, back, sizeof(struct tb_cell));
			send_attr(back->fg, back->bg);
			if (w > 1 && x >= front_buffer.width - (w - 1)) {
				// Not enough room for wide ch, so send spaces
				for (i = x; i < front_buffer.width; ++i) {
					send_char(i, y, ' ');
				}
			} else {
				send_char(x, y, back->ch);
				for (i = 1; i < w; ++i) {
					front = &CELL(&front_buffer, x + i, y);
					front->ch = 0;
					front->fg = back->fg;
					front->bg = back->bg;
				}
			}
			x += w;
		}
	}
	if (!IS_CURSOR_HIDDEN(cursor_x, cursor_y))
		write_cursor(cursor_x, cursor_y);
	bytebuffer_flush(&output_buffer, inout);
}

void tb_set_cursor(int cx, int cy)
{
	if (IS_CURSOR_HIDDEN(cursor_x, cursor_y) && !IS_CURSOR_HIDDEN(cx, cy))
		bytebuffer_puts(&output_buffer, funcs[T_SHOW_CURSOR]);

	if (!IS_CURSOR_HIDDEN(cursor_x, cursor_y) && IS_CURSOR_HIDDEN(cx, cy))
		bytebuffer_puts(&output_buffer, funcs[T_HIDE_CURSOR]);

	cursor_x = cx;
	cursor_y = cy;
	if (!IS_CURSOR_HIDDEN(cursor_x, cursor_y))
		write_cursor(cursor_x, cursor_y);
}

void tb_set_title(const char * title) {
	title_set = true;
	write_title(title);
}

void tb_put_cell(int x, int y, const struct tb_cell *cell)
{
	if ((unsigned)x >= (unsigned)back_buffer.width)
		return;
	if ((unsigned)y >= (unsigned)back_buffer.height)
		return;
	CELL(&back_buffer, x, y) = *cell;
}

void tb_change_cell(int x, int y, uint32_t ch, uint16_t fg, uint16_t bg)
{
	struct tb_cell c = {ch, fg, bg};
	tb_put_cell(x, y, &c);
}

void tb_blit(int x, int y, int w, int h, const struct tb_cell *cells)
{
	if (x + w < 0 || x >= back_buffer.width)
		return;
	if (y + h < 0 || y >= back_buffer.height)
		return;
	int xo = 0, yo = 0, ww = w, hh = h;
	if (x < 0) {
		xo = -x;
		ww -= xo;
		x = 0;
	}
	if (y < 0) {
		yo = -y;
		hh -= yo;
		y = 0;
	}
	if (ww > back_buffer.width - x)
		ww = back_buffer.width - x;
	if (hh > back_buffer.height - y)
		hh = back_buffer.height - y;

	int sy;
	struct tb_cell *dst = &CELL(&back_buffer, x, y);
	const struct tb_cell *src = cells + yo * w + xo;
	size_t size = sizeof(struct tb_cell) * ww;

	for (sy = 0; sy < hh; ++sy) {
		memcpy(dst, src, size);
		dst += back_buffer.width;
		src += w;
	}
}

struct tb_cell *tb_cell_buffer(void)
{
	return back_buffer.cells;
}

int tb_poll_event(struct tb_event *event)
{
	return wait_fill_event(event, 0);
}

int tb_peek_event(struct tb_event *event, int timeout)
{
	struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout - (tv.tv_sec * 1000)) * 1000;
	return wait_fill_event(event, &tv);
}

int tb_width(void)
{
	return termw;
}

int tb_height(void)
{
	return termh;
}

void tb_clear(void)
{
	if (buffer_size_change_request) {
		update_size();
		buffer_size_change_request = 0;
	}
	cellbuf_clear(&back_buffer);
}

int tb_select_input_mode(int mode)
{
	if (mode) {
		if ((mode & (TB_INPUT_ESC | TB_INPUT_ALT)) == 0)
			mode |= TB_INPUT_ESC;

		/* technically termbox can handle that, but let's be nice and show here
		   what mode is actually used */
		if ((mode & (TB_INPUT_ESC | TB_INPUT_ALT)) == (TB_INPUT_ESC | TB_INPUT_ALT))
			mode &= ~TB_INPUT_ALT;

		inputmode = mode;
		if (mode&TB_INPUT_MOUSE) {
			bytebuffer_puts(&output_buffer, funcs[T_ENTER_MOUSE]);
			bytebuffer_flush(&output_buffer, inout);
		} else {
			bytebuffer_puts(&output_buffer, funcs[T_EXIT_MOUSE]);
			bytebuffer_flush(&output_buffer, inout);
		}
	}
	return inputmode;
}

int tb_select_output_mode(int mode)
{
	if (mode)
		outputmode = mode;
	return outputmode;
}

void tb_set_clear_attributes(uint16_t fg, uint16_t bg)
{
	foreground = fg;
	background = bg;
}

/* -------------------------------------------------------- */

static int convertnum(uint32_t num, char* buf) {
	int i, l = 0;
	int ch;
	do {
		buf[l++] = '0' + (num % 10);
		num /= 10;
	} while (num);
	for(i = 0; i < l / 2; i++) {
		ch = buf[i];
		buf[i] = buf[l - 1 - i];
		buf[l - 1 - i] = ch;
	}
	return l;
}

#define WRITE_LITERAL(X) bytebuffer_append(&output_buffer, (X), sizeof(X)-1)
#define WRITE_INT(X) bytebuffer_append(&output_buffer, buf, convertnum((X), buf))

static void write_cursor(int x, int y) {
	char buf[32];
	WRITE_LITERAL("\033[");
	WRITE_INT(y+1);
	WRITE_LITERAL(";");
	WRITE_INT(x+1);
	WRITE_LITERAL("H");
}

static void write_sgr(uint16_t fg, uint16_t bg) {
	char buf[32];

	if (fg == TB_DEFAULT && bg == TB_DEFAULT)
		return;

	switch (outputmode) {
	case TB_OUTPUT_256:
	case TB_OUTPUT_216:
	case TB_OUTPUT_GRAYSCALE:
		WRITE_LITERAL("\033[");
		if (fg != TB_DEFAULT) {
			WRITE_LITERAL("38;5;");
			WRITE_INT(fg);
			if (bg != TB_DEFAULT) {
				WRITE_LITERAL(";");
			}
		}
		if (bg != TB_DEFAULT) {
			WRITE_LITERAL("48;5;");
			WRITE_INT(bg);
		}
		WRITE_LITERAL("m");
		break;
	case TB_OUTPUT_NORMAL:
	default:
		WRITE_LITERAL("\033[");
		if (fg != TB_DEFAULT) {
			WRITE_LITERAL("3");
			WRITE_INT(fg - 1);
			if (bg != TB_DEFAULT) {
				WRITE_LITERAL(";");
			}
		}
		if (bg != TB_DEFAULT) {
			WRITE_LITERAL("4");
			WRITE_INT(bg - 1);
		}
		WRITE_LITERAL("m");
		break;
	}
}

static void write_title(const char * title) {
	printf("%c]0;%s%c\n", '\033', title, '\007');
}

static void cellbuf_init(struct cellbuf *buf, int width, int height)
{
	buf->cells = (struct tb_cell*)malloc(sizeof(struct tb_cell) * width * height);
	assert(buf->cells);
	buf->width = width;
	buf->height = height;
}

static void cellbuf_resize(struct cellbuf *buf, int width, int height)
{
	if (buf->width == width && buf->height == height)
		return;

	int oldw = buf->width;
	int oldh = buf->height;
	struct tb_cell *oldcells = buf->cells;

	cellbuf_init(buf, width, height);
	cellbuf_clear(buf);

	int minw = (width < oldw) ? width : oldw;
	int minh = (height < oldh) ? height : oldh;
	int i;

	for (i = 0; i < minh; ++i) {
		struct tb_cell *csrc = oldcells + (i * oldw);
		struct tb_cell *cdst = buf->cells + (i * width);
		memcpy(cdst, csrc, sizeof(struct tb_cell) * minw);
	}

	free(oldcells);
}

static void cellbuf_clear(struct cellbuf *buf)
{
	int i;
	int ncells = buf->width * buf->height;

	for (i = 0; i < ncells; ++i) {
		buf->cells[i].ch = ' ';
		buf->cells[i].fg = foreground;
		buf->cells[i].bg = background;
	}
}

static void cellbuf_free(struct cellbuf *buf)
{
	free(buf->cells);
}

static void get_term_size(int *w, int *h)
{
	struct winsize sz;
	memset(&sz, 0, sizeof(sz));

	ioctl(inout, TIOCGWINSZ, &sz);

	if (w) *w = sz.ws_col;
	if (h) *h = sz.ws_row;
}

static void update_term_size(void)
{
	struct winsize sz;
	memset(&sz, 0, sizeof(sz));

	ioctl(inout, TIOCGWINSZ, &sz);

	termw = sz.ws_col;
	termh = sz.ws_row;
}

static void send_attr(uint16_t fg, uint16_t bg)
{
#define LAST_ATTR_INIT 0xFFFF
	static uint16_t lastfg = LAST_ATTR_INIT, lastbg = LAST_ATTR_INIT;
	if (fg != lastfg || bg != lastbg) {
		bytebuffer_puts(&output_buffer, funcs[T_SGR0]);

		uint16_t fgcol;
		uint16_t bgcol;

		switch (outputmode) {
		case TB_OUTPUT_256:
			fgcol = fg & 0xFF;
			bgcol = bg & 0xFF;
			break;

		case TB_OUTPUT_216:
			fgcol = fg & 0xFF; if (fgcol > 215) fgcol = 7;
			bgcol = bg & 0xFF; if (bgcol > 215) bgcol = 0;
			fgcol += 0x10;
			bgcol += 0x10;
			break;

		case TB_OUTPUT_GRAYSCALE:
			fgcol = fg & 0xFF; if (fgcol > 23) fgcol = 23;
			bgcol = bg & 0xFF; if (bgcol > 23) bgcol = 0;
			fgcol += 0xe8;
			bgcol += 0xe8;
			break;

		case TB_OUTPUT_NORMAL:
		default:
			fgcol = fg & 0x0F;
			bgcol = bg & 0x0F;
		}

		if (fg & TB_BOLD)
			bytebuffer_puts(&output_buffer, funcs[T_BOLD]);
		if (bg & TB_BOLD)
			bytebuffer_puts(&output_buffer, funcs[T_BLINK]);
		if (fg & TB_UNDERLINE)
			bytebuffer_puts(&output_buffer, funcs[T_UNDERLINE]);
		if ((fg & TB_REVERSE) || (bg & TB_REVERSE))
			bytebuffer_puts(&output_buffer, funcs[T_REVERSE]);

		write_sgr(fgcol, bgcol);

		lastfg = fg;
		lastbg = bg;
	}
}

static void send_char(int x, int y, uint32_t c)
{
	char buf[7];
	int bw = tb_utf8_unicode_to_char(buf, c);
	if (x-1 != lastx || y != lasty)
		write_cursor(x, y);
	lastx = x; lasty = y;
	if(!c) buf[0] = ' '; // replace 0 with whitespace
	bytebuffer_append(&output_buffer, buf, bw);
}

static void send_clear(void)
{
	send_attr(foreground, background);
	bytebuffer_puts(&output_buffer, funcs[T_CLEAR_SCREEN]);
	if (!IS_CURSOR_HIDDEN(cursor_x, cursor_y))
		write_cursor(cursor_x, cursor_y);
	bytebuffer_flush(&output_buffer, inout);

	/* we need to invalidate cursor position too and these two vars are
	 * used only for simple cursor positioning optimization, cursor
	 * actually may be in the correct place, but we simply discard
	 * optimization once and it gives us simple solution for the case when
	 * cursor moved */
	lastx = LAST_COORD_INIT;
	lasty = LAST_COORD_INIT;
}

static void sigwinch_handler(int xxx)
{
	(void) xxx;
	const int zzz = 1;

  int unused __attribute__((unused));
	unused = write(winch_fds[1], &zzz, sizeof(int));
}

static void update_size(void)
{
	update_term_size();
	cellbuf_resize(&back_buffer, termw, termh);
	cellbuf_resize(&front_buffer, termw, termh);
	cellbuf_clear(&front_buffer);
	send_clear();
}

static int parse_bracket_esc(struct tb_event *event, const char *seq, int len) {
  int last = seq[len-1];
  int res  = 0;

  if (len == 3) {

    if (last == 'Z') { // shift + tab
      event->meta = TB_META_SHIFT;
      event->key  = TB_KEY_TAB;

// not needed.
//    } else if ('A' <= last && last <= 'H') { // arrow keys linux and xterm
//      event->key = 0xFFFF + (last - 86);

    } else if ('a' <= last && last <= 'd') { // mrxvt shift + left/right or ctrl+shift + up/down
      // TODO: handle ctrl + shift + arrow in mrxvt
      event->meta = TB_META_SHIFT;
      event->key  = 0xFFFF + (last - 118);
    } else {
    	res = -1;
    }

//  not needed.
//  } else if (len == 4 && ('A' <= last && last <= 'Z')) { // F1-F5 xterm
//      event->key = last - 54;

  } else if (len > 5) { // xterm shift or control + f1/keys/arrows

    if (last == '~') {

    	if (seq[3] == ';') { // xfce shift/ctrl + keys (delete, pageup/down)

  			int offset = seq[2] < 53 ? 38 : seq[2] > 54 ? 41 : 37;
	      event->key = 0xFFFF - (seq[2] - offset);
    		event->meta = seq[4] - 48;

    	} else {

    		return -1;
/*
				// TODO: check this
	      event->key = (seq[4] == ';') ? (int)seq[3] : ((int)seq[2] * 10 + (int)seq[3]);

	      // num may be two digit number, so meta key can be at index 4 or 5
	      event->meta = '2' <= seq[5] && seq[5] <= '8' ? (int)seq[4]
	                  : '2' <= seq[6] && seq[6] <= '8' ? (int)seq[5] : -1;
*/
    	}

    } else if ('A' <= last && last <= 'Z') {
      event->meta = seq[4] - 48;

      if (last >= 80) { // f1-f4 xterm
        // event->key = last - 69; // TODO: verify
        event->key = 0xFFFF + (last - 86);
      } else {  // ctrl + arrows urxvt
        event->key = 0xFFFF + (last - 86);
      }

    } else {
      res = -1;
    }

  } else if (last == '^' || last == '@' || last == '$') { // 4 or 5 in length

  	if ('1' <= seq[2] && seq[2] <= '8') { // ctrl/shift + keys urxvt

  			// 50 ins      // -12
  			// 51 del      // -13
  		  // 53 pageup   // -16
  		  // 54 pagedown // -17
  			// 55 home     // -14
  			// 56 end      // -15

  			int offset = seq[2] < 53 ? 38 : seq[2] > 54 ? 41 : 37;
	      event->key = 0xFFFF - (seq[2] - offset);
	      event->meta = last == '@' ? 6 : last >> 4;

  	} else {

	    int num = len == 5 ? ((int)seq[3] * 10 + (int)seq[4]) : (int)seq[3];

	    if (num >= 25) { // ctrl + shift f1-f12 urxvt
	      int offset = num == 25 || num == 26 || num == 29 ? 12 : 13;
	      event->key = num - offset; // TODO
	      event->meta = TB_META_CTRLSHIFT;
	    } else {
	      event->meta = last == '@' ? 6 : last >> 4;
	      event->key  = num; // ALSO TODO
	    }

  	}

  } else if (last == '~') { // 4 or 5 in length

    if (len == 5) { // shift + f1-f8, linux/urxvt

  		int num = (seq[2]-48) * 10 + seq[3]-48; // f9 is 33, and should be 8
      int offset = 25;

      // TODO: mega fix this.

/*
      if (term == 'linux') {
        offset = 15;

        if (num == 25 || num == 26) // offset disparity
          offset -= 1;
        else if (num == 31) // F5
          offset += 1;
      } else {
        offset = 13;

        if (num == 25 || num == 26 || num == 29) // offset disparity
          offset -= 1;
      }
*/
      event->meta = TB_META_SHIFT;
      event->key  = 0xFFFF - (num - offset);

/*
		// TODO: is this actually sent?
    } else if (seq[3] == 3) { // mrxvt shift + insert

      event->meta = TB_META_SHIFT;
      event->key  = TB_KEY_INSERT;
*/

    } else {
    	// not needed.
      // event->key = len == 5 ? ((int)seq[3] * 10 + (int)seq[4]) : (int)seq[3];
      res = -1;

    }

  } else {
    res = -1;
  }

  return res;
}

static int parse_esc_seq(struct tb_event *event, const char *seq, int len) {

	if (len == 1) {
	  event->key  = TB_KEY_ESC;
    return 1;

  } else if (len == 2) { // alt+char or alt+shift+char or alt + enter
    event->meta = seq[1] >= 'A' && seq[1] <= 'Z' ? TB_META_ALTSHIFT : TB_META_ALT;

    switch(seq[1]) {
    	case 10:
    	  event->key = TB_KEY_ENTER; break;
    	case 127:
    	  event->key = TB_KEY_BACKSPACE; break;
    	default:
    		event->ch  = seq[1]; break;
    }

    return 1;
  }

	int i;
	for (i = 0; keys[i]; i++) {
		if (starts_with(seq, len, keys[i])) {
		  event->ch = 0;
		  event->key = 0xFFFF-i;
		  return 1; // strlen(keys[i]);
		}
  }

  int last, num;
	switch(seq[1]) {
		case '[':
		  if (parse_bracket_esc(event, seq, len) == 0)
		    return 1;
			break;

		case 'O':
      if (seq[2] > 96 && seq[2] < 101) { // ctrl + arrows mrxvt/urxvt
        event->meta = TB_META_CTRL;
        event->key  = 0xFFFF + (seq[2] - 118);

      } else if (seq[2] == 49) { // xfce4 arrow keys
        event->key  = 0xFFFF + (seq[len-1] - 80);
        event->meta = TB_META_CTRLSHIFT;

// not needed. parsed previously.
//      } else if (65 <= seq[2] && seq[2] <= 68) { // arrows xterm/mrxvt
//        event->key  = 0xFFFF + (seq[2] - 86);
//        event->meta = 0;

      } else { /* unknown */ }

      return 1;
			break;

    case 27: // double esc, urxvt territory
      last = seq[len-1];
      // TODO: check what happens when issuing CTRL+PAGEDOWN and then ALT+PAGEUP repeatedly

      switch(last) {
        case 'R': // mrxvt alt + f3
          event->meta = TB_META_ALT;
          event->key  = 0xFFFF - 2;
          break;

        case '~': // urxvt alt + key or f1-f12
          event->meta = TB_META_ALT;

        	if (len == 6) { // f1-f12
        		num = (seq[3]-48) * 10 + seq[4]-48; // f9 is 20
        		event->key = 0xFFFF - (num - 12);
        	} else { // ins/del/etc
        		num = (int)seq[3];
		  			int offset = num < 53 ? 38 : num > 54 ? 41 : 37;
			      event->key = 0xFFFF - (num - offset);
        	}

          break;

        case '^':
        case '@': // ctrl + alt + arrows
          event->meta = last == '^' ? TB_META_ALTCTRL : TB_META_ALTCTRLSHIFT;
          event->key  = seq[5];
          break;

        default:
          if ('a' <= last && last <= 'z') { // urxvt ctr/alt arrow or ctr/shift/alt arrow

            event->meta = seq[4] == 'O' ? TB_META_ALTCTRL : TB_META_ALTCTRLSHIFT;
            event->key  = 0xFFFF + (last - 118);

          } else if ('A' <= last && last <= 'Z') { // urxvt alt + arrow keys
            event->meta = TB_META_ALT;
            event->key  = 0xFFFF + (last - 86);
          } else {
            return -1;
          }

          break;
        }

    	break; // case 27

		default:

      if ('A' <= seq[3] && seq[3] <= 'Z') { // linux ctrl+alt+key
        event->meta = TB_META_ALTCTRL;
        event->ch = seq[3];
      } else {
        return -1;
      }

		  printf("Unknown: %d\n", seq[1]);
			break;
	}

  return 1;
}

int maxseq = 8;
int cutesc = 0;
static char seq[8];

static int read_and_extract_event(struct tb_event * event) {
  int nread, rs;
  int c;

  if (cutesc) {
    c = 27;
    cutesc = 0;
  } else {
    while ((nread = read(inout, &c, 1)) == 0);
    if (nread == -1) return -1;
  }

  seq[0] = c;
	event->type = TB_EVENT_KEY;
  event->meta = 0;
	// event->key  = 0;
	event->ch   = 0;
	// tb_clear();

	if (c != 27 && 0 <= c && c <= 127) { // from ctrl-a to z, not esc

    if (c == 127) {
      event->key = TB_KEY_BACKSPACE2;

    } else if (c < 32) { // ctrl + a-z or number up to 7

    	// TODO: figure out whether leave enter (13) out of this or not.
      event->meta = c == 13 ? 0 : TB_META_CTRL;
      event->key = c;
      // event->ch  = c + 97; // we don't want it to be printed

    } else if ('A' <= c && c <= 'Z') {
      event->meta = TB_META_SHIFT;
      event->ch = c;

    } else { // 1, 9, 0
      event->ch = c;
    }

    return 1;

  } else { // either esc or unicode

    nread = 1;
    while (nread < maxseq) {
      rs = read(inout, seq + nread++, 1);
      if (rs == -1) return -1;
      if (rs == 0) break;

      // handle urxvt alt + keys
      if (seq[nread-1] == 27) { // found another escape char!
      	if (seq[nread-2] == 27) { // double esc
      	  if (read(inout, seq + nread++, 1) == 0) { // end of the road, so it's alt+esc
        		event->key  = TB_KEY_ESC;
        		event->meta = TB_META_ALT;
        		return 1;
      	  } // if not end of road, then it must be ^[^[[A (urxvt alt+arrows)
      	} else {
	        cutesc = 1;
	        break;
      	}
      }
    }

    if (nread == maxseq) return 0;
    seq[nread] = '\0';

    if (c == 27 || c == 32539) { // TODO: figure this one out.

/*
	    int i, ch;
	  	// printf("\nseq [%d] --> ", nread); // key);
	    tb_change_cell(2, 1, nread + 48, TB_WHITE, TB_DEFAULT);
	    tb_change_cell(3, 1, ':', TB_WHITE, TB_DEFAULT);

	  	for (i = 1; i < maxseq; i++) {
	  	  ch = i > nread ? ' ' : seq[i] > 0 ? seq[i] : '-';
	      tb_change_cell(4+i, 1, ch, TB_WHITE, TB_DEFAULT);
	  	}
*/

	  	int mouse_parsed = parse_mouse_event(event, seq, nread-1);
	  	if (mouse_parsed != 0)
	  	  return mouse_parsed;

	  	return parse_esc_seq(event, seq, nread-1);

    } else if (nread-1 >= tb_utf8_char_length(seq[0])) {
			/* everything ok, fill event, pop buffer, return success */
			tb_utf8_char_to_unicode(&event->ch, seq);
			event->key = 0;
			// bytebuffer_truncate(inbuf, tb_utf8_char_length(seq[0]));
			return 1;
		} else {
			// unknown sequence
			return -1;
		}
  }
}

/*
static int read_up_to(int n) {
	assert(n > 0);
	const int prevlen = input_buffer.len;
	bytebuffer_resize(&input_buffer, prevlen + n);

	int read_n = 0;
	while (read_n <= n) {
		ssize_t r = 0;
		if (read_n < n) {
			r = read(inout, input_buffer.buf + prevlen + read_n, n - read_n);
		}
#ifdef __CYGWIN__
		// While linux man for tty says when VMIN == 0 && VTIME == 0, read
		// should return 0 when there is nothing to read, cygwin's read returns
		// -1. Not sure why and if it's correct to ignore it, but let's pretend
		// it's zero.
		if (r < 0) r = 0;
#endif
		if (r < 0) {
			// EAGAIN / EWOULDBLOCK shouldn't occur here
			assert(errno != EAGAIN && errno != EWOULDBLOCK);
			return -1;
		} else if (r > 0) {
			read_n += r;
		} else {
			bytebuffer_resize(&input_buffer, prevlen + read_n);
			return read_n;
		}
	}
	assert(!"unreachable");
	return 0;
}
*/

static int wait_fill_event(struct tb_event *event, struct timeval *timeout) {
  int n;
	fd_set events;
	memset(event, 0, sizeof(struct tb_event));

	if (cutesc) { // there's a part of an escape sequence left!
	  n = read_and_extract_event(event);
	  if (n < 0) return -1;
	  if (n > 0) return event->type;
	}

  while (1) {
    FD_ZERO(&events);
    FD_SET(inout, &events);
    FD_SET(winch_fds[0], &events);
    int maxfd  = (winch_fds[0] > inout) ? winch_fds[0] : inout;
    int result = select(maxfd+1, &events, 0, 0, timeout);
    if (!result) return 0;

    if (FD_ISSET(winch_fds[0], &events)) {
      event->type = TB_EVENT_RESIZE;
      int zzz = 0;
      n = read(winch_fds[0], &zzz, sizeof(int));
      buffer_size_change_request = 1;
      get_term_size(&event->w, &event->h);
      return TB_EVENT_RESIZE;
    }

    if (FD_ISSET(inout, &events)) {
      n = read_and_extract_event(event) > 0;
      if (n < 0) return -1;
      if (n > 0) return event->type;
    }

  }
}
