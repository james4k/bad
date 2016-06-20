#define SNG_TERMINAL_IMPLEMENTATION
#include "sng_terminal.h"

void testCSIParse() {
	_SngTermCSI c;
	_sngTermCSIReset(&c);
	c.bufLen = snprintf(c.buf, sizeof(c.buf), "s");
	_sngTermCSIParse(&c);
	if (c.mode != 's' || _sngTermCSIArg(&c, 0, 17) != 17 || c.argsLen != 0) {
		fprintf(stderr, "%s:%d: CSI parse mismatch\n", __FILE__, __LINE__);
	}

	_sngTermCSIReset(&c);
	c.bufLen = snprintf(c.buf, sizeof(c.buf), "31T");
	_sngTermCSIParse(&c);
	if (c.mode != 'T' || _sngTermCSIArg(&c, 0, 0) != 31 || c.argsLen != 1) {
		fprintf(stderr, "%s:%d: CSI parse mismatch\n", __FILE__, __LINE__);
	}

	_sngTermCSIReset(&c);
	c.bufLen = snprintf(c.buf, sizeof(c.buf), "48;2f");
	_sngTermCSIParse(&c);
	if (
		c.mode != 'f' ||
		_sngTermCSIArg(&c, 0, 0) != 48 ||
		_sngTermCSIArg(&c, 1, 0) != 2 ||
		c.argsLen != 2
	) {
		fprintf(stderr, "%s:%d: CSI parse mismatch\n", __FILE__, __LINE__);
	}

	_sngTermCSIReset(&c);
	c.bufLen = snprintf(c.buf, sizeof(c.buf), "?25l");
	_sngTermCSIParse(&c);
	if (c.mode != 'l' || _sngTermCSIArg(&c, 0, 0) != 25 || c.priv == 0 || c.argsLen != 1) {
		fprintf(stderr, "%s:%d: CSI parse mismatch\n", __FILE__, __LINE__);
	}
}

void testSTRParse() {
	_SngTermSTR s;
	_sngTermSTRReset(&s);
	s.bufLen = snprintf(s.buf, sizeof(s.buf), "0;some text");
	_sngTermSTRParse(&s);
	if (
		_sngTermSTRArg(&s, 0, 17) != 0 ||
		strcmp(_sngTermSTRArgString(&s, 1, ""), "some text") != 0
	) {
		fprintf(stderr, "%s:%d: STR parse mismatch\n", __FILE__, __LINE__);
	}
}

#if 0
func extractStr(t *State, x0, x1, row int) string {
    var s []rune
    for i := x0; i <= x1; i++ {
        c, _, _ := t.Cell(i, row)
        s = append(s, c)
    }
    return string(s)
}

func TestPlainChars(t *testing.T) {
    var st State
    term, err := Create(&st, nil)
    if err != nil {
        t.Fatal(err)
    }
    expected := "Hello world!"
    _, err = term.Write([]byte(expected))
    if err != nil && err != io.EOF {
        t.Fatal(err)
    }
    actual := extractStr(&st, 0, len(expected)-1, 0)
    if expected != actual {
        t.Fatal(actual)
    }
}
#endif

void extractString(SngTerm *t, char *dest, size_t destSize, int x0, int x1, int y) {
	uintptr_t i = 0;
	for (int x = x0; x <= x1; x++) {
		if (i >= destSize) {
			dest[destSize-1] = 0;
			return;
		}
		dest[i++] = (char)t->lines[y][x].codepoint;
	}
	dest[i] = 0;
}

void testPlainChars() {
	int maxWidth = 40;
	int maxHeight = 20;
	size_t memSize = sngTermAllocSize(maxWidth, maxHeight);
	SngTerm *t = sngTermInit(malloc(memSize), memSize, maxWidth, maxHeight, NULL);
	sngTermSetSize(t, maxWidth, maxHeight);

	const char *expected = "Hello world!";
	for (const char *c = expected; *c != 0; c++) {
		sngTermUpdate(t, (u32)*c);
	}

	char actual[256];
	extractString(t, actual, sizeof(actual), 0, (int)strlen(expected)-1, 0);
	if (strcmp(expected, actual) != 0) {
		fprintf(stderr, "%s:%d: testPlainChars expected='%s' actual='%s'\n", __FILE__, __LINE__, expected, actual);
	}
}

void testNewline() {
	int maxWidth = 40;
	int maxHeight = 20;
	size_t memSize = sngTermAllocSize(maxWidth, maxHeight);
	SngTerm *t = sngTermInit(malloc(memSize), memSize, maxWidth, maxHeight, NULL);
	sngTermSetSize(t, maxWidth, maxHeight);

	const char *crlfMode = "\033[20h";
	const char *expected0 = "Hello world!";
	const char *expected1 = "...and more.";
	for (const char *c = crlfMode; *c != 0; c++) {
		sngTermUpdate(t, (u32)*c);
	}
	for (const char *c = expected0; *c != 0; c++) {
		sngTermUpdate(t, (u32)*c);
	}
	sngTermUpdate(t, (u32)'\n');
	for (const char *c = expected1; *c != 0; c++) {
		sngTermUpdate(t, (u32)*c);
	}

	char actual0[256];
	extractString(t, actual0, sizeof(actual0), 0, (int)strlen(expected0)-1, 0);
	if (strcmp(expected0, actual0) != 0) {
		fprintf(
			stderr,
			"%s:%d: testNewline expected0='%s' actual0='%s'\n",
			__FILE__, __LINE__,
			expected0, actual0
		);
	}
	char actual1[256];
	extractString(t, actual1, sizeof(actual1), 0, (int)strlen(expected1)-1, 1);
	if (strcmp(expected1, actual1) != 0) {
		fprintf(
			stderr,
			"%s:%d: testNewline expected1='%s' actual1='%s'\n",
			__FILE__, __LINE__,
			expected1, actual1
		);
	}


	// A newline with a color set should not make the next line that
	// color, which used to happen if it caused a scroll event.
	_sngTermMoveTo(t, 0, t->height-1);
	const char *output = "\033[1;37m\n$ \033[m";
	for (const char *c = output; *c != 0; c++) {
		sngTermUpdate(t, (u32)*c);
	}
	SngTermCell c = t->lines[t->cur.y][t->cur.x];
	if (c.fg != SNG_TERM_COLOR_DEFAULT_FG) {
		fprintf(
			stderr,
			"%s:%d: testNewline %d %d %d %d\n",
			__FILE__, __LINE__,
			t->cur.x, t->cur.y, (int)c.fg, (int)c.bg
		);
	}
}

int main(int argc, char **argv) {
	// suppress -Wunused-parameter
	(void)argc;
	(void)argv;
	testCSIParse();
	testSTRParse();
	testPlainChars();
	testNewline();
	return 0;
}
