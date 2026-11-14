"""make_mac_files.py -- cut the 6 sealed-test files that come from macOS system files (1 MB from the middle of each).
They are not redistributed. Run on a Mac; the sizes and CRCs in tests_all.txt match only the macOS build they were cut
on (Darwin 25.5.0), since system binaries and fonts change between releases."""
import os
HERE = os.path.dirname(os.path.abspath(__file__))
for p in ('/bin/bash', '/bin/zsh', '/bin/ksh', '/System/Library/Fonts/Helvetica.ttc', '/System/Library/Fonts/Supplemental/Arial.ttf', '/usr/share/dict/web2'):
    sz = os.path.getsize(p); key = ''.join(ch if ch.isalnum() else '_' for ch in os.path.basename(p))
    with open(p, 'rb') as f: f.seek(max(0, sz // 2 - (1 << 19))); data = f.read(1 << 20)
    open(os.path.join(HERE, 'test', key), 'wb').write(data); print(key, len(data))
