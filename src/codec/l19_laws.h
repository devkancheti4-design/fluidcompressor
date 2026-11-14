/* l19_laws.h -- question 19 (memory): the long-recall guesser's key length and table size, as MEASURED on mtrain10
 * (best of KL 3..32 x TB 22..26; held-out mtest10 -0.295%). Measured constants, said plainly: q19 adds no new integer
 * law; its quality is the already-authored bucket law L_BKT. */
#ifndef L19_LAWS_H
#define L19_LAWS_H
#define L19_KL() 3u
#define L19_TB() 24u
#endif
