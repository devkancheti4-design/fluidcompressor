/* router_law.h -- FluidCompressor's router law: one machine-authored law per memory class;
 * proven on every design eye vector by prove_router.c */
#ifndef ROUTER_LAW_H
#define ROUTER_LAW_H
#include <stdint.h>
static inline int32_t lw_RT_none(int32_t x) { return ((12 >> ((x + 12) & 31)) + (1 >> ((x + (2 + 3)) & 31))); }
static inline int32_t lw_RT_logs(int32_t x) { return ((1 - (1 >> ((49152 >> ((x >> (3 & 31)) & 31)) & 31))) + (1 >> ((x - (x >> ((3 + 3) & 31))) & 31))); }
static inline int32_t lw_RT_chat(int32_t x) { return ((1 >> ((x - 1) & 31)) + (1 >> ((x - 2) & 31))); }
static inline int32_t lw_RT_emb(int32_t x) { return (((x - 48) >> (12 & 31)) - ((x - 192) >> (12 & 31))); }
static inline int32_t lw_RT_lp(int32_t x) { return ((1 >> ((2 + (x >> (3 & 31))) & 31)) + (2 >> ((48 - (x | 12)) & 31))); }
static inline int32_t lw_RT_text(int32_t x) { return ((1 >> (((x - 49152) >> ((x >> 1) & 31)) & 31)) + (2 >> ((48 + (x | (x >> (12 & 31)))) & 31))); }
static inline int32_t L_ROUTE(int32_t s) {      /* the class whose law fires; -1 = none fired or several (abstain) */
    int32_t n = 0, k = -1;
    if (lw_RT_none(s) & 1) { n++; k = 0; }
    if (lw_RT_logs(s) & 1) { n++; k = 1; }
    if (lw_RT_chat(s) & 1) { n++; k = 2; }
    if (lw_RT_emb(s) & 1) { n++; k = 3; }
    if (lw_RT_lp(s) & 1) { n++; k = 4; }
    if (lw_RT_text(s) & 1) { n++; k = 5; }
    return n == 1 ? k : -1;
}
#endif
