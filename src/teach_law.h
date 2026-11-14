/* teach_law.h -- FluidCompressor's teach law: machine-authored by integer program synthesis.
 * state = g(lesson gain) | g(real gain) << 2 | size bucket << 4; proven on every observed state by prove_teach.c */
#ifndef TEACH_LAW_H
#define TEACH_LAW_H
#include <stdint.h>
static inline int32_t lw_TL_REAL(int32_t x) { return (1 - (1 >> ((x + x) & 31))); }
static inline int32_t lw_TL_LESSON(int32_t x) { return ((1 - (1 >> ((x & 3) & 31))) - (x & (8 >> ((2 | (x >> (3 & 31))) & 31)))); }
static inline uint32_t L_TEACH(int32_t s) { return (uint32_t)(lw_TL_LESSON(s) & 1) | (uint32_t)(lw_TL_REAL(s) & 1) << 1; }   /* 0 cold, 1 lesson, 2 real, 3 both */
#endif
