# FluidCompressor

Lossless compression that knows what it is compressing and remembers what it has seen.

FluidCompressor reads the first 64 KB of a file and recognises the kind of data: text, logs and records, AI chat
transcripts, AI-API responses (embeddings, token log-probabilities) or binary. It picks a memory of similar data, lets a
context-mixing coder study that memory first, then compresses the file. Every round trip is exact, and the archive is the
same, byte for byte, on every machine: an Apple M4 Pro on macOS and an Intel Core i3 on Windows produce identical files.

## Results

### Sealed test: 38 files the design never touched

Text from unused parts of enwik9, the 16 Loghub structured CSVs (a format it was never taught), unopened WildChat
transcripts, unused regions of AI-API files, and six system binaries and fonts. Total compressed bytes by kind:

| data | bytes | **fluid** | xz -9e | brotli -q 11 | zstd -22 | zstd + dictionary | bzip2 -9 | gzip -9 | bsc |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| text | 1,441,792 | **157,962** | 253,956 | 236,256 | 270,720 | 261,284 | 248,263 | 318,772 | 227,654 |
| logs and records | 4,659,751 | **128,320** | 284,524 | 285,738 | 312,243 | 298,458 | 327,650 | 427,583 | 302,204 |
| AI chat | 4,456,448 | **533,421** | 766,240 | 719,083 | 837,694 | 833,629 | 808,947 | 1,009,165 | 723,970 |
| AI-API | 1,048,576 | **207,226** | 260,744 | 252,701 | 259,351 | 254,676 | 252,166 | 300,020 | 235,780 |
| binary | 6,016,116 | **1,908,424** | 2,217,096 | 2,271,769 | 2,398,931 | 2,412,359 | 2,723,382 | 2,775,602 | 2,486,096 |
| **all 38** | 17,622,683 | **2,935,353** | 3,782,560 | 3,765,547 | 4,078,939 | 4,060,406 | 4,360,408 | 4,831,142 | 3,975,704 |

- Smallest on **38 of 38** files.
- **20.1% smaller** than the best of the seven tools chosen file by file, and 22% smaller than brotli or xz overall.
- All 38 round trips exact. "zstd + dictionary" is zstd -19 with dictionaries trained on FluidCompressor's own memories,
  the best of five per file. Per-file sizes: `bench/results/sealed_results.json`.

### Matt Mahoney's two largest public benchmarks

