# Windows: an ASUS laptop with a Core i3

| | |
|---|---|
| machine | ASUS laptop, 12th Gen Intel Core i3-1215U, 8 logical cores, 7.7 GB RAM |
| system | Windows, run from PowerShell 5.1 |
| program | `fluid.exe`, cross-built on a Mac (clang, lld-link, the small runtime in `src/wincrt`); nothing installed |
| reference | every archive compared with the one made on an Apple M4 Pro (size and CRC-32) |

## Full sealed test, all 38 files

| mode | round trips exact | archives byte-identical to the Mac | time (1 thread) | peak memory |
|---|---|---|---|---|
| plain (`t`) | 38 of 38 | 38 of 38 | not recorded | — |
| hive (`h`) | 38 of 38 | 38 of 38 | 493.1 s compress + 185.8 s decompress = 11.3 min | 1,342 MB |

The same 38 files take 172.5 s on the M4 Pro in hive mode on one thread (3.9 times faster than the i3). The plain run
on the i3 was not timed; at the quick test's ratio it would take about 32 minutes, so the hive was about 2.9 times faster.

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

## Since this run

The `fluid.exe` in this repository adds one change after these measurements: in hive mode each thread now reuses one
working copy of the warmed model instead of allocating fresh memory for every file. The archives are unchanged (checked
on all 38 files on the Mac); on the i3 the change targets the roughly 200 seconds the hive spent copying into fresh
memory. It has not yet been re-timed on the i3.
