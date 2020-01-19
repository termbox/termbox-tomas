#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for strcasestr
#endif

// comment this below to disable fuzzy search
#define FUZZY_SEARCH 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "lib/menu.h"

#include <pthread.h>
#include <dirent.h>
#include <signal.h>
#include "../src/termbox.h"

////////////////////////////////////////////////////////////////
// menu code

#include <stdarg.h>
#include <unistd.h>

#include <errno.h>  // for ENOMEM
#include <stdint.h> // for SIZE_MAX
#include <stdlib.h> // for realloc

#ifdef FUZZY_SEARCH
#include "lib/filter.h"
#endif

int num_lines = 0;
int selected = 0;
int scroll_pos  = 0;
int num_choices = 0;

#ifndef HAVE_REALLOCARRAY

/*
 * This is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
 * if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
 */
#define MUL_NO_OVERFLOW ((size_t)1 << (sizeof(size_t) * 4))

void * reallocarray(void *optr, size_t nmemb, size_t size) {
  if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
      nmemb > 0 && SIZE_MAX / nmemb < size) {
    errno = ENOMEM;
    return NULL;
  }
  return realloc(optr, size * nmemb);
}

#endif /* !HAVE_REALLOCARRAY */

int add_choice(const char * str) {

  // make room for more choices if not enough available
  if (choices.length == choices.size) {
    choices.size *= 2;
    if ((choices.v = reallocarray(choices.v, choices.size, sizeof(struct choice))) == NULL) {
      fprintf(stderr, "Cannot allocate more mem.\n");
      return -1;
    }
  }

  choices.v[choices.length].string = strdup(str);
#ifdef FUZZY_SEARCH
  choices.v[choices.length].length = strlen(str);
  choices.v[choices.length].match_start = -1;
  choices.v[choices.length].match_end = -1;
  choices.v[choices.length].score = 0;
#endif

  return choices.length++;
}

////////////////////////////////////////////////////////////

void cursor_up(int times) {
  tb_sendf("\033[%dA", times);
}

void cursor_down(int times) {
  tb_sendf("\033[%dB", times);
}

void cursor_right(int times) {
  tb_sendf("\033[%dC", times);
}

void cursor_left(int times) {
  tb_sendf("\033[%dD", times);
}

void clear_eol() {
  tb_sendf("\033[K");
}

void print_clear(const char * str) {
  tb_sendf("%s\033[K", str);
}

void print_line(const char * str) {
  tb_sendf("%s\033[K\n", str);
  cursor_left(tb_width());
}

void tb_menu_reset() {
  selected = 0;
  scroll_pos = 0;
}

void tb_menu_prompt(char * query, int len, int pos) {
  cursor_left(tb_width());
  tb_sendf(" Choose from %d options: ", num_choices);

  if (query != NULL) {
    query[len] = '\0';
    print_clear(query);
    if (pos < len) {
      cursor_left(len - pos);
    }
  }

  tb_show_cursor();
}

void tb_menu_redraw(int lines) {
  uint8_t i;
  int w = tb_width();
  int prev_lines = 0;

#ifndef FUZZY_SEARCH
  char line[w];
#endif

  cursor_down(1);
  cursor_left(w);

  if (lines > 0)
    num_lines = lines;

  for (i = scroll_pos; i < scroll_pos + num_lines; i++) {
    if (choices.length <= i) {
      print_line("");
      continue;
    }

    if (i == selected) tb_send("\033[7m"); // reverse

#ifdef FUZZY_SEARCH
    uint8_t col = 0;
    tb_sendf(" [%d] ", i);

    while (col < (w - 3) && col < choices.v[i].length) {
      if (col == choices.v[i].match_start)
        tb_send("\033[4m");
      else if (col == choices.v[i].match_end)
        tb_send("\033[24m");

      tb_sendf("%c", choices.v[i].string[col]);
      col++;
    }
    print_line("");
#else
    sprintf(line, " [%d] %s", i+1, choices.v[i].string);
    print_line(line);
#endif

    tb_send("\033[0m"); // reset
  }

  if (prev_lines && prev_lines > num_lines) {
    for (i = 0; i < prev_lines - num_lines; i++) {
      print_line("");
    }

    clear_eol();
    cursor_up(prev_lines - num_lines);
  } else {
    cursor_up(num_lines + 1);
  }

  tb_hide_cursor();
  tb_flush();
}

void tb_menu_remove_option(int idx) {
  if (idx >= num_choices)
    return;
}

int tb_menu_add_option(const char * option) {
  add_choice(option);
  return ++num_choices;
}

