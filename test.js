var assert = require('assert');

var metaKeyCodeReAnywhere = /(?:\x1b)([a-zA-Z0-9])/;
var metaKeyCodeRe = new RegExp('^' + metaKeyCodeReAnywhere.source + '$');
var functionKeyCodeReAnywhere = new RegExp('(?:\x1b+)(O|N|\\[|\\[\\[)(?:' + [
  '(\\d+)(?:;(\\d+))?([~^$])',
  '(?:M([@ #!a`])(.)(.))', // mouse
  '(?:1;)?(\\d+)?([a-zA-Z])'
].join('|') + ')');

var functionKeyCodeRe = new RegExp('^' + functionKeyCodeReAnywhere.source);

var colors = {
  light_cyan:   '1;36', // debug
  white:        '1;37', // info
  yellow:       '1;33', // notice
  yellow_two:   '1;33', // warn
  light_red:    '1;31', // error
  light_purple: '1;35', // critical
  light_gray:   '37',
  gray:         '90',
  black:        '30',
  dark_gray:    '1;30',
  red:          '31',
  green:        '32',
  light_green:  '1;32',
  brown:        '33',
  blue:         '34',
  purple:       '35',
  cyan:         '36',
  bold:         '1'
};

function paint(str, color) {
  if (!str || str == '') return '';
  return['\033[', colors[color], 'm', str, '\033[0m'].join('');
};

function check(s) {

    var ch,
        key = {
          sequence: s,
          name: undefined,
          ctrl: false,
          meta: false,
          shift: false
        },
        parts;

    if (s === '\r') {
      // carriage return
      key.name = 'return';

    } else if (s === '\n') {
      // enter, should have been called linefeed
      key.name = 'enter';
      // linefeed
      // key.name = 'linefeed';

    } else if (s === '\t') {
      // tab
      key.name = 'tab';

    } else if (s === '\b' || s === '\x7f' ||
               s === '\x1b\x7f' || s === '\x1b\b') {
      // backspace or ctrl+h
      key.name = 'backspace';
      key.meta = (s.charAt(0) === '\x1b');

    } else if (s === '\x1b' || s === '\x1b\x1b') {
      // escape key
      key.name = 'escape';
      key.meta = (s.length === 2);

    } else if (s === ' ' || s === '\x1b ') {
      key.name = 'space';
      key.meta = (s.length === 2);

    } else if (s.length === 1 && s <= '\x1a') {
      // ctrl+letter
      key.name = String.fromCharCode(s.charCodeAt(0) + 'a'.charCodeAt(0) - 1);
      key.ctrl = true;

    } else if (s.length === 1 && s >= 'a' && s <= 'z') {
      // lowercase letter
      key.name = s;

    } else if (s.length === 1 && s >= 'A' && s <= 'Z') {
      // shift+letter
      key.name = s.toLowerCase();
      key.shift = true;

    } else if (parts = metaKeyCodeRe.exec(s)) {
      // meta+character key
      key.name = parts[1].toLowerCase();
      key.meta = true;
      key.shift = /^[A-Z]$/.test(parts[1]);

    } else if (parts = functionKeyCodeRe.exec(s)) {
      // ansi escape sequence

      // reassemble the key code leaving out leading \x1b's,
      // the modifier key bitflag and any meaningless "1;" sequence
      var code = (parts[1] || '') + (parts[2] || '') +
                 (parts[4] || '') + (parts[9] || ''),
          modifier = (parts[3] || parts[8] || 1) - 1;

      // Parse the key modifier
      key.ctrl = !!(modifier & 4);
      key.meta = !!(modifier & 10);
      key.shift = !!(modifier & 1);
      key.code = code;

      // Parse the key itself
      switch (code) {
        /* xterm/gnome ESC O letter */
        case 'OP': key.name = 'f1'; break;
        case 'OQ': key.name = 'f2'; break;
        case 'OR': key.name = 'f3'; break;
        case 'OS': key.name = 'f4'; break;

        /* xterm/rxvt ESC [ number ~ */
        case '[11~': key.name = 'f1'; break;
        case '[12~': key.name = 'f2'; break;
        case '[13~': key.name = 'f3'; break;
        case '[14~': key.name = 'f4'; break;

        /* from Cygwin and used in libuv */
        case '[[A': key.name = 'f1'; break;
        case '[[B': key.name = 'f2'; break;
        case '[[C': key.name = 'f3'; break;
        case '[[D': key.name = 'f4'; break;
        case '[[E': key.name = 'f5'; break;

        /* common */
        case '[15~': key.name = 'f5'; break;
        case '[17~': key.name = 'f6'; break;
        case '[18~': key.name = 'f7'; break;
        case '[19~': key.name = 'f8'; break;
        case '[20~': key.name = 'f9'; break;
        case '[21~': key.name = 'f10'; break;
        case '[23~': key.name = 'f11'; break;
        case '[24~': key.name = 'f12'; break;

        /* xterm ESC [ letter */
        case '[A': key.name = 'up'; break;
        case '[B': key.name = 'down'; break;
        case '[C': key.name = 'right'; break;
        case '[D': key.name = 'left'; break;
        case '[E': key.name = 'clear'; break;
        case '[F': key.name = 'end'; break;
        case '[H': key.name = 'home'; break;

        /* xterm/gnome ESC O letter */
        case 'OA': key.name = 'up'; break;
        case 'OB': key.name = 'down'; break;
        case 'OC': key.name = 'right'; break;
        case 'OD': key.name = 'left'; break;
        case 'OE': key.name = 'clear'; break;
        case 'OF': key.name = 'end'; break;
        case 'OH': key.name = 'home'; break;

        /* xterm/rxvt ESC [ number ~ */
        case '[1~': key.name = 'home'; break;
        case '[2~': key.name = 'insert'; break;
        case '[3~': key.name = 'delete'; break;
        case '[4~': key.name = 'end'; break;
        case '[5~': key.name = 'pageup'; break;
        case '[6~': key.name = 'pagedown'; break;

        /* putty */
        case '[[5~': key.name = 'pageup'; break;
        case '[[6~': key.name = 'pagedown'; break;

        /* rxvt */
        case '[7~': key.name = 'home'; break;
        case '[8~': key.name = 'end'; break;

        /* rxvt keys with modifiers */
        case '[a': key.name = 'up'; key.shift = true; break;
        case '[b': key.name = 'down'; key.shift = true; break;
        case '[c': key.name = 'right'; key.shift = true; break;
        case '[d': key.name = 'left'; key.shift = true; break;
        case '[e': key.name = 'clear'; key.shift = true; break;

        case '[2$': key.name = 'insert'; key.shift = true; break;
        case '[3$': key.name = 'delete'; key.shift = true; break;
        case '[5$': key.name = 'pageup'; key.shift = true; break;
        case '[6$': key.name = 'pagedown'; key.shift = true; break;
        case '[7$': key.name = 'home'; key.shift = true; break;
        case '[8$': key.name = 'end'; key.shift = true; break;

        case 'Oa': key.name = 'up'; key.ctrl = true; break;
        case 'Ob': key.name = 'down'; key.ctrl = true; break;
        case 'Oc': key.name = 'right'; key.ctrl = true; break;
        case 'Od': key.name = 'left'; key.ctrl = true; break;
        case 'Oe': key.name = 'clear'; key.ctrl = true; break;

        case '[2^': key.name = 'insert'; key.ctrl = true; break;
        case '[3^': key.name = 'delete'; key.ctrl = true; break;
        case '[5^': key.name = 'pageup'; key.ctrl = true; break;
        case '[6^': key.name = 'pagedown'; key.ctrl = true; break;
        case '[7^': key.name = 'home'; key.ctrl = true; break;
        case '[8^': key.name = 'end'; key.ctrl = true; break;

        /* misc. */
        case '[Z': key.name = 'tab'; key.shift = true; break;
        default: key.name = 'undefined'; break;

      }
    }

    // Don't emit a key if no name was found
    if (key.name === undefined) {
      key = undefined;
    }

    if (s.length === 1) {
      ch = s;
    }

    if (key || ch) {
      console.log(ch, key);
    } else {
      console.log('no key or ch');
    }

}

