// if s1 starts with s2 returns true, else false
// len is the length of s1
// s2 should be null-terminated
static bool starts_with(const char *s1, int len, const char *s2)
{
	int n = 0;
	while (*s2 && n < len) {
		if (*s1++ != *s2++)
			return false;
		n++;
	}
	return *s2 == 0;
}

static int parse_mouse_event(struct tb_event *event, const char *buf, int len) {
	if (len >= 6 && starts_with(buf, len, "\033[M")) {
		// X10 mouse encoding, the simplest one
		// \033 [ M Cb Cx Cy
		int b = buf[3] - 32;
		switch (b & 3) {
		case 0:
			if ((b & 64) != 0)
				event->key = TB_KEY_MOUSE_WHEEL_UP;
			else
				event->key = TB_KEY_MOUSE_LEFT;
			break;
		case 1:
			if ((b & 64) != 0)
				event->key = TB_KEY_MOUSE_WHEEL_DOWN;
			else
				event->key = TB_KEY_MOUSE_MIDDLE;
			break;
		case 2:
			event->key = TB_KEY_MOUSE_RIGHT;
			break;
		case 3:
			event->key = TB_KEY_MOUSE_RELEASE;
			break;
		default:
			return -6;
		}
		event->type = TB_EVENT_MOUSE; // TB_EVENT_KEY by default
		if ((b & 32) != 0)
			event->mod |= TB_MOD_MOTION;

    // if (event->key != TB_KEY_MOUSE_RELEASE)
      event->meta = (b >> 2) - 7;

		// the coord is 1,1 for upper left
		event->x = (uint8_t)buf[4] - 1 - 32;
		event->y = (uint8_t)buf[5] - 1 - 32;

		return 6;
	} else if (starts_with(buf, len, "\033[<") || starts_with(buf, len, "\033[")) {
		// xterm 1006 extended mode or urxvt 1015 extended mode
		// xterm: \033 [ < Cb ; Cx ; Cy (M or m)
		// urxvt: \033 [ Cb ; Cx ; Cy M
		int i, mi = -1, starti = -1;
		int isM, isU, s1 = -1, s2 = -1;
		int n1 = 0, n2 = 0, n3 = 0;

		for (i = 0; i < len; i++) {
			// We search the first (s1) and the last (s2) ';'
			if (buf[i] == ';') {
				if (s1 == -1)
					s1 = i;
				s2 = i;
			}

			// We search for the first 'm' or 'M'
			if ((buf[i] == 'm' || buf[i] == 'M') && mi == -1) {
				mi = i;
				break;
			}
		}
		if (mi == -1)
			return 0;

		// whether it's a capital M or not
		isM = (buf[mi] == 'M');

		if (buf[2] == '<') {
			isU = 0;
			starti = 3;
		} else {
			isU = 1;
			starti = 2;
		}

		if (s1 == -1 || s2 == -1 || s1 == s2)
			return 0;

		n1 = strtoul(&buf[starti], NULL, 10);
		n2 = strtoul(&buf[s1 + 1], NULL, 10);
		n3 = strtoul(&buf[s2 + 1], NULL, 10);

		if (isU)
			n1 -= 32;

		switch (n1 & 3) {
		case 0:
			if ((n1&64) != 0) {
				event->key = TB_KEY_MOUSE_WHEEL_UP;
			} else {
				event->key = TB_KEY_MOUSE_LEFT;
			}
			break;
		case 1:
			if ((n1&64) != 0) {
				event->key = TB_KEY_MOUSE_WHEEL_DOWN;
			} else {
				event->key = TB_KEY_MOUSE_MIDDLE;
			}
			break;
		case 2:
			event->key = TB_KEY_MOUSE_RIGHT;
			break;
		case 3:
			event->key = TB_KEY_MOUSE_RELEASE;
			break;
		default:
			return mi + 1;
		}

		if (!isM) {
			// on xterm mouse release is signaled by lowercase m
			event->key = TB_KEY_MOUSE_RELEASE;
		}

		event->type = TB_EVENT_MOUSE; // TB_EVENT_KEY by default
		if ((n1&32) != 0)
			event->mod |= TB_MOD_MOTION;

    // if (event->key != TB_KEY_MOUSE_RELEASE)
      event->meta = (n1 >> 2) - 7;

		event->x = (uint8_t)n2 - 1;
		event->y = (uint8_t)n3 - 1;

		return mi + 1;
	}

	return 0;
}

