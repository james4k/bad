// bad_terminal.h v0.0.0 (incomplete)
// James Gray, Oct 2015
//
// OVERVIEW
//
// bad_terminal implements a vt100 terminal code parser.
//
// Use for terminal muxing, a terminal emulation frontend, or wherever
// else you need terminal emulation. There are no dependencies.
// Influenced largely by st, rxvt, xterm, and iTerm as reference.
//
// USAGE
//
// define IMPLEMENT_BAD_TERMINAL, etc. etc.
//
// DEPENDENCIES
//
// C standard library - stdlib.h stdio.h string.h
//
// LICENSE
//
// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy,
// distribute, and modify this file as you see fit.
//
// No warranty. Use at your own risk.

#ifndef BAD_TERMINAL_H
#define BAD_TERMINAL_H

#include <stdlib.h> // strtol
#include <stdio.h>  // printf
#include <string.h> // memmove

typedef          float f32;
typedef         double f64;
typedef   signed char  s8;
typedef   signed short s16;
typedef   signed int   s32;
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef          u8    b8;
typedef          u32   b32;

#ifndef BAD_ASSERT
#define BAD_ASSERT(x)
#endif

// BADTERM_ATTR_* represent cell attributes.
enum {
	BADTERM_ATTR_REVERSE   = (1 << 0),
	BADTERM_ATTR_UNDERLINE = (1 << 1),
	BADTERM_ATTR_BOLD      = (1 << 2),
	BADTERM_ATTR_GFX       = (1 << 3),
	BADTERM_ATTR_ITALIC    = (1 << 4),
	BADTERM_ATTR_BLINK     = (1 << 5),
	BADTERM_ATTR_WRAP      = (1 << 6),
};

// BADTERM_CHANGED_* represent change flags to inform the user of any
// state changes.
enum {
	BADTERM_CHANGED_SCREEN = (1 << 0),
	BADTERM_CHANGED_TITLE  = (1 << 1),
};

// BADTERM_COLOR_* represent color codes.
enum {
	// ASCII colors
	BADTERM_COLOR_BLACK = 0,
	BADTERM_COLOR_RED,
	BADTERM_COLOR_GREEN,
	BADTERM_COLOR_YELLOW,
	BADTERM_COLOR_BLUE,
	BADTERM_COLOR_MAGENTA,
	BADTERM_COLOR_CYAN,
	BADTERM_COLOR_LIGHT_GREY,
	BADTERM_COLOR_DARK_GREY,
	BADTERM_COLOR_LIGHT_RED,
	BADTERM_COLOR_LIGHT_GREEN,
	BADTERM_COLOR_LIGHT_YELLOW,
	BADTERM_COLOR_LIGHT_BLUE,
	BADTERM_COLOR_LIGHT_MAGENTA,
	BADTERM_COLOR_LIGHT_CYAN,
	
	// Default colors are potentially distinct to allow for special
	// behavior. For example, rendering with a transparent background.
	// Otherwise, the simple case is to map default colors to another
	// color. These are specific to bad_terminal.
	BADTERM_COLOR_DEFAULT_FG = 0xff80,
	BADTERM_COLOR_DEFAULT_BG = 0xff81
};

// BADTERM_MODE_* represent terminal modes.
enum {
	BADTERM_MODE_WRAP          = (1 << 0),
	BADTERM_MODE_INSERT        = (1 << 1),
	BADTERM_MODE_APP_KEYPAD    = (1 << 2),
	BADTERM_MODE_ALT_SCREEN    = (1 << 3),
	BADTERM_MODE_CRLF          = (1 << 4),
	BADTERM_MODE_MOUSE_BUTTON  = (1 << 5),
	BADTERM_MODE_MOUSE_MOTION  = (1 << 6),
	BADTERM_MODE_REVERSE       = (1 << 7),
	BADTERM_MODE_KEYBOARD_LOCK = (1 << 8),
	BADTERM_MODE_HIDE          = (1 << 9),
	BADTERM_MODE_ECHO          = (1 << 10),
	BADTERM_MODE_APP_CURSOR    = (1 << 11),
	BADTERM_MODE_MOUSE_SGR     = (1 << 12),
	BADTERM_MODE_8BIT          = (1 << 13),
	BADTERM_MODE_BLINK         = (1 << 14),
	BADTERM_MODE_FBLINK        = (1 << 15),
	BADTERM_MODE_FOCUS         = (1 << 16),
	BADTERM_MODE_MOUSE_X10     = (1 << 17),
	BADTERM_MODE_MOUSE_MANY    = (1 << 18),
	BADTERM_MODE_MOUSE_MASK    =
		BADTERM_MODE_MOUSE_BUTTON |
		BADTERM_MODE_MOUSE_MOTION |
		BADTERM_MODE_MOUSE_X10    |
		BADTERM_MODE_MOUSE_MANY,
};

