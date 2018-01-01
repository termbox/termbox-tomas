#pragma once

#include <err.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

// #include "menu.h"

int sort = 1;

int isu8cont(unsigned char c) {
  return MB_CUR_MAX > 1 && (c & (0x80 | 0x40)) == 0x80;
}

int isu8start(unsigned char c) {
  return MB_CUR_MAX > 1 && (c & (0x80 | 0x40)) == (0x80 | 0x40);
}

/*
 * Returns a pointer to first occurrence of the first character in s2 in s1 with
 * respect to Unicode characters, ANSI escape sequences and disregarding case.
 */
const char * strcasechr(const char *s1, const char *s2) {
  wchar_t wc1, wc2;
  size_t  i;
  int in_esc_seq, nbytes, res;

  res = mbtowc(&wc2, s2, MB_CUR_MAX);
  switch (res) {
  case -1:
    res = mbtowc(NULL, NULL, MB_CUR_MAX);
  /* FALLTHROUGH */
  case 0:
    return NULL;
  }

  in_esc_seq = 0;
  for (i = 0; s1[i] != '\0';) {
    nbytes = 1;

    if (in_esc_seq) {
      if (s1[i] >= '@' && s1[i] <= '~')
        in_esc_seq = 0;
    } else if (i > 0 && s1[i - 1] == '\033' && s1[i] == '[') {
      in_esc_seq = 1;
    } else if ((nbytes = mbtowc(&wc1, &s1[i], MB_CUR_MAX)) == -1) {
      res = mbtowc(NULL, NULL, MB_CUR_MAX);
    } else if (wcsncasecmp(&wc1, &wc2, 1) == 0) {
      return &s1[i];
    }

    if (nbytes > 0)
      i += nbytes;
    else
      i++;
  }

  return NULL;
}

size_t min_match(char * q, size_t query_len, const char *string, size_t offset, ssize_t *start, ssize_t *end) {
  const char  *e, *s;
  size_t length;

  if ((s = e = strcasechr(&string[offset], q)) == NULL)
    return INT_MAX;

  for (;;) {
    for (e++, q++; isu8cont(*q); e++, q++)
      continue;
    if (*q == '\0')
      break;
    if ((e = strcasechr(e, q)) == NULL)
      return INT_MAX;
  }

  length = e - s;
  /* LEQ is used to obtain the shortest left-most match. */
  if (length == query_len || length <= min_match(q, query_len, string, s - string + 1, start, end)) {
    *start = s - string;
    *end = e - string;
  }

  return length;
}

int choicecmp(const void *p1, const void *p2) {
  const struct choice *c1, *c2;

  c1 = p1, c2 = p2;
  if (c1->score < c2->score)
    return 1;
  if (c1->score > c2->score)
    return -1;

  return c1->string - c2->string;
}

/*
 * Filter choices using the current query while there is no new user input
 * available.
 */
void filter_choices(char * query, size_t query_len) {
  struct choice *c;
  // struct pollfd  pfd;
  size_t i, match_length;
  // int nready;

  for (i = 0; i < choices.length; i++) {
    c = &choices.v[i];
    if (min_match(query, query_len, c->string, 0, &c->match_start, &c->match_end) == INT_MAX) {
      c->match_start = c->match_end = -1;
      c->score = 0;
    } else if (!sort) {
      c->score = 1;
    } else {
      match_length = c->match_end - c->match_start;
      c->score = (float)query_len / match_length / c->length;
    }

    /*
     * Regularly check if there is any new user input available. If
     * true, abort filtering since the currently used query is
     * outdated. This improves the performance when the cardinality
     * of the choices is large.
     */
/*
    if (i > 0 && i % 50 == 0) {
      pfd.fd = fileno(tty_in);
      pfd.events = POLLIN;
      if ((nready = poll(&pfd, 1, 0)) == -1)
        err(1, "poll");
      if (nready == 1 && pfd.revents & (POLLIN | POLLHUP))
        break;
    }
*/

  }

  qsort(choices.v, choices.length, sizeof(struct choice), choicecmp);
}