/*
var stdin = process.stdin;

// without this, we would only get streams once enter is pressed
stdin.setRawMode( true );

// resume stdin in the parent process (node app won't quit all by itself
// unless an error or process.exit() happens)
stdin.resume();

// i don't want binary, do you?
stdin.setEncoding( 'utf8' );

// on any data into stdin
stdin.on( 'data', function( key ){
  // ctrl-c ( end of text )
  if ( key === '\u0003' ) {
    process.exit();
  }
  // write the key to stdout all normal like
  // process.stdout.write( key );
  read(key);
});
*/

const NONE     = 0;
const CTRL     = 100;
const ALT      = 200;
const SHIFT    = 300;

const ESCAPE   = -1;
const HOME     = 1;
const INSERT   = 2;
const DELETE   = 3;
const END      = 4;
const PAGEUP   = 5;
const PAGEDOWN = 6;

// home and end for urxvt
const HOME2    = 7;
const END2     = 8;

const TAB      = 10;

const UP       = 1065;
const DOWN     = 1066;
const RIGHT    = 1067;
const LEFT     = 1068;

// home and end for xterm
const HOME3    = 1072;
const END3    = 1070;

const F1  = 11;
const F2  = 12;
const F3  = 13;
const F4  = 14;
const F5  = 15;
const F6  = 17;
const F7  = 18;
const F8  = 19;
const F9  = 20;
const F10  = 21;
const F11  = 23;
const F12  = 24;


