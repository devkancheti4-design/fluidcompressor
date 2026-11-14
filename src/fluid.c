/* fluid.c -- FluidCompressor in one program: EYES -> ROUTER LAW -> TEACH LAW -> MEMORY replay -> law-loop codec.
   Same decisions and bytes as the original Python driver, with no Python and no helper processes.
     fluid c IN OUT      compress            fluid d IN OUT      decompress
     fluid t LIST [P]    edge test on P threads: compress, decompress, check, time every file in LIST and compare each
                         archive with the reference machine's (size + CRC-32); writes results.txt / results.tsv
   Memories live in the folder "memory" next to the program. Archive = 1 byte (class << 2 | act) + codec stream. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fluid_codec.c"
#include "router_law.h"
#include "teach_law.h"
#include "fluid_tables.h"
#include "plat.h"

#define ZCAP(n) ((n) + (n) / 8 + 128)
static const char *CL[6] = {"none", "logs", "chat", "emb", "lp", "text"};
static char HOME[2048];
typedef struct { uint8_t *b; size_t n; int ok; } Blob;
static Blob REAL[6], LES[6], BOTH[6];

static void *xalloc(size_t n) { void *p = malloc(n ? n : 1); if (!p) { fprintf(stderr, "fluid: out of memory (%zu bytes)\n", n); exit(2); } return p; }
static uint8_t *slurp(const char *path, size_t *n) {           /* NULL when the file cannot be opened */
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    long z = -1; if (fseek(f, 0, SEEK_END) == 0) z = ftell(f);
    if (z < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); fprintf(stderr, "fluid: cannot size %s\n", path); exit(1); }
    uint8_t *b = xalloc((size_t)z + 1);
    if (z && fread(b, 1, (size_t)z, f) != (size_t)z) { fclose(f); fprintf(stderr, "fluid: cannot read %s\n", path); exit(1); }
    fclose(f); *n = (size_t)z; return b;
}
static int spit(const char *path, const uint8_t *b, size_t n) {
    FILE *f = fopen(path, "wb"); if (!f) { perror(path); return 0; }
    int ok = fwrite(b, 1, n, f) == n; if (fclose(f)) ok = 0;
    if (!ok) fprintf(stderr, "fluid: cannot write %s\n", path);
    return ok;
}
static void memories_load(void) {                             /* every memory once, before any thread starts */
    char p[2300];
    for (int c = 1; c < 6; c++) {
        snprintf(p, sizeof p, "%s/memory/real_%s", HOME, CL[c]);
        REAL[c].b = slurp(p, &REAL[c].n); REAL[c].ok = REAL[c].b != NULL;
        if (!REAL[c].ok) { fprintf(stderr, "fluid: missing memory file %s\n", p); exit(1); }
        snprintf(p, sizeof p, "%s/memory/lesson_%s", HOME, CL[c]);
        LES[c].b = slurp(p, &LES[c].n); LES[c].ok = LES[c].b != NULL;
        size_t ln = LES[c].ok ? LES[c].n : 0;
        BOTH[c].n = ln + REAL[c].n; BOTH[c].b = xalloc(BOTH[c].n); BOTH[c].ok = 1;
        if (ln) memcpy(BOTH[c].b, LES[c].b, ln);
        memcpy(BOTH[c].b + ln, REAL[c].b, REAL[c].n);
    }
}
static const uint8_t *memory(int cls, int act, size_t *ml) {  /* memory(): {1: les or None, 2: real, 3: les + real} */
    *ml = 0;
    if (cls <= 0 || act == 0) return NULL;
    if (act == 1) { if (!LES[cls].ok || !LES[cls].n) return NULL; *ml = LES[cls].n; return LES[cls].b; }
    if (act == 2) { *ml = REAL[cls].n; return REAL[cls].b; }
    *ml = BOTH[cls].n; return BOTH[cls].b;
}
static int q3(double v, double a, double b, double c) { return v < a ? 0 : v < b ? 1 : v < c ? 2 : 3; }
static int32_t eyes(const uint8_t *b, size_t len) {          /* eyes(): 8 features of the first 64 KB, 2 bits each */
    size_t m = len < 65536 ? len : 65536; uint64_t cnt[256], z0 = 0, s;
    memset(cnt, 0, sizeof cnt);
    for (size_t i = 0; i < m; i++) { cnt[b[i]]++; if (b[i] == '0' && i + 1 < m && b[i + 1] == '.') z0++; }
    double n = (double)(m ? m : 1);
    s = 0; for (int c = 128; c < 256; c++) s += cnt[c]; double hi = (double)s / n;
    double zero = (double)cnt[0] / n;
    s = 0; for (int c = '0'; c <= '9'; c++) s += cnt[c]; double dig = (double)s / n;
    double js = (double)(cnt['"'] + cnt['{'] + cnt['}'] + cnt[':']) / n;
    double mark = (double)(cnt['['] + cnt[']'] + cnt['<'] + cnt['>'] + cnt['|'] + cnt['=']) / n;
    s = 0; for (int c = 'A'; c <= 'Z'; c++) s += cnt[c]; for (int c = 'a'; c <= 'z'; c++) s += cnt[c]; double alpha = (double)s / n;
    double nl = (double)(cnt[10] * 1024) / n, flt = (double)(z0 * 1024) / n;
    int f[8] = {q3(hi, 0.01, 0.1, 0.3), q3(zero, 1e-9, 0.01, 0.1), q3(dig, 0.05, 0.15, 0.3), q3(js, 0.02, 0.05, 0.1),
                q3(mark, 0.01, 0.03, 0.06), q3(alpha, 0.3, 0.5, 0.7), q3(nl, 2, 8, 20), q3(flt, 1, 10, 50)};
    int32_t sig = 0; for (int i = 0; i < 8; i++) sig += (int32_t)f[i] << (2 * i);
    return sig;
}
static int in_seen(int32_t s) { int lo = 0, hi = F_NSEEN - 1; while (lo <= hi) { int md = (lo + hi) / 2; if (F_SEEN[md] == s) return 1; if (F_SEEN[md] < s) lo = md + 1; else hi = md - 1; } return 0; }
static int sb(size_t n) { return n <= 65536 ? 0 : n <= 262144 ? 1 : n <= 1048576 ? 2 : 3; }
static int teach_act(int cls, size_t n) {
    if (cls == 0) return 0;
    int a = (int)L_TEACH((int32_t)(F_GKL[cls] | F_GKR[cls] << 2 | sb(n) << 4));
    if (!LES[cls].ok) a &= 2;                                 /* no lesson exists: drop its bit */
    return a;
}
typedef struct { int32_t sig; int cls, act, measured; } Route;
static size_t fl_compress(const uint8_t *in, size_t n, uint8_t *out, Route *r) {   /* out: ZCAP(n) + 1 bytes */
    int32_t sig = eyes(in, n); int cls = in_seen(sig) ? (int)L_ROUTE(sig) : -1, act; size_t ml, z;
    r->sig = sig; r->measured = 0;
    if (cls < 0) {                                            /* abstain: measure each memory, keep the smallest (first wins ties) */
        r->measured = 1;
        uint8_t *bufs[2] = {out, xalloc(ZCAP(n) + 1)}, *best = NULL; int w = 0, bc = 0, ba = 0; size_t bz = 0;
        for (int c = 0; c < 6; c++) {
            int a = teach_act(c, n); const uint8_t *m = memory(c, a, &ml);
            size_t s = fluid_compress(m, ml, in, n, bufs[w] + 1);
            if (!best || s < bz) { best = bufs[w]; bz = s; bc = c; ba = a; w ^= 1; }
        }
        cls = bc; act = ba; z = bz;
        if (best != out) memcpy(out + 1, best + 1, z);
        free(bufs[1]);
    } else {
        act = teach_act(cls, n); const uint8_t *m = memory(cls, act, &ml);
        z = fluid_compress(m, ml, in, n, out + 1);
    }
    out[0] = (uint8_t)(cls << 2 | act); r->cls = cls; r->act = act;
    return z + 1;
}
static uint8_t *fl_decompress(const uint8_t *a, size_t len, size_t *outn) {
    if (len < 1 + FHDR || (a[0] >> 2) > 5) return NULL;
    size_t ml; const uint8_t *m = memory(a[0] >> 2, a[0] & 3, &ml);
    size_t cap = get32(a + 1 + 5); uint8_t *out = xalloc(cap + 1);
    size_t r = fluid_decompress(m, ml, a + 1, len - 1, out, cap);
    if (r == (size_t)-1) { free(out); return NULL; }
    *outn = r; return out;
}

