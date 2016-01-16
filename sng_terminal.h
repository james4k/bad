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
#include <stdio.h>  // printf
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

// SNGTERM_ATTR_* represent cell attributes.
enum {
	SNGTERM_ATTR_REVERSE   = (1 << 0),
	SNGTERM_ATTR_UNDERLINE = (1 << 1),
	SNGTERM_ATTR_BOLD      = (1 << 2),
	SNGTERM_ATTR_GFX       = (1 << 3),
	SNGTERM_ATTR_ITALIC    = (1 << 4),
	SNGTERM_ATTR_BLINK     = (1 << 5),
	SNGTERM_ATTR_WRAP      = (1 << 6),
};

// SNGTERM_CHANGED_* represent change flags to inform the user of any
// state changes.
enum {
	SNGTERM_CHANGED_SCREEN = (1 << 0),
	SNGTERM_CHANGED_TITLE  = (1 << 1),
};

// SNGTERM_COLOR_* represent color codes.
enum {
	// ASCII colors
	SNGTERM_COLOR_BLACK = 0,
	SNGTERM_COLOR_RED,
	SNGTERM_COLOR_GREEN,
	SNGTERM_COLOR_YELLOW,
	SNGTERM_COLOR_BLUE,
	SNGTERM_COLOR_MAGENTA,
	SNGTERM_COLOR_CYAN,
	SNGTERM_COLOR_LIGHT_GREY,
	SNGTERM_COLOR_DARK_GREY,
	SNGTERM_COLOR_LIGHT_RED,
	SNGTERM_COLOR_LIGHT_GREEN,
	SNGTERM_COLOR_LIGHT_YELLOW,
	SNGTERM_COLOR_LIGHT_BLUE,
	SNGTERM_COLOR_LIGHT_MAGENTA,
	SNGTERM_COLOR_LIGHT_CYAN,
	
	// Default colors are potentially distinct to allow for special
	// behavior. For example, rendering with a transparent background.
	// Otherwise, the simple case is to map default colors to another
	// color. These are specific to sng_terminal.
	SNGTERM_COLOR_DEFAULT_FG = 0xff80,
	SNGTERM_COLOR_DEFAULT_BG = 0xff81
};

// SNGTERM_MODE_* represent terminal modes.
enum {
	SNGTERM_MODE_WRAP          = (1 << 0),
	SNGTERM_MODE_INSERT        = (1 << 1),
	SNGTERM_MODE_APP_KEYPAD    = (1 << 2),
	SNGTERM_MODE_ALT_SCREEN    = (1 << 3),
	SNGTERM_MODE_CRLF          = (1 << 4),
	SNGTERM_MODE_MOUSE_BUTTON  = (1 << 5),
	SNGTERM_MODE_MOUSE_MOTION  = (1 << 6),
	SNGTERM_MODE_REVERSE       = (1 << 7),
	SNGTERM_MODE_KEYBOARD_LOCK = (1 << 8),
	SNGTERM_MODE_HIDE          = (1 << 9),
	SNGTERM_MODE_ECHO          = (1 << 10),
	SNGTERM_MODE_APP_CURSOR    = (1 << 11),
	SNGTERM_MODE_MOUSE_SGR     = (1 << 12),
	SNGTERM_MODE_8BIT          = (1 << 13),
	SNGTERM_MODE_BLINK         = (1 << 14),
	SNGTERM_MODE_FBLINK        = (1 << 15),
	SNGTERM_MODE_FOCUS         = (1 << 16),
	SNGTERM_MODE_MOUSE_X10     = (1 << 17),
	SNGTERM_MODE_MOUSE_MANY    = (1 << 18),
	SNGTERM_MODE_MOUSE_MASK    =
		SNGTERM_MODE_MOUSE_BUTTON |
		SNGTERM_MODE_MOUSE_MOTION |
		SNGTERM_MODE_MOUSE_X10    |
		SNGTERM_MODE_MOUSE_MANY,
};

