/* loop_hooks3.h -- the law loop's switch for every question so far:
 *   R  bucket counters' rate by count (q1)      B  the APM's weight in the final blend, by bit position (q2)
 *   M  the three mixers' combining weights (q3) A  the APM's learning rate, by bit position (q4)
 *   G  the guessers' confidence-counter rate by count (q5)   W  the mixers' learning rate, eighths per mixer and bit (q6)
 *   U  the order-1 APM's share against the order-2 APM (q7)   Q  the order-1 APM's learning rate (q7)
 *   S  which context selects mixer C's weight row (q8)
 * TABLE: measured tables (search phase); LAW: the machine-authored laws (loop_laws_all.h); default: the codec's own. */
#include <stdint.h>
#if defined(LOOP_TABLE)
static uint32_t T_R[256], T_RN = 62, T_B[8] = {6, 6, 6, 6, 6, 6, 6, 6}, T_M[24], T_A[8], T_G[1024], T_GN = 62,
                T_W[24] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
                T_U[8] = {0, 0, 0, 0, 0, 0, 0, 0}, T_Q[8] = {7, 7, 7, 7, 7, 7, 7, 7}, T_S = 0, T_V = 0, T_Y = 0,
                T_D[8] = {0, 0, 0, 0, 0, 0, 0, 0}, T_E[8] = {8, 8, 8, 8, 8, 8, 8, 8}, T_X = 0,
                T_H[8] = {0, 0, 0, 0, 0, 0, 0, 0}, T_J[8] = {7, 7, 7, 7, 7, 7, 7, 7}, T_C = 2, T_K = 3, T_L = 1, T_F[8] = {0, 0, 0, 0, 0, 0, 0, 0};
#define XRATE(n)  T_R[(n)]
#define XNINC(n)  ((n) < T_RN ? (n) + 1u : (n))
#define XBLEND(pm, p2, k) ((((8u - T_B[(k)]) * (pm)) + T_B[(k)] * (p2) + 4u) >> 3)
#define XMIX(sA, sB, sC, k) L_CLAMP12((((int)T_M[3 * (k)] * (sA) + (int)T_M[3 * (k) + 1] * (sB) + (int)T_M[3 * (k) + 2] * (sC)) * 8192) >> 16)
#define XWSTEP(x, j, k) (((int32_t)L_WSTEP(x) * (int32_t)T_W[3 * (k) + (j)]) >> 3)
/* question 11: mixer D (word-selected), its share in the mix, its learning rate, its selector candidates */
#define XMIX4(sA, sB, sC, sD, k) L_CLAMP12((((int)T_M[3 * (k)] * (sA) + (int)T_M[3 * (k) + 1] * (sB) + (int)T_M[3 * (k) + 2] * (sC) + (int)T_D[(k)] * (sD)) * 8192) >> 16)
#define XWDSTEP(x, k) (((int32_t)L_WSTEP(x) * (int32_t)T_E[(k)]) >> 3)
#define XWDSEL(w, c0, k) (T_X == 0 ? ((w) & 0xFFFFu) : T_X == 1 ? ((w) >> 16) : T_X == 2 ? ((((w) & 0x1FFFu) << 3) | (k)) : \
                          T_X == 3 ? ((((w) & 0xFFu) << 8) | (c0)) : (((w) ^ ((w) >> 16)) & 0xFFFFu))
#define XAPMMIX(p2, p1, k) ((((8u - T_U[(k)]) * (p2)) + T_U[(k)] * (p1) + 4u) >> 3)
#define XAPMMIX3(pa, p3, k) ((((8u - T_H[(k)]) * (pa)) + T_H[(k)] * (p3) + 4u) >> 3)
#define XA3R(k) T_J[(k)]
#define XCTX12(c4, c8, w, pw) (T_C == 1 ? (L_HM1(c4) + ((c8) & 0xFFu)) : T_C == 2 ? (L_HM1((c4) ^ L_HM1(c8))) : \
                               T_C == 3 ? ((c4) & 0xFFFF0000u) : T_C == 4 ? ((c4) & 0x00FF00FFu) : \
                               T_C == 5 ? ((w) ^ (((c4) & 0xFFu) << 24)) : ((c4) & 0xFF0000FFu))
#define XCTX13(c4, c8, c12, c16) (T_K == 1 ? L_HM1((c4) ^ L_HM1((c8) & 0xFFFFFFu)) : T_K == 2 ? L_HM1((c4) ^ L_HM1((c8) ^ L_HM1((c12) & 0xFFFFu))) : \
                                  T_K == 3 ? L_HM1(L_HM1((c4) ^ L_HM1(c8)) ^ (c12)) : L_HM1(L_HM1(L_HM1((c4) ^ L_HM1(c8)) ^ (c12)) ^ (c16)))
