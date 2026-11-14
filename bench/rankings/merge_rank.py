"""merge_rank.py -- one merged rank from Matt Mahoney's two largest public compression tables:
the Large Text Compression Benchmark (enwik9) and the Silesia Open Source Compression Benchmark.
    python3 bench/rankings/merge_rank.py   -> prints the ranking, writes merged_rank.md and merged.json next to itself
Tables: the pages as saved on 13 Nov 2026, parsed into rank_tables/ltcb_rows.json and rank_tables/silesia_rows.json.
Rules:
  - a program is its first name token, minus a separated version ("_v217", "-1.65.final"); digits that are part of a
    name are kept (paq8l, paq9a, fp8, bzip2, bzip3, cmm4 stay distinct); pipelines ("precomp | cmix") are their own
    program, except a preprocessor packaged by the same author (drt|lpaq9m counts as lpaq9m: LTCB lists it that way);
    two spelling aliases: zling/zling_demo = libzling, ocamyd-1.65.final = ocamyd
  - each program is represented by its BEST entry in each table: LTCB by its own measure (enwik9 + decompressor),
    Silesia by the total of its 12 files
  - only programs present in BOTH tables are ranked, plus FluidCompressor (a local measurement, not a listing)
  - score = geometric mean of the two sizes, each divided by the best in the merged set (both tables weigh equally)"""
import json, math, os, re
HERE = os.path.dirname(os.path.abspath(__file__)); T = os.path.join(HERE, 'rank_tables')
FLUID_E9, FLUID_DEC, FLUID_SIL = 132510285, 161853, 38820251       # enwik9 archive, zipped decompressor, Silesia total
ALIAS = {'zling': 'libzling', 'zling_demo': 'libzling', 'ocamyd-1.65.final': 'ocamyd'}
PACKAGED = ('drt', 'epmopt')       # LTCB lists a same-author preprocessor + compressor as that compressor
def prog(name):
    out = []
    parts = [p.strip() for p in name.lower().split('|') if p.strip()]
    if len(parts) == 2 and parts[0].split()[0] in PACKAGED: parts = parts[1:]
    for part in parts:
        tok = part.split()[0]
        tok = re.sub(r'(_v\d.*|-v?\d+(\.\d+)+.*)$', '', tok) or part.split()[0]
        out.append(ALIAS.get(tok, tok))
    return '+'.join(out)
def best(rows):
    out = {}
    for r in rows:
        f = prog(r['name'])
        if f not in out or r['total'] < out[f]['total']: out[f] = r
    return out
lt = json.load(open(os.path.join(T, 'ltcb_rows.json')))
st = [r for r in json.load(open(os.path.join(T, 'silesia_rows.json'))) if r['name'] != 'Uncompressed']
L, S = best(lt), best(st)
L['FluidCompressor'] = {'name': 'FluidCompressor (enwik9 132,510,285 + decompressor 161,853)', 'total': FLUID_E9 + FLUID_DEC}
S['FluidCompressor'] = {'name': 'FluidCompressor (no installed memory)', 'total': FLUID_SIL}
P = sorted(set(L) & set(S)); n = len(P)
lmin = min(L[p]['total'] for p in P); smin = min(S[p]['total'] for p in P)
lr = {p: i + 1 for i, p in enumerate(sorted(P, key=lambda p: L[p]['total']))}
sr = {p: i + 1 for i, p in enumerate(sorted(P, key=lambda p: S[p]['total']))}
score = {p: math.sqrt(L[p]['total'] / lmin * S[p]['total'] / smin) for p in P}
order = sorted(P, key=lambda p: score[p])
full = lambda rows, tot: 1 + sum(r['total'] < tot for r in rows)
rows = [dict(rank=i + 1, program=p, ours=p == 'FluidCompressor', score=round(score[p], 4),
             enwik9_total=L[p]['total'], enwik9_entry=' '.join(L[p]['name'].split()), enwik9_rank_in_set=lr[p], enwik9_rank_full=full(lt, L[p]['total']),
             silesia_total=S[p]['total'], silesia_entry=' '.join(S[p]['name'].split()), silesia_rank_in_set=sr[p], silesia_rank_full=full(st, S[p]['total']))
        for i, p in enumerate(order)]
json.dump(dict(meta=dict(ltcb_programs=len(lt), silesia_programs=len(st), merged_programs=n, tables_saved='13 Nov 2026',
                         ltcb_url='https://mattmahoney.net/dc/text.html', silesia_url='https://mattmahoney.net/dc/silesia.html'), rows=rows),
          open(os.path.join(HERE, 'merged.json'), 'w'), indent=1)
md = ['# Merged rank: enwik9 (LTCB) + Silesia', '', 'Tables saved 13 Nov 2026. Each program is its best entry in each table; only programs in both',
      'tables are ranked. FluidCompressor is a local measurement, not an official listing.', '',
      '| rank | program | score | enwik9 + decompressor | # | Silesia total | # |', '|---:|---|---:|---:|---:|---:|---:|']
for r in rows:
    nm = '**FluidCompressor**' if r['ours'] else r['program']
    md.append('| %d | %s | %.4f | %s | %d | %s | %d |' % (r['rank'], nm, r['score'], format(r['enwik9_total'], ','), r['enwik9_rank_in_set'], format(r['silesia_total'], ','), r['silesia_rank_in_set']))
open(os.path.join(HERE, 'merged_rank.md'), 'w').write('\n'.join(md) + '\n')
f = [r for r in rows if r['ours']][0]
print('FluidCompressor: %d of %d merged | LTCB %d of %d | Silesia %d of %d' % (f['rank'], n, f['enwik9_rank_full'], len(lt) + 1, f['silesia_rank_full'], len(st) + 1))
for r in rows[:8]: print('  %2d  %-16s %.4f' % (r['rank'], r['program'], r['score']))