typedef struct BadTerm BadTerm;

// badterm_alloc_size returns how many bytes should be allocated for the
// memory passed into badterm_init.
int badterm_alloc_size(int max_width, int max_height);

// badterm_init initializes memory, expected to be sized by
// badterm_alloc_size, and returns a pointer to BadTerm. If seed_term is
// not NULL, we initialize with seed_term's state.
BadTerm *badterm_init(
	void *memory, int memory_size,
	int max_width, int max_height,
	BadTerm *seed_term
);

// badterm_set_size resizes the terminal, where width and height must be
// less than max_width and max_height. Returns non-zero on error.
int badterm_set_size(BadTerm *t, int width, int height);

// badterm_update updates t's state as it parses another codepoint.
void badterm_update(BadTerm *t, u32 codepoint);

#endif // BAD_TERMINAL_H


#ifdef IMPLEMENT_BAD_TERMINAL

enum {
	_BADTERM_CURSOR_DEFAULT   = (1 << 0),
	_BADTERM_CURSOR_WRAP_NEXT = (1 << 1),
	_BADTERM_CURSOR_ORIGIN    = (1 << 2),
};

enum {
	_BADTERM_TAB_SPACES = 8,
};

typedef struct {
	u32 codepoint;
	u16 fg, bg;
	u16 attr;
	u8 _pad[2];
} BadTerm_Cell;

typedef struct {
	BadTerm_Cell cell;
	int x, y;
	u16 state;
	u8 _pad[2];
} BadTerm_Cursor;

// _BadTerm_CSI stores state for "Control Sequence Introducor"
// sequences. (ESC+[)
typedef struct {
	char buf[256];
	int buf_len;
	int args[8];
	int args_len;
	char mode;
	b8 priv;
	u8 _pad[2];
} _BadTerm_CSI;

// _BadTerm_STR stores state for STR sequences.
// STR sequences are similar to CSI sequences, but have string arguments
// (and as far as I can tell, STR is just for "string"; STR is the name
// I took from suckless which I imagine comes from rxvt or xterm).
typedef struct {
	u32 type_codepoint;
	char buf[256];
	int buf_len;
	char *args[8];
	int args_len;
	u8 _pad[4];
} _BadTerm_STR;

typedef void (*_BadTerm_State)(BadTerm *, u32);

struct BadTerm {
	BadTerm_Cell **lines;
	BadTerm_Cell **alt_lines;
	b8 *dirty_lines;
	BadTerm_Cursor cur;
	BadTerm_Cursor cur_saved;
	int max_width, max_height;
	int width, height;
	int top, bottom; // TODO: could use better names
	s32 mode;
	s32 changed; // user should zero this when changes are rendered
	_BadTerm_State state;
	b8 *tabs;
	int tabs_len;
	u8 _pad[4];
	union {
		_BadTerm_CSI csi;
		_BadTerm_STR str;
	};
};

static int _badterm_clamp(int value, int min, int max) {
	if (value < min) {
		return min;
	}
	if (value > max) {
		return max;
	}
	return value;
}

static int _badterm_min(int a, int b) {
	if (a < b) {
		return a;
	}
	return b;
}

