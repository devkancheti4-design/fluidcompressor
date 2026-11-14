# Notices

The code in this repository (everything under `src/`, `proofs/`, `tools/`, the scripts, and `fluid.exe`) is licensed
under the GNU Affero General Public License v3.0; see `LICENSE`. The data files keep their own licenses, listed here.

## Memories and test files

| files | what they are | source and license |
|---|---|---|
| `memory/real_text`, `bench/sealed/test/enwik9_*` | excerpts of enwik9, English Wikipedia text | [enwik9](https://mattmahoney.net/dc/textdata.html), from Wikipedia; [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/). Cut into excerpts, otherwise unchanged. |
| `memory/real_logs`, `bench/sealed/test/*_csv` | system logs and their structured CSVs | [Loghub](https://github.com/logpai/loghub); license notice below. Cut into excerpts, otherwise unchanged. |
| `memory/real_chat`, `bench/sealed/test/wildchat_*` | chatbot conversation transcripts (JSON lines) | [WildChat-1M](https://huggingface.co/datasets/allenai/WildChat-1M), Allen Institute for AI; [ODC-BY 1.0](https://opendatacommons.org/licenses/by/1-0/). Cut into excerpts, otherwise unchanged. |
| `memory/real_emb`, `bench/sealed/test/embeddings_*` | OpenAI-style embeddings responses | generated locally: mean-pooled, L2-normalised hidden states of Qwen2.5-0.5B (Apache-2.0) over texts from the public ag_news, imdb, emotion and trec datasets |
| `memory/real_lp`, `bench/sealed/test/logprobs_*` | OpenAI-style chat completions with token log-probabilities | generated locally with Llama-3.2-1B-Instruct (Llama 3.2 Community License Agreement; Built with Llama) from the same public texts |
| `memory/lesson_*` | lessons: example data written for this project | covered by this repository's license |

The six sealed-test files cut from macOS system files (`bash`, `zsh`, `ksh`, `Helvetica_ttc`, `Arial_ttf`, `web2`) are
not included; `bench/sealed/make_mac_files.py` recreates them on a Mac.

## Benchmark tables

`bench/rankings/rank_tables/` holds program names and sizes parsed from Matt Mahoney's public pages, the
[Large Text Compression Benchmark](https://mattmahoney.net/dc/text.html) and the
[Silesia Open Source Compression Benchmark](https://mattmahoney.net/dc/silesia.html), as saved on 13 November 2026.
FluidCompressor's own entries are local measurements, not official listings.

## The enwik9 entry

FluidCompressor's enwik9 result uses the enwik9-specific preprocessing published with
[fx2-cmix](https://github.com/kaitz/fx2-cmix) by Kaido Orav and Byron Knoll (article reordering, the wiki transform
and its dictionary; GPL-3.0) in front of FluidCompressor's coder. That preprocessing is not part of this repository.

The rankings page in `docs/` loads d3 (ISC license) from cdnjs and the Source Sans 3 and Source Serif 4 fonts (SIL Open
Font License) from Google Fonts.

## Loghub license notice

```
LICENSE OF LOGHUB

The datasets are freely available for research or academic work, subject to
the following condition: For any usage or distribution of the loghub datasets,
please refer to the loghub repository URL (https://github.com/logpai/loghub)
and cite the following loghub paper where applicable.

Jieming Zhu, Shilin He, Pinjia He, Jinyang Liu, Michael R. Lyu. Loghub: A Large
Collection of System Log Datasets for AI-driven Log Analytics. In ISSRE, 2023.

The above license notice shall be included in all copies of the datasets.
```

## WildChat citation

Wenting Zhao, Xiang Ren, Jack Hessel, Claire Cardie, Yejin Choi, Yuntian Deng. WildChat: 1M ChatGPT Interaction Logs
in the Wild. In ICLR, 2024.
