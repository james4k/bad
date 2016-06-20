// sng_terminal.h v0.0.0 (incomplete)
// James Gray, Oct 2015
//
// OVERVIEW
//
// sng_terminal implements a vt100 terminal code parser.
//
// Use for terminal muxing, a terminal emulation frontend, or wherever
// else you need terminal emulation. There are no dependencies aside
// from the C standard library. Influenced largely by st, rxvt, xterm,
// and iTerm as reference.
//
// USAGE
//
// define SNG_TERMINAL_IMPLEMENTATION, etc. etc.
//
// DEPENDENCIES
//
// C standard library - stdint.h stdio.h stdlib.h string.h
//
// LICENSE
//
// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy,
// distribute, and modify this file as you see fit.
//
// No warranty. Use at your own risk.

#ifndef SNG_TERMINAL_H
#define SNG_TERMINAL_H

#include <stdint.h>
#include <stdlib.h> // strtol
#include <stdio.h>  // fprintf
#include <string.h> // memmove

typedef    float f32;
typedef   double f64;
typedef   int8_t s8;
typedef  int16_t s16;
typedef  int32_t s32;
typedef  uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef       u8 b8;
typedef      u32 b32;

#ifndef SNG_ASSERT
#define SNG_ASSERT(x)
#endif

// SNG_TERM_ATTR_* represent cell attributes.
enum {
	SNG_TERM_ATTR_REVERSE   = (1 << 0),
	SNG_TERM_ATTR_UNDERLINE = (1 << 1),
	SNG_TERM_ATTR_BOLD      = (1 << 2),
	SNG_TERM_ATTR_GFX       = (1 << 3),
	SNG_TERM_ATTR_ITALIC    = (1 << 4),
	SNG_TERM_ATTR_BLINK     = (1 << 5),
	SNG_TERM_ATTR_WRAP      = (1 << 6),
};

// SNG_TERM_CHANGED_* represent change flags to inform the user of any
// state changes.
enum {
	SNG_TERM_CHANGED_SCREEN = (1 << 0),
	SNG_TERM_CHANGED_TITLE  = (1 << 1),
};

// SNG_TERM_COLOR_* represent color codes.
enum {
	// ASCII colors
	SNG_TERM_COLOR_BLACK = 0,
	SNG_TERM_COLOR_RED,
	SNG_TERM_COLOR_GREEN,
	SNG_TERM_COLOR_YELLOW,
	SNG_TERM_COLOR_BLUE,
	SNG_TERM_COLOR_MAGENTA,
	SNG_TERM_COLOR_CYAN,
	SNG_TERM_COLOR_LIGHT_GREY,
	SNG_TERM_COLOR_DARK_GREY,
	SNG_TERM_COLOR_LIGHT_RED,
	SNG_TERM_COLOR_LIGHT_GREEN,
	SNG_TERM_COLOR_LIGHT_YELLOW,
	SNG_TERM_COLOR_LIGHT_BLUE,
	SNG_TERM_COLOR_LIGHT_MAGENTA,
	SNG_TERM_COLOR_LIGHT_CYAN,
	
	// Default colors are potentially distinct to allow for special
	// behavior. For example, rendering with a transparent background.
	// Otherwise, the simple case is to map default colors to another
	// color. These values are specific to sng_terminal.
	SNG_TERM_COLOR_DEFAULT_FG = 0xff80,
	SNG_TERM_COLOR_DEFAULT_BG = 0xff81
};

// SNG_TERM_MODE_* represent terminal modes.
enum {
	SNG_TERM_MODE_WRAP          = (1 << 0),
	SNG_TERM_MODE_INSERT        = (1 << 1),
	SNG_TERM_MODE_APP_KEYPAD    = (1 << 2),
	SNG_TERM_MODE_ALT_SCREEN    = (1 << 3),
	SNG_TERM_MODE_CRLF          = (1 << 4),
	SNG_TERM_MODE_MOUSE_BUTTON  = (1 << 5),
	SNG_TERM_MODE_MOUSE_MOTION  = (1 << 6),
	SNG_TERM_MODE_REVERSE       = (1 << 7),
	SNG_TERM_MODE_KEYBOARD_LOCK = (1 << 8),
	SNG_TERM_MODE_HIDE          = (1 << 9),
	SNG_TERM_MODE_ECHO          = (1 << 10),
	SNG_TERM_MODE_APP_CURSOR    = (1 << 11),
	SNG_TERM_MODE_MOUSE_SGR     = (1 << 12),
	SNG_TERM_MODE_8BIT          = (1 << 13),
	SNG_TERM_MODE_BLINK         = (1 << 14),
	SNG_TERM_MODE_FBLINK        = (1 << 15),
	SNG_TERM_MODE_FOCUS         = (1 << 16),
	SNG_TERM_MODE_MOUSE_X10     = (1 << 17),
	SNG_TERM_MODE_MOUSE_MANY    = (1 << 18),
	SNG_TERM_MODE_MOUSE_MASK    =
		SNG_TERM_MODE_MOUSE_BUTTON |
		SNG_TERM_MODE_MOUSE_MOTION |
		SNG_TERM_MODE_MOUSE_X10    |
		SNG_TERM_MODE_MOUSE_MANY,
};

typedef struct SngTerm SngTerm;

// sngTermAllocSize returns how many bytes should be allocated for the
// memory passed into sngTermInit.
size_t sngTermAllocSize(int maxWidth, int maxHeight);

// sngTermInit initializes memory, expected to be sized by
// sngTermAllocSize, and returns a pointer to SngTerm. If seedTerm is
// not NULL, we initialize with seedTerm's state.
SngTerm *sngTermInit(
	void *memory, size_t memorySize,
	int maxWidth, int maxHeight,
	SngTerm *seedTerm
);

// sngTermSetSize resizes the terminal, where width and height must be
// less than maxWidth and maxHeight. Returns zero on error.
b32 sngTermSetSize(SngTerm *t, int width, int height);

// sngTermUpdate updates t's state as it parses another codepoint.
void sngTermUpdate(SngTerm *t, u32 codepoint);

#endif // SNG_TERMINAL_H


#ifdef SNG_TERMINAL_IMPLEMENTATION

enum {
	_SNG_TERM_CURSOR_DEFAULT   = (1 << 0),
	_SNG_TERM_CURSOR_WRAP_NEXT = (1 << 1),
	_SNG_TERM_CURSOR_ORIGIN    = (1 << 2),
};

enum {
	_SNG_TERM_TAB_SPACES = 8,
};

typedef struct {
	u32 codepoint;
	u16 fg, bg;
	u16 attr;
	u8 _pad[2];
} SngTermCell;

typedef struct {
	SngTermCell attr;
	int x, y;
	u16 state;
	u8 _pad[2];
} SngTermCursor;