static b32 _badterm_handle_control_code(BadTerm *t, u32 c);
static void _badterm_move_to(BadTerm *t, int x, int y);
static void _badterm_state_parse(BadTerm *t, u32 c);
static void _badterm_state_parse_esc(BadTerm *t, u32 c);
static void _badterm_state_parse_esc_alt_charset(BadTerm *t, u32 c);
static void _badterm_state_parse_esc_csi(BadTerm *t, u32 c);
static void _badterm_state_parse_esc_str(BadTerm *t, u32 c);
static void _badterm_state_parse_esc_str_end(BadTerm *t, u32 c);
static void _badterm_state_parse_esc_test(BadTerm *t, u32 c);

static BadTerm_Cursor _badterm_default_cursor() {
	BadTerm_Cursor cur = {};
	cur.cell.fg = BADTERM_COLOR_DEFAULT_FG;
	cur.cell.bg = BADTERM_COLOR_DEFAULT_BG;
	return cur;
}

static void _badterm_save_cursor(BadTerm *t) {
	t->cur_saved = t->cur;
}

static void _badterm_restore_cursor(BadTerm *t) {
	t->cur = t->cur_saved;
	_badterm_move_to(t, t->cur.x, t->cur.y);
}

static void _badterm_clear(BadTerm *t, int x0, int y0, int x1, int y1) {
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
	x0 = _badterm_clamp(x0, 0, t->width-1);
	x1 = _badterm_clamp(x1, 0, t->width-1);
	y0 = _badterm_clamp(y0, 0, t->height-1);
	y1 = _badterm_clamp(y1, 0, t->height-1);
	t->changed |= BADTERM_CHANGED_SCREEN;
	for (int y = y0; y <= y1; y++) {
		t->dirty_lines[y] = 1;
		for (int x = x0; x <= x1; x++) {
			t->lines[y][x] = t->cur.cell;
			t->lines[y][x].codepoint = ' ';
		}
	}
}

static void _badterm_clear_all(BadTerm *t) {
	_badterm_clear(t, 0, 0, t->width-1, t->height-1);
}

static void _badterm_dirty_all(BadTerm *t) {
	t->changed |= BADTERM_CHANGED_SCREEN;
	for (int y = 0; y < t->height; y++) {
		t->dirty_lines[y] = 1;
	}
}

static void _badterm_move_to(BadTerm *t, int x, int y) {
	int miny, maxy;	
	if (t->cur.state & _BADTERM_CURSOR_ORIGIN) {
		miny = t->top;
		maxy = t->bottom;
	} else {
		miny = 0;
		maxy = t->height - 1;
	}
	t->changed |= BADTERM_CHANGED_SCREEN;
	t->cur.state &= ~_BADTERM_CURSOR_WRAP_NEXT;
	t->cur.x = _badterm_clamp(x, 0, t->width-1);
	t->cur.y = _badterm_clamp(y, miny, maxy);
}

#if 0
static void _badterm_move_abs_to(BadTerm *t, int x, int y) {
	if (t->cur.state & _BADTERM_CURSOR_ORIGIN) {
		y += t->top;
	}
	_badterm_move_to(t, x, y);
}
#endif

static void _badterm_reset(BadTerm *t) {
	t->cur = _badterm_default_cursor();
	_badterm_save_cursor(t);
	for (int i = 0; i < t->tabs_len; i++) {
		t->tabs[i] = 0;
	}
	for (int i = _BADTERM_TAB_SPACES; i < t->tabs_len; i += _BADTERM_TAB_SPACES) {
		t->tabs[i] = 1;
	}
	t->top = 0;
	t->bottom = t->height - 1;
	_badterm_clear_all(t);
	_badterm_move_to(t, 0, 0);
}

static void _badterm_set_scroll(BadTerm *t, int top, int bottom) {
	top = _badterm_clamp(top, 0, t->height-1);
	bottom = _badterm_clamp(bottom, 0, t->height-1);
	if (top > bottom) {
		int tmp = top;
		top = bottom;
		bottom = tmp;
	}
	t->top = top;
	t->bottom = bottom;
}

static void _badterm_scroll_down(BadTerm *t, int orig, int n) {
	n = _badterm_clamp(n, 0, t->bottom-orig+1);
	_badterm_clear(t, 0, t->bottom-n+1, t->width-1, t->bottom);
	for (int i = t->bottom; i >= orig+n; i--) {
		BadTerm_Cell *tmp = t->lines[i];
		t->lines[i] = t->lines[i+n];
		t->lines[i+n] = tmp;
		t->dirty_lines[i] = 1;
		t->dirty_lines[i+n] = 1;
	}
}

