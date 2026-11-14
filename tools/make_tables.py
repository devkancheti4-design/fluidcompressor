"""make_tables.py -- writes src/fluid_tables.h from router_rows.json and teach_rows.json: the eye vectors the router law
was designed on (SEEN; on any other vector the router abstains and measures every memory) and the teach-time gain
buckets per memory class (for logprobs, which has no lesson, the smallest measured gain)."""
import json, os
HERE = os.path.dirname(os.path.abspath(__file__)); SRC = os.path.join(os.path.dirname(HERE), 'src')
R = json.load(open(os.path.join(HERE, 'router_rows.json'))); T = json.load(open(os.path.join(HERE, 'teach_rows.json')))
CL = ['none', 'logs', 'chat', 'emb', 'lp', 'text']
TK = {'logs': 'logs', 'chat': 'ai-chat', 'emb': 'ai-api', 'text': 'text'}
SEEN = sorted({r['sig'] for r in R['design']})
lp_gain = sorted(100 * (1 - r['sizes']['lp'] / r['sizes']['none']) for r in R['design'] if r['kind'] == 'lp')[0]
g = lambda gain: 0 if gain <= 0 else 1 if gain < 2 else 2 if gain < 8 else 3
gkl, gkr = [], []
for k in CL:
    if k == 'none': gkl.append(0); gkr.append(0)
    elif k == 'lp': gkl.append(0); gkr.append(g(lp_gain))
    else: gkl.append(g(T['KL'][TK[k]])); gkr.append(g(T['KR'][TK[k]]))
out = ['/* fluid_tables.h -- written by tools/make_tables.py from router_rows.json and teach_rows.json; do not edit */',
       '#define F_NSEEN %d' % len(SEEN),
       'static const int32_t F_SEEN[F_NSEEN] = {%s};' % ', '.join(map(str, SEEN)),
       'static const uint8_t F_GKL[6] = {%s};' % ', '.join(map(str, gkl)),
       'static const uint8_t F_GKR[6] = {%s};' % ', '.join(map(str, gkr)), '']
open(os.path.join(SRC, 'fluid_tables.h'), 'w').write('\n'.join(out))
print('%d seen vectors, KL buckets %s, KR buckets %s (lp gain %.3f)' % (len(SEEN), gkl, gkr, lp_gain))
