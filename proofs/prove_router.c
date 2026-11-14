#include <stdio.h>
#include <stdlib.h>
#include "router_law.h"
static const int S[29] = {267, 271, 283, 4375, 4400, 5216, 6496, 8198, 8206, 9264, 9840, 10848, 11008, 11776, 12301, 12550, 15360, 16880, 17569, 17570, 21552, 21616, 21856, 22880, 26208, 26720, 26976, 32816, 38240}, V[29] = {4, 4, 0, 0, 5, 1, 1, 5, 5, 1, 1, 1, 5, 5, 5, 5, 5, 4, 2, 2, 1, 1, 1, 1, 1, 1, 1, 3, 1};
int main(int argc, char **argv) {
  if (argc > 1) { for (int i = 1; i < argc; i++) printf("%d\n", L_ROUTE(atoi(argv[i]))); return 0; }
  int bad = 0; for (int i = 0; i < 29; i++) bad += L_ROUTE(S[i]) != V[i];
  printf(bad ? "prove_router: %d WRONG\n" : "prove_router: the router law routes all 29 design vectors to their measured best memory\n", bad); return bad != 0; }