typedef struct SngTerm SngTerm;

// sngterm_alloc_size returns how many bytes should be allocated for the
// memory passed into sngterm_init.
int sngterm_alloc_size(int max_width, int max_height);

// sngterm_init initializes memory, expected to be sized by
// sngterm_alloc_size, and returns a pointer to SngTerm. If seed_term is
// not NULL, we initialize with seed_term's state.
SngTerm *sngterm_init(
	void *memory, int memory_size,
	int max_width, int max_height,
	SngTerm *seed_term
);

// sngterm_set_size resizes the terminal, where width and height must be
// less than max_width and max_height. Returns non-zero on error.
int sngterm_set_size(SngTerm *t, int width, int height);

// sngterm_update updates t's state as it parses another codepoint.
void sngterm_update(SngTerm *t, u32 codepoint);

#endif // SNG_TERMINAL_H


#ifdef SNG_TERMINAL_IMPLEMENTATION

enum {
	_SNGTERM_CURSOR_DEFAULT   = (1 << 0),
	_SNGTERM_CURSOR_WRAP_NEXT = (1 << 1),
	_SNGTERM_CURSOR_ORIGIN    = (1 << 2),
};

enum {
	_SNGTERM_TAB_SPACES = 8,
};

typedef struct {
	u32 codepoint;
	u16 fg, bg;
	u16 attr;
	u8 _pad[2];
} SngTerm_Cell;

typedef struct {
	SngTerm_Cell cell;
	int x, y;
	u16 state;
	u8 _pad[2];
} SngTerm_Cursor;

// _SngTerm_CSI stores state for "Control Sequence Introducor"
// sequences. (ESC+[)
typedef struct {
	char buf[256];
	int buf_len;
	int args[8];
	int args_len;
	char mode;
	b8 priv;
	u8 _pad[2];
} _SngTerm_CSI;

// _SngTerm_STR stores state for STR sequences.
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
} _SngTerm_STR;

typedef void (*_SngTerm_State)(SngTerm *, u32);

struct SngTerm {
	SngTerm_Cell **lines;
	SngTerm_Cell **alt_lines;
	b8 *dirty_lines;
	SngTerm_Cursor cur;
	SngTerm_Cursor cur_saved;
	int max_width, max_height;
	int width, height;
	int top, bottom; // TODO: could use better names
	s32 mode;
	s32 changed; // user should zero this when changes are rendered
	_SngTerm_State state;
	b8 *tabs;
	int tabs_len;
	u8 _pad[4];
	union {
		_SngTerm_CSI csi;
		_SngTerm_STR str;
	};
};

static int _sngterm_clamp(int value, int min, int max) {
	if (value < min) {
		return min;
	}
	if (value > max) {
		return max;
	}
	return value;
}

static int _sngterm_min(int a, int b) {
	if (a < b) {
		return a;
	}
	return b;
}

static b32 _sngterm_handle_control_code(SngTerm *t, u32 c);
static void _sngterm_insert_blanks(SngTerm *t, int n);
static void _sngterm_move_to(SngTerm *t, int x, int y);
static void _sngterm_state_parse(SngTerm *t, u32 c);
static void _sngterm_state_parse_esc(SngTerm *t, u32 c);
static void _sngterm_state_parse_esc_alt_charset(SngTerm *t, u32 c);
static void _sngterm_state_parse_esc_csi(SngTerm *t, u32 c);
static void _sngterm_state_parse_esc_str(SngTerm *t, u32 c);
static void _sngterm_state_parse_esc_str_end(SngTerm *t, u32 c);
static void _sngterm_state_parse_esc_test(SngTerm *t, u32 c);

static SngTerm_Cursor _sngterm_default_cursor() {
	SngTerm_Cursor cur = {};
	cur.cell.fg = SNGTERM_COLOR_DEFAULT_FG;
	cur.cell.bg = SNGTERM_COLOR_DEFAULT_BG;
	return cur;
}