/* ------------------------------------------------------------ edge test ------------------------------------------------ */
typedef struct {
    char name[256], path[2400]; size_t bytes, mac_z; uint32_t mac_crc; double mac_tc, mac_td;
    size_t n, z; uint32_t crc; double tc, td; int err, exact, same; Route r;
} Job;
static Job *JOBS; static int NJOBS, NEXT, NDONE; static char LOCKB; static FILE *RES;
static void lock_(void) { while (__atomic_test_and_set(&LOCKB, __ATOMIC_ACQUIRE)) ; }
static void unlock_(void) { __atomic_clear(&LOCKB, __ATOMIC_RELEASE); }
static void emit(const char *s) { fputs(s, stdout); fflush(stdout); if (RES) { fputs(s, RES); fflush(RES); } }
static double num(const char *s) {                           /* plain decimal: digits [. digits] */
    double v = 0, f = 0.1; if (!s) return 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    if (*s == '.') for (s++; *s >= '0' && *s <= '9'; s++, f /= 10) v += (*s - '0') * f;
    return v;
}
static void line(const Job *J) {
    char b[600], rt[40];
    if (J->err) { snprintf(b, sizeof b, "%3d  %-26s  cannot read %s\n", NDONE, J->name, J->path); emit(b); return; }
    snprintf(rt, sizeof rt, "%s/%c/%s", CL[J->r.cls], "-LRB"[J->r.act], J->r.measured ? "measured" : "route");
    snprintf(b, sizeof b, "%3d  %-26s %9zu %9zu  %-16s %8.2f %7.2f  %-5s  %s\n", NDONE, J->name, J->n, J->z, rt, J->tc, J->td,
             J->exact ? "yes" : "NO", !J->mac_z ? "-" : J->same ? "yes" : J->bytes != J->n ? "NO (input differs)" : "NO");
    emit(b);
}
static void worker(void *unused) {
    (void)unused;
    for (;;) {
        int j = __atomic_fetch_add(&NEXT, 1, __ATOMIC_SEQ_CST); if (j >= NJOBS) return;
        Job *J = &JOBS[j]; size_t n = 0; uint8_t *in = slurp(J->path, &n);
        if (!in) J->err = 1;
        else {
            uint8_t *z = xalloc(ZCAP(n) + 1); size_t bn = 0;
            double t0 = plat_now(); J->z = fl_compress(in, n, z, &J->r); double t1 = plat_now();
            uint8_t *back = fl_decompress(z, J->z, &bn); double t2 = plat_now();
            J->n = n; J->tc = t1 - t0; J->td = t2 - t1;
            J->exact = back && bn == n && (!n || !memcmp(back, in, n));
            J->crc = crc32(z, J->z); J->same = J->mac_z && J->z == J->mac_z && J->crc == J->mac_crc;
            free(back); free(z); free(in);
        }
        lock_(); NDONE++; line(J); unlock_();
    }
}
static int cmd_t_parse(const char *list, char *dir, size_t dcap) {   /* LIST -> JOBS; dir = the list's folder */
    size_t ln; char *L = (char *)slurp(list, &ln); if (!L) { perror(list); return 1; }
    L[ln] = 0;
    snprintf(dir, dcap, "%s", list);
    { char *a = strrchr(dir, '\\'), *b = strrchr(dir, '/'), *c = a > b ? a : b; if (c) *c = 0; else snprintf(dir, dcap, "."); }
    int cap = 1; for (size_t i = 0; i < ln; i++) cap += L[i] == '\n';
    JOBS = calloc((size_t)cap, sizeof(Job)); if (!JOBS) return 2;
    for (char *p = L; p && *p; ) {
        char *e = strchr(p, '\n'); if (e) *e = 0;
        size_t k = strlen(p); if (k && p[k - 1] == '\r') p[--k] = 0;
        if (k && p[0] != '#') {
            Job *J = &JOBS[NJOBS++]; char *f[6] = {p, 0, 0, 0, 0, 0}; int nf = 1;
            for (char *q = p; *q && nf < 6; q++) if (*q == '\t') { *q = 0; f[nf++] = q + 1; }
            snprintf(J->name, sizeof J->name, "%s", f[0]); snprintf(J->path, sizeof J->path, "%s/%s", dir, f[0]);
            J->bytes = (size_t)num(f[1]); J->mac_z = (size_t)num(f[2]); J->mac_crc = f[3] ? (uint32_t)strtoul(f[3], NULL, 16) : 0;
            J->mac_tc = num(f[4]); J->mac_td = num(f[5]);
        }
        p = e ? e + 1 : NULL;
    }
    return 0;
}
static int cmd_t(const char *list, int P) {
    char dir[2048]; if (cmd_t_parse(list, dir, sizeof dir)) return 1;
    if (P < 1) P = 1;
    char mach[512], b[1200], rp[2400]; plat_machine(mach, sizeof mach);
    snprintf(rp, sizeof rp, "%s/results.txt", dir); RES = fopen(rp, "wb");
    snprintf(b, sizeof b, "FluidCompressor edge test\nmachine: %s\nfiles: %d from %s, %d thread%s\n\n  #  %-26s %9s %9s  %-16s %8s %7s  %-5s  %s\n",
             mach, NJOBS, list, P, P > 1 ? "s" : "", "file", "bytes", "archive", "route", "c s", "d s", "exact", "same as Mac");
    emit(b);
    if (!CRC_T[1]) { crc_init(); tables_init(); }
    (void)q24_form(); memories_load();
    double w0 = plat_now(); plat_run(P, worker, NULL); double wall = plat_now() - w0;
    size_t tb = 0, tz = 0; double tc = 0, td = 0, mc = 0, md = 0; int ex = 0, sm = 0, ok = 0, hasmac = 0;
    for (int j = 0; j < NJOBS; j++) {
        Job *J = &JOBS[j]; if (J->err) continue;
        ok++; tb += J->n; tz += J->z; tc += J->tc; td += J->td; ex += J->exact; sm += J->same;
        if (J->mac_tc > 0) { mc += J->mac_tc; md += J->mac_td; hasmac = 1; }
    }
    snprintf(b, sizeof b, "\ntotal: %d files, %zu bytes -> %zu (%.2fx) | compress %.1f s, decompress %.1f s (summed) | wall %.1f s on %d thread%s\n"
                          "round trips exact: %d of %d | archives byte-identical to the Mac: %d of %d | peak memory %.0f MB\n",
             ok, tb, tz, tz ? (double)tb / (double)tz : 0.0, tc, td, wall, P, P > 1 ? "s" : "", ex, NJOBS, sm, NJOBS, (double)plat_peak() / 1048576.0);
    emit(b);
    if (hasmac) { snprintf(b, sizeof b, "Mac M4 Pro, 1 thread: compress %.1f s, decompress %.1f s -> this machine takes %.2fx / %.2fx as long per file\n", mc, md, mc > 0 ? tc / mc : 0.0, md > 0 ? td / md : 0.0); emit(b); }
    if (RES) fclose(RES);
    snprintf(rp, sizeof rp, "%s/results.tsv", dir); FILE *t = fopen(rp, "wb");
    if (t) {
        for (int j = 0; j < NJOBS; j++) { Job *J = &JOBS[j]; if (!J->err) fprintf(t, "%s\t%zu\t%zu\t%08x\t%.3f\t%.3f\t%d\t%d\t%d\t%d\t%d\n", J->name, J->n, J->z, J->crc, J->tc, J->td, J->r.cls, J->r.act, J->r.measured, J->exact, J->same); }
        fclose(t);
    }
    return ex == NJOBS && ok == NJOBS ? 0 : 1;
}
/* ------------------------------------------------------------ the hive ------------------------------------------------- */
/* Warm each memory once and give every file its own copy of the warmed model: the thread version of the fork hive
   (Windows has no fork).  A copy is a few hundred MB of memcpy, about 0.1 s; a replay is seconds.  The warm state
   depends only on the memory bytes and the table size (lg of memory + file), so files are grouped on exactly that and
   every archive is byte-identical to fl_compress's. */
