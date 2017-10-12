#include <stdio.h>
#include "../src/termbox.h"

int main() {
#ifdef WITH_TRUECOLOR
	tb_init();
	tb_select_output_mode(TB_OUTPUT_TRUECOLOR);

	int w = tb_width();
	int h = tb_height();
	tb_color bg = 0x000000, fg = 0x000000;

	tb_clear();
	int z = 0;

	for (int y = 1; y < h; y++) {
		for (int x = 1; x < w; x++) {
			uint32_t ch;
			tb_utf8_char_to_unicode(&ch, "x");
			fg = 0;

			if (z % 2 == 0) fg |= TB_BOLD;
			if (z % 3 == 0) fg |= TB_UNDERLINE;
			if (z % 5 == 0) fg |= TB_REVERSE;

			tb_change_cell(x, y, ch, fg, bg);
			bg += 0x000101;
			z++;
		}
		bg += 0x080000;
		if (bg > 0xFFFFFF) {
			bg = 0;
		}
	}

	tb_present();

	while (1) {
		struct tb_event ev;
		int t = tb_poll_event(&ev);

		if (t == -1) {
			break;
		}

		if (t == TB_EVENT_KEY) {
			break;
		}
	}

	tb_shutdown();
	return 0;
#else
	printf("True color support not enabled. Please recompile with WITH_TRUECOLOR option.\n");
	return 1;
#endif
}
