"""Print every program of the given chapter files with its real output, for checking prose against behaviour.

    python book/review.py 06            (chapter file prefix)
"""
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_book as bb

cache = bb.load_cache()
for pref in sys.argv[1:]:
    for path in sorted(glob.glob(os.path.join(bb.HERE, 'src', pref + '*.txt'))):
        title, blocks = bb.parse_chapter(path)
        print('=' * 70, os.path.basename(path), title)
        for b in blocks:
            if b[0] != 'block':
                continue
            kind, body = b[1], b[2]
            k = kind.split(' ')[0]
            stdin = ''
            if ' stdin=' in kind:
                stdin = kind.split('stdin=', 1)[1].replace(chr(92) + 'n', chr(10)) + chr(10)
            if k in ('code', 'err'):
                r = bb.run_program(body, stdin, cache, False)
                first = body.strip().split(chr(10))
                print('--- %s (exit %s): %s' % (k, r['code'], ' / '.join(l for l in first[1:4])[:90]))
                print(r['out'].rstrip()[:700])
            elif k == 'cmd':
                pass
bb.json.dump(cache, open(bb.CACHE, 'w', encoding='utf-8'))