static void _badterm_scroll_up(BadTerm *t, int orig, int n) {
	n = _badterm_clamp(n, 0, t->bottom-orig+1);
	_badterm_clear(t, 0, orig, t->width-1, orig+n-1);
	for (int i = orig; i <= t->bottom-n; i++) {
		BadTerm_Cell *tmp = t->lines[i];
		t->lines[i] = t->lines[i+n];
		t->lines[i+n] = tmp;
		t->dirty_lines[i] = 1;
		t->dirty_lines[i+n] = 1;
	}
}

static void _badterm_swap_screen(BadTerm *t) {
	BadTerm_Cell **tmp_lines = t->lines;
	t->lines = t->alt_lines;
	t->alt_lines = tmp_lines;
	t->mode ^= BADTERM_MODE_ALT_SCREEN;
	_badterm_dirty_all(t);
}

static void _badterm_put_tab(BadTerm *t, b32 forward) {
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
	_badterm_move_to(t, x, t->cur.y);
}

static void _badterm_newline(BadTerm *t, b32 first_column) {
	int y = t->cur.y;
	if (y == t->bottom) {
		BadTerm_Cursor cur = t->cur;
		t->cur = _badterm_default_cursor();
		_badterm_scroll_up(t, t->top, 1);
		t->cur = cur;
	} else {
		y++;
	}
	if (first_column) {
		_badterm_move_to(t, 0, y);
	} else {
		_badterm_move_to(t, t->cur.x, y);
	}
}

static void _badterm_csi_parse(_BadTerm_CSI *c) {
	BAD_ASSERT(c->buf_len > 0);
	c->mode = c->buf[c->buf_len-1];
	if (c->buf_len == 1) {
		return;
	}
	char *s = &c->buf[0];
	int s_len = c->buf_len;
	c->args_len = 0;
	if (s[0] == '?') {
		c->priv = 1;
		s = &s[1];
		s_len--;
	}
	s_len--;
	while (1) {
		char *end;
		int num = (int)strtol(s, &end, 10);
		if (*end == '\0') {
			break;
		} else if (*end == ';') {
			s = &end[1];
			BAD_ASSERT(c->args_len < 8);
			if (c->args_len < 8) {
				c->args[c->args_len] = num;
				c->args_len++;
			}
		} else {
			// TODO: inform about bad input
			break;
		}
	}
}

#if 0
static int _badterm_csi_arg(_BadTerm_CSI *c, int i, int def) {
	if (i >= c->args_len || i < 0) {
		return def;
	}
	return c->args[i];
}
#endif

#if 0
// _badterm_csi_maxarg takes the max of _badterm_csi_arg(c, i, def) and def.
static int _badterm_csi_maxarg(_BadTerm_CSI *c, int i, int def) {
	int arg = _badterm_csi_arg(c, i, def);
	if (def > arg) {
		return def;
	}
	return arg;
}
#endif

static void _badterm_csi_reset(_BadTerm_CSI *c) {
	c->buf[0] = 0;
	c->args_len = 0;
	c->mode = 0;
	c->priv = 0;
}

static b32 _badterm_csi_put(_BadTerm_CSI *c, char b) {
	c->buf[c->buf_len] = b;
	c->buf_len++;
	if ((b >= 0x40 && b <= 0x7e) || c->buf_len >= 256) {
		_badterm_csi_parse(c);
		return 1;
	}
	return 0;
}

static void _badterm_str_reset(_BadTerm_STR *s) {
	s->type_codepoint = 0;
	s->buf[0] = 0;
	s->args_len = 0;
}

static void _badterm_str_put(_BadTerm_STR *s, char c) {
	if (s->buf_len < 256-1) {
		s->buf[s->buf_len] = c;
		s->buf[s->buf_len+1] = '\0';
		s->buf_len++;
	}
	// Remain silent if STR sequence does not end so that it is apparent
	// to user that something is wrong.
}

