#include <stdio.h>
#include <string.h>
#include "../src/termbox.h"

// static const char chars[] = "nnnnnnnnnnnnnnnnnbbbbbbbbbbbbbbbbbuuuuuuuuuuuuuuuuuBBBBBBBBBBBBBBBBB";
static int line_length = (16 + 1) * 4;

static const tb_color all_attrs[] = {
	0,
	TB_BOLD,
	TB_UNDERLINE,
	TB_BOLD | TB_UNDERLINE,
};

static void draw_line(int x, int y, tb_color bg) {
	int a, c;
	// int current_char = 0;
	for (a = 0; a < 4; a++) {
		for (c = TB_DEFAULT; c <= TB_WHITE; c++) {
			tb_color fg = all_attrs[a] | c;
			// tb_char(x, y, fg, bg, chars[current_char]);
			tb_char(x, y, fg, bg, 'a');
			// current_char = next_char(current_char);
			x++;
		}
	}
}

static void print_combinations_table(int sx, int sy, const tb_color attr) {
	int c;
	for (c = TB_DEFAULT; c <= TB_BLACK; c++) {
		tb_color bg = attr | c;
		draw_line(sx, sy, bg);
		sy++;
	}
}

static void draw_all() {
	tb_resize();

	uint8_t w = line_length; // strlen(chars)+1;
	uint8_t h = 18; // 16 colors plus space

	tb_string(1, 0, 0, 0, "Normal");
	print_combinations_table(1, (h * 0) + 1, 0);
	// print_combinations_table(1, (h * 1) + 1, TB_BOLD);

	tb_string(1, (h * 1)+1, 0, 0, "Normal (inverted)");
	print_combinations_table(1, (h * 1) + 2, TB_REVERSE);
	tb_render();

	int c, x, y = 1;
	tb_string(w, 0, 0, 0, "256 color");

	for (c = 0, x = 0; c < 256; c++, x++) {
		if ((c == 8 || c == 16) || (c > 16 && (c - 16) % 6 == 0)) {
			x = 0;
			y++;
		}

		// tb_char(x, y, c | ((y & 1) ? TB_UNDERLINE : 0), 0, '+');
		tb_stringf(w + x*4, y, 0, c, "%3d", c);
	}

	tb_render();
}

int main(int argc, char **argv) {
	(void)argc; (void)argv;
	int ret = tb_init();
	if (ret) {
		fprintf(stderr, "tb_init() failed with error code %d\n", ret);
		return 1;
	}

	draw_all();

	struct tb_event ev;
	while (tb_poll_event(&ev)) {
		switch (ev.type) {
		case TB_EVENT_KEY:
			switch (ev.key) {
			case TB_KEY_ESC:
				goto done;
				break;
			}
			break;
		case TB_EVENT_RESIZE:
			draw_all();
			break;
		}
	}
done:
	tb_shutdown();
	return 0;
}
