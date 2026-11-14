# Windows: an ASUS laptop with a Core i3

| | |
|---|---|
| machine | ASUS laptop, 12th Gen Intel Core i3-1215U, 8 logical cores, 7.7 GB RAM |
| system | Windows, run from PowerShell 5.1 |
| program | `fluid.exe`, cross-built on a Mac (clang, lld-link, the small runtime in `src/wincrt`); nothing installed |
| reference | every archive compared with the one made on an Apple M4 Pro (size and CRC-32) |

## Full sealed test, all 38 files

| mode | threads | round trips exact | archives byte-identical to the Mac | time | peak memory |
|---|---:|---|---|---|---|
| plain (`t`) | 1 | 38 of 38 | 38 of 38 | not recorded | — |
| hive (`h`), earlier build | 1 | 38 of 38 | 38 of 38 | 493.1 s compress + 185.8 s decompress = 11.3 min | 1,342 MB |
| hive (`h`), this repository's build | 2 | 38 of 38 | 38 of 38 | 280.7 s compress + 92.9 s decompress = **6.2 min** | 2,595 MB |

The plain run on the i3 was not timed; at the quick test's ratio it would take about 32 minutes, so the current hive on
two threads is about 5.2 times faster than plain mode on one. On the M4 Pro the same 38 files take 96.2 s in hive mode
on two threads, 3.9 times faster than the i3.

## Quick test, 6 files, plain mode, 1 thread

```
FluidCompressor edge test
machine: 12th Gen Intel(R) Core(TM) i3-1215U | 8 logical cores | 7.7 GB RAM
files: 6 from quick.txt, 1 thread

  #  file                           bytes   archive  route                 c s     d s  exact  same as Mac
  1  test/enwik9_150M_64K           65536     14265  text/R/route         8.52    8.24  yes    yes
  2  test/Apache_csv               258805      3184  logs/B/measured     50.67    6.36  yes    yes
  3  test/wildchat_05_64K           65536      8201  chat/B/route        13.58   13.28  yes    yes
  4  test/embeddings_00_256K       262144     84867  emb/B/route         12.39   12.56  yes    yes
  5  test/logprobs_00_256K         262144     18272  lp/R/route           6.68    6.52  yes    yes
  6  test/Arial_ttf                773236    319408  text/B/route        16.07   16.16  yes    yes

total: 6 files, 1687401 bytes -> 448197 (3.76x) | compress 107.9 s, decompress 63.1 s (summed) | wall 171.1 s on 1 thread
round trips exact: 6 of 6 | archives byte-identical to the Mac: 6 of 6 | peak memory 672 MB
```

Against the M4 Pro on the same six files (one thread): 2.79 times as long to compress, 2.72 times as long to decompress;
every file falls between 2.4 and 3.3 times.

## Between the two hive builds

This repository's build differs from the earlier one in one way: in hive mode each thread reuses one working copy of the
warmed model instead of allocating fresh memory for every file. The archives are unchanged on both machines. The two i3
runs also differ in thread count, so the 1.8 times between them is not split between the two changes; on the Mac, going
from one thread to two gives 1.79 times on its own, which suggests the second thread did most of the work on the i3.