// _SngTermCSI stores state for "Control Sequence Introducor"
// sequences. (ESC+[)
typedef struct {
	char buf[256];
	int bufLen;
	int args[8];
	int argsLen;
	char mode;
	b8 priv;
	u8 _pad[2];
} _SngTermCSI;

// _SngTermSTR stores state for STR sequences.
// STR sequences are similar to CSI sequences, but have string arguments
// (and as far as I can tell, STR is just for "string"; STR is the name
// I took from suckless which I imagine comes from rxvt or xterm).
typedef struct {
	u32 typeCodepoint;
	char buf[256];
	int bufLen;
	char *args[8];
	int argsLen;
	u8 _pad[4];
} _SngTermSTR;

typedef void (*_SngTermState)(SngTerm *, u32);

struct SngTerm {
	SngTermCell **lines;
	SngTermCell **altLines;
	b8 *dirtyLines;
	SngTermCursor cur;
	SngTermCursor cur_saved;
	int maxWidth, maxHeight;
	int width, height;
	int top, bottom; // TODO: could use better names
	s32 mode;
	s32 changed; // user should zero this when changes are rendered
	_SngTermState state;
	b8 *tabs;
	int tabsLen;
	char title[256];
	u8 _pad[4];
	union {
		_SngTermCSI csi;
		_SngTermSTR str;
	};
};

static int _sngTermClamp(int value, int min, int max) {
	if (value < min) {
		return min;
	}
	if (value > max) {
		return max;
	}
	return value;
}

static int _sngTermMin(int a, int b) {
	if (a < b) {
		return a;
	}
	return b;
}

static b32 _sngTermBetween(int val, int min, int max) {
	if (val < min || val > max) {
		return 0;
	}
	return 1;
}

static b32 _sngTermHandleControlCode(SngTerm *t, u32 c);
static void _sngTermInsertBlanks(SngTerm *t, int n);
static void _sngTermMoveTo(SngTerm *t, int x, int y);
static void _sngTermStateParse(SngTerm *t, u32 c);
static void _sngTermStateParseEsc(SngTerm *t, u32 c);
static void _sngTermStateParseEscAltCharset(SngTerm *t, u32 c);
static void _sngTermStateParseEscCSI(SngTerm *t, u32 c);
static void _sngTermStateParseEscSTR(SngTerm *t, u32 c);
static void _sngTermStateParseEscSTREnd(SngTerm *t, u32 c);
static void _sngTermStateParseEscTest(SngTerm *t, u32 c);

static SngTermCursor _sngTermDefaultCursor() {
	SngTermCursor cur = {};
	cur.attr.fg = SNG_TERM_COLOR_DEFAULT_FG;
	cur.attr.bg = SNG_TERM_COLOR_DEFAULT_BG;
	return cur;
}

static void _sngTermSaveCursor(SngTerm *t) {
	t->cur_saved = t->cur;
}

static void _sngTermRestoreCursor(SngTerm *t) {
	t->cur = t->cur_saved;
	_sngTermMoveTo(t, t->cur.x, t->cur.y);
}

static void _sngTermClear(SngTerm *t, int x0, int y0, int x1, int y1) {
	if (x0 > x1) {
		int tmp = x1;
		x1 = x0;
		x0 = tmp;
	}
	if (y0 > y1) {
		int tmp = y1;
		y1 = y0;
		y0 = tmp;
	}
	x0 = _sngTermClamp(x0, 0, t->width-1);
	x1 = _sngTermClamp(x1, 0, t->width-1);
	y0 = _sngTermClamp(y0, 0, t->height-1);
	y1 = _sngTermClamp(y1, 0, t->height-1);
	t->changed |= SNG_TERM_CHANGED_SCREEN;
	for (intptr_t y = y0; y <= y1; y++) {
		t->dirtyLines[y] = 1;
		for (intptr_t x = x0; x <= x1; x++) {
			t->lines[y][x] = t->cur.attr;
			t->lines[y][x].codepoint = ' ';
		}
	}
}

static void _sngTermClearAll(SngTerm *t) {
	_sngTermClear(t, 0, 0, t->width-1, t->height-1);
}

static void _sngTermDirtyAll(SngTerm *t) {
	t->changed |= SNG_TERM_CHANGED_SCREEN;
	for (intptr_t y = 0; y < t->height; y++) {
		t->dirtyLines[y] = 1;
	}
}

static void _sngTermMoveTo(SngTerm *t, int x, int y) {
	int miny, maxy;	
	if (t->cur.state & _SNG_TERM_CURSOR_ORIGIN) {
		miny = t->top;
		maxy = t->bottom;
	} else {
		miny = 0;
		maxy = t->height - 1;
	}
	t->changed |= SNG_TERM_CHANGED_SCREEN;
	t->cur.state &= ~_SNG_TERM_CURSOR_WRAP_NEXT;
	t->cur.x = _sngTermClamp(x, 0, t->width-1);
	t->cur.y = _sngTermClamp(y, miny, maxy);
}

static void _sngTermMoveAbsTo(SngTerm *t, int x, int y) {
	if (t->cur.state & _SNG_TERM_CURSOR_ORIGIN) {
		y += t->top;
	}
	_sngTermMoveTo(t, x, y);
}

static void _sngTermReset(SngTerm *t) {
	t->cur = _sngTermDefaultCursor();
	_sngTermSaveCursor(t);
	for (intptr_t i = 0; i < t->tabsLen; i++) {
		t->tabs[i] = 0;
	}
	for (intptr_t i = _SNG_TERM_TAB_SPACES; i < t->tabsLen; i += _SNG_TERM_TAB_SPACES) {
		t->tabs[i] = 1;
	}
	t->top = 0;
	t->bottom = t->height - 1;
	_sngTermClearAll(t);
	_sngTermMoveTo(t, 0, 0);
}

static void _sngTermSetScroll(SngTerm *t, int top, int bottom) {
	top = _sngTermClamp(top, 0, t->height-1);
	bottom = _sngTermClamp(bottom, 0, t->height-1);
	if (top > bottom) {
		int tmp = top;
		top = bottom;
		bottom = tmp;
	}
	t->top = top;
	t->bottom = bottom;
}

static void _sngTermScrollDown(SngTerm *t, int orig, int n) {
	n = _sngTermClamp(n, 0, t->bottom-orig+1);
	_sngTermClear(t, 0, t->bottom-n+1, t->width-1, t->bottom);
	for (intptr_t i = t->bottom; i >= orig+n; i--) {
		SngTermCell *tmp = t->lines[i];
		t->lines[i] = t->lines[i+n];
		t->lines[i+n] = tmp;
		t->dirtyLines[i] = 1;
		t->dirtyLines[i+n] = 1;
	}
}