int tb_menu_update_options(int count, const char * list[]) {
  tb_menu_reset();

  int i;
  for (i = 0; i < count; i++) {
    tb_menu_add_option(list[i]);
  }

  return i;
}

void tb_menu_move_up(int count) {
  (void)count; // silence unused var warnings

  if (selected > 0) {
    if (selected - 1 < scroll_pos)
      scroll_pos--;

    --selected;
    tb_menu_redraw(0);
  }
}

void tb_menu_move_down(int count) {
  (void)count; // silence unused var warnings

  if (selected < num_choices) {
   if (selected + 2 > (scroll_pos + num_lines)) {
      if (scroll_pos + num_lines < num_choices) {
        scroll_pos++;
      } else {
        return;
      }
    }

    ++selected;
    tb_menu_redraw(0);
  }
}

int tb_menu_init(void) {
  choices.size = 16;
  if ((choices.v = reallocarray(NULL, choices.size, sizeof(struct choice))) == NULL) {
    fprintf(stderr, "Cannot continue\n");
    return 1;
  }

  return 0;
}

int tb_menu_print_selected(void) {
  clear_eol();
  tb_flush();
  fprintf(stderr, "%s", choices.v[selected].string);
  return 1;
}

int tb_menu_get_num_choices(void) {
  return num_choices;
}

////////////////////////////////////////////////////////////////

#define newline() printf("\n");

int default_lines = 5;
int last_choices;
static int search_type = 1; // directory

pthread_t tid;
int follow_symlinks = 1;
int max_depth = 3;
int max_length = 128;

#define true 1
#define false 0

#define SUPPORTED_FORMATS 12
const char * valid_extensions[SUPPORTED_FORMATS] = { ".js", ".rb", ".html", ".css", ".erb", "file", ".yml", ".conf", ".py", ".c", ".h", ".cpp" };

#define INVALID_DIRS 6
const char * skip_dirs[INVALID_DIRS] = { ".git", ".gitbackup", "node_modules", ".cache", ".npm", ".config" };

// char last_search[32]; // = NULL;
int counter, offset = 0;

extern char *basename(const char *path) {
  const char *s = strrchr(path, '/');
  if (!s)
    return strdup(path);
  else
    return strdup(s + 1);
}

int ends_with(const char *str, const char *end) {
  size_t slen = strlen(str);
  size_t elen = strlen(end);
  if (slen < elen)
    return 0;

  return (strcasestr(&(str[slen-elen]), end) != NULL);
}

int valid_file(const char * filename) {
  int i;
  for (i = 0; i < SUPPORTED_FORMATS; i++) {
    if (ends_with(filename, valid_extensions[i]))
      return true;
  }
  return false;
}

int is_directory(const char * path) {
  struct stat statbuf;
  if (stat(path, &statbuf) != 0)
    return 0;

  return S_ISDIR(statbuf.st_mode);
}

int should_skip_dir(const char * name) {
  int i;

  for (i = 0; i < INVALID_DIRS; i++) {
    if (strcmp(name, skip_dirs[i]) == 0) {
      // printf("Skipping dir: %s\n", name);
      return true;
    }
  }

  return false;
}

int add_dir(const char * path) {
  if (search_type != 1) return true;
  tb_menu_add_option(path);
  // printf("Dir: %s -> %s\n", basename(path), path);
  return true;
}

int add_file(const char * path) {
  if (search_type != 2) return true;
  tb_menu_add_option(path);
  // printf("File: %s -> %s\n", basename(path), path);
  return true;
}

int walk_dir(const char * path, int depth) {
  DIR* dir;
  struct dirent *ent;
  int success = 1;

  if (depth > max_depth)
    return success;

  if ((dir = opendir(path)) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if (should_skip_dir(ent->d_name))
        continue;

      char * full_path = (char*)malloc(strlen(path) + strlen(ent->d_name) + 2);
      sprintf(full_path, "%s/%s", path, ent->d_name);

      // if we have a directory or a symlink to a directory, and we're following them
      if (ent->d_type == DT_DIR || (ent->d_type == DT_LNK && follow_symlinks && is_directory(full_path))) {
        // make sure we only follow valid directories
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
          add_dir(full_path);
          walk_dir(full_path, depth+1);
        }
      } else if ((ent->d_type == DT_REG || ent->d_type == DT_LNK) && valid_file(ent->d_name)) {
        if (!add_file(full_path)) {
          success = false;
          break;
        }
      }
    }
    closedir(dir);
  }

  return success;
}

int process_path(const char * path) {
  if (is_directory(path)) {
    if (!walk_dir(path, 0)) {
      return -1;
    }
  } else if (valid_file(path)) {
    if (!add_file(path)) {
      return -1;
    }
  }
  return 0;
}

