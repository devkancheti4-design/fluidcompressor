#include <stdio.h>
#include "teach_law.h"
static const int S[18] = {0, 5, 8, 12, 13, 14, 16, 21, 24, 28, 29, 30, 32, 37, 40, 44, 45, 46}, V[18] = {0, 3, 2, 2, 2, 3, 0, 3, 2, 2, 2, 3, 0, 3, 2, 2, 3, 3};
int main(void) { int bad = 0; for (int i = 0; i < 18; i++) bad += L_TEACH(S[i]) != (unsigned)V[i];
  printf(bad ? "prove_teach: %d WRONG\n" : "prove_teach: the teach law equals the measured best on all 18 observed states\n", bad);
  printf("decisions on all 64 states (lesson g0..g3 across, real g0..g3 down; size 0 | 1 | 2 | 3):\n");
  for (int r = 0; r < 4; r++) { for (int z = 0; z < 4; z++) { for (int l = 0; l < 4; l++) printf("%c", "-LRB"[L_TEACH(l | r << 2 | z << 4)]); printf("  "); } printf("\n"); }
  return bad != 0; }