static void _sngTermScrollUp(SngTerm *t, int orig, int n) {
	n = _sngTermClamp(n, 0, t->bottom-orig+1);
	_sngTermClear(t, 0, orig, t->width-1, orig+n-1);
	for (intptr_t i = orig; i <= t->bottom-n; i++) {
		SngTermCell *tmp = t->lines[i];
		t->lines[i] = t->lines[i+n];
		t->lines[i+n] = tmp;
		t->dirtyLines[i] = 1;
		t->dirtyLines[i+n] = 1;
	}
}

static void _sngTermSwapScreen(SngTerm *t) {
	SngTermCell **tmp_lines = t->lines;
	t->lines = t->altLines;
	t->altLines = tmp_lines;
	t->mode ^= SNG_TERM_MODE_ALT_SCREEN;
	_sngTermDirtyAll(t);
}

static void _sngTermPutTab(SngTerm *t, b32 forward) {
	int x = t->cur.x;
	if (forward) {
		if (x == t->width) {
			return;
		}
		x++;
		while (x < t->width && !t->tabs[x]) {
			x++;
		}
	} else {
		if (x == 0) {
			return;
		}
		x--;
		while (x > 0 && !t->tabs[x]) {
			x--;
		}
	}
	_sngTermMoveTo(t, x, t->cur.y);
}

static void _sngTermNewLine(SngTerm *t, b32 first_column) {
	int y = t->cur.y;
	if (y == t->bottom) {
		SngTermCursor cur = t->cur;
		t->cur = _sngTermDefaultCursor();
		_sngTermScrollUp(t, t->top, 1);
		t->cur = cur;
	} else {
		y++;
	}
	if (first_column) {
		_sngTermMoveTo(t, 0, y);
	} else {
		_sngTermMoveTo(t, t->cur.x, y);
	}
}

static void _sngTermInsertBlanks(SngTerm *t, int n) {
	int src = t->cur.x;
	int dst = src + n;
	int size = t->width - dst;
	t->changed |= SNG_TERM_CHANGED_SCREEN;
	t->dirtyLines[t->cur.y] = 1;
	if (dst >= t->width) {
		_sngTermClear(t, t->cur.x, t->cur.y, t->width-1, t->cur.y);
	} else {
		memmove(
			&t->lines[t->cur.y][dst],
			&t->lines[t->cur.y][src],
			(u32)size * sizeof(t->lines[0][0])
		);
		_sngTermClear(t, src, t->cur.y, dst-1, t->cur.y);
	}
}

static void _sngTermInsertBlankLines(SngTerm *t, int n) {
	if (t->cur.y < t->top || t->cur.y > t->bottom) {
		return;
	}
	_sngTermScrollDown(t, t->cur.y, n);
}

static void _sngTermDeleteLines(SngTerm *t, int n) {
	if (t->cur.y < t->top || t->cur.y > t->bottom) {
		return;
	}
	_sngTermScrollUp(t, t->cur.y, n);
}

static void _sngTermDeleteChars(SngTerm *t, int n) {
	int src = t->cur.x + n;
	int dst = t->cur.x;
	u32 size = (u32)(t->width - src);
	t->changed |= SNG_TERM_CHANGED_SCREEN;
	t->dirtyLines[t->cur.y] = 1;

	if (src >= t->width) {
		_sngTermClear(t, t->cur.x, t->cur.y, t->width-1, t->cur.y);
	} else {
		memmove(&t->lines[t->cur.y][dst], &t->lines[t->cur.y][src], size * sizeof(t->lines[0][0]));
		_sngTermClear(t, t->width-n, t->cur.y, t->width-1, t->cur.y);
	}
}

static void _sngTermModMode(SngTerm *t, b32 set, s32 bit) {
	if (set) {
		t->mode |= bit;
	} else {
		t->mode &= ~bit;
	}
}

static void _sngTermSetMode(SngTerm *t, b32 priv, b32 set, int *args, int argsLen) {
	if (priv) {
		for (intptr_t i = 0; i < argsLen; i++) {
			switch (args[i]) {
				// DECCKM - cursor key
				case 1: {
					_sngTermModMode(t, set, SNG_TERM_MODE_APP_CURSOR);
				} break;
				// DECSCNM - reverse video
				case 5: {
					s32 mode = t->mode;
					_sngTermModMode(t, set, SNG_TERM_MODE_REVERSE);
					if (mode != t->mode) {
						// TODO: redraw
					}
				} break;
				// DECOM - origin
				case 6: {
					if (set) {
						t->cur.state |= _SNG_TERM_CURSOR_ORIGIN;
					} else {
						t->cur.state &= ~(_SNG_TERM_CURSOR_ORIGIN);
					}
					_sngTermMoveAbsTo(t, 0, 0);
				} break;
				// DECAWM - auto wrap
				case 7: {
					_sngTermModMode(t, set, SNG_TERM_MODE_WRAP);
				} break;
				// IGNORED
				case 0:  // error
				case 2:  // DECANM - ANSI/VT52
				case 3:  // DECCOLM - column
				case 4:  // DECSCLM - scroll
				case 8:  // DECARM - auto repeat
				case 18: // DECPFF - printer feed
				case 19: // DECPEX - printer extent
				case 42: // DECNRCM - national characters
				case 12: // att610 - start blinking cursor
					break;
				// DECTCEM - text cursor enable mode
				case 25: {
					_sngTermModMode(t, !set, SNG_TERM_MODE_HIDE);
				} break;
				// X10 mouse compatibility mode
				case 9: {
					_sngTermModMode(t, 0, SNG_TERM_MODE_MOUSE_MASK);
					_sngTermModMode(t, set, SNG_TERM_MODE_MOUSE_X10);
				} break;
				// report button press
				case 1000: {
					_sngTermModMode(t, 0, SNG_TERM_MODE_MOUSE_MASK);
					_sngTermModMode(t, set, SNG_TERM_MODE_MOUSE_BUTTON);
				} break;
				// report motion on button press
				case 1002: {
					_sngTermModMode(t, 0, SNG_TERM_MODE_MOUSE_MASK);
					_sngTermModMode(t, set, SNG_TERM_MODE_MOUSE_MOTION);
				} break;
				// enable all mouse motions
				case 1003: {
					_sngTermModMode(t, 0, SNG_TERM_MODE_MOUSE_MASK);
					_sngTermModMode(t, set, SNG_TERM_MODE_MOUSE_MANY);
				} break;
				// send focus events to tty
				case 1004: {
					_sngTermModMode(t, set, SNG_TERM_MODE_FOCUS);
				} break;
				// extended reporting mode
				case 1006: {
					_sngTermModMode(t, set, SNG_TERM_MODE_MOUSE_SGR);
				} break;
				case 1034: {
					_sngTermModMode(t, set, SNG_TERM_MODE_8BIT);
				} break;
				case 47:
				case 1047:
				case 1049: {
					b32 alt = ((t->mode & SNG_TERM_MODE_ALT_SCREEN) != 0);
					if (alt) {
						_sngTermClear(t, 0, 0, t->width-1, t->height-1);
					}
					if (!set || !alt) {
						_sngTermSwapScreen(t);
					}
					if (args[i] != 1049) {
						break;
					}
				} // fallthrough
				case 1048: {
					if (set) {
						_sngTermSaveCursor(t);
					} else {
						_sngTermRestoreCursor(t);
					}
				} break;
				// mouse highlight mode; can hang the terminal by design when
				// implemented
				case 1001: {
				} break;
				// utf8 mouse mode; will confuse applications not supporting
				// utf8 and luit
				case 1005: {
				} break;
				// urxvt mangled mouse mode; incompatible and can be mistaken
				// for other control codes
				case 1015: {
				} break;
				default: {
					fprintf(stderr, "unknown private set/reset mode %d\n", args[i]);
				} break;
			}
		}
	} else {
		for (intptr_t i = 0; i < argsLen; i++) {
			switch (args[i]) {
				// Error (ignored)
				case 0: break;
				// KAM - keyboard action
				case 2: {
					_sngTermModMode(t, set, SNG_TERM_MODE_KEYBOARD_LOCK);
				} break;
				// IRM - insert-replacement
				case 4: {
					_sngTermModMode(t, set, SNG_TERM_MODE_INSERT);
					fprintf(stderr, "SNG_TERM_MODE_INSERT not implemented\n");
				} break;
				// SRM - send/receive
				case 12: {
					_sngTermModMode(t, set, SNG_TERM_MODE_ECHO);
				} break;
				// LNM - linefeed/newline
				case 20: {
					_sngTermModMode(t, set, SNG_TERM_MODE_CRLF);
				} break;
				case 34: {
					fprintf(stderr, "right-to-left mode not implemented\n");
				} break;
				case 96: {
					fprintf(stderr, "right-to-left copy mode not implemented\n");
				} break;
				default: {
					fprintf(stderr, "unknown set/reset mode %d\n", args[i]);
				} break;
			}
		}
	}
}