static void _sngterm_save_cursor(SngTerm *t) {
	t->cur_saved = t->cur;
}

static void _sngterm_restore_cursor(SngTerm *t) {
	t->cur = t->cur_saved;
	_sngterm_move_to(t, t->cur.x, t->cur.y);
}

static void _sngterm_clear(SngTerm *t, int x0, int y0, int x1, int y1) {
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
	x0 = _sngterm_clamp(x0, 0, t->width-1);
	x1 = _sngterm_clamp(x1, 0, t->width-1);
	y0 = _sngterm_clamp(y0, 0, t->height-1);
	y1 = _sngterm_clamp(y1, 0, t->height-1);
	t->changed |= SNGTERM_CHANGED_SCREEN;
	for (int y = y0; y <= y1; y++) {
		t->dirty_lines[y] = 1;
		for (int x = x0; x <= x1; x++) {
			t->lines[y][x] = t->cur.cell;
			t->lines[y][x].codepoint = ' ';
		}
	}
}

static void _sngterm_clear_all(SngTerm *t) {
	_sngterm_clear(t, 0, 0, t->width-1, t->height-1);
}

static void _sngterm_dirty_all(SngTerm *t) {
	t->changed |= SNGTERM_CHANGED_SCREEN;
	for (int y = 0; y < t->height; y++) {
		t->dirty_lines[y] = 1;
	}
}

static void _sngterm_move_to(SngTerm *t, int x, int y) {
	int miny, maxy;	
	if (t->cur.state & _SNGTERM_CURSOR_ORIGIN) {
		miny = t->top;
		maxy = t->bottom;
	} else {
		miny = 0;
		maxy = t->height - 1;
	}
	t->changed |= SNGTERM_CHANGED_SCREEN;
	t->cur.state &= ~_SNGTERM_CURSOR_WRAP_NEXT;
	t->cur.x = _sngterm_clamp(x, 0, t->width-1);
	t->cur.y = _sngterm_clamp(y, miny, maxy);
}

#if 0
static void _sngterm_move_abs_to(SngTerm *t, int x, int y) {
	if (t->cur.state & _SNGTERM_CURSOR_ORIGIN) {
		y += t->top;
	}
	_sngterm_move_to(t, x, y);
}
#endif

static void _sngterm_reset(SngTerm *t) {
	t->cur = _sngterm_default_cursor();
	_sngterm_save_cursor(t);
	for (int i = 0; i < t->tabs_len; i++) {
		t->tabs[i] = 0;
	}
	for (int i = _SNGTERM_TAB_SPACES; i < t->tabs_len; i += _SNGTERM_TAB_SPACES) {
		t->tabs[i] = 1;
	}
	t->top = 0;
	t->bottom = t->height - 1;
	_sngterm_clear_all(t);
	_sngterm_move_to(t, 0, 0);
}

static void _sngterm_set_scroll(SngTerm *t, int top, int bottom) {
	top = _sngterm_clamp(top, 0, t->height-1);
	bottom = _sngterm_clamp(bottom, 0, t->height-1);
	if (top > bottom) {
		int tmp = top;
		top = bottom;
		bottom = tmp;
	}
	t->top = top;
	t->bottom = bottom;
}

static void _sngterm_scroll_down(SngTerm *t, int orig, int n) {
	n = _sngterm_clamp(n, 0, t->bottom-orig+1);
	_sngterm_clear(t, 0, t->bottom-n+1, t->width-1, t->bottom);
	for (int i = t->bottom; i >= orig+n; i--) {
		SngTerm_Cell *tmp = t->lines[i];
		t->lines[i] = t->lines[i+n];
		t->lines[i+n] = tmp;
		t->dirty_lines[i] = 1;
		t->dirty_lines[i+n] = 1;
	}
}