#define XO12(c4, c8, c12) L_HM1(L_HM1((c4) ^ L_HM1(c8)) ^ (c12))
#define XMW(k, j) ((j) < 3 ? T_M[3 * (k) + (j)] : T_D[(k)])
#define XFG(k) T_F[(k)]
#define XCTX14(c4, c8, c12, c16, c20, c24, w) (T_L == 1 ? L_HM1(XO12(c4, c8, c12) ^ (c16)) : \
                                               T_L == 2 ? L_HM1(L_HM1(L_HM1(XO12(c4, c8, c12) ^ (c16)) ^ (c20)) ^ (c24)) : \
                                               T_L == 3 ? L_HM1((w) ^ ((c4) & 0xFFFFu)) : L_HM1((w) ^ ((c4) & 0xFFFFFFu)))
#define XA1R(k) T_Q[(k)]
/* question 9: candidate selectors of mixer B's row, as laws of P = (last two bytes) << 16 | mq << 8 | c0 */
#define XPACK(c4, c0, mq) ((((c4) & 0xFFFFu) << 16) | ((mq) << 8) | (c0))
#define XWBSEL(c4, c0, k, mq) XWBSEL_P(XPACK(c4, c0, mq))
/* question 10: candidate selectors of mixer A's row (today mq * 8 + k) */
#define XWASEL(c4, c0, k, mq) (T_Y == 0 ? ((mq) * 8u + (k)) : T_Y == 1 ? (XPACK(c4, c0, mq) & 0xFFFu) : \
                               T_Y == 2 ? (((mq) * 8u + (k)) | ((((c4) >> 4) & 0xFu) << 7)) : T_Y == 3 ? (((mq) * 8u + (k)) | (((c4) & 0xFFu) << 7)) : \
                               ((XPACK(c4, c0, mq) & 0xFFFu) | ((XPACK(c4, c0, mq) >> 16) & 0xF000u)))
#define XWBSEL_P(P) (T_V == 0 ? ((P) & 0xFFu) : T_V == 1 ? (((P) & 0xFFu) | (((P) >> 12) & 0xF00u)) : \
                     T_V == 2 ? (((P) & 0xFFu) | (((P) >> 8) & 0xFF00u)) : T_V == 3 ? ((P) & 0xFFFu) : \
                     (((P) & 0xFFFu) | (((P) >> 8) & 0xF000u)))
/* question 8: candidate selectors of mixer C's row, from c4 (last 4 bytes), c0 (partial byte), k (bit position) */
#define XWCSEL(c4, c0, k) (T_S == 0 ? ((c4) & 0xFFu) : T_S == 1 ? ((((c4) & 0xFFu) << 3) | (k)) : \
                           T_S == 2 ? ((((c4) & 0xF0u) << 4) | (c0)) : T_S == 3 ? (((c4) >> 8) & 0xFFu) : \
                           T_S == 4 ? ((((c4) & 0xFFu) << 4) | (((c4) >> 12) & 0xFu)) : T_S == 5 ? ((c4) & 0xFFFFu) : \
                           T_S == 6 ? ((((((c4) & 0xFFu) << 4) | (((c4) >> 12) & 0xFu)) << 3) | (k)) : \
                           ((((c4) & 0xFFu) << 8) | ((((c4) >> 12) & 0xFu) << 4) | (((c4) >> 20) & 0xFu)))
#define XAPMR(k)  T_A[(k)]
#define XGRATE(n) T_G[(n)]
#define XGNINC(n) ((n) < T_GN ? (n) + 1u : (n))
#elif defined(LOOP_LAW)
#include "loop_laws_all.h"
#else
#define XRATE(n)  L_RATE(n)
#define XNINC(n)  L_NINC(n)
#define XBLEND(pm, p2, k) (((pm) + 3u * (p2) + 2u) >> 2)
#define XMIX(sA, sB, sC, k) ((((sA) + (sB) + 2 * (sC)) * 16384) >> 16)
#define XAPMR(k)  7u
#define XGRATE(n) L_RATE(n)
#define XGNINC(n) L_NINC(n)
#define XWSTEP(x, j, k) L_WSTEP(x)
#define XAPMMIX(p2, p1, k) (p2)
#define XA1R(k) 7u
#define XWCSEL(c4, c0, k) ((c4) & 0xFFu)
#define XWBSEL(c4, c0, k, mq) (c0)
#define XWASEL(c4, c0, k, mq) ((mq) * 8u + (k))
#define XMIX4(sA, sB, sC, sD, k) XMIX(sA, sB, sC, k)
#define XWDSTEP(x, k) L_WSTEP(x)
#define XWDSEL(w, c0, k) ((w) & 0xFFFFu)
#define XAPMMIX3(pa, p3, k) (pa)
#define XA3R(k) 7u
#define XCTX12(c4, c8, w, pw) (L_HM1((c4) ^ L_HM1(c8)))
#define XCTX13(c4, c8, c12, c16) L_HM1(L_HM1((c4) ^ L_HM1(c8)) ^ (c12))
#define XCTX14(c4, c8, c12, c16, c20, c24, w) L_HM1(L_HM1(L_HM1((c4) ^ L_HM1(c8)) ^ (c12)) ^ (c16))
#define XMW(k, j) ((j) == 2 ? 4u : (j) == 3 ? 0u : 2u)
#define XFG(k) 0u
#endif