// #define DEBUGGING 1
// function ported from tcolar's termbox-go fork:
// https://github.com/tcolar/termbox-go/commit/8e386c1b6b783dee1904d688347e2d282450b8e7

// example bufs:

//       xterm:
//        27, 91, 49, 59, 50, 65  --> shift + up arrow
//        27, 91, 49, 59, 50, 66  --> shift + down arrow
//       esc   [  1   ;   mod key

//       urxvt:
//        27  91  98 -13 -91, 126
//        27  91  98 -95  40  127 <-- with URxvt.iso14755 = false
//       esc mod key

//       mrxvt:
//        27, 91, 66, 59, 50, 66
//        27, 91, 49, 59, 50, 66

static bool parse_meta_key(struct tb_event *event, const char *buf, const char *key) {

	unsigned int kl = strlen(key);

#ifdef DEBUGGING
	int i;
	printf("\nbuf: %s --> ", ""); // buf);
	for (i = 0; i < strlen(buf); i++)
		printf("%d ", buf[i]);

	printf("\nkey: %s --> ", ""); // key);
	for (i = 0; i < strlen(key); i++)
		printf("%d ", key[i]);
#endif

	if (strlen(buf) < kl+2) {
		return false;
	}

	if (buf[0] != key[0] || buf[1] != key[1]) {
		return false;
	}

	if (buf[kl-1] != 59) { // ';'
		return false;
	}

	if (buf[kl] < 50 || buf[kl] > 57) { // 2 to 9 ASCII
		return false;
	}

	if (buf[kl+1] != key[kl-1]) {
		// printf("%d != %d\n", buf[kl+1], key[kl-1]);
		return false;
	}

	event->ch = 0;
	event->meta = buf[kl] - 48;

	return true;
}

#define known_codes_length 30

const char * known_codes[known_codes_length] = {
	"\033[Z", // Shift+TAB
	// "\033[2$", // Shift+Insert
	"\033[3$", // Shift+Delete
	"\033[5$", // Shift+PageUp
	"\033[6$", // Shift+PageDown
	"\033[7$", // Shift+Home
	"\033[8$", // Shift+End
  "\033[2^", // Ctrl+Insert
	"\033[3^", // Ctrl+Delete
	"\033[5^", // Ctrl+PageUp
	"\033[6^", // Ctrl+PageDown
	"\033[7^", // Ctrl+Home
	"\033[8^", // Ctrl+End
	"\033\033[a", // Alt+Shift+Up
	"\033\033[b", // Alt+Shift+Down
	"\033\033[c", // Alt+Shift+Left
	"\033\033[d", // Alt+Shift+Right
	"\033[a", // Shift+Up
	"\033[b", // Shift+Down
	"\033[c", // Shift+Left
	"\033[d", // Shift+Right
	"\033Oa", // Control+Up
	"\033Ob", // Control+Down
	"\033Oc", // Control+Left
	"\033Od",  // Control+Right

	// mrxvt
	// "\033[7;5~", // Ctrl+Home
	// "\033[8;5~", // Ctrl+End
	"\033[5;6~", // CtrlShift+PageUp
	"\033[6;6~", // CtrlShift+PageDown
	"\033[7;6~", // CtrlShift+Home
	"\033[8;6~", // CtrlShift+End

  // xterm
	"\033[5;5~", // Ctrl+PageUp
	"\033[6;5~", // Ctrl+PageDown
	// "\033OH",    // Ctrl+Home (xterm)
	// "\033OF",    // Ctrl+End (xterm)
};