static void _sngTermSetAttr(SngTerm *t, int *args, int argsLen) {
	int argsReset = 0;
	if (argsLen == 0) {
		args = &argsReset;
		argsLen = 1;
	}
	for (intptr_t i = 0; i < argsLen; i++) {
		int a = args[i];
		switch (a) {
			case 0: {
				t->cur.attr.attr &= ~(
					SNG_TERM_ATTR_REVERSE | 
					SNG_TERM_ATTR_UNDERLINE | 
					SNG_TERM_ATTR_BOLD | 
					SNG_TERM_ATTR_ITALIC | 
					SNG_TERM_ATTR_BLINK
				);
				t->cur.attr.fg = SNG_TERM_COLOR_DEFAULT_FG;
				t->cur.attr.bg = SNG_TERM_COLOR_DEFAULT_BG;
			} break;
			case 1: {
				t->cur.attr.attr |= SNG_TERM_ATTR_BOLD;
			} break;
			case 3: {
				t->cur.attr.attr |= SNG_TERM_ATTR_ITALIC;
			} break;
			case 4: {
				t->cur.attr.attr |= SNG_TERM_ATTR_UNDERLINE;
			} break;
			case 5:
			case 6: {
				// slow or rapid blink; we treat both the same
				t->cur.attr.attr |= SNG_TERM_ATTR_BLINK;
			} break;
			case 7: {
				t->cur.attr.attr |= SNG_TERM_ATTR_REVERSE;
			} break;
			case 21:
			case 22: {
				t->cur.attr.attr &= ~(SNG_TERM_ATTR_BOLD);
			} break;
			case 23: {
				t->cur.attr.attr &= ~(SNG_TERM_ATTR_ITALIC);
			} break;
			case 24: {
				t->cur.attr.attr &= ~(SNG_TERM_ATTR_UNDERLINE);
			} break;
			case 25:
			case 26: {
				t->cur.attr.attr &= ~(SNG_TERM_ATTR_BLINK);
			} break;
			case 27: {
				t->cur.attr.attr &= ~(SNG_TERM_ATTR_REVERSE);
			} break;
			case 38: {
				if (i+2 < argsLen && args[i+1] == 5) {
					i += 2;
					if (_sngTermBetween(args[i], 0, 255)) {
						t->cur.attr.fg = (u16)args[i];
					} else {
						fprintf(stderr, "sng_terminal: bad fgcolor %d\n", args[i]);
					}
				} else {
					fprintf(stderr, "sng_terminal: gfx attr %d had unexpected args\n", a);
				}
			} break;
			case 39: {
				t->cur.attr.fg = SNG_TERM_COLOR_DEFAULT_FG;
			} break;
			case 48: {
				if (i+2 < argsLen && args[i+1] == 5) {
					i += 2;
					if (_sngTermBetween(args[i], 0, 255)) {
						t->cur.attr.bg = (u16)args[i];
					} else {
						fprintf(stderr, "sng_terminal: bad bgcolor %d\n", args[i]);
					}
				} else {
					fprintf(stderr, "sng_terminal: gfx attr %d had unexpected args\n", a);
				}
			} break;
			case 49: {
				t->cur.attr.bg = SNG_TERM_COLOR_DEFAULT_BG;
			} break;
			default: {
				if (_sngTermBetween(args[i], 30, 37)) {
					t->cur.attr.fg = (u16)(a - 30);
				} else if (_sngTermBetween(args[i], 40, 47)) {
					t->cur.attr.bg = (u16)(a - 40);
				} else if (_sngTermBetween(args[i], 90, 97)) {
					t->cur.attr.fg = (u16)(a - 90 + 8);
				} else if (_sngTermBetween(args[i], 100, 107)) {
					t->cur.attr.bg = (u16)(a - 100 + 8);
				} else {
					fprintf(stderr, "sng_terminal: gfx attr %d unknown \n", a);
				}
				t->cur.attr.bg = SNG_TERM_COLOR_DEFAULT_BG;
			} break;
		}
	}
}

static void _sngTermCSIParse(_SngTermCSI *c) {
	SNG_ASSERT(c->bufLen > 0);
	c->mode = c->buf[c->bufLen-1];
	if (c->bufLen == 1) {
		return;
	}
	char *s = &c->buf[0];
	int sLen = c->bufLen;
	c->argsLen = 0;
	if (s[0] == '?') {
		c->priv = 1;
		s = &s[1];
		sLen--;
	}
	sLen--;
	s[sLen] = 0;
	while (1) {
		char *end;
		int num = (int)strtol(s, &end, 10);
		if (*end == ';') {
			s = &end[1];
			SNG_ASSERT(c->argsLen < 8);
			if (c->argsLen < 8) {
				c->args[c->argsLen] = num;
				c->argsLen++;
			}
		} else if (*end == '\0') {
			SNG_ASSERT(c->argsLen < 8);
			if (c->argsLen < 8) {
				c->args[c->argsLen] = num;
				c->argsLen++;
			}
			break;
		} else {
			// TODO: inform about bad input
			fprintf(stderr, "sadness\n");
			break;
		}
	}
}