void process_list(int len, const char * list[]) {
  int i;
  for (i = 0; i < len; i++) {
    if (process_path(list[i]) == -1)
      break;
  }
}

void * get_choices(void * arg) {
  char * path = arg;
  process_path(path);
  return NULL;
}

void get_choices_in_parallel(const char * argv) {
  char * dir = argv == NULL ? "." : (char *)argv;
  pthread_create(&tid, NULL, get_choices, dir);
}

void stop_thread() {
#ifdef __ANDROID__
  pthread_kill(tid, SIGTERM);
#else
  pthread_cancel(tid);
#endif
}

void update_query(char * query, int len, int pos) {
  query[len] = '\0';
#ifdef FUZZY_SEARCH
  filter_choices(query, (size_t)len);
#endif

  tb_menu_reset();
  tb_menu_prompt(query, len, pos);
  tb_menu_redraw(0);
}

int draw_and_get_input() {

  char * query;
  query = (char *)malloc(max_length * sizeof(char));

  int pos = 0, query_len = 0;
  struct tb_event ev;

#ifdef FUZZY_SEARCH
  int char_len = 1; // length of char
  char buf[3];
#endif

  while (1) {

    // while (get_input(&ev)) {
    while (tb_peek_event(&ev, 150)) {

      switch (ev.type) {
      case TB_EVENT_KEY:
        switch (ev.key) {

          case TB_KEY_CTRL_C:
          case TB_KEY_CTRL_D:
          case TB_KEY_ESC:
            goto done;
            break;

          case TB_KEY_ARROW_DOWN:
            tb_menu_move_down(1);
            break;
          case TB_KEY_ARROW_UP:
            tb_menu_move_up(-1);
            break;
          case TB_KEY_ENTER:
            goto submit;
            break;

#ifdef FUZZY_SEARCH
          case TB_KEY_DELETE:
            break;
          case TB_KEY_BACKSPACE:
          // case TB_KEY_CTRL_BACKSPACE:
            if (pos > 0) {
              query_len -= char_len;
              pos -= char_len;
              update_query(query, query_len, pos);
            }
            break;
          case TB_KEY_ARROW_LEFT:
            if (pos > 0) {
              pos -= 1;
              cursor_left(1);
            }
            break;
          case TB_KEY_ARROW_RIGHT:
            if (pos < query_len) {
              pos += 1;
              cursor_right(1);
            }
            break;

          default:
            if (query_len < max_length) {
              if (pos < query_len)
                memmove(query + pos + char_len, query + pos, query_len - pos);

              // convert spaces to /
              sprintf(buf, "%c", ev.ch == 32 ? '/' : ev.ch);

              memcpy(query + pos, buf, char_len);
              pos += char_len;
              query_len += char_len;

              update_query(query, query_len, pos);
            }
            break;
#else
          default:
            break;
#endif
        }
        break;
      case TB_EVENT_RESIZE:
        // draw_window(num_lines, scroll_pos, sel);
        break;
      }
    }

#ifdef FUZZY_SEARCH
    int choice_list = tb_menu_get_num_choices();

    if (query_len > 0 && choice_list != last_choices) {
      update_query(query, query_len, pos);
    }

    last_choices = choice_list;
#endif

    tb_menu_redraw(0);
    tb_menu_prompt(query, query_len, pos);
  }

submit:
  // query[query_len] = '\0';
  tb_menu_print_selected();

done:
  return 0;
}

void shutdown(void) {
  stop_thread();

  cursor_down(num_lines);
  cursor_left(tb_width());

  // reset signals
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  tb_shutdown();
}

void error_exit(char * msg) {
  shutdown();
  (void)msg;
  // printf("%s\n", msg);
  exit(1);
}

void signal_exit(int signal_code) {
  (void)signal_code;
  char * msg = "Exit signal received.";
  error_exit(msg);
}

int main(int argc, const char * argv[]) {
  if (tb_init_file("/dev/tty") != 0) {
    printf("Unable to set up terminal\n.");
    return 1;
  }

  tb_init_screen(TB_INIT_KEYPAD);
  setbuf(stdout, NULL); // disable buffering on stdout

  signal(SIGTERM, signal_exit);
  signal(SIGINT, signal_exit);

  tb_menu_init();

  // if -f is second argument, search files instead of dirs
  if (argc > 2 && strcmp(argv[2], "-f") == 0) {
    search_type = 2; // files
  }

  get_choices_in_parallel(argv[1]);

  newline();
  tb_menu_redraw(default_lines);
  draw_and_get_input();

  shutdown();
  newline();
  return 0;
}