#if 0
static void _badterm_str_parse(_BadTerm_STR *s) {
	BAD_ASSERT(s->buf_len > 0);
	char *arg = &s->buf[0];
	for (int i = 0; i < s->buf_len; i++) {
		if (s->buf[i] == ';') {
			s->buf[i] = '\0';
			BAD_ASSERT(s->args_len < 8);
			if (s->args_len < 8) {
				s->args[s->args_len] = arg;
				s->args_len++;
			}
		}
	}
	// last one is already null terminated, since it is the end of the string.
	BAD_ASSERT(s->args_len < 8);
	if (s->args_len < 8) {
		s->args[s->args_len] = arg;
		s->args_len++;
	}
}
#endif

static b32 _badterm_is_control_code(u32 c) {
	return (c < 0x20 || c == 0177);
}

static b32 _badterm_handle_control_code(BadTerm *t, u32 c) {
	if (_badterm_is_control_code(c)) {
		return 0;
	}
	switch (c) {
		// HT
		case '\t': {
			b32 forward = 1;
			_badterm_put_tab(t, forward);
		} break;
		// BS
		case '\b': {
			_badterm_move_to(t, t->cur.x-1, t->cur.y);
		} break;
		// CR
		case '\r': {
			_badterm_move_to(t, 0, t->cur.y);
		} break;
		// LF, VT, LF
		case '\f':
		case '\v':
		case '\n': {
			b32 first_column = ((t->mode & BADTERM_MODE_CRLF) != 0);
			_badterm_newline(t, first_column);
		} break;
		// BEL
		case '\a': {
			// TODO: notify user somehow? maybe BadTerm will have an
			// event field, which ofc would only be good until the next
			// badterm_update call.
		} break;
		// ESC
		case 033: {
			_badterm_csi_reset(&t->csi);
			t->state = _badterm_state_parse_esc;
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
			_badterm_csi_reset(&t->csi);
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

static void _badterm_handle_csi(BadTerm *t) {
	_BadTerm_CSI *c = &t->csi;
	switch (c->mode) {
		case '@': { // ICH - insert <n> blank chars
			// TODO: _badterm_insert_blanks(t, _badterm_arg(c, 0, 1))
		} break;
		// TODO: implement
		default: {
			// TODO: notify on unexpected input
		} break;
	}
}

static void _badterm_handle_str(BadTerm *t) {
	_BadTerm_STR *s = &t->str;
	switch (s->type_codepoint) {
		// TODO: implement
		default: {
			// TODO: notify on unexpected input
		} break;
	}
}

static u32 _badterm_gfx_char_table[62] = {
	L'↑', L'↓', L'→', L'←', L'█', L'▚', L'☃',       // A - G
	0, 0, 0, 0, 0, 0, 0, 0,                         // H - O
	0, 0, 0, 0, 0, 0, 0, 0,                         // P - W
	0, 0, 0, 0, 0, 0, 0, ' ',                       // X - _
	L'◆', L'▒', L'␉', L'␌', L'␍', L'␊', L'°', L'±', // ` - g
	L'␤', L'␋', L'┘', L'┐', L'┌', L'└', L'┼', L'⎺', // h - o
	L'⎻', L'─', L'⎼', L'⎽', L'├', L'┤', L'┴', L'┬', // p - w
	L'│', L'≤', L'≥', L'π', L'≠', L'£', L'·',       // x - ~
};

static void _badterm_set_char(
	BadTerm *t,
	u32 c,
	BadTerm_Cell *cell,
	int x, int y
) {
	if (
		(cell->attr & BADTERM_ATTR_GFX) != 0 &&
		(c >= 0x41 && c <= 0x7e) &&
		_badterm_gfx_char_table[c-0x41] != 0
	) {
		c = _badterm_gfx_char_table[c-0x41];
	}
	t->changed |= BADTERM_CHANGED_SCREEN;
	t->dirty_lines[y] = 1;
	t->lines[y][x] = *cell;
	t->lines[y][x].codepoint = c;
	if ((cell->attr & BADTERM_ATTR_BOLD) && cell->fg < 8) {
		t->lines[y][x].fg = cell->fg + 8;
	}
	if (cell->attr & BADTERM_ATTR_REVERSE) {
		t->lines[y][x].fg = cell->bg;
		t->lines[y][x].bg = cell->fg;
	}
}

static void _badterm_state_parse(BadTerm *t, u32 codepoint) {
	if (_badterm_is_control_code(codepoint)) {
		b32 handled = _badterm_handle_control_code(t, codepoint);
		if (handled || (t->cur.cell.attr & BADTERM_ATTR_GFX) == 0) {
			return;
		}
	}

	// TODO: update selection; see st.c:2450
	
	if (
		(t->mode & BADTERM_MODE_WRAP) != 0 &&
		(t->cur.state & _BADTERM_CURSOR_WRAP_NEXT) != 0
	) {
		t->lines[t->cur.y][t->cur.x].attr |= BADTERM_ATTR_WRAP;
		
		b32 first_column = 1;
		_badterm_newline(t, first_column);
	}

	if (
		(t->mode & BADTERM_MODE_INSERT) != 0 &&
		t->cur.x+1 < t->width
	) {
		// TODO: move stuff; see st.c:2458
		// TODO: remove printf
		printf("insert mode not implemented\n");
	}

	_badterm_set_char(t, codepoint, &t->cur.cell, t->cur.x, t->cur.y);
	if (t->cur.x+1 < t->width) {
		_badterm_move_to(t, t->cur.x+1, t->cur.y);
	} else {
		t->cur.state |= _BADTERM_CURSOR_WRAP_NEXT;
	}
}

static void _badterm_state_parse_esc(BadTerm *t, u32 c) {
	if (!_badterm_handle_control_code(t, c)) {
		return;
	}
	_BadTerm_State next = _badterm_state_parse;
	switch (c) {
		case '[': {
			next = _badterm_state_parse_esc_csi;
		} break;
		case '#': {
			next = _badterm_state_parse_esc_test;
		} break;
		case 'P':   // DCS - Device Control String
		case '_':   // APC - Application Program Command
		case '^':   // PM - Privacy Message
		case ']':   // OSC - Operation System Command
		case 'k': { // old title set compatibility
			_badterm_str_reset(&t->str);
			t->str.type_codepoint = c;
			next = _badterm_state_parse_esc_str;
		} break;
		case '(': { // set primary charset G0
			next = _badterm_state_parse_esc_alt_charset;
		} break;
		case ')':   // set primary charset G1 (ignored)
		case '*':   // set primary charset G2 (ignored)
		case '+': { // set primary charset G3 (ignored)
		} break;
		case 'D': { // IND - linefeed
			if (t->cur.y == t->bottom) {
				_badterm_scroll_up(t, t->top, 1);
			} else {
				_badterm_move_to(t, t->cur.x, t->cur.y+1);
			}
		} break;
		case 'E': { // NEL - next line
			b32 first_column = 1;
			_badterm_newline(t, first_column);
		} break;
		case 'H': { // HTS - horizontal tab stop
			t->tabs[t->cur.x] = 1;
		} break;
		case 'M': { // RI - reverse index
			if (t->cur.y == t->top) {
				_badterm_scroll_down(t, t->top, 1);
			} else {
				_badterm_move_to(t, t->cur.x, t->cur.y-1);
			}
		} break;
		case 'Z': { // DECID - identify terminal
			// TODO:
		} break;
		case 'c': { // RIS - reset to initial state
			_badterm_reset(t);
		} break;
		case '=': { // DECPAM - application keypad
			t->mode |= BADTERM_MODE_APP_KEYPAD;
		} break;
		case '>': { // DECPNM - normal keypad
			t->mode &= ~BADTERM_MODE_APP_KEYPAD;
		} break;
		case '7': { // DECSC - save cursor
			_badterm_save_cursor(t);
		} break;
		case '8': { // DECRC - restore cursor
			_badterm_restore_cursor(t);
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

static void _badterm_state_parse_esc_alt_charset(BadTerm *t, u32 c) {
	if (!_badterm_handle_control_code(t, c)) {
		return;
	}
	switch (c) {
		case '0': { // line drawing set
			t->cur.cell.attr |= BADTERM_ATTR_GFX;
		} break;
		case 'B': { // USASCII
			t->cur.cell.attr &= ~BADTERM_ATTR_GFX;
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
	t->state = _badterm_state_parse;
}

static void _badterm_state_parse_esc_csi(BadTerm *t, u32 c) {
	if (!_badterm_handle_control_code(t, c)) {
		return;
	}
	if (_badterm_csi_put(&t->csi, (char)c)) {
		t->state = _badterm_state_parse;
		_badterm_handle_csi(t);
	}
}

static void _badterm_state_parse_esc_str(BadTerm *t, u32 c) {
	switch (c) {
		case '\033': {
			t->state = _badterm_state_parse_esc_str_end;
		} break;
		case '\a': { // backwards compatibility to xterm
			t->state = _badterm_state_parse;
			_badterm_handle_str(t);
		} break;
		default: {
			_badterm_str_put(&t->str, (char)c);
		} break;
	}
}

static void _badterm_state_parse_esc_str_end(BadTerm *t, u32 c) {
	if (!_badterm_handle_control_code(t, c)) {
		return;
	}
	t->state = _badterm_state_parse;
	if (c == '\\') {
		_badterm_handle_str(t);
	}
}

static void _badterm_state_parse_esc_test(BadTerm *t, u32 c) {
	if (!_badterm_handle_control_code(t, c)) {
		return;
	}
	// DEC screen alignment test
	if (c == '8') {
		for (int y = 0; y < t->height; y++) {
			for (int x = 0; x < t->width; x++) {
				_badterm_set_char(t, 'E', &t->cur.cell, x, y);
			}
		}
	}
	t->state = _badterm_state_parse;
}

int badterm_alloc_size(int max_width, int max_height) {
	return (s32)sizeof(BadTerm) + (s32)sizeof(BadTerm_Cell)*max_width*max_height;
}

BadTerm *badterm_init(
	void *memory, int memory_size,
	int max_width, int max_height,
	BadTerm *seed_term
) {
	int expected_size = badterm_alloc_size(max_width, max_height);
	if (memory_size < expected_size) {
		return 0;
	}
	if (seed_term != NULL) {
		// TODO: support copying state from seed_term
		return 0;
	}
	BadTerm *t = (BadTerm *)memory;
	// TODO: needs reworking
	t->lines = (BadTerm_Cell **)&t[1];
	t->max_width = max_width;
	t->max_height = max_height;
	t->top = 0;
	t->bottom = max_height;
	t->cur = _badterm_default_cursor();
	t->state = _badterm_state_parse;
	return t;
}

int badterm_set_size(BadTerm *t, int width, int height) {
	if (t->width == width && t->height == height) {
		return 0;
	}
	if (width < 1 || height < 1) {
		return 1;
	}
	if (width > t->max_width || height > t->max_height) {
		return 1;
	}
	int slide = t->cur.y - height + 1;
	if (slide > 0) {
		memmove(t->lines, &t->lines[slide], sizeof(t->lines[0])*(u32)height);
		memmove(t->alt_lines, &t->alt_lines[slide], sizeof(t->alt_lines[0])*(u32)height);
	}
	int min_width = _badterm_min(t->width, width);
	int min_height = _badterm_min(t->height, height);
	t->changed |= BADTERM_CHANGED_SCREEN;
	t->width = width;
	t->height = height;
	_badterm_set_scroll(t, 0, height-1);
	_badterm_move_to(t, t->cur.x, t->cur.y);
	for (int i = 0; i < 2; i++) {
		if (min_width < width && min_height > 0) {
			_badterm_clear(t, min_width, 0, width-1, height-1);
		}
		if (width > 0 && min_height < height) {
			_badterm_clear(t, 0, min_height, width-1, height-1);
		}
		_badterm_swap_screen(t);
	}
	// TODO: should we really return 1 in this case?
	if (slide <= 0) {
		return 1;
	}
	return 0;
}

void badterm_update(BadTerm *t, u32 codepoint) {
	t->state(t, codepoint);
}

#endif // IMPLEMENT_BAD_TERMINAL