int known_codes_keys[known_codes_length][2] = {
	// global, apparently
	{ TB_KEY_TAB,          TB_META_SHIFT },

	// urxvt
	// { TB_KEY_INSERT,      TB_META_SHIFT },
	{ TB_KEY_DELETE,      TB_META_SHIFT },
	{ TB_KEY_PGUP,        TB_META_SHIFT },
	{ TB_KEY_PGDN,        TB_META_SHIFT },
	{ TB_KEY_HOME,        TB_META_SHIFT },
	{ TB_KEY_END,         TB_META_SHIFT },
	{ TB_KEY_INSERT,      TB_META_CTRL },
	{ TB_KEY_DELETE,      TB_META_CTRL },
	{ TB_KEY_PGUP,        TB_META_CTRL },
	{ TB_KEY_PGDN,        TB_META_CTRL },
	{ TB_KEY_HOME,        TB_META_CTRL },
	{ TB_KEY_END,         TB_META_CTRL },
	{ TB_KEY_ARROW_UP,    TB_META_ALTSHIFT },
	{ TB_KEY_ARROW_DOWN,  TB_META_ALTSHIFT },
	{ TB_KEY_ARROW_RIGHT, TB_META_ALTSHIFT },
	{ TB_KEY_ARROW_LEFT,  TB_META_ALTSHIFT },
	{ TB_KEY_ARROW_UP,    TB_META_SHIFT },
	{ TB_KEY_ARROW_DOWN,  TB_META_SHIFT },
	{ TB_KEY_ARROW_RIGHT, TB_META_SHIFT },
	{ TB_KEY_ARROW_LEFT,  TB_META_SHIFT },
	{ TB_KEY_ARROW_UP,    TB_META_CTRL  },
	{ TB_KEY_ARROW_DOWN,  TB_META_CTRL  },
	{ TB_KEY_ARROW_RIGHT, TB_META_CTRL  },
	{ TB_KEY_ARROW_LEFT,  TB_META_CTRL  },

	// mrxvt
	{ TB_KEY_PGDN,        TB_META_CTRLSHIFT },
	{ TB_KEY_PGUP,        TB_META_CTRLSHIFT },
	{ TB_KEY_HOME,        TB_META_CTRLSHIFT },
	{ TB_KEY_END,         TB_META_CTRLSHIFT },

	// xterm
	{ TB_KEY_PGDN,        TB_META_CTRL },
	{ TB_KEY_PGUP,        TB_META_CTRL },
	// { TB_KEY_HOME,        TB_META_CTRL },
	// { TB_KEY_END,         TB_META_CTRL }
};