function doubleTrouble(orig, term) {
  var seq   = Array.apply(null, orig);
  var first = seq.shift();
  var last  = seq.pop();
  
  if (seq.length == 2) {

    if (last == 'Z') {
      return [ SHIFT, TAB ];

    } else if ( 'A' <= last && last <= 'H') { // arrow keys linux and xterm
      return [ NONE, last.charCodeAt(0) + 1000 ];

    } else if ('a' <= last && last <= 'd') { // mrxvt shift + left/right or ctrl+shift + up/down
      var meta = last == 'a' || last == 'b' ? CTRL | SHIFT : SHIFT;
      return [ meta, last.charCodeAt(0) + 968 ];
    }

  } else if (seq.length > 4) { // xterm shift or control + f1/keys/arrows

    if (last == '~') {
      // handle shift + delete case
      var num = (seq[3] == ';') ? Number(seq[2]) : Number(seq[2] + seq[3]);
  
      // num may be two digit number, so meta key can be at index 4 or 5
      var meta = (seq[4] == '5' || seq[5] == '5') ? CTRL
               : (seq[4] == '3' || seq[5] == '3') ? ALT
               : (seq[4] == '6' || seq[5] == '6') ? CTRL | SHIFT
               : (seq[4] == '7' || seq[5] == '7') ? CTRL | ALT
               : (seq[4] == '8' || seq[5] == '8') ? CTRL | ALT | SHIFT
               : SHIFT;
      
      
      return [ meta, num ];

    } else {
      var key = last.charCodeAt(0);
      var meta = seq[4] == '5' ? CTRL
               : seq[4] == '3' ? ALT
               : seq[4] == '6' ? CTRL | SHIFT
               : seq[4] == '7' ? CTRL | ALT
               : seq[4] == '8' ? CTRL | ALT | SHIFT
               : SHIFT;

      if (key >= 80) { // f1-f4 xterm
        return [ meta, key - 69 ];
      } else {// ctrl + arrows urxvt
        return [ meta, key + 1000 ];
      }

    }

  } else if (last == '~') { // 3 or 4 in length

    if (seq[3] && Number(seq[2] + seq[3]) > 24) { // shift + f1-f8, linux/urxvt

      var num = Number(seq[2] + seq[3]);

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

      return [ SHIFT , num - offset ];

    } else if (term == 'mrxvt' && seq[2] == 3) { // mrxvt shift + insert
      return [ SHIFT , INSERT ];

    } else {
      return [ NONE , Number(seq[2] + (seq[3] || '')) ];
    }


  } else if (seq.length == 3) {

    if ('A' <= last && last <= 'Z') { // // F1-F5 xterm
      return [ NONE, last.charCodeAt(0) - 54 ];

    } else if ('a' <= seq[2] && seq[2] <= 'z') { // shift + arrows urxvt
      return [ SHIFT, seq[2].charCodeAt(0) + 968 ];

    } else if (last == '^' || last == '@' || last == '$') { // shift, shift+control, control + key urxvt
      var meta = last == '^' ? CTRL : last == '@' ? CTRL | SHIFT : SHIFT;
      return [ meta, Number(seq[2]) ];

    } else {
      console.log('dont know', orig);
    }

  } else if (seq.length == 4) {
  
    if (last == '^' || last == '@' || last == '$') {
      var num = Number(seq[2] + seq[3]);

      if (num >= 25) { // ctrl + shift f1-f12 urxvt
        var offset = num == 25 || num == 26 || num == 29 ? 12 : 13;
        return [ CTRL | SHIFT, num - offset ];
      } else {
        var meta = last == '^' ? CTRL : last == '@' ? CTRL | SHIFT : SHIFT;
        return [ meta, num ];
      }
    } else {
      console.log('unknown suffix.', orig);
    }
      
  } else {

    console.log('unknown seq', orig);
  }
}

var prev = [];

function parse(seq, term) {
  seq = seq.split('');

  if (prev[0]) {
    seq = prev.concat(seq);
    prev.length = 0;

    // check for second escape in sequence, but account for valid ending ^
    var second_esc = seq.slice(3, seq.length-1).indexOf('^');
    if (second_esc != -1) { // found escape sequence
      prev = seq.splice(second_esc + 3); // account for the padding above
    }
  }

  if (seq[0] != '^')
    return;

  if (seq[1] != '[') {
    return [ CTRL, seq[1] ];
  }

  switch(seq[2]) {

    case '[':
      res = doubleTrouble(seq, term);
      if (res) return res;

      break;

    case 'O':
      var key = seq[3].charCodeAt(0);
      if (key < 90) { // f1-f4 xterm
        return [ NONE, key - 69 ];
      } else {        // ctrl + arrows urxvt
        return [ CTRL, key + 968 ];
      }
      break;

    case '^':
      if (seq[3] == '[') {
        if (!seq[4]) return [ ALT, ESCAPE ];
  
        // urxvt territory
        var last = seq.pop();
  
        if (last == '^' || last == '@') { // ctrl + alt + arrows
          var meta = last == '^' ? CTRL | ALT : CTRL | ALT | SHIFT;
          return [ meta, Number(seq[5]) ];
  
        } else if ('a' <= last && last <= 'z') { // urxvt ctr/alt arrow or ctr/shift/alt arrow
          var meta = seq[4] == 'O' ? CTRL | ALT : CTRL | ALT | SHIFT;
          return [meta, last.charCodeAt(0) + 968 ];
  
        } else if (last == '~') { // urxvt alt + key
          return [ ALT, Number(seq[5] + (seq[6] || '')) ];
  
        } else if (last == 'R') {
          return [ ALT, F3 ]; // mrxvt alt + f3
  
        } else if (seq.length > 4) { // urxvt alt + arrow keys
          return [ ALT, seq[5].charCodeAt(0) + 1000 ];
        } else {
          console.log(seq)
        }
  
      } else if (seq[2] == '^') { // linux ctrl+alt+key
        return [ CTRL | ALT, seq[3]]

      } else {
        console.log('unknown', seq);
      }
      break;
    default:
      var letter = seq[2];
      if (letter >= 'A' && letter <= 'Z') {
        return [ SHIFT | ALT, letter];
      } else {
        return [ ALT, letter];
      }
      break;
  }

  prev = prev.concat(seq);
}

var linux_keys = {

'^A'  : [ CTRL, 'A' ],  // ctrl + a / ctrl + shift + a
'^[a' : [ ALT,  'a' ],  // alt + a
'^[A' : [ ALT | SHIFT, 'A' ], // alt + shift + a
'^[^A': [ CTRL | ALT, 'A' ], // ctrl + alt + a

'^[[1~': [ NONE, HOME], // home
'^[[2~': [ NONE, INSERT ], // ins
'^[[3~': [ NONE, DELETE ], // del
'^[[4~': [ NONE, END ], // end
'^[[5~': [ NONE, PAGEUP ], // pageup
'^[[6~': [ NONE, PAGEDOWN ], // pagedn

'^[[A' : [ NONE, UP ],  // up
'^[[B' : [ NONE, DOWN ],  // down
'^[[C' : [ NONE, RIGHT ],  // right
'^[[D' : [ NONE, LEFT ],  // left

'^[[[A':  [ NONE, F1 ], // f1
'^[[[B':  [ NONE, F2 ], // f2
'^[[[C':  [ NONE, F3 ], // f3
'^[[[D':  [ NONE, F4 ], // f4
'^[[[E':  [ NONE, F5 ], // f5
'^[[17~': [ NONE, F6 ], // f6
'^[[18~': [ NONE, F7 ], // f7
'^[[19~': [ NONE, F8 ], // f8
'^[[20~': [ NONE, F9 ], // f9
'^[[21~': [ NONE, F10 ], // f10
'^[[23~': [ NONE, F11 ], // f11
'^[[24~': [ NONE, F12 ], // f12

// shift'

'^[[25~': [ SHIFT, F1 ], // f1
'^[[26~': [ SHIFT, F2 ], // f2
'^[[28~': [ SHIFT, F3 ], // f3
'^[[29~': [ SHIFT, F4 ], // f4
'^[[31~': [ SHIFT, F5 ], // f5
'^[[32~': [ SHIFT, F6 ], // f6
'^[[33~': [ SHIFT, F7 ], // f7
'^[[34~': [ SHIFT, F8 ]  // f8
// f9-f12 nothing

};