static int _sngTermCSIArg(_SngTermCSI *c, int i, int def) {
	if (i >= c->argsLen || i < 0) {
		return def;
	}
	return c->args[i];
}

// _sngTermCSIMaxArg takes the max of _sngTermCSIArg(c, i, def) and def.
static int _sngTermCSIMaxArg(_SngTermCSI *c, int i, int def) {
	int arg = _sngTermCSIArg(c, i, def);
	if (def > arg) {
		return def;
	}
	return arg;
}

static void _sngTermCSIReset(_SngTermCSI *c) {
	c->buf[0] = 0;
	c->bufLen = 0;
	c->argsLen = 0;
	c->mode = 0;
	c->priv = 0;
}

static b32 _sngTermCSIPut(_SngTermCSI *c, char b) {
	c->buf[c->bufLen] = b;
	c->bufLen++;
	if (((u8)b >= 0x40 && (u8)b <= 0x7e) || c->bufLen >= 256) {
		_sngTermCSIParse(c);
		return 1;
	}
	return 0;
}

static b32 _sngTermIsControlCode(u32 c) {
	return (c < 0x20 || c == 0177);
}

static b32 _sngTermHandleControlCode(SngTerm *t, u32 c) {
	if (!_sngTermIsControlCode(c)) {
		return 0;
	}
	switch (c) {
		// HT
		case '\t': {
			b32 forward = 1;
			_sngTermPutTab(t, forward);
		} break;
		// BS
		case '\b': {
			_sngTermMoveTo(t, t->cur.x-1, t->cur.y);
		} break;
		// CR
		case '\r': {
			_sngTermMoveTo(t, 0, t->cur.y);
		} break;
		// LF, VT, LF
		case '\f':
		case '\v':
		case '\n': {
			b32 first_column = ((t->mode & SNG_TERM_MODE_CRLF) != 0);
			_sngTermNewLine(t, first_column);
		} break;
		// BEL
		case '\a': {
			// TODO: notify user somehow? maybe SngTerm will have an
			// event field, which ofc would only be good until the next
			// sngTermUpdate call.
		} break;
		// ESC
		case 033: {
			_sngTermCSIReset(&t->csi);
			t->state = _sngTermStateParseEsc;
		} break;
		// SO, SI
		case 016:
		case 017: {
			// different charsets not supported. apps should use the
			// correct alt charset escapes, probably for line drawing.
		} break;
		// SUB, CAN
		case 032:
		case 030: {
			_sngTermCSIReset(&t->csi);
		} break;
		// ignore ENQ, NUL, XON, XOFF, DEL
		case 005:
		case 000:
		case 021:
		case 023:
		case 0117: {
		} break;
		default: {
			return 0;
		}
	}
	return 1;
}

static void _sngTermHandleCSI(SngTerm *t) {
	_SngTermCSI *c = &t->csi;
	switch (c->mode) {
		// ICH - insert <n> blank chars
		case '@': {
			_sngTermInsertBlanks(t, _sngTermCSIArg(c, 0, 1));
		} break;
		// CUU - cursor <n> up
		case 'A': {
			_sngTermMoveTo(t, t->cur.x, t->cur.y - _sngTermCSIMaxArg(c, 0, 1));
		} break;
		// CUD, VPR - cursor <n> down
		case 'B':
		case 'e': {
			_sngTermMoveTo(t, t->cur.x, t->cur.y + _sngTermCSIMaxArg(c, 0, 1));
		} break;
		// DA - device attributes
		case 'c': {
			if (_sngTermCSIArg(c, 0, 0) == 0) {
				// TODO: write vt102 id
			}
		} break;
		// CUF, HPR - cursor <n> forward
		case 'C': case 'a': {
			_sngTermMoveTo(t, t->cur.x + _sngTermCSIMaxArg(c, 0, 1), t->cur.y);
		} break;
		// CUB - cursor <n> backward
		case 'D': {
			_sngTermMoveTo(t, t->cur.x - _sngTermCSIMaxArg(c, 0, 1), t->cur.y);
		} break;
		// CNL - cursor <n> down and first col
		case 'E': {
			_sngTermMoveTo(t, 0, t->cur.y + _sngTermCSIArg(c, 0, 1));
		} break;
		// CPL - cursor <n> up and first col
		case 'F': {
			_sngTermMoveTo(t, 0, t->cur.y - _sngTermCSIArg(c, 0, 1));
		} break;
		// TBC - tabulation clear
		case 'g': {
			switch (_sngTermCSIArg(c, 0, 0)) {
				// clear current tab stop
				case 0: {
					t->tabs[t->cur.x] = 0;
				} break;
				// clear all tabs
				case 3: {
					for (intptr_t i = 0; i < t->tabsLen; i++) {
						t->tabs[i] = 0;
					}
				} break;
				default: {
					goto unexpected;
				}
			}
		} break;
		// CHA, HPA - move to <col>
		case 'G': case '`': {
			_sngTermMoveTo(t, _sngTermCSIArg(c, 0, 1) - 1, t->cur.y);
		} break;
		// CUP, HVP - move to <row> <col>
		case 'H': case 'f': {
			_sngTermMoveAbsTo(t, _sngTermCSIArg(c, 1, 1) - 1, _sngTermCSIArg(c, 0, 1) - 1);
		} break;
		// CHT - cursor forward tabulation <n> tab stops
		case 'I': {
			intptr_t n = _sngTermCSIArg(c, 0, 1);
			for (intptr_t i = 0; i < n; i++) {
				_sngTermPutTab(t, 1);
			}
		} break;
		// ED - clear screen
		case 'J': {
			// TODO: sel.ob.x = -1
			switch (_sngTermCSIArg(c, 0, 0)) {
 				// below
				case 0: {
					_sngTermClear(t, t->cur.x, t->cur.y, t->width - 1, t->cur.y);
					if (t->cur.y < t->height - 1) {
						_sngTermClear(t, 0, t->cur.y + 1, t->width - 1, t->height - 1);
					}
				} break;
				// above
				case 1: {
					if (t->cur.y > 1) {
						_sngTermClear(t, 0, 0, t->width - 1, t->height - 1);
					}
					_sngTermClear(t, 0, t->cur.y, t->cur.x, t->cur.y);
				} break;
				// all
				case 2: {
					_sngTermClear(t, 0, 0, t->width - 1, t->height - 1);
				} break;
				default: {
					goto unexpected;
				} break;
			}
		} break;
		// EL - clear line
		case 'K': {
			switch (_sngTermCSIArg(c, 0, 0)) {
				// right
				case 0: {
					_sngTermClear(t, t->cur.x, t->cur.y, t->width - 1, t->cur.y);
				} break;
				// left
				case 1: {
					_sngTermClear(t, 0, t->cur.y, t->cur.x, t->cur.y);
				} break;
				// all
				case 2:{
					_sngTermClear(t, 0, t->cur.y, t->width - 1, t->cur.y);
				} break;
				default: {
					goto unexpected;
				} break;
			}
		} break;
		// SU - scroll <n> lines up
		case 'S': {
			_sngTermScrollUp(t, t->top, _sngTermCSIArg(c, 0, 1));
		} break;
		// SD - scroll <n> lines down
		case 'T': {
			_sngTermScrollDown(t, t->top, _sngTermCSIArg(c, 0, 1));
		} break;
		// IL - insert <n> blank lines
		case 'L': {
			_sngTermInsertBlankLines(t, _sngTermCSIArg(c, 0, 1));
		} break;
		// RM - reset mode
		case 'l': {
			_sngTermSetMode(t, c->priv, 0, c->args, c->argsLen);
		} break;
		// DL - delete <n> lines
		case 'M': {
			_sngTermDeleteLines(t, _sngTermCSIArg(c, 0, 1));
		} break;
		// ECH - erase <n> chars
		case 'X': {
			_sngTermClear(t, t->cur.x, t->cur.y, t->cur.x + _sngTermCSIArg(c, 0, 1) - 1, t->cur.y);
		} break;
		// DCH - delete <n> chars
		case 'P': {
			_sngTermDeleteChars(t, _sngTermCSIArg(c, 0, 1));
		} break;
		// CBT - cursor backward tabulation <n> tab stops
		case 'Z': {
			intptr_t n = _sngTermCSIArg(c, 0, 1);
			for (intptr_t i = 0; i < n; i++) {
				_sngTermPutTab(t, 0);
			}
		} break;
		// VPA - move to <row>
		case 'd': {
			_sngTermMoveAbsTo(t, t->cur.x, _sngTermCSIArg(c, 0, 1) - 1);
		} break;
		// SM - set terminal mode
		case 'h': {
			_sngTermSetMode(t, c->priv, 1, c->args, c->argsLen);
		} break;
		// SGR - terminal attribute (color)
		case 'm': {
			_sngTermSetAttr(t, c->args, c->argsLen);
		} break;
		// DECSTBM - set scrolling region
		case 'r': {
			if (c->priv) {
				goto unexpected;
			} else {
				_sngTermSetScroll(t, _sngTermCSIArg(c, 0, 1) - 1, _sngTermCSIArg(c, 1, t->height) - 1);
				_sngTermMoveAbsTo(t, 0, 0);
			}
		} break;
		// DECSC - save cursor position (ANSI.SYS)
		case 's': {
			_sngTermSaveCursor(t);
		} break;
		// DECRC - restore cursor position (ANSI.SYS)
		case 'u': {
			_sngTermRestoreCursor(t);
		} break;
		default: {
			// TODO: log to a user buffer.
			fprintf(stderr, "unknown CSI sequence '%c'\n", c->mode);
		} break;
	}
	return;
unexpected:
	// TODO: log to a user buffer.
	fprintf(stderr, "unexpected args for CSI sequence '%c'\n", c->mode);
}

