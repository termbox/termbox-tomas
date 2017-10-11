# Termbox

A small library for building text-based user interfaces. This is a fork of nsf's [original work](https://github.com/nsf/termbox), with a few changes:

 - Better input handling, so Ctrl/Alt/Shift key combos are property detected
 - Additional init options, (e.g. skip clearing full screen, cursor hiding, etc)
 - True color support (soon)

## Installation

First thing's first:

    git clone github.com/tomas/termbox
    cd termbox

Now run cmake to generate the Makefiles.

    mkdir build
    cd build
    cmake ..

And release the kraken:

    make
    make install

## Usage

Termbox's has a very clean interface. Here's your basic 'hello world':

    #include <unistd.h> // for sleep()
    #include "termbox.h"

    int main(int argc, char **argv) {
      if (tb_init() != 0) {
        return 1; // couldn't initialize our screen
      }

      // set up our colors and test string
      int bg_color = TB_DEFAULT;
      int fg_color = TB_DEFAULT;
      char str[13] = "Hello world!";

      // get the screen resolution
      int w = tb_width();
      int h = tb_height();

      // now, put some text in the middle of the screen
      tb_print((w/2)-6, h/2, bg_color, fg_color, str);

      // flush the output to the screen
      tb_present();

      // wait a few secs
      sleep(3);

      // and finally, shutdown the screen so we exit cleanly
      tb_shutdown();

      return 0;
    }


## Links

- https://github.com/nsf/termbox - Original version of this library
- http://pecl.php.net/package/termbox - PHP Termbox wrapper
- https://github.com/nsf/termbox-go - Go pure Termbox implementation
- https://github.com/gchp/rustbox - Rust Termbox wrapper
- https://github.com/fouric/cl-termbox - Common Lisp Termbox wrapper
- https://github.com/zyedidia/termbox-d - D Termbox wrapper

## Bugs

Report bugs to the https://github.com/tomas/termbox issue tracker.

## Changelog

v1.1.0:

- API: tb_width() and tb_height() are guaranteed to be negative if the termbox
  wasn't initialized.
- API: Output mode switching is now possible, adds 256-color and grayscale color
  modes.
- API: Better tb_blit() function. Thanks, Gunnar Zötl <gz@tset.de>.
- API: New tb_cell_buffer() function for direct back buffer access.
- API: Add new init function variants which allow using arbitrary file
  descriptor as a terminal.
- Improvements in input handling code.
- Calling tb_shutdown() twice is detected and results in abort() to discourage
  doing so.
- Mouse event handling is ported from termbox-go.
- Paint demo port from termbox-go to demonstrate mouse handling capabilities.
- Bug fixes in code and documentation.

v1.0.0:

- Remove the Go directory. People generally know about termbox-go and where
  to look for it.
- Remove old terminfo-related python scripts and backport the new one from
  termbox-go.
- Remove cmake/make-based build scripts, use waf.
- Add a simple terminfo database parser. Now termbox prefers using the
  terminfo database if it can be found. Otherwise it still has a fallback
  built-in database for most popular terminals.
- Some internal code cleanups and refactorings. The most important change is
  that termbox doesn't leak meaningless exported symbols like 'keys' and
  'funcs' now. Only the ones that have 'tb_' as a prefix are being exported.
- API: Remove unsigned ints, use plain ints instead.
- API: Rename UTF-8 functions 'utf8_*' -> 'tb_utf8_*'.
- API: TB_DEFAULT equals 0 now, it means you can use attributes alones
  assuming the default color.
- API: Add TB_REVERSE.
- API: Add TB_INPUT_CURRENT.
- Move python module to its own directory and update it due to changes in the
  termbox library.