var urxvt_keys = {
  // ---- no meta
  
  '^[[11~' : [ NONE, F1 ], // f1
  '^[[12~' : [ NONE, F2 ], // f2
  '^[[13~' : [ NONE, F3 ], // f3
  '^[[14~' : [ NONE, F4 ], // f4
  '^[[15~' : [ NONE, F5 ], // f5
  '^[[17~' : [ NONE, F6 ], // f6
  '^[[18~' : [ NONE, F7 ], // f7
  '^[[19~' : [ NONE, F8 ], // f8
  '^[[20~' : [ NONE, F9 ], // f9
  '^[[21~' : [ NONE, F10 ], // f10
  '^[[23~' : [ NONE, F11 ], // f11
  '^[[24~' : [ NONE, F12 ], // f12

  '^[[2~'  : [ NONE, INSERT ], // insert
  '^[[3~'  : [ NONE, DELETE ], // delete
  '^[[5~'  : [ NONE, PAGEUP ], // pageup
  '^[[6~'  : [ NONE, PAGEDOWN ], // pagedown
  '^[[7~'  : [ NONE, HOME2 ], // home
  '^[[8~'  : [ NONE, END2 ], // end
  '^[[A'   : [ NONE, UP ], // up
  '^[[B'   : [ NONE, DOWN ], // down
  '^[[C'   : [ NONE, RIGHT ], // right
  '^[[D'   : [ NONE, LEFT ], // left

  
  // ---- shift
  // these override f11 and f12 !
  // '^[[23~' : [ SHIFT, F1 ], // f1
  // '^[[24~' : [ SHIFT, F2 ],
  '^[[25~' : [ SHIFT, F3 ],
  '^[[26~' : [ SHIFT, F4 ],
  '^[[28~' : [ SHIFT, F5 ],
  '^[[29~' : [ SHIFT, F6 ],
  '^[[31~' : [ SHIFT, F7 ],
  '^[[32~' : [ SHIFT, F8 ],
  '^[[33~' : [ SHIFT, F9 ],
  '^[[34~' : [ SHIFT, F10 ],
  '^[[23$' : [ SHIFT, F11 ],
  '^[[24$' : [ SHIFT, F12 ], // f12
  
  // insert does something weird
  '^[[3$'  : [ SHIFT, DELETE ], // delete
  // pageup scrolls
  // pagedn scrolls
  '^[[7$'  : [ SHIFT, HOME2 ], // home
  '^[[8$'  : [ SHIFT, END2 ], // end
  '^[[a '  : [ SHIFT, UP ], // up
  '^[[b '  : [ SHIFT, DOWN ], // down
  '^[[c '  : [ SHIFT, RIGHT ], // right
  '^[[d '  : [ SHIFT, LEFT ], // left

  // ---- ctrl
  
  '^[[11^' : [ CTRL, F1 ], // f1
  '^[[12^' : [ CTRL, F2 ], // f2
  '^[[13^' : [ CTRL, F3 ], // f3
  '^[[14^' : [ CTRL, F4 ], // f4
  '^[[15^' : [ CTRL, F5 ], // f5
  '^[[17^' : [ CTRL, F6 ], // f6
  '^[[18^' : [ CTRL, F7 ], // f7
  '^[[19^' : [ CTRL, F8 ], // f8
  '^[[20^' : [ CTRL, F9 ], // f9
  '^[[21^' : [ CTRL, F10 ], // f10
  '^[[23^' : [ CTRL, F11 ], // f11
  '^[[24^' : [ CTRL, F12 ], // f12

  
  '^[[2^'  : [ CTRL, INSERT ], // insert
  '^[[3^'  : [ CTRL, DELETE ], // delete
  '^[[5^'  : [ CTRL, PAGEUP ], // pageup
  '^[[6^'  : [ CTRL, PAGEDOWN ], // pagedn
  '^[[7^'  : [ CTRL, HOME2 ], // home
  '^[[8^'  : [ CTRL, END2 ], // end
  '^[Oa'  :  [ CTRL, UP ],  // up
  '^[Ob'  :  [ CTRL, DOWN ],  // down
  '^[Oc'  :  [ CTRL, RIGHT ],  // right
  '^[Od'  :  [ CTRL, LEFT ],  // left

  // ---- alt
  
  '^[^['  : [ ALT, ESCAPE ],  // esc
  
  // f1 system menu
  // f2 app menu
  '^[^[[13~' : [ ALT, F3 ], // f3
  // f4 close window
  '^[^[[15~' : [ ALT, F5 ], // f5
  '^[^[[17~' : [ ALT, F6 ], // f6
  '^[^[[18~' : [ ALT, F7 ], // f7
  '^[^[[19~' : [ ALT, F8 ], // f8
  '^[^[[20~' : [ ALT, F9 ], // f9
  '^[^[[21~' : [ ALT, F10 ], // f10
  '^[^[[23~' : [ ALT, F11 ], // f11
  '^[^[[24~' : [ ALT, F12 ], // f12
  
  '^[^[[2~'  : [ ALT, INSERT ],  // ins
  '^[^[[3~'  : [ ALT, DELETE ],  // del
  '^[^[[5~'  : [ ALT, PAGEUP ],  // pup
  '^[^[[6~'  : [ ALT, PAGEDOWN ],  // pdn
  '^[^[[7~'  : [ ALT, HOME2 ],  // home
  '^[^[[8~'  : [ ALT, END2 ],  // end
  '^[^[[A '  : [ ALT, UP ],  // up
  '^[^[[B '  : [ ALT, DOWN ],  // down
  '^[^[[C '  : [ ALT, RIGHT ],  // right
  '^[^[[D '  : [ ALT, LEFT ],  // left

  // ---- ctrl + shift
  
  // overwrites ctrl + f11/f12
  // '^[[23^'   : [ CTRL | SHIFT, F1 ], // f1
  // '^[[24^'   : [ CTRL | SHIFT, F2 ],

  '^[[25^'   : [ CTRL | SHIFT, F3 ],
  '^[[26^'   : [ CTRL | SHIFT, F4 ],
  '^[[28^'   : [ CTRL | SHIFT, F5 ],
  '^[[29^'   : [ CTRL | SHIFT, F6 ],
  '^[[31^'   : [ CTRL | SHIFT, F7 ],
  '^[[32^'   : [ CTRL | SHIFT, F8 ],
  '^[[33^'   : [ CTRL | SHIFT, F9 ],
  '^[[34^'   : [ CTRL | SHIFT, F10 ],
  '^[[23@'   : [ CTRL | SHIFT, F11 ],
  '^[[24@'   : [ CTRL | SHIFT, F12 ], // f12
  
  '^[[2@'    : [ CTRL | SHIFT, INSERT ],   // ins
  '^[[3@'    : [ CTRL | SHIFT, DELETE ],   // del
  '^[[5@'    : [ CTRL | SHIFT, PAGEUP ],   // pup
  '^[[6@'    : [ CTRL | SHIFT, PAGEDOWN ],   // pdn
  '^[[7@'    : [ CTRL | SHIFT, HOME2 ],   // home
  '^[[8@'    : [ CTRL | SHIFT, END2 ],   // end
  
  // same as shift + arrows
  // '^[[a '    : [ CTRL | SHIFT, UP ],   // up
  // '^[[b '    : [ CTRL | SHIFT, DOWN ],   // down
  // '^[[c '    : [ CTRL | SHIFT, RIGHT ],   // right
  // '^[[d '    : [ CTRL | SHIFT, LEFT ],   // left

  // ---- ctrl + alt
  
  // f1-f12 changes tty
  
  '^[^[[2^'  : [ CTRL | ALT, INSERT ],  // ins
  '^[^[[3^'  : [ CTRL | ALT, DELETE ],
  '^[^[[5^'  : [ CTRL | ALT, PAGEUP ],
  '^[^[[6^'  : [ CTRL | ALT, PAGEDOWN ],
  '^[^[[7^'  : [ CTRL | ALT, HOME2 ],
  '^[^[[8^'  : [ CTRL | ALT, END2 ],
  '^[^[Oa'  : [ CTRL | ALT,  UP ],
  '^[^[Ob'  : [ CTRL | ALT,  DOWN ],
  '^[^[Oc'  : [ CTRL | ALT,  RIGHT ],
  '^[^[Od'  : [ CTRL | ALT,  LEFT ],
  
  // ---- ctrl + shift + alt
  
  // f1-f12 changes tty
  
  '^[^[[2@' : [ CTRL | ALT | SHIFT, INSERT ], // ins
  '^[^[[3@' : [ CTRL | ALT | SHIFT, DELETE ], // del
  '^[^[[5@' : [ CTRL | ALT | SHIFT, PAGEUP ], // pageup
  '^[^[[6@' : [ CTRL | ALT | SHIFT, PAGEDOWN ], // pagedn
  '^[^[[7@' : [ CTRL | ALT | SHIFT, HOME2 ], // home
  '^[^[[8@' : [ CTRL | ALT | SHIFT, END2 ], // end
  '^[^[[a'  : [ CTRL | ALT | SHIFT, UP ], // up
  '^[^[[b'  : [ CTRL | ALT | SHIFT, DOWN ], // down
  '^[^[[c'  : [ CTRL | ALT | SHIFT, RIGHT ], // right
  '^[^[[d'  : [ CTRL | ALT | SHIFT, LEFT ] // left
};