static void _sngTermSTRReset(_SngTermSTR *s) {
	s->typeCodepoint = 0;
	s->buf[0] = 0;
	s->argsLen = 0;
}

static void _sngTermSTRPut(_SngTermSTR *s, char c) {
	if (s->bufLen < 256-1) {
		s->buf[s->bufLen] = c;
		s->buf[s->bufLen+1] = '\0';
		s->bufLen++;
	}
	// Remain silent if STR sequence does not end so that it is apparent
	// to user that something is wrong.
}

static void _sngTermSTRParse(_SngTermSTR *s) {
	SNG_ASSERT(s->bufLen > 0);
	char *arg = &s->buf[0];
	for (intptr_t i = 0; i < s->bufLen; i++) {
		if (s->buf[i] == ';') {
			s->buf[i] = '\0';
			SNG_ASSERT(s->argsLen < 8);
			if (s->argsLen < 8) {
				s->args[s->argsLen] = arg;
				s->argsLen++;
				arg = &s->buf[i+1];
			}
		}
	}
	// last one is already null terminated, since it is the end of the string.
	SNG_ASSERT(s->argsLen < 8);
	if (s->argsLen < 8) {
		s->args[s->argsLen] = arg;
		s->argsLen++;
	}
}

static int _sngTermSTRArg(_SngTermSTR *s, int i, int def) {
	if (i >= s->argsLen || i < 0) {
		return def;
	}
	char *end;
	long val = strtol(s->args[i], &end, 10);
	if (*end != 0) {
		return def;
	}
	return (int)val;
}

static const char *_sngTermSTRArgString(_SngTermSTR *s, int i, const char *def) {
	if (i >= s->argsLen || i < 0) {
		return def;
	}
	return s->args[i];
}

static void _sngTermHandleSTR(SngTerm *t) {
	_SngTermSTR *s = &t->str;
	_sngTermSTRParse(s);
	switch (s->typeCodepoint) {
		// OSC - operating system command
		case ']': {
			int cmd = _sngTermSTRArg(s, 0, 0);
			switch (cmd) {
				// title
				case 0:
				case 1:
				case 2: {
					t->changed |= SNG_TERM_CHANGED_TITLE;
					snprintf(t->title, sizeof(t->title), "%s", _sngTermSTRArgString(s, 1, ""));
				} break;
				// color set
				case 4: {
					// TODO(james4k): color set
				} break;
				// color reset
				case 104: {
					// TODO(james4k): color reset
				} break;
				default: {
					// TODO: log to a user buffer.
					fprintf(stderr, "unknown OSC command '%d'\n", cmd);
				} break;
			}
		} break;
		// old title set compatibility
		case 'k': {
			t->changed |= SNG_TERM_CHANGED_TITLE;
			snprintf(t->title, sizeof(t->title), "%s", _sngTermSTRArgString(s, 1, ""));
		} break;
		default: {
			// TODO: log to a user buffer.
			fprintf(stderr, "unknown STR sequence '0x%x'\n", s->typeCodepoint);
		} break;
	}
}

static u32 _sngTermGfxCharTable[62] = {
	L'↑', L'↓', L'→', L'←', L'█', L'▚', L'☃',       // A - G
	0, 0, 0, 0, 0, 0, 0, 0,                         // H - O
	0, 0, 0, 0, 0, 0, 0, 0,                         // P - W
	0, 0, 0, 0, 0, 0, 0, ' ',                       // X - _
	L'◆', L'▒', L'␉', L'␌', L'␍', L'␊', L'°', L'±', // ` - g
	L'␤', L'␋', L'┘', L'┐', L'┌', L'└', L'┼', L'⎺', // h - o
	L'⎻', L'─', L'⎼', L'⎽', L'├', L'┤', L'┴', L'┬', // p - w
	L'│', L'≤', L'≥', L'π', L'≠', L'£', L'·',       // x - ~
};