static void _sngterm_scroll_up(SngTerm *t, int orig, int n) {
	n = _sngterm_clamp(n, 0, t->bottom-orig+1);
	_sngterm_clear(t, 0, orig, t->width-1, orig+n-1);
	for (int i = orig; i <= t->bottom-n; i++) {
		SngTerm_Cell *tmp = t->lines[i];
		t->lines[i] = t->lines[i+n];
		t->lines[i+n] = tmp;
		t->dirty_lines[i] = 1;
		t->dirty_lines[i+n] = 1;
	}
}

static void _sngterm_swap_screen(SngTerm *t) {
	SngTerm_Cell **tmp_lines = t->lines;
	t->lines = t->alt_lines;
	t->alt_lines = tmp_lines;
	t->mode ^= SNGTERM_MODE_ALT_SCREEN;
	_sngterm_dirty_all(t);
}

static void _sngterm_put_tab(SngTerm *t, b32 forward) {
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
	_sngterm_move_to(t, x, t->cur.y);
}

static void _sngterm_newline(SngTerm *t, b32 first_column) {
	int y = t->cur.y;
	if (y == t->bottom) {
		SngTerm_Cursor cur = t->cur;
		t->cur = _sngterm_default_cursor();
		_sngterm_scroll_up(t, t->top, 1);
		t->cur = cur;
	} else {
		y++;
	}
	if (first_column) {
		_sngterm_move_to(t, 0, y);
	} else {
		_sngterm_move_to(t, t->cur.x, y);
	}
}