var mrxvt_keys = {
  // ---- no meta
  
  '^[OP' : [ NONE, F1 ], // f1
  '^[OQ' : [ NONE, F2 ], // f2
  '^[OR' : [ NONE, F3 ], // f3
  '^[OS' : [ NONE, F4 ], // f4
  '^[[15~' : [ NONE, F5 ], // f5
  '^[[17~' : [ NONE, F6 ], // f6
  '^[[18~' : [ NONE, F7 ], // f7
  '^[[19~' : [ NONE, F8 ], // f8
  '^[[20~' : [ NONE, F9 ], // f9
  '^[[21~' : [ NONE, F10 ], // f10
  '^[[23~' : [ NONE, F11 ], // f11
  '^[[24~' : [ NONE, F12 ], // f12

  '^[[2~'  : [ NONE, INSERT ], // insert
  '^[[3~'  : [ NONE, DELETE ], // delete
  '^[[5~'  : [ NONE, PAGEUP ], // pageup
  '^[[6~'  : [ NONE, PAGEDOWN ], // pagedown
  '^[[7~'  : [ NONE, HOME2 ], // home
  '^[[8~'  : [ NONE, END2 ], // end
  '^[[A'   : [ NONE, UP ], // up
  '^[[B'   : [ NONE, DOWN ], // down
  '^[[C'   : [ NONE, RIGHT ], // right
  '^[[D'   : [ NONE, LEFT ], // left

  
  // ---- shift
  
  '^[[Z'   : [ SHIFT, TAB ],
  
  // these override f11 and f12 !
  // '^[[23~' : [ SHIFT, F1 ], // f1
  // '^[[24~' : [ SHIFT, F2 ],
  '^[[25~' : [ SHIFT, F3 ],
  '^[[26~' : [ SHIFT, F4 ],
  '^[[28~' : [ SHIFT, F5 ],
  '^[[29~' : [ SHIFT, F6 ],
  '^[[31~' : [ SHIFT, F7 ],
  '^[[32~' : [ SHIFT, F8 ],
  '^[[33~' : [ SHIFT, F9 ],
  '^[[34~' : [ SHIFT, F10 ],
  '^[[23$' : [ SHIFT, F11 ],
  '^[[24$' : [ SHIFT, F12 ], // f12
  
  '^[[3~'  : [ SHIFT, INSERT ], // insert
  '^[[3$'  : [ SHIFT, DELETE ], // delete
  // pageup scrolls
  // pagedn scrolls
  // home changes font size
  // end changes font size
  // up scrolls line
  // down scrolls line
  '^[[c '  : [ SHIFT, RIGHT ], // right
  '^[[d '  : [ SHIFT, LEFT ], // left

  // ---- ctrl
  
  // f1-f12 same as no meta
  
  '^[[2^'  : [ CTRL, INSERT ], // insert
  '^[[3^'  : [ CTRL, DELETE ], // delete
  '^[[5^'  : [ CTRL, PAGEUP ], // pageup
  '^[[6^'  : [ CTRL, PAGEDOWN ], // pagedn
  '^[[7^'  : [ CTRL, HOME2 ], // home
  '^[[8^'  : [ CTRL, END2 ], // end
  '^[Oa'  :  [ CTRL, UP ],  // up
  '^[Ob'  :  [ CTRL, DOWN ],  // down
  '^[Oc'  :  [ CTRL, RIGHT ],  // right
  '^[Od'  :  [ CTRL, LEFT ],  // left

  // ---- alt
  
  '^[^['  : [ ALT, ESCAPE ],  // esc
  
  // f1 system menu
  // f2 app menu
  '^[^[OR'   : [ ALT, F3 ], // f3
  // f4 close window
  '^[^[[15~' : [ ALT, F5 ], // f5
  '^[^[[17~' : [ ALT, F6 ], // f6
  '^[^[[18~' : [ ALT, F7 ], // f7
  '^[^[[19~' : [ ALT, F8 ], // f8
  '^[^[[20~' : [ ALT, F9 ], // f9
  '^[^[[21~' : [ ALT, F10 ], // f10
  '^[^[[23~' : [ ALT, F11 ], // f11
  '^[^[[24~' : [ ALT, F12 ], // f12
  
  '^[^[[2~'  : [ ALT, INSERT ],  // ins
  '^[^[[3~'  : [ ALT, DELETE ],  // del
  '^[^[[5~'  : [ ALT, PAGEUP ],  // pup
  '^[^[[6~'  : [ ALT, PAGEDOWN ],  // pdn
  '^[^[[7~'  : [ ALT, HOME2 ],  // home
  '^[^[[8~'  : [ ALT, END2 ],  // end
  '^[^[[A '  : [ ALT, UP ],  // up
  '^[^[[B '  : [ ALT, DOWN ],  // down
  '^[^[[C '  : [ ALT, RIGHT ],  // right
  '^[^[[D '  : [ ALT, LEFT ],  // left

  // ---- ctrl + shift
  
  // overwrites ctrl + f11/f12
  // '^[[23^'   : [ CTRL | SHIFT, F1 ], // f1
  // '^[[24^'   : [ CTRL | SHIFT, F2 ],

  '^[[25^'   : [ CTRL | SHIFT, F3 ],
  '^[[26^'   : [ CTRL | SHIFT, F4 ],
  '^[[28^'   : [ CTRL | SHIFT, F5 ],
  '^[[29^'   : [ CTRL | SHIFT, F6 ],
  '^[[31^'   : [ CTRL | SHIFT, F7 ],
  '^[[32^'   : [ CTRL | SHIFT, F8 ],
  '^[[33^'   : [ CTRL | SHIFT, F9 ],
  '^[[34^'   : [ CTRL | SHIFT, F10 ],
  '^[[23@'   : [ CTRL | SHIFT, F11 ],
  '^[[24@'   : [ CTRL | SHIFT, F12 ], // f12
  
  '^[[2@'    : [ CTRL | SHIFT, INSERT ],   // ins
  '^[[3@'    : [ CTRL | SHIFT, DELETE ],   // del
  // pageup scrolls
  // pagedn scrolls
  '^[[7@'    : [ CTRL | SHIFT, HOME2 ],   // home
  '^[[8@'    : [ CTRL | SHIFT, END2 ],   // end
  
  '^[[a'     : [ CTRL | SHIFT, UP ],   // up
  '^[[b'     : [ CTRL | SHIFT, DOWN ],   // down
  // right nothing
  // left nothing

  // ---- ctrl + alt
  
  // f1-f12 changes tty
  '^[^[[2^'  : [ CTRL | ALT, INSERT ],  // ins
  '^[^[[3^'  : [ CTRL | ALT, DELETE ],
  '^[^[[5^'  : [ CTRL | ALT, PAGEUP ],
  '^[^[[6^'  : [ CTRL | ALT, PAGEDOWN ],
  '^[^[[7^'  : [ CTRL | ALT, HOME2 ],
  '^[^[[8^'  : [ CTRL | ALT, END2 ],
  '^[^[Oa'  : [ CTRL | ALT,  UP ],
  '^[^[Ob'  : [ CTRL | ALT,  DOWN ],
  '^[^[Oc'  : [ CTRL | ALT,  RIGHT ],
  '^[^[Od'  : [ CTRL | ALT,  LEFT ],
  
  // ---- ctrl + shift + alt
  
  // f1-f12 changes tty
  '^[^[[2@' : [ CTRL | ALT | SHIFT, INSERT ], // ins
  '^[^[[3@' : [ CTRL | ALT | SHIFT, DELETE ], // del
  // pageup scrolls
  // pagedn scrolls
  '^[^[[7@' : [ CTRL | ALT | SHIFT, HOME2 ], // home
  '^[^[[8@' : [ CTRL | ALT | SHIFT, END2 ], // end
  '^[^[[a'  : [ CTRL | ALT | SHIFT, UP ], // up
  '^[^[[b'  : [ CTRL | ALT | SHIFT, DOWN ], // down
  '^[^[[c'  : [ CTRL | ALT | SHIFT, RIGHT ], // right
  '^[^[[d'  : [ CTRL | ALT | SHIFT, LEFT ] // left
}