// convert escape sequence to event, and return consumed bytes on success (failure == 0)
static int parse_escape_seq(struct tb_event *event, const char *buf, int len)
{
	int mouse_parsed = parse_mouse_event(event, buf, len);

	if (mouse_parsed != 0)
		return mouse_parsed;

	// it's pretty simple here, find 'starts_with' match and return
	// success, else return failure
	int i;

  // printf("term: %s\n", term_name);
  int is_linux = strcmp(term_name, "linux") == 0 ? 1 : 0;

  if (is_linux) {
		for (i = 0; i < known_codes_length; i++) {
			// printf("\nbuf: [%d] --> ", len); for (int a = 0; a < len; a++) { printf("%d ", buf[a]); }
			if (starts_with(buf, len, known_codes[i])) {
				// printf(" ------ found: %d\n", i);
				event->ch   = 0;
				event->key  = known_codes_keys[i][0];
				event->meta = known_codes_keys[i][1];
				return strlen(known_codes[i]);
			}
		}
	}

	char desc[20];
	for (i = 0; keys[i]; i++) {

		if (starts_with(buf, len, keys[i])) {
			event->ch = 0;
			event->key = 0xFFFF-i;
			return strlen(keys[i]);
		}

    if (is_linux) continue;

		if (parse_meta_key(event, buf, keys[i])) {
			event->key = 0xFFFF-i;
			return strlen(keys[i]) + 2;
		}

		if (keys[i][1] == 79 || (keys[i][1] == 91 && strlen(keys[i]) == 3)) { // 'O'
			// For some crazy reason xterm sends LeftArrow as [27,79,68]
			// but Shift+LeftArrow as [27,91,49,59,50,68]
			// the extra [59, 50] was expected but not the [79] -> [91,49]
			// Basically seems to be sent in SS3 format in the first case
			// but in CSI format in the second !
			// http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-PC-Style-Function-Keys
			int keylen = strlen(keys[i]);
			char k2[4]; // k2[keylen+1];
			k2[0] = keys[i][0];
			k2[1] = 91;
			k2[2] = 49;

			int a;
			for (a = 2; a < keylen; a++) {
				k2[a+1] = keys[i][a];
			}

			sprintf(desc, " k2 (%d): %d, %d, %d", (int)strlen(k2), k2[0], k2[1], k2[2]);
			if (parse_meta_key(event, buf, k2)) {
				if (buf[2] != 49) {
					switch(buf[2]) {
						case 66:
						  event->key = TB_KEY_ARROW_DOWN;
						  break;
						case 67:
							event->key = TB_KEY_ARROW_RIGHT;
							break;
						case 68:
							event->key = TB_KEY_ARROW_LEFT;
							break;
						default:
							break;
					}
					event->meta = 0;
				} else {
					event->key = 0xFFFF - i;
				}

				return strlen(k2) + 2;
			}
		}
	}
	return 0;
}

static bool extractx_event(struct tb_event *event, struct bytebuffer *inbuf, int inputmode)
{
	const char *buf = inbuf->buf;
	const int len = inbuf->len;
	if (len == 0)
		return false;

	if (buf[0] == '\033') {

		int n = parse_escape_seq(event, buf, len);
		if (n != 0) {
			bool success = true;
			if (n < 0) {
				success = false;
				n = -n;
			}
			bytebuffer_truncate(inbuf, n);
			return success;
		} else {
			// it's not escape sequence, then it's ALT or ESC,
			// check inputmode
			if (inputmode&TB_INPUT_ESC) {
				// if we're in escape mode, fill ESC event, pop
				// buffer, return success
				event->ch = 0;
				event->key = TB_KEY_ESC;
				event->mod = 0;
				bytebuffer_truncate(inbuf, 1);
				return true;
			} else if (inputmode&TB_INPUT_ALT) {
				// if we're in alt mode, set ALT modifier to
				// event and redo parsing
				event->mod = TB_MOD_ALT;
				bytebuffer_truncate(inbuf, 1);
				return extractx_event(event, inbuf, inputmode);
			}
			assert(!"never got here");
		}
	}

	// if we're here, this is not an escape sequence and not an alt sequence
	// so, it's a FUNCTIONAL KEY or a UNICODE character

	// first of all check if it's a functional key
	if ((unsigned char)buf[0] <= TB_KEY_SPACE ||
			(unsigned char)buf[0] == TB_KEY_BACKSPACE2)
	{
		// fill event, pop buffer, return success */
		event->ch = 0;
		event->key = (uint16_t)buf[0];
		bytebuffer_truncate(inbuf, 1);
		return true;
	}

	// feh... we got utf8 here

	// check if there is all bytes
	if (len >= tb_utf8_char_length(buf[0])) {
		/* everything ok, fill event, pop buffer, return success */
		tb_utf8_char_to_unicode(&event->ch, buf);
		event->key = 0;
		bytebuffer_truncate(inbuf, tb_utf8_char_length(buf[0]));
		return true;
	}

	// event isn't recognized, perhaps there is not enough bytes in utf8
	// sequence
	return false;
}