static Model *model_clone(const Model *s, const uint8_t *buf) {
    Model *m = xalloc(sizeof(Model)); memcpy(m, s, sizeof(Model));
    size_t h = ((size_t)1 << HB) * 4, f = ((size_t)1 << 20) * 4;
    m->H = xalloc(h); memcpy(m->H, s->H, h);
    m->FR = xalloc(f); memcpy(m->FR, s->FR, f);
    m->L3 = xalloc(f); memcpy(m->L3, s->L3, f);
    m->buf = buf; return m;
}
static XModel *xmodel_clone(const XModel *s, const uint8_t *buf) {
    XModel *x = xalloc(sizeof(XModel)); memcpy(x, s, sizeof(XModel));
    x->m = model_clone(s->m, buf);
    for (int j = 0; j < XC; j++) {
        size_t sz = ((size_t)1 << (32 - s->shift[j])) * sizeof(XB);
        x->T[j] = xalloc(sz); memcpy(x->T[j], s->T[j], sz);
        x->b[j] = (s->b[j] == NULL || s->b[j] == &XB_OFF) ? s->b[j] : x->T[j] + (s->b[j] - s->T[j]);
    }
    size_t a = (size_t)65536 * 33 * 2, a3 = (size_t)4096 * 33 * 2;
    x->A2 = xalloc(a); memcpy(x->A2, s->A2, a);
    x->A1 = xalloc(a); memcpy(x->A1, s->A1, a);
    x->A3 = xalloc(a3); memcpy(x->A3, s->A3, a3);
    for (int r = 0; r < XR; r++) {
        size_t sz = ((size_t)1 << (32 - s->rshift[r])) * 2;
        x->RT[r] = xalloc(sz); memcpy(x->RT[r], s->RT[r], sz);
        x->rcur[r] = s->rcur[r] ? x->RT[r] + (s->rcur[r] - s->RT[r]) : NULL;
    }
    if (s->LT) { size_t sz = ((size_t)1 << s->ltb) * 4; x->LT = xalloc(sz); memcpy(x->LT, s->LT, sz); }
    return x;
}
static int lg_of(size_t n) { int lg = 10; while (lg < 31 && ((size_t)1 << lg) < n) lg++; return lg; }   /* as xmodel_new */
static XModel *hive_warm(const uint8_t *mem, size_t ml, size_t total, uint8_t **bufp) {
    uint8_t *buf = xalloc(ml); memcpy(buf, mem, ml);
    XModel *x = xmodel_new(buf, total);                      /* total = memory + file: fixes the table size */
    xgather(x); xcontexts(x); fluid_replay(x, buf, ml);
    *bufp = buf; return x;
}
static int same_shape(const XModel *x, const XModel *s) {      /* same table sizes: s can be copied into x's memory */
    for (int j = 0; j < XC; j++) if (x->shift[j] != s->shift[j]) return 0;
    for (int r = 0; r < XR; r++) if (x->rshift[r] != s->rshift[r]) return 0;
    return (x->LT == NULL) == (s->LT == NULL) && (!s->LT || x->ltb == s->ltb);
}
static void xmodel_copy_into(XModel *x, const XModel *s) {     /* copy into memory this thread already owns: no fresh pages */
    Model *m = x->m; uint32_t *H = m->H, *FR = m->FR, *L3 = m->L3, *LT = x->LT;
    XB *T[XC]; uint16_t *RT[XR], *A2 = x->A2, *A1 = x->A1, *A3 = x->A3;
    memcpy(T, x->T, sizeof T); memcpy(RT, x->RT, sizeof RT);
    memcpy(x, s, sizeof(XModel));
    x->m = m; memcpy(m, s->m, sizeof(Model)); m->H = H; m->FR = FR; m->L3 = L3;
    memcpy(H, s->m->H, ((size_t)1 << HB) * 4); memcpy(FR, s->m->FR, ((size_t)1 << 20) * 4); memcpy(L3, s->m->L3, ((size_t)1 << 20) * 4);
    for (int j = 0; j < XC; j++) {
        x->T[j] = T[j]; memcpy(T[j], s->T[j], ((size_t)1 << (32 - s->shift[j])) * sizeof(XB));
        x->b[j] = (s->b[j] == NULL || s->b[j] == &XB_OFF) ? s->b[j] : T[j] + (s->b[j] - s->T[j]);
    }
    x->A2 = A2; x->A1 = A1; x->A3 = A3;
    memcpy(A2, s->A2, (size_t)65536 * 33 * 2); memcpy(A1, s->A1, (size_t)65536 * 33 * 2); memcpy(A3, s->A3, (size_t)4096 * 33 * 2);
    for (int r = 0; r < XR; r++) {
        x->RT[r] = RT[r]; memcpy(RT[r], s->RT[r], ((size_t)1 << (32 - s->rshift[r])) * 2);
        x->rcur[r] = s->rcur[r] ? RT[r] + (s->rcur[r] - s->RT[r]) : NULL;
    }
    x->LT = LT; if (s->LT) memcpy(LT, s->LT, ((size_t)1 << s->ltb) * 4);
}
/* code / decode one file in a model already warmed on its memory; bw = memory bytes, then the file (= fluid_compress tail) */
static size_t code_piece(XModel *x, const uint8_t *bw, size_t ml, uint32_t mcrc, const uint8_t *in, size_t n, uint8_t *out) {
    memcpy(out, "FLD0", 4); out[4] = 2; put32(out + 5, (uint32_t)n); put32(out + 9, crc32(in, n));
    put32(out + 13, mcrc); put32(out + 17, (uint32_t)ml);
    AC a = {0, 0xFFFFFFFFu, 0, out + FHDR, out + FHDR + n + 8};
    for (size_t i = ml; i < ml + n && a.p < a.e; i++) { xcode_byte(x, &a, bw[i], 1); xupdate(x, bw[i]); }
    for (int k = 0; k < 4; k++) { if (a.p < a.e) *a.p = (uint8_t)(a.x1 >> 24); a.p++; a.x1 <<= 8; }
    size_t z = (size_t)(a.p - (out + FHDR));
    if (z < n) return FHDR + z;
    out[4] = 0; memcpy(out + FHDR, in, n); return FHDR + n;
}
static size_t decode_piece(XModel *x, uint8_t *bw, size_t ml, const uint8_t *in, size_t len, uint8_t *out, size_t cap) {
    if (len < FHDR || memcmp(in, "FLD0", 4)) return (size_t)-1;                                       /* = fluid_decompress */
    size_t n = get32(in + 5);
    if (n > cap || get32(in + 17) != (uint32_t)ml) return (size_t)-1;
    if (in[4] == 0) { memcpy(out, in + FHDR, n); return crc32(out, n) == get32(in + 9) ? n : (size_t)-1; }
    AC a = {0, 0xFFFFFFFFu, 0, (uint8_t *)in + FHDR, (uint8_t *)in + len};
    for (int k = 0; k < 4; k++) a.x = (a.x << 8) | (a.p < a.e ? *a.p++ : 0);
    for (size_t i = ml; i < ml + n; i++) { int c = xcode_byte(x, &a, 0, 0); bw[i] = (uint8_t)c; xupdate(x, c); }
    memcpy(out, bw + ml, n);
    return crc32(out, n) == get32(in + 9) ? n : (size_t)-1;
}
typedef struct { int file, c, act, lg; size_t ml; const uint8_t *mem; uint32_t mcrc; } HTask;
typedef struct { int t0, nt; double work; } HUnit;            /* tasks [t0, t0 + nt) share one warmed model (or none) */
typedef struct { size_t z; int c, act, have; uint8_t *buf; double tcode; } HBest;
static HTask *HTK; static int NHT, HTCAP; static HUnit *HU; static int NHU, HNEXT, HPHASE, WARMS, UDONE; static uint8_t **INB; static HBest *BEST;
static uint32_t MCRC[6][4];                                    /* CRC-32 of each memory, once */
static void ht_add(int f, int c, int act) {
    if (NHT == HTCAP) { HTCAP = HTCAP ? 2 * HTCAP : 256; HTK = realloc(HTK, (size_t)HTCAP * sizeof(HTask)); if (!HTK) exit(2); }
    HTask *T = &HTK[NHT++]; size_t n = JOBS[f].n;
    T->file = f; T->c = c; T->act = act; T->mem = memory(c, act, &T->ml);
    T->lg = T->ml && n ? lg_of(T->ml + n) : 0;               /* lg 0: no warm model (no memory, or an empty file) */
    T->mcrc = T->ml ? MCRC[c][act] : 0;
}
static int ht_key_less(const HTask *a, const HTask *b) {       /* group order: warmed tasks by (class, act, lg) */
    if (!a->lg != !b->lg) return a->lg != 0;
    if (a->c != b->c) return a->c < b->c;
    if (a->act != b->act) return a->act < b->act;
    if (a->lg != b->lg) return a->lg < b->lg;
    return a->file < b->file;
}
static void hu_build(void) {
    for (int i = 1; i < NHT; i++) { HTask t = HTK[i]; int j = i - 1; while (j >= 0 && ht_key_less(&t, &HTK[j])) { HTK[j + 1] = HTK[j]; j--; } HTK[j + 1] = t; }
    free(HU); HU = xalloc(sizeof(HUnit) * (size_t)(NHT + 1)); NHU = 0;
    for (int i = 0; i < NHT; ) {
        int j = i + 1;
        if (HTK[i].lg) while (j < NHT && HTK[j].lg == HTK[i].lg && HTK[j].c == HTK[i].c && HTK[j].act == HTK[i].act) j++;
        HUnit *U = &HU[NHU++]; U->t0 = i; U->nt = j - i; U->work = HTK[i].lg ? (double)HTK[i].ml : 0;
        for (int k = i; k < j; k++) U->work += (double)JOBS[HTK[k].file].n;
        i = j;
    }
    for (int i = 1; i < NHU; i++) { HUnit u = HU[i]; int j = i - 1; while (j >= 0 && u.work > HU[j].work) { HU[j + 1] = HU[j]; j--; } HU[j + 1] = u; }
    HNEXT = 0;
}
static void hive_worker(void *unused) {
    (void)unused;
    XModel *slot = NULL; uint8_t *bw = NULL; size_t bwcap = 0;   /* this thread's working copy and buffer, reused for every file */
    for (;;) {
        int k = __atomic_fetch_add(&HNEXT, 1, __ATOMIC_SEQ_CST); if (k >= NHU) break;
        HUnit *U = &HU[k]; HTask *T0 = &HTK[U->t0]; XModel *S = NULL; uint8_t *sbuf = NULL; double u0 = plat_now(), tw = 0;
        if (T0->lg) { S = hive_warm(T0->mem, T0->ml, T0->ml + JOBS[T0->file].n, &sbuf); __atomic_fetch_add(&WARMS, 1, __ATOMIC_SEQ_CST); tw = plat_now() - u0; }
        for (int i = 0; i < U->nt; i++) {
            HTask *T = &HTK[U->t0 + i]; Job *J = &JOBS[T->file]; size_t n = J->n; XModel *x = NULL;
            double t0 = plat_now();
            if (S) {                                          /* memory, then the file (or room for it when decoding) */
                size_t need = T->ml + (HPHASE == 0 ? n : (size_t)get32(BEST[T->file].buf + 1 + 5));
                if (need + 1 > bwcap) { free(bw); bwcap = need + 1; bw = xalloc(bwcap); }
                memcpy(bw, T->mem, T->ml);
                if (HPHASE == 0) memcpy(bw + T->ml, INB[T->file], n);
                if (i == U->nt - 1) x = S;                    /* the group's last file takes the warmed model itself */
                else {
                    if (slot && !same_shape(slot, S)) { xmodel_free(slot); slot = NULL; }
                    if (!slot) slot = xmodel_clone(S, bw); else xmodel_copy_into(slot, S);
                    x = slot;
                }
                x->m->buf = bw;
            }
            if (HPHASE == 0) {
                uint8_t *out = xalloc(ZCAP(n) + 1);
                size_t z = x ? code_piece(x, bw, T->ml, T->mcrc, INB[T->file], n, out + 1) : fluid_compress(T->mem, T->ml, INB[T->file], n, out + 1);
                double t1 = plat_now(); out[0] = (uint8_t)(T->c << 2 | T->act);
                lock_(); HBest *B = &BEST[T->file]; B->tcode += t1 - t0;
                if (!B->have || z < B->z || (z == B->z && T->c < B->c)) { free(B->buf); B->buf = out; B->z = z; B->c = T->c; B->act = T->act; B->have = 1; out = NULL; }
                unlock_(); free(out);
            } else {
                HBest *B = &BEST[T->file]; size_t cap = get32(B->buf + 1 + 5); uint8_t *out = xalloc(cap + 1);
                size_t r = x ? decode_piece(x, bw, T->ml, B->buf + 1, B->z, out, cap) : fluid_decompress(T->mem, T->ml, B->buf + 1, B->z, out, cap);
                double t1 = plat_now(); int ok = r == n && (!n || !memcmp(out, INB[T->file], n)); free(out);
                lock_(); J->exact = ok; J->td = t1 - t0; unlock_();
            }
        }
        if (S) { xmodel_free(S); free(sbuf); }
        lock_(); UDONE++;                                     /* progress, so a long run never looks frozen */
        if (T0->lg) {
            char pb[300];
            snprintf(pb, sizeof pb, "  %s %2d/%d  memory %s/%c, %d file%s: warm-up %.1f s, all %.1f s\n", HPHASE ? "decode" : "encode", UDONE, NHU,
                     CL[T0->c], "-LRB"[T0->act], U->nt, U->nt > 1 ? "s" : "", tw, plat_now() - u0);
            emit(pb);
        }
        unlock_();
    }
    if (slot) xmodel_free(slot);
    free(bw);
}
static int cmd_h(const char *list, int P) {
    if (P < 1) P = 1;
    if (!CRC_T[1]) { crc_init(); tables_init(); }
    (void)q24_form(); memories_load();
    for (int c = 1; c < 6; c++) for (int a = 1; a < 4; a++) { size_t ml; const uint8_t *m = memory(c, a, &ml); MCRC[c][a] = ml ? crc32(m, ml) : 0; }
    char dir[2048], mach[512], b[1200], rp[2400];
    if (cmd_t_parse(list, dir, sizeof dir)) return 1;
    plat_machine(mach, sizeof mach);
    snprintf(rp, sizeof rp, "%s/results_hive.txt", dir); RES = fopen(rp, "wb");
    snprintf(b, sizeof b, "FluidCompressor edge test, HIVE (each memory warmed once, every file a copy)\nmachine: %s\nfiles: %d from %s, %d thread%s\n\n",
             mach, NJOBS, list, P, P > 1 ? "s" : "");
    emit(b);
    INB = xalloc(sizeof(uint8_t *) * (size_t)NJOBS); BEST = calloc((size_t)NJOBS, sizeof(HBest)); if (!BEST) return 2;
    double w0 = plat_now();
    for (int f = 0; f < NJOBS; f++) {                        /* eyes + router law + teach law for every file, up front */
        Job *J = &JOBS[f]; INB[f] = slurp(J->path, &J->n);
        if (!INB[f]) { J->err = 1; INB[f] = xalloc(1); J->n = 0; continue; }
        J->r.sig = eyes(INB[f], J->n); int cls = in_seen(J->r.sig) ? (int)L_ROUTE(J->r.sig) : -1;
        if (cls >= 0) { J->r.measured = 0; ht_add(f, cls, teach_act(cls, J->n)); }
        else { J->r.measured = 1; for (int c = 0; c < 6; c++) ht_add(f, c, teach_act(c, J->n)); }
    }
    int tasks1 = NHT; hu_build(); int units1 = NHU; HPHASE = 0; UDONE = 0;
    snprintf(b, sizeof b, "encoding: %d codings in %d groups (one warm-up per memory group)\n", tasks1, units1); emit(b);
    plat_run(P, hive_worker, NULL);
    double w1 = plat_now(); int warms1 = WARMS;
    NHT = 0;                                                 /* decode: one task per file, under the memory it chose */
    for (int f = 0; f < NJOBS; f++) if (!JOBS[f].err) { HBest *B = &BEST[f]; JOBS[f].r.cls = B->c; JOBS[f].r.act = B->act; ht_add(f, B->c, B->act); }
    hu_build(); HPHASE = 1; WARMS = 0; UDONE = 0;
    snprintf(b, sizeof b, "decoding: %d files in %d groups\n", NHT, NHU); emit(b);
    plat_run(P, hive_worker, NULL);
    double w2 = plat_now();
    snprintf(b, sizeof b, "  #  %-26s %9s %9s  %-16s %8s %7s  %-5s  %s\n", "file", "bytes", "archive", "route", "code s", "d s", "exact", "same as Mac"); emit(b);
    size_t tb = 0, tz = 0; int ex = 0, sm = 0, ok = 0;
    for (int f = 0; f < NJOBS; f++) {
        Job *J = &JOBS[f]; NDONE = f + 1;
        if (J->err) { line(J); continue; }
        HBest *B = &BEST[f]; J->z = B->z + 1; J->crc = crc32(B->buf, J->z); J->tc = B->tcode;
        J->same = J->mac_z && J->z == J->mac_z && J->crc == J->mac_crc;
        line(J); ok++; tb += J->n; tz += J->z; ex += J->exact; sm += J->same;
    }
    snprintf(b, sizeof b, "\ntotal: %d files, %zu bytes -> %zu (%.2fx)\nhive compress: %.1f s wall (%d warm-ups for %d codings) | decompress: %.1f s wall (%d warm-ups) | %d thread%s\n"
                          "round trips exact: %d of %d | archives byte-identical to the Mac: %d of %d | peak memory %.0f MB\n",
             ok, tb, tz, tz ? (double)tb / (double)tz : 0.0, w1 - w0, warms1, tasks1, w2 - w1, WARMS, P, P > 1 ? "s" : "", ex, NJOBS, sm, NJOBS, (double)plat_peak() / 1048576.0);
    emit(b); (void)units1;
    if (RES) fclose(RES);
    return ex == NJOBS && ok == NJOBS ? 0 : 1;
}
int main(int argc, char **argv) {
    plat_home(argc > 0 ? argv[0] : ".", HOME, sizeof HOME);
    if (argc >= 3 && !strcmp(argv[1], "t")) return cmd_t(argv[2], argc > 3 ? atoi(argv[3]) : 1);
    if (argc >= 3 && !strcmp(argv[1], "h")) return cmd_h(argv[2], argc > 3 ? atoi(argv[3]) : 1);
    if (argc < 4 || (strcmp(argv[1], "c") && strcmp(argv[1], "d"))) {
        fprintf(stderr, "fluid c IN OUT | fluid d IN OUT | fluid t LIST [threads] | fluid h LIST [threads]  (h = hive)\n"); return 2;
    }
    if (!CRC_T[1]) { crc_init(); tables_init(); }
    memories_load();
    size_t n; uint8_t *in = slurp(argv[2], &n); if (!in) { perror(argv[2]); return 1; }
    if (argv[1][0] == 'c') {
        uint8_t *z = xalloc(ZCAP(n) + 1); Route r; size_t zl = fl_compress(in, n, z, &r);
        if (!spit(argv[3], z, zl)) return 1;
        printf("%s -> %zu bytes  %s/%c/%s\n", argv[2], zl, CL[r.cls], "-LRB"[r.act], r.measured ? "measured" : "route");
        return 0;
    }
    size_t bn; uint8_t *back = fl_decompress(in, n, &bn);
    if (!back) { fprintf(stderr, "fluid: decode failed\n"); return 3; }
    return spit(argv[3], back, bn) ? 0 : 1;
}
