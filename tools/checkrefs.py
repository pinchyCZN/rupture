"""Check every relative doc cross-reference resolves.

Refs are written relative to docs/ wherever they appear - so
formats/pe.txt and subsystems/music.txt read the same way from
docs/README.txt and from docs/effects/<part>.txt. CLAUDE.md rule 9.

    py -3.10 tools/checkrefs.py        from the repo root

Only a-z0-9_ names are matched, so a template placeholder written as
<name>.txt is deliberately invisible to this check.
"""
import io
import os
import sys
import re

DOCS = 'docs'

PAT = re.compile(r'(?<![\w/.])((?:\.\./)*'
                 r'(?:effects|subsystems|formats|structs)/[a-z0-9_]+\.txt)')


def main():
    bad, tot = [], 0
    for root, _dirs, files in os.walk(DOCS):
        for f in files:
            if not f.endswith('.txt'):
                continue
            p = os.path.join(root, f)
            s = io.open(p, encoding='ascii').read()
            for m in PAT.finditer(s):
                tot += 1
                r = m.group(1)
                # A ../ prefix is relative to the file; everything else
                # is relative to docs/.
                if r.startswith('../'):
                    t = os.path.normpath(os.path.join(root, r))
                else:
                    t = os.path.normpath(os.path.join(DOCS, r))
                if not os.path.exists(t):
                    bad.append((p, r, t))
    print('%d refs checked against %s/, %d unresolved' % (tot, DOCS, len(bad)))
    for p, r, t in bad:
        print('   %-44s %-32s -> %s' % (p, r, t))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