static void _sngTermSetChar(
	SngTerm *t,
	u32 c,
	SngTermCell *cell,
	int x, int y
) {
	if (
		(cell->attr & SNG_TERM_ATTR_GFX) != 0 &&
		(c >= 0x41 && c <= 0x7e) &&
		_sngTermGfxCharTable[c-0x41] != 0
	) {
		c = _sngTermGfxCharTable[c-0x41];
	}
	t->changed |= SNG_TERM_CHANGED_SCREEN;
	t->dirtyLines[y] = 1;
	t->lines[y][x] = *cell;
	t->lines[y][x].codepoint = c;
	if ((cell->attr & SNG_TERM_ATTR_BOLD) && cell->fg < 8) {
		t->lines[y][x].fg = cell->fg + 8;
	}
	if (cell->attr & SNG_TERM_ATTR_REVERSE) {
		t->lines[y][x].fg = cell->bg;
		t->lines[y][x].bg = cell->fg;
	}
}

static void _sngTermStateParse(SngTerm *t, u32 codepoint) {
	if (_sngTermIsControlCode(codepoint)) {
		b32 handled = _sngTermHandleControlCode(t, codepoint);
		if (handled || (t->cur.attr.attr & SNG_TERM_ATTR_GFX) == 0) {
			return;
		}
	}

	// TODO: update selection; see st.c:2450
	
	if (
		(t->mode & SNG_TERM_MODE_WRAP) != 0 &&
		(t->cur.state & _SNG_TERM_CURSOR_WRAP_NEXT) != 0
	) {
		t->lines[t->cur.y][t->cur.x].attr |= SNG_TERM_ATTR_WRAP;
		
		b32 first_column = 1;
		_sngTermNewLine(t, first_column);
	}

	if (
		(t->mode & SNG_TERM_MODE_INSERT) != 0 &&
		t->cur.x+1 < t->width
	) {
		// TODO: move stuff; see st.c:2458
		// TODO: log to user buffer
		fprintf(stderr, "insert mode not implemented\n");
	}

	_sngTermSetChar(t, codepoint, &t->cur.attr, t->cur.x, t->cur.y);
	if (t->cur.x+1 < t->width) {
		_sngTermMoveTo(t, t->cur.x+1, t->cur.y);
	} else {
		t->cur.state |= _SNG_TERM_CURSOR_WRAP_NEXT;
	}
}

static void _sngTermStateParseEsc(SngTerm *t, u32 c) {
	if (_sngTermHandleControlCode(t, c)) {
		return;
	}
	_SngTermState next = _sngTermStateParse;
	switch (c) {
		case '[': {
			next = _sngTermStateParseEscCSI;
		} break;
		case '#': {
			next = _sngTermStateParseEscTest;
		} break;
		case 'P':   // DCS - Device Control String
		case '_':   // APC - Application Program Command
		case '^':   // PM - Privacy Message
		case ']':   // OSC - Operation System Command
		case 'k': { // old title set compatibility
			_sngTermSTRReset(&t->str);
			t->str.typeCodepoint = c;
			next = _sngTermStateParseEscSTR;
		} break;
		case '(': { // set primary charset G0
			next = _sngTermStateParseEscAltCharset;
		} break;
		case ')':   // set primary charset G1 (ignored)
		case '*':   // set primary charset G2 (ignored)
		case '+': { // set primary charset G3 (ignored)
		} break;
		case 'D': { // IND - linefeed
			if (t->cur.y == t->bottom) {
				_sngTermScrollUp(t, t->top, 1);
			} else {
				_sngTermMoveTo(t, t->cur.x, t->cur.y+1);
			}
		} break;
		case 'E': { // NEL - next line
			b32 first_column = 1;
			_sngTermNewLine(t, first_column);
		} break;
		case 'H': { // HTS - horizontal tab stop
			t->tabs[t->cur.x] = 1;
		} break;
		case 'M': { // RI - reverse index
			if (t->cur.y == t->top) {
				_sngTermScrollDown(t, t->top, 1);
			} else {
				_sngTermMoveTo(t, t->cur.x, t->cur.y-1);
			}
		} break;
		case 'Z': { // DECID - identify terminal
			// TODO:
		} break;
		case 'c': { // RIS - reset to initial state
			_sngTermReset(t);
		} break;
		case '=': { // DECPAM - application keypad
			t->mode |= SNG_TERM_MODE_APP_KEYPAD;
		} break;
		case '>': { // DECPNM - normal keypad
			t->mode &= ~SNG_TERM_MODE_APP_KEYPAD;
		} break;
		case '7': { // DECSC - save cursor
			_sngTermSaveCursor(t);
		} break;
		case '8': { // DECRC - restore cursor
			_sngTermRestoreCursor(t);
		} break;
		case '\\': { // ST - stop
			// TODO: need to do anything here?
		} break;
		default: {
			// TODO: remove printf
			printf("unknown escape sequence '%c'\n", (s8)c);
		} break;
	}
	t->state = next;
}

static void _sngTermStateParseEscAltCharset(SngTerm *t, u32 c) {
	if (_sngTermHandleControlCode(t, c)) {
		return;
	}
	switch (c) {
		case '0': { // line drawing set
			t->cur.attr.attr |= SNG_TERM_ATTR_GFX;
		} break;
		case 'B': { // USASCII
			t->cur.attr.attr &= ~SNG_TERM_ATTR_GFX;
		} break;
		case 'A':   // UK (ignored)
		case '<':   // multinational (ignored)
		case '5':   // Finnish (ignored)
		case 'C':   // Finnish (ignored)
		case 'K': { // German (ignored)
		} break;
		default: {
			// TODO: remove printf
			printf("unknown alt. charset '%c' (%x0\n", (u8)c, c);
		} break;
	}
	t->state = _sngTermStateParse;
}

static void _sngTermStateParseEscCSI(SngTerm *t, u32 c) {
	if (_sngTermHandleControlCode(t, c)) {
		return;
	}
	if (_sngTermCSIPut(&t->csi, (char)c)) {
		t->state = _sngTermStateParse;
		_sngTermHandleCSI(t);
	}
}

static void _sngTermStateParseEscSTR(SngTerm *t, u32 c) {
	switch (c) {
		case '\033': {
			t->state = _sngTermStateParseEscSTREnd;
		} break;
		case '\a': { // backwards compatibility to xterm
			t->state = _sngTermStateParse;
			_sngTermHandleSTR(t);
		} break;
		default: {
			_sngTermSTRPut(&t->str, (char)c);
		} break;
	}
}

