// sng_rand.h v0.1.0
// James Gray, Jan 2016
//
// OVERVIEW
//
// sng_rand implements a psuedo-random number generator using the Complementary
// Multiply-With-Carry algorithm by George Marsaglia, Raymond Couture, and
// Pierre L'Ecuyer.
//
// Literally copied from Wikipedia (like to live on the edge?), and then
// cleaned up into a single-file library.
//
// https://en.wikipedia.org/wiki/Multiply-with-carry (2016-01-03)
//
// USAGE
//
// In a .c or .cpp file, define SNG_RAND_IMPLEMENTATION before including the
// header.
//
// TODO
//
//  - Build tests for C and C++
//  - Diehard validation tests (TestU01?)
//  - User defined parameters (cycle, c max, etc.)
//  - Maybe provide some distribution functions
//
// LICENSE
//
// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy,
// distribute, and modify this file as you see fit.
//
// No warranty. Use at your own risk.

#ifndef SNG_RAND_H
#define SNG_RAND_H

#include <stdint.h>

typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;

#ifndef SNG_RAND_API
#define SNG_RAND_API
#endif

// Values from Marsaglia.
#define SNG_RAND_CMWC_CYCLE 4096
#define SNG_RAND_CMWC_C_MAX 809430660

// SngRand stores the state of the random number generator.
typedef struct {
	u32 Q[SNG_RAND_CMWC_CYCLE];
	u32 c;
} SngRand;

#ifndef SNG_RAND_NO_STDLIB
// sngRandInit initializes state with the provided seed and values from C's
// rand() implementation. Define SNG_RAND_NO_STDLIB if you plan to initialize
// the Q and c fields yourself, and do not want to include the stdlib.
SNG_RAND_API void sngRandInit(SngRand *state, u32 seed);
#endif

// sngRandU32 returns a random integer between 0 and 2^32.
SNG_RAND_API u32 sngRandU32(SngRand *state);

// sngRandF32 returns a random float between 0.0 and 1.0
SNG_RAND_API f32 sngRandF32(SngRand *state);

#endif // SNG_RAND_H

#ifdef SNG_RAND_IMPLEMENTATION

#ifndef SNG_RAND_NO_STDLIB

#include <stdlib.h>

// _sngRandRand32 makes 32 bit random number (some systems use 16 bit RAND_MAX)
static u32 _sngRandRand32() {
    u32 result = ((u32)rand() << 16) | (u32)rand();
    return result;
}

SNG_RAND_API void sngRandInit(SngRand *state, u32 seed) {
    srand(seed);
    for (int i = 0; i < SNG_RAND_CMWC_CYCLE; i++) {
		state->Q[i] = _sngRandRand32();
	}
    do {
		state->c = _sngRandRand32();
	} while (state->c >= SNG_RAND_CMWC_C_MAX);
}

#endif // !SNG_RAND_NO_STDLIB

SNG_RAND_API u32 sngRandU32(SngRand *state) {
    static u32 i = SNG_RAND_CMWC_CYCLE - 1;
    u64 t = 0;
    u64 a = 18782;      // from Marsaglia
    u32 r = 0xfffffffe; // from Marsaglia
    u32 x = 0;

    i = (i + 1) & (SNG_RAND_CMWC_CYCLE - 1);
    t = a * state->Q[i] + state->c;
    state->c = t >> 32;
    x = (u32)t + state->c;
    if (x < state->c) {
        x++;
        state->c++;
    }

    return state->Q[i] = r - x;
}

SNG_RAND_API f32 sngRandF32(SngRand *state) {
	return (f32)sngRandU32(state) / 4294967296.0f;
}

#endif // SNG_RAND_IMPLEMENTATION
