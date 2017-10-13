#pragma once

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

void filter_choices(char * query, size_t len);

