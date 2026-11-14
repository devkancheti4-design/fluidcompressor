"""parse_ltcb.py -- the main table of the Large Text Compression Benchmark page (mattmahoney.net/dc/text.html) as rows:
{name, e8, e9, dec, total, rest}. The main table is the page's first <pre><ol> block, one <li> per entry; total is the
last size of 10,000,000 or more on the line (compressed enwik9 plus the zipped decompressor), name is the text before
the first size, rest is what follows the total (times per byte, memory, algorithm, note).
    python3 tools/parse_ltcb.py PAGE.html OUT.json"""
import html, json, re, sys
def rows(page):
    block = page[page.index('<pre><ol>'):]
    block = block[:block.index('</ol>')]
    out = []
    for li in block.split('<li>')[1:]:
        text = html.unescape(re.sub(r'<[^>]+>', '', li)).rstrip('\n')
        sizes = [(m.start(), m.end(), int(m.group().replace(',', ''))) for m in re.finditer(r'\b\d{1,3}(?:,\d{3})+\b', text)]
        big = [s for s in sizes if s[2] >= 10_000_000]
        if not big: continue
        name = ' '.join(text[:sizes[0][0]].split())
        tot = big[-1]; e9 = big[-2][2] if len(big) >= 2 else None
        e8 = big[-3][2] if len(big) >= 3 else None
        mid = text[(big[-2][1] if len(big) >= 2 else sizes[0][1]):tot[0]].split()
        dec = int(mid[0].replace(',', '')) if mid and re.fullmatch(r'[\d,]+', mid[0]) else None
        out.append(dict(name=name, e8=e8, e9=e9, dec=dec, total=tot[2], rest=' '.join(text[tot[1]:].split())))
    return out
if __name__ == '__main__':
    r = rows(open(sys.argv[1], encoding='latin-1').read())
    json.dump(r, open(sys.argv[2], 'w'), indent=1); print(len(r), 'rows')
