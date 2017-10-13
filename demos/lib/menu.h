#pragma once

#include <errno.h>  // for ENOMEM
#include <stdint.h> // for SIZE_MAX
#include <stdlib.h> // for realloc

extern int num_lines;
extern int selected;

struct choice {
  const char  *string;
#ifdef FUZZY_SEARCH
  size_t     length;
  ssize_t    match_start; /* inclusive match start offset */
  ssize_t    match_end; /* exclusive match end offset */
  double     score;
#endif
};

static struct {
  size_t size;
  size_t length;
  struct choice *v;
} choices;

void cursor_up(int times);
void cursor_down(int times);
void cursor_right(int times);
void cursor_left(int times);

void tb_menu_prompt(char * query, int len, int pos);
void tb_menu_redraw(int lines);
int tb_menu_add_option(const char * option);
void tb_menu_move_up(int count);
void tb_menu_move_down(int count);
int tb_menu_init(void);
void tb_menu_reset(void);
int tb_menu_print_selected(void);
int tb_menu_get_num_choices(void);

void filter_choices(char * query, size_t len);