static void _sngterm_csi_parse(_SngTerm_CSI *c) {
	SNG_ASSERT(c->buf_len > 0);
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
			SNG_ASSERT(c->args_len < 8);
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
static int _sngterm_csi_arg(_SngTerm_CSI *c, int i, int def) {
	if (i >= c->args_len || i < 0) {
		return def;
	}
	return c->args[i];
}
#endif

#if 0
// _sngterm_csi_maxarg takes the max of _sngterm_csi_arg(c, i, def) and def.
static int _sngterm_csi_maxarg(_SngTerm_CSI *c, int i, int def) {
	int arg = _sngterm_csi_arg(c, i, def);
	if (def > arg) {
		return def;
	}
	return arg;
}
#endif

static void _sngterm_csi_reset(_SngTerm_CSI *c) {
	c->buf[0] = 0;
	c->args_len = 0;
	c->mode = 0;
	c->priv = 0;
}

static b32 _sngterm_csi_put(_SngTerm_CSI *c, char b) {
	c->buf[c->buf_len] = b;
	c->buf_len++;
	if ((b >= 0x40 && b <= 0x7e) || c->buf_len >= 256) {
		_sngterm_csi_parse(c);
		return 1;
	}
	return 0;
}

static void _sngterm_str_reset(_SngTerm_STR *s) {
	s->type_codepoint = 0;
	s->buf[0] = 0;
	s->args_len = 0;
}

static void _sngterm_str_put(_SngTerm_STR *s, char c) {
	if (s->buf_len < 256-1) {
		s->buf[s->buf_len] = c;
		s->buf[s->buf_len+1] = '\0';
		s->buf_len++;
	}
	// Remain silent if STR sequence does not end so that it is apparent
	// to user that something is wrong.
}

#if 0
static void _sngterm_str_parse(_SngTerm_STR *s) {
	SNG_ASSERT(s->buf_len > 0);
	char *arg = &s->buf[0];
	for (int i = 0; i < s->buf_len; i++) {
		if (s->buf[i] == ';') {
			s->buf[i] = '\0';
			SNG_ASSERT(s->args_len < 8);
			if (s->args_len < 8) {
				s->args[s->args_len] = arg;
				s->args_len++;
			}
		}
	}
	// last one is already null terminated, since it is the end of the string.
	SNG_ASSERT(s->args_len < 8);
	if (s->args_len < 8) {
		s->args[s->args_len] = arg;
		s->args_len++;
	}
}
#endif

static b32 _sngterm_is_control_code(u32 c) {
	return (c < 0x20 || c == 0177);
}

static b32 _sngterm_handle_control_code(SngTerm *t, u32 c) {
	if (_sngterm_is_control_code(c)) {
		return 0;
	}
	switch (c) {
		// HT
		case '\t': {
			b32 forward = 1;
			_sngterm_put_tab(t, forward);
		} break;
		// BS
		case '\b': {
			_sngterm_move_to(t, t->cur.x-1, t->cur.y);
		} break;
		// CR
		case '\r': {
			_sngterm_move_to(t, 0, t->cur.y);
		} break;
		// LF, VT, LF
		case '\f':
		case '\v':
		case '\n': {
			b32 first_column = ((t->mode & SNGTERM_MODE_CRLF) != 0);
			_sngterm_newline(t, first_column);
		} break;
		// BEL
		case '\a': {
			// TODO: notify user somehow? maybe SngTerm will have an
			// event field, which ofc would only be good until the next
			// sngterm_update call.
		} break;
		// ESC
		case 033: {
			_sngterm_csi_reset(&t->csi);
			t->state = _sngterm_state_parse_esc;
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
			_sngterm_csi_reset(&t->csi);
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

static void _sngterm_handle_csi(SngTerm *t) {
	_SngTerm_CSI *c = &t->csi;
	switch (c->mode) {
		case '@': { // ICH - insert <n> blank chars
			_sngterm_insert_blanks(t, _sngterm_arg(c, 0, 1))
		} break;
		// TODO: implement
		default: {
			// TODO: notify on unexpected input
		} break;
	}
}

static void _sngterm_handle_str(SngTerm *t) {
	_SngTerm_STR *s = &t->str;
	switch (s->type_codepoint) {
		// TODO: implement
		default: {
			// TODO: notify on unexpected input
		} break;
	}
}

static void _sngterm_insert_blanks(SngTerm *t, int n) {
	int src = t->cur.x;
	int dst = src + n;
	int size = t->width - dst;
	t->changed |= SNGTERM_CHANGED_SCREEN;
	t->dirty_lines[t->cur.y] = 1;
	if (dst >= t->width) {
		_sngterm_clear(t, t->cur.x, t->cur.y, t->width-1, t->cur.y);
	} else {
		memmove(
			&t->lines[t->cur.y][dst],
			&t->lines[t->cur.y][src],
			(u32)size * sizeof(t->lines[0][0])
		);
		_sngterm_clear(t, src, t->cur.y, dst-1, t->cur.y);
	}
}

static u32 _sngterm_gfx_char_table[62] = {
	L'↑', L'↓', L'→', L'←', L'█', L'▚', L'☃',       // A - G
	0, 0, 0, 0, 0, 0, 0, 0,                         // H - O
	0, 0, 0, 0, 0, 0, 0, 0,                         // P - W
	0, 0, 0, 0, 0, 0, 0, ' ',                       // X - _
	L'◆', L'▒', L'␉', L'␌', L'␍', L'␊', L'°', L'±', // ` - g
	L'␤', L'␋', L'┘', L'┐', L'┌', L'└', L'┼', L'⎺', // h - o
	L'⎻', L'─', L'⎼', L'⎽', L'├', L'┤', L'┴', L'┬', // p - w
	L'│', L'≤', L'≥', L'π', L'≠', L'£', L'·',       // x - ~
};

static void _sngterm_set_char(
	SngTerm *t,
	u32 c,
	SngTerm_Cell *cell,
	int x, int y
) {
	if (
		(cell->attr & SNGTERM_ATTR_GFX) != 0 &&
		(c >= 0x41 && c <= 0x7e) &&
		_sngterm_gfx_char_table[c-0x41] != 0
	) {
		c = _sngterm_gfx_char_table[c-0x41];
	}
	t->changed |= SNGTERM_CHANGED_SCREEN;
	t->dirty_lines[y] = 1;
	t->lines[y][x] = *cell;
	t->lines[y][x].codepoint = c;
	if ((cell->attr & SNGTERM_ATTR_BOLD) && cell->fg < 8) {
		t->lines[y][x].fg = cell->fg + 8;
	}
	if (cell->attr & SNGTERM_ATTR_REVERSE) {
		t->lines[y][x].fg = cell->bg;
		t->lines[y][x].bg = cell->fg;
	}
}

static void _sngterm_state_parse(SngTerm *t, u32 codepoint) {
	if (_sngterm_is_control_code(codepoint)) {
		b32 handled = _sngterm_handle_control_code(t, codepoint);
		if (handled || (t->cur.cell.attr & SNGTERM_ATTR_GFX) == 0) {
			return;
		}
	}

	// TODO: update selection; see st.c:2450
	
	if (
		(t->mode & SNGTERM_MODE_WRAP) != 0 &&
		(t->cur.state & _SNGTERM_CURSOR_WRAP_NEXT) != 0
	) {
		t->lines[t->cur.y][t->cur.x].attr |= SNGTERM_ATTR_WRAP;
		
		b32 first_column = 1;
		_sngterm_newline(t, first_column);
	}

	if (
		(t->mode & SNGTERM_MODE_INSERT) != 0 &&
		t->cur.x+1 < t->width
	) {
		// TODO: move stuff; see st.c:2458
		// TODO: remove printf
		printf("insert mode not implemented\n");
	}

	_sngterm_set_char(t, codepoint, &t->cur.cell, t->cur.x, t->cur.y);
	if (t->cur.x+1 < t->width) {
		_sngterm_move_to(t, t->cur.x+1, t->cur.y);
	} else {
		t->cur.state |= _SNGTERM_CURSOR_WRAP_NEXT;
	}
}

static void _sngterm_state_parse_esc(SngTerm *t, u32 c) {
	if (!_sngterm_handle_control_code(t, c)) {
		return;
	}
	_SngTerm_State next = _sngterm_state_parse;
	switch (c) {
		case '[': {
			next = _sngterm_state_parse_esc_csi;
		} break;
		case '#': {
			next = _sngterm_state_parse_esc_test;
		} break;
		case 'P':   // DCS - Device Control String
		case '_':   // APC - Application Program Command
		case '^':   // PM - Privacy Message
		case ']':   // OSC - Operation System Command
		case 'k': { // old title set compatibility
			_sngterm_str_reset(&t->str);
			t->str.type_codepoint = c;
			next = _sngterm_state_parse_esc_str;
		} break;
		case '(': { // set primary charset G0
			next = _sngterm_state_parse_esc_alt_charset;
		} break;
		case ')':   // set primary charset G1 (ignored)
		case '*':   // set primary charset G2 (ignored)
		case '+': { // set primary charset G3 (ignored)
		} break;
		case 'D': { // IND - linefeed
			if (t->cur.y == t->bottom) {
				_sngterm_scroll_up(t, t->top, 1);
			} else {
				_sngterm_move_to(t, t->cur.x, t->cur.y+1);
			}
		} break;
		case 'E': { // NEL - next line
			b32 first_column = 1;
			_sngterm_newline(t, first_column);
		} break;
		case 'H': { // HTS - horizontal tab stop
			t->tabs[t->cur.x] = 1;
		} break;
		case 'M': { // RI - reverse index
			if (t->cur.y == t->top) {
				_sngterm_scroll_down(t, t->top, 1);
			} else {
				_sngterm_move_to(t, t->cur.x, t->cur.y-1);
			}
		} break;
		case 'Z': { // DECID - identify terminal
			// TODO:
		} break;
		case 'c': { // RIS - reset to initial state
			_sngterm_reset(t);
		} break;
		case '=': { // DECPAM - application keypad
			t->mode |= SNGTERM_MODE_APP_KEYPAD;
		} break;
		case '>': { // DECPNM - normal keypad
			t->mode &= ~SNGTERM_MODE_APP_KEYPAD;
		} break;
		case '7': { // DECSC - save cursor
			_sngterm_save_cursor(t);
		} break;
		case '8': { // DECRC - restore cursor
			_sngterm_restore_cursor(t);
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

static void _sngterm_state_parse_esc_alt_charset(SngTerm *t, u32 c) {
	if (!_sngterm_handle_control_code(t, c)) {
		return;
	}
	switch (c) {
		case '0': { // line drawing set
			t->cur.cell.attr |= SNGTERM_ATTR_GFX;
		} break;
		case 'B': { // USASCII
			t->cur.cell.attr &= ~SNGTERM_ATTR_GFX;
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
	t->state = _sngterm_state_parse;
}

static void _sngterm_state_parse_esc_csi(SngTerm *t, u32 c) {
	if (!_sngterm_handle_control_code(t, c)) {
		return;
	}
	if (_sngterm_csi_put(&t->csi, (char)c)) {
		t->state = _sngterm_state_parse;
		_sngterm_handle_csi(t);
	}
}

static void _sngterm_state_parse_esc_str(SngTerm *t, u32 c) {
	switch (c) {
		case '\033': {
			t->state = _sngterm_state_parse_esc_str_end;
		} break;
		case '\a': { // backwards compatibility to xterm
			t->state = _sngterm_state_parse;
			_sngterm_handle_str(t);
		} break;
		default: {
			_sngterm_str_put(&t->str, (char)c);
		} break;
	}
}

static void _sngterm_state_parse_esc_str_end(SngTerm *t, u32 c) {
	if (!_sngterm_handle_control_code(t, c)) {
		return;
	}
	t->state = _sngterm_state_parse;
	if (c == '\\') {
		_sngterm_handle_str(t);
	}
}

static void _sngterm_state_parse_esc_test(SngTerm *t, u32 c) {
	if (!_sngterm_handle_control_code(t, c)) {
		return;
	}
	// DEC screen alignment test
	if (c == '8') {
		for (int y = 0; y < t->height; y++) {
			for (int x = 0; x < t->width; x++) {
				_sngterm_set_char(t, 'E', &t->cur.cell, x, y);
			}
		}
	}
	t->state = _sngterm_state_parse;
}

int sngterm_alloc_size(int max_width, int max_height) {
	return (s32)sizeof(SngTerm) + (s32)sizeof(SngTerm_Cell)*max_width*max_height;
}

SngTerm *sngterm_init(
	void *memory, int memory_size,
	int max_width, int max_height,
	SngTerm *seed_term
) {
	int expected_size = sngterm_alloc_size(max_width, max_height);
	if (memory_size < expected_size) {
		return 0;
	}
	if (seed_term != NULL) {
		// TODO: support copying state from seed_term
		return 0;
	}
	SngTerm *t = (SngTerm *)memory;
	// TODO: needs reworking
	t->lines = (SngTerm_Cell **)&t[1];
	t->max_width = max_width;
	t->max_height = max_height;
	t->top = 0;
	t->bottom = max_height;
	t->cur = _sngterm_default_cursor();
	t->state = _sngterm_state_parse;
	return t;
}

int sngterm_set_size(SngTerm *t, int width, int height) {
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
	int min_width = _sngterm_min(t->width, width);
	int min_height = _sngterm_min(t->height, height);
	t->changed |= SNGTERM_CHANGED_SCREEN;
	t->width = width;
	t->height = height;
	_sngterm_set_scroll(t, 0, height-1);
	_sngterm_move_to(t, t->cur.x, t->cur.y);
	for (int i = 0; i < 2; i++) {
		if (min_width < width && min_height > 0) {
			_sngterm_clear(t, min_width, 0, width-1, height-1);
		}
		if (width > 0 && min_height < height) {
			_sngterm_clear(t, 0, min_height, width-1, height-1);
		}
		_sngterm_swap_screen(t);
	}
	// TODO: should we really return 1 in this case?
	if (slide <= 0) {
		return 1;
	}
	return 0;
}

void sngterm_update(SngTerm *t, u32 codepoint) {
	t->state(t, codepoint);
}

#endif // SNG_TERMINAL_IMPLEMENTATION