| benchmark | FluidCompressor | rank |
|---|---:|---|
| [Large Text Compression Benchmark](https://mattmahoney.net/dc/text.html): enwik9 + zipped decompressor | 132,672,138 | **31 of 234** |
| [Silesia Open Source Compression Benchmark](https://mattmahoney.net/dc/silesia.html): 12 files, 211,938,580 bytes | 38,820,251 | **60 of 324** |
| merged: the 75 programs listed in both tables | score 1.311 | **4 of 75**, behind paq8px, cmix and gleipnir |

The merged score is the geometric mean of a program's two sizes, each divided by the best in the set; each program is its
best entry in each table. Tables saved 13 November 2026. FluidCompressor's entries are local measurements on an Apple
M4 Pro, not official listings. `python3 bench/rankings/merge_rank.py` rebuilds the ranking from the saved tables
(`bench/rankings/merged_rank.md` has all 75 rows). The enwik9 entry puts the enwik9-specific preprocessing of fx2-cmix
(GPL-3.0, not included) in front of FluidCompressor's coder; see `NOTICE.md`.

### Windows: the same bytes on a budget laptop

On an ASUS laptop with a 12th Gen Intel Core i3-1215U and 7.7 GB of RAM, running `fluid.exe` with nothing installed:

- the full sealed test: **38 of 38 archives byte-identical to the Mac's**, 38 of 38 round trips exact;
- 2.8 times the M4 Pro's time per file, peaking at 672 MB on the quick test;
- hive mode, all 38 files: **6.2 minutes on two threads** (peak 2.6 GB), against about 32 minutes for plain mode on one
  thread.

Details: `bench/results/windows_i3.md`.

### Speed and memory

Context mixing is slow, and this is no exception:

- About 0.6 MB/s per thread on large files on an Apple M4 Pro (1,200 ns per byte to compress, 1,140 to decompress).
  Decompressing costs as much as compressing.
- Each file first replays its memory (2 to 4 MB) before coding. Hive mode (`h`) warms each memory once for a whole
  batch: 4.1 times faster on the 38 sealed files, one thread.
- About 1 to 2 GB of RAM per thread.

It fits data written once and kept, where storage costs more than CPU time: AI transcripts, logs, archives. It does not
fit streaming or frequently read data.

## How it works

1. **Eyes.** Eight byte statistics of the first 64 KB (high bytes, zeros, digits, JSON marks, markup, letters, newlines,
   decimals), two bits each: a 16-bit eye vector.
2. **Router law.** A small integer law maps the eye vectors it was designed on to a memory class. On any other vector it
   abstains, and the compressor measures every memory and keeps the smallest (the first one on a tie).
3. **Teach law.** Chooses the lesson, the real memory, both, or neither, from the gains measured at teach time and the
   file's size.
4. **Memory replay.** The coder codes the chosen memory into scratch first, so its models start warm; the decoder does
   the same, so both sides stay in step.
5. **Coder.** A bitwise context-mixing coder: the max mode of [shz](https://github.com/devkancheti4-design/shz-k) plus
   further contexts (words, open brackets, long-range recall).

The archive is one route byte (memory class and teach act) followed by the coder's stream, whose header carries the
size and the CRC-32 of the data and of the memory. The integer laws were machine-authored by integer program synthesis;
each is an int32 expression checked on its whole input domain (compile with `-fwrapv`). `proofs/` holds the checks of
the router and teach laws.

## Build and run

macOS or Linux, any C11 compiler:

```sh
./build.sh
./fluid c IN OUT              # compress
./fluid d IN OUT              # decompress
./fluid t LIST [threads]      # test: compress, decompress, check and time every file in LIST
./fluid h LIST [threads]      # the same test in hive mode
```

Windows: use the included `fluid.exe` (a reproducible build: `./build_windows.sh` cross-builds the identical file from
macOS or Linux with clang and lld-link, with no Windows SDK or MinGW). In PowerShell:

```powershell
.\fluid.exe h bench\sealed\tests.txt 2
```

The program reads its memories from the `memory` folder next to it.

## Reproduce the benchmark

`bench/sealed/tests.txt` lists the 32 sealed files included here, each with the Mac's archive size and CRC-32 and its
one-thread M4 Pro times. Run `./fluid h bench/sealed/tests.txt 4`: every round trip is checked, every archive is compared
with the Mac's, and the results go to `bench/sealed/results_hive.txt`. `quick.txt` is a five-file smoke test. The six
files cut from macOS system files are not redistributed; `bench/sealed/make_mac_files.py` cuts them on a Mac and
`tests_all.txt` lists all 38 (their reference values match only the macOS build they were cut on).

Mac reference runs: `bench/results/mac_m4pro_plain_1thread.txt` and `bench/results/mac_m4pro_hive_1thread.txt`.

## Layout

| path | contents |
|---|---|
| `src/fluid.c` | the program: eyes, router and teach laws, memories, test and hive modes |
| `src/fluid_codec.c`, `src/codec/` | the coder and its integer laws |
| `src/router_law.h`, `src/teach_law.h`, `src/fluid_tables.h` | the decision laws and the tables they were designed on |
| `src/plat_posix.c`, `src/wincrt/` | the machine layer: POSIX, and a small C runtime written on kernel32 for Windows |
| `memory/` | the memories and lessons |
| `bench/` | the sealed test, the rankings, and the reference results |
| `proofs/`, `tools/` | the law checks; the tool that writes `fluid_tables.h` |

## License

Copyright 2026 Devieswar Kancheti. The code is licensed under the GNU Affero General Public License v3.0 (`LICENSE`).
The data files keep their own licenses: see `NOTICE.md`.