static void _sngTermStateParseEscSTREnd(SngTerm *t, u32 c) {
	if (_sngTermHandleControlCode(t, c)) {
		return;
	}
	t->state = _sngTermStateParse;
	if (c == '\\') {
		_sngTermHandleSTR(t);
	}
}

static void _sngTermStateParseEscTest(SngTerm *t, u32 c) {
	if (_sngTermHandleControlCode(t, c)) {
		return;
	}
	// DEC screen alignment test
	if (c == '8') {
		for (int y = 0; y < t->height; y++) {
			for (int x = 0; x < t->width; x++) {
				_sngTermSetChar(t, 'E', &t->cur.attr, x, y);
			}
		}
	}
	t->state = _sngTermStateParse;
}

#define _SNG_TERM_PTR_ALIGN (sizeof(void *) - 1)

#define _SNG_TERM_SIZEOF_W(w) \
	(((size_t)w + _SNG_TERM_PTR_ALIGN) & ~_SNG_TERM_PTR_ALIGN)

#define _SNG_TERM_SIZEOF_H(h) \
	(((size_t)h + _SNG_TERM_PTR_ALIGN) & ~_SNG_TERM_PTR_ALIGN)

#define _SNG_TERM_SIZEOF_TERM \
	((sizeof(SngTerm) + _SNG_TERM_PTR_ALIGN) & ~_SNG_TERM_PTR_ALIGN)

#define _SNG_TERM_SIZEOF_LINES(w, h) \
	((sizeof(SngTermCell *)*h + _SNG_TERM_PTR_ALIGN) & ~_SNG_TERM_PTR_ALIGN)

#define _SNG_TERM_SIZEOF_LINES_DATA(w, h) \
	((sizeof(SngTermCell)*w*h + _SNG_TERM_PTR_ALIGN) & ~_SNG_TERM_PTR_ALIGN)

#define _SNG_TERM_SIZEOF_DIRTYLINES(h) \
	((sizeof(b8)*h + _SNG_TERM_PTR_ALIGN) & ~_SNG_TERM_PTR_ALIGN)

#define _SNG_TERM_SIZEOF_TABS(w) \
	((sizeof(b8)*w + _SNG_TERM_PTR_ALIGN) & ~_SNG_TERM_PTR_ALIGN)

size_t sngTermAllocSize(int maxWidth, int maxHeight) {
	size_t w = _SNG_TERM_SIZEOF_W(maxWidth);
	size_t h = _SNG_TERM_SIZEOF_H(maxHeight);
	size_t term = _SNG_TERM_SIZEOF_TERM;
	size_t lines = _SNG_TERM_SIZEOF_LINES(w, h) + _SNG_TERM_SIZEOF_LINES_DATA(w, h);
	size_t altLines = _SNG_TERM_SIZEOF_LINES(w, h) + _SNG_TERM_SIZEOF_LINES_DATA(w, h);
	size_t dirtyLines = _SNG_TERM_SIZEOF_DIRTYLINES(h);
	size_t tabs = _SNG_TERM_SIZEOF_TABS(w);
	return term + lines + altLines + dirtyLines + tabs;
}

SngTerm *sngTermInit(
	void *memory, size_t memorySize,
	int maxWidth, int maxHeight,
	SngTerm *seedTerm
) {
	size_t expectedSize = sngTermAllocSize(maxWidth, maxHeight);
	if (memorySize < expectedSize) {
		return 0;
	}
	if (seedTerm != NULL) {
		// TODO: support copying state from seedTerm
		return 0;
	}
	SngTerm *t = (SngTerm *)memory;
	memset(memory, 0, memorySize);
	// w and h may be adjusted for alignment
	size_t w = _SNG_TERM_SIZEOF_W(maxWidth);
	size_t h = _SNG_TERM_SIZEOF_H(maxHeight);
	void *extraMem = (u8 *)t + _SNG_TERM_SIZEOF_TERM;
	t->lines = (SngTermCell **)extraMem;
	extraMem = (u8 *)extraMem + _SNG_TERM_SIZEOF_LINES(w, h);
	for (size_t y = 0; y < h; y++) {
		SngTermCell *cell = (SngTermCell *)extraMem;
		t->lines[y] = &cell[y * w];
	}
	extraMem = (u8 *)extraMem + _SNG_TERM_SIZEOF_LINES_DATA(w, h);
	t->altLines = (SngTermCell **)extraMem;
	extraMem = (u8 *)extraMem + _SNG_TERM_SIZEOF_LINES(w, h);
	for (size_t y = 0; y < h; y++) {
		SngTermCell *cell = (SngTermCell *)extraMem;
		t->altLines[y] = &cell[y * w];
	}
	extraMem = (u8 *)extraMem + _SNG_TERM_SIZEOF_LINES_DATA(w, h);
	t->dirtyLines = (b8 *)extraMem;
	extraMem = (u8 *)extraMem + _SNG_TERM_SIZEOF_DIRTYLINES(h);
	t->tabs = (b8 *)extraMem;
	//extraMem = (u8 *)extraMem + _SNG_TERM_SIZEOF_DIRTYLINES(w);
	t->maxWidth = maxWidth;
	t->maxHeight = maxHeight;
	t->top = 0;
	t->bottom = maxHeight;
	t->cur = _sngTermDefaultCursor();
	t->state = _sngTermStateParse;
	return t;
}

b32 sngTermSetSize(SngTerm *t, int width, int height) {
	if (t->width == width && t->height == height) {
		return 1;
	}
	if (width < 1 || height < 1) {
		return 0;
	}
	if (width > t->maxWidth || height > t->maxHeight) {
		return 0;
	}
	int slide = t->cur.y - height + 1;
	if (slide > 0) {
		memmove(t->lines, &t->lines[slide], sizeof(t->lines[0])*(u32)height);
		memmove(t->altLines, &t->altLines[slide], sizeof(t->altLines[0])*(u32)height);
	}
	int min_width = _sngTermMin(t->width, width);
	int min_height = _sngTermMin(t->height, height);
	t->changed |= SNG_TERM_CHANGED_SCREEN;
	t->width = width;
	t->height = height;
	_sngTermSetScroll(t, 0, height-1);
	_sngTermMoveTo(t, t->cur.x, t->cur.y);
	for (size_t i = 0; i < 2; i++) {
		if (min_width < width && min_height > 0) {
			_sngTermClear(t, min_width, 0, width-1, height-1);
		}
		if (width > 0 && min_height < height) {
			_sngTermClear(t, 0, min_height, width-1, height-1);
		}
		_sngTermSwapScreen(t);
	}
	// TODO: should we really return 1 in this case?
	if (slide <= 0) {
		return 0;
	}
	return 1;
}

void sngTermUpdate(SngTerm *t, u32 codepoint) {
	t->state(t, codepoint);
}

#endif // SNG_TERMINAL_IMPLEMENTATION