var xterm_keys = {
  // ---- no meta
  
  '^[OP' :   [ NONE, F1  ],   // f1
  '^[OQ' :   [ NONE, F2  ],   // f2
  '^[OR' :   [ NONE, F3  ],   // f3
  '^[OS' :   [ NONE, F4  ],   // f4
  '^[[15~' : [ NONE, F5  ],  // f5
  '^[[17~' : [ NONE, F6  ],  // f6
  '^[[18~' : [ NONE, F7  ],  // f7
  '^[[19~' : [ NONE, F8  ],  // f8
  '^[[20~' : [ NONE, F9  ],  // f9
  '^[[21~' : [ NONE, F10 ],  // f10
  '^[[23~' : [ NONE, F11 ],  // f11
  '^[[24~' : [ NONE, F12 ],  // f12
  '^[[2~'  : [ NONE, INSERT ], // insert
  '^[[3~'  : [ NONE, DELETE ], // delete
  '^[[5~'  : [ NONE, PAGEUP ], // pageup
  '^[[6~'  : [ NONE, PAGEDOWN ], // pagedown
  '^[[H' :   [ NONE, HOME3 ],  // home
  '^[[F' :   [ NONE, END3 ],  // end
  '^[[A' :   [ NONE, UP ],  // up
  '^[[B' :   [ NONE, DOWN ],  // down
  '^[[C' :   [ NONE, RIGHT ],  // right
  '^[[D' :   [ NONE, LEFT ],  // left

  // ---- shift
  
  '^[[1;2P' :  [ SHIFT, F1 ], // f1
  '^[[1;2Q' :  [ SHIFT, F2 ],
  '^[[1;2R' :  [ SHIFT, F3 ],
  '^[[1;2S' :  [ SHIFT, F4 ],
  '^[[15;2~' : [ SHIFT, F5 ],
  '^[[17;2~' : [ SHIFT, F6 ],
  '^[[18;2~' : [ SHIFT, F7 ],
  '^[[19;2~' : [ SHIFT, F8 ],
  '^[[20;2~' : [ SHIFT, F9 ],
  '^[[21;2~' : [ SHIFT, F10 ],
  '^[[23;2~' : [ SHIFT, F11 ],
  '^[[24;2~' : [ SHIFT, F12 ],  // f12
  // insert pastes
  '^[[3;2~': [ SHIFT, DELETE ],  // delete
  // pageup scrolls
  // pagedn scrolls
  '^[[1;2H' : [ SHIFT, HOME3 ], // home
  '^[[1;2F' : [ SHIFT, END3 ], // end
  '^[[1;2A' : [ SHIFT, UP ], // up
  '^[[1;2B' : [ SHIFT, DOWN ], // down
  '^[[1;2C' : [ SHIFT, RIGHT ], // left
  '^[[1;2D' : [ SHIFT, LEFT ], // right

  // ---- ctrl
  
  '^[[1;5P'  : [ CTRL, F1 ], // f1
  '^[[1;5Q'  : [ CTRL, F2 ],
  '^[[1;5R'  : [ CTRL, F3 ],
  '^[[1;5S'  : [ CTRL, F4 ],
  '^[[15;5~' : [ CTRL, F5 ],
  '^[[17;5~' : [ CTRL, F6 ],
  '^[[18;5~' : [ CTRL, F7 ],
  '^[[19;5~' : [ CTRL, F8 ],
  '^[[20;5~' : [ CTRL, F9 ],
  '^[[21;5~' : [ CTRL, F10 ],
  '^[[23;5~' : [ CTRL, F11 ],
  '^[[24;5~' : [ CTRL, F12 ],  // f12
  '^[[2;5~'  : [ CTRL, INSERT ], // ins
  '^[[3;5~'  : [ CTRL, DELETE ],
  '^[[5;5~'  : [ CTRL, PAGEUP ],
  '^[[6;5~'  : [ CTRL, PAGEDOWN ],
  '^[[1;5H'  : [ CTRL, HOME3 ],
  '^[[1;5F'  : [ CTRL, END3 ], // pdn
  '^[[1;5A'  : [ CTRL, UP ],
  '^[[1;5B'  : [ CTRL, DOWN ],
  '^[[1;5C'  : [ CTRL, RIGHT ],
  '^[[1;5D'  : [ CTRL, LEFT ],

  // ---- alt
  
  '^[^[' : [ ALT, ESCAPE ],  // esc
  // f1 system menu
  // f2 app menu
  '^[[1;3R': [ ALT, F3 ],  // f3
  // f4 close window
  '^[[15;3~' : [ ALT, F5 ], // f5
  '^[[17;3~' : [ ALT, F6 ], // f6
  '^[[18;3~' : [ ALT, F7 ],
  '^[[19;3~' : [ ALT, F8 ],
  '^[[20;3~' : [ ALT, F9 ],
  '^[[21;3~' : [ ALT, F10 ],
  '^[[23;3~' : [ ALT, F11 ],
  '^[[24;3~' : [ ALT, F12 ], // f12
  '^[[2;3~'  : [ ALT, INSERT ], // ins
  '^[[3;3~'  : [ ALT, DELETE ],
  '^[[5;3~'  : [ ALT, PAGEUP ],
  '^[[6;3~'  : [ ALT, PAGEDOWN ],
  '^[[1;3H'  : [ ALT, HOME3 ],
  '^[[1;3F'  : [ ALT, END3 ], // end
  '^[[1;3A'  : [ ALT, UP ], // up
  '^[[1;3B'  : [ ALT, DOWN ], // down
  '^[[1;3C'  : [ ALT, RIGHT ], // right
  '^[[1;3D'  : [ ALT, LEFT ], // left

  // ---- ctrl + shift
  
  '^[[1;6P'  : [ CTRL | SHIFT, F1 ], // f1
  '^[[1;6Q'  : [ CTRL | SHIFT, F2 ],
  '^[[1;6R'  : [ CTRL | SHIFT, F3 ],
  '^[[1;6S'  : [ CTRL | SHIFT, F4 ],
  '^[[15;6~' : [ CTRL | SHIFT, F5 ],
  '^[[17;6~' : [ CTRL | SHIFT, F6 ],
  '^[[18;6~' : [ CTRL | SHIFT, F7 ],
  '^[[19;6~' : [ CTRL | SHIFT, F8 ],
  '^[[20;6~' : [ CTRL | SHIFT, F9 ],
  '^[[21;6~' : [ CTRL | SHIFT, F10 ],
  '^[[23;6~' : [ CTRL | SHIFT, F11 ],
  '^[[24;6~' : [ CTRL | SHIFT, F12 ], // f12
  // ins weirdness
  '^[[3;6~'  : [ CTRL | SHIFT, DELETE ], // del
  // pageup scrolls
  // pagedn scrolls
  '^[[1;6H'  : [ CTRL | SHIFT, HOME3 ], // home
  '^[[1;6F'  : [ CTRL | SHIFT, END3 ], // end
  '^[[1;6A'  : [ CTRL | SHIFT, UP ], // up
  '^[[1;6B'  : [ CTRL | SHIFT, DOWN ], // down
  '^[[1;6C'  : [ CTRL | SHIFT, RIGHT ], // right
  '^[[1;6D'  : [ CTRL | SHIFT, LEFT ], // left

  // ---- ctrl + alt
  
  // f1-f12 changes tty
  
  '^[[2;7~'  : [ CTRL | ALT, INSERT ],  // ins
  '^[[3;7~'  : [ CTRL | ALT, DELETE ],
  '^[[5;7~'  : [ CTRL | ALT, PAGEUP ],
  '^[[6;7~'  : [ CTRL | ALT, PAGEDOWN ],
  '^[[1;7H'  : [ CTRL | ALT, HOME3 ],
  '^[[1;7F'  : [ CTRL | ALT, END3 ],
  '^[[1;7A'  : [ CTRL | ALT, UP ],  // up
  '^[[1;7B'  : [ CTRL | ALT, DOWN ],
  '^[[1;7C'  : [ CTRL | ALT, RIGHT ],
  '^[[1;7D'  : [ CTRL | ALT, LEFT ],  // left
  
  // ---- ctrl + shift + alt
  
  // f1-f12 changes tty
  
  // ins weirdness
  '^[[3;8~'  : [ CTRL | SHIFT | ALT, DELETE ], // del
  // pageup scrolls
  // pagedn scrolls
  '^[[1;8H'  : [ CTRL | SHIFT | ALT, HOME3 ], // home
  '^[[1;8F'  : [ CTRL | SHIFT | ALT, END3 ], // end
  '^[[1;8A'  : [ CTRL | SHIFT | ALT, UP ], // up
  '^[[1;8B'  : [ CTRL | SHIFT | ALT, DOWN ], // down
  '^[[1;8C'  : [ CTRL | SHIFT | ALT, RIGHT ], // right
  '^[[1;8D'  : [ CTRL | SHIFT | ALT, LEFT ] // left
}

var good = 0;
var bad  = 0;

function ok(res, key) {
  return res && res[0] == key[0] && res[1] == key[1];
}

function test(keys, term) {
  for (var seq in keys) {
    res = parse(seq, term);
  
    if (ok(res, keys[seq])) {
      good++;
      // console.log(paint(' -- OK:', 'green'), seq);
    } else {
      bad++;
      console.log(paint('-- NOPE: ', 'red'), seq, res);
      // console.log(' -- NOPE: ', seq);
    }
  }
}

console.log(' ===== linux');
test(linux_keys, 'linux');
console.log(' ===== urxvt');
test(urxvt_keys, 'urxvt');
console.log(' ===== xterm');
test(xterm_keys, 'xterm');
console.log(' ===== mrxvt');
test(mrxvt_keys, 'mrxvt');

// test buffer cut
['^[[1;8B', '^[[1;', '8B^[[', '1;8B'].forEach(function(key) {
  res = parse(key);
  console.log(key, ' - \t', res);
})

console.log(' --- Good %d, bad %d', good, bad);