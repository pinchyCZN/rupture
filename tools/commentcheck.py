#!/usr/bin/env python
"""Comment-anchor integrity for src/.

    py -3.10 tools/commentcheck.py            every finding
    py -3.10 tools/commentcheck.py --count    just the totals

journalcheck.py polices what a comment SAYS. This polices where it
SITS and whether it still points at anything.

Four failures, all of which accrue silently:

  orphaned      a block comment with nothing attached under it,
                because something was inserted between the comment
                and the thing it described
  stacked       two block comments in a row at file scope, which is
                how a second explanation gets bolted onto a first
                instead of replacing it
  dead-origin   an ORIGIN address that no function or global in
                reverse/exports/ASD-Rapture.exe/ carries any more, and that
                falls inside no named function either
  bare-address  a 0x00xxxxxx in a comment that is in no export, so a
                reader cannot check the claim

An ORIGIN address is the only thing in a comment that survives a
rename, so a stale one is worse than no comment at all - see
CLAUDE.md rule 18.
"""

import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..'))
SRC = os.path.join(ROOT, 'src')
EXPORTS = os.path.join(ROOT, 'reverse', 'exports', 'ASD-Rapture.exe')

SKIP_DIRS = ('third_party', 'build', '__pycache__', 'cgeffects')

ADDR = re.compile(r'0x00([0-9a-fA-F]{6})')


def known_addresses():
    """Every address the exported maps name, lower case, 6 hex digits."""
    out = set()
    if not os.path.isdir(EXPORTS):
        return None
    for name in os.listdir(EXPORTS):
        if not name.endswith('.txt'):
            continue
        p = os.path.join(EXPORTS, name)
        for line in io.open(p, encoding='ascii', errors='replace'):
            for m in ADDR.finditer(line):
                out.add(m.group(1).lower())
            # the maps also write addresses bare, as 002ca9e4
            for m in re.finditer(r'\b00([0-9a-fA-F]{6})\b', line):
                out.add(m.group(1).lower())
    return out


def known_ranges():
    """Every named function as (start, end), from function_map.txt.

    AN ADDRESS INSIDE A NAMED FUNCTION IS CHECKABLE. An ORIGIN often
    points at the one instruction that matters - a branch, a shift, a
    staged constant - rather than at the entry, and the function map
    carries a size for exactly this. Without it the only way to keep
    a precise ORIGIN was to blunt it back to the entry address, which
    is the opposite of what rule 18 wants.
    """
    out = []
    p = os.path.join(EXPORTS, 'function_map.txt')
    if not os.path.isfile(p):
        return out
    for line in io.open(p, encoding='ascii', errors='replace'):
        if line.startswith('#'):
            continue
        f = line.strip().split(',')
        if len(f) < 2:
            continue
        try:
            start = int(f[0], 16)
            size = int(f[1], 16) if f[1].startswith('0x') else int(f[1])
        except ValueError:
            continue
        if size > 0:
            out.append((start, start + size))
    return out


def in_a_function(addr, ranges):
    for start, end in ranges:
        if start <= addr < end:
            return True
    return False


def c_files():
    for dirpath, dirnames, names in os.walk(SRC):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for f in sorted(names):
            if f.endswith(('.c', '.h', '.cpp')):
                yield os.path.join(dirpath, f)


RULE = re.compile(r'[-=]{3,}')


def is_divider(text):
    """A one-line /* ---- section ---- */ marker, not an explanation.

    These legitimately sit above a comment that explains the first
    declaration under them, so they are not an accretion.

    IT MUST ACTUALLY CARRY A RULE. Accepting any short one-line
    comment lets the real accretion - two plain comments stacked -
    through unnoticed, which is what the selftest caught.
    """
    t = text.strip()
    if '\n' in t or not t.startswith('/*') or not t.endswith('*/'):
        return False
    return bool(RULE.search(t[2:-2]))


def blocks(lines):
    """Yield (start, end, text) for every /* ... */ block at column 0."""
    i = 0
    while i < len(lines):
        if lines[i].startswith('/*'):
            j = i
            while j < len(lines) and '*/' not in lines[j]:
                j += 1
            yield (i, min(j, len(lines) - 1),
                   '\n'.join(lines[i:j + 1]))
            i = j + 1
        else:
            i += 1


def next_code_line(lines, after):
    """The first line below `after` that is not blank."""
    k = after + 1
    while k < len(lines) and not lines[k].strip():
        k += 1
    return k if k < len(lines) else None


FUNC = re.compile(r'^[A-Za-z_][A-Za-z0-9_ \t*&:<>,~]*?\b(~?[A-Za-z_]\w*)\s*\(')

# Hosts are ours by definition - they are the window, not the demo.
NO_PROVENANCE_NEEDED = ('main_ppm.c', 'main_sdl.c')


def definitions(lines):
    """Yield (line_index, name) for every function DEFINITION.

    A definition is a declarator at column 0 whose next non-blank
    line opens a brace. That misses nothing in this tree's style and
    does not fire on prototypes in headers.
    """
    for i, ln in enumerate(lines):
        if not ln or ln[0].isspace() or ln.startswith(('/*', '*', '#')):
            continue
        m = FUNC.match(ln)
        if not m:
            continue
        j = i
        while j < len(lines) and '{' not in lines[j]:
            if ';' in lines[j]:
                break
            j += 1
            if j - i > 4:
                break
        if j < len(lines) and lines[j].strip().endswith('{'):
            yield i, m.group(1)


def preceding_comment(lines, at):
    """The block comment directly above `at`, or ''."""
    k = at - 1
    while k >= 0 and not lines[k].strip():
        k -= 1
    if k < 0 or '*/' not in lines[k]:
        return ''
    end = k
    while k >= 0 and not lines[k].startswith('/*'):
        k -= 1
    return '\n'.join(lines[max(0, k):end + 1])


def check(path, known, ranges=()):
    try:
        rel = os.path.relpath(path, ROOT).replace(os.sep, '/')
    except ValueError:
        rel = path                  # a temp file on another drive

    lines = io.open(path, encoding='ascii', errors='replace').read().split('\n')
    out = []
    bs = list(blocks(lines))

    for n, (start, end, text) in enumerate(bs):
        # the file header comment is allowed to stand alone, and so
        # is a section divider
        is_header = (start == 0)
        if is_divider(text):
            continue

        k = next_code_line(lines, end)
        if not is_header:
            if k is None:
                out.append((start + 1, 'orphaned',
                            'block comment at end of file, attached to '
                            'nothing'))
            elif lines[k].startswith('/*'):
                out.append((start + 1, 'stacked',
                            'block comment followed by another block '
                            'comment'))
            elif not lines[k].strip():
                out.append((start + 1, 'orphaned',
                            'nothing follows this block comment'))

        if known is not None:
            for m in ADDR.finditer(text):
                a = m.group(1).lower()
                if a in known:
                    continue
                if in_a_function(int('00' + a, 16), ranges):
                    continue
                kind = 'dead-origin' if 'ORIGIN' in text else 'bare-address'
                out.append((start + 1, kind,
                            '0x00%s is in no export and inside no named '
                            'function - rename it in Ghidra and re-export, '
                            'or fix the comment' % a))

    # EVERY FUNCTION IS EITHER TRANSCRIBED OR OURS, and must say
    # which. ORIGIN carries the address it came from - the only part
    # of a comment that survives a rename - and OURS marks a routine
    # with no counterpart in the demo. A function with neither leaves
    # a reader unable to tell whether its arithmetic is the demo's or
    # an invention, which is exactly what rule 12 turns on.
    if path.endswith(('.c', '.cpp')) and os.path.basename(path) \
            not in NO_PROVENANCE_NEEDED:
        for at, name in definitions(lines):
            pre = preceding_comment(lines, at)
            if 'ORIGIN' in pre or 'OURS' in pre:
                continue
            out.append((at + 1, 'no-provenance',
                        '%s() says neither ORIGIN nor OURS' % name))
    return rel, out


FIXTURES = [
    ('orphaned', """/* a header, which is allowed to stand alone */

#include "x.h"

/* ORIGIN 0x002ca9e4 - this used to sit on the function below */

"""),
    ('stacked', """/* a header */

/* one explanation */
/* a second bolted on beside it */
void f(void);
"""),
    ('dead-origin', """/* a header */

/* ORIGIN 0x00123456 - an address no export carries */
void f(void);
"""),
    ('clean', """/* a header */

/* ---- a divider ------------------------------------------- */

/* ORIGIN 0x002ca9e4  Beam_BuildBoltPath */
void f(void);
"""),
]


def selftest():
    """Prove each rule fires, because a silent checker is not trusted."""
    import tempfile
    bad = 0
    known = set(['2ca9e4'])
    ranges = ()
    for want, text in FIXTURES:
        fd, path = tempfile.mkstemp(suffix='.c')
        os.close(fd)
        io.open(path, 'w', encoding='ascii').write(text)
        try:
            _rel, hits = check(path, known, ranges)
        finally:
            os.remove(path)
        kinds = sorted(set(k for _l, k, _m in hits))
        if want == 'clean':
            ok = not hits
        else:
            ok = want in kinds
        print('  %-12s -> %-28s %s'
              % (want, ','.join(kinds) if kinds else '(nothing)',
                 'ok' if ok else 'FAILED'))
        if not ok:
            bad += 1
    print()
    print('selftest: %s' % ('every rule fires' if not bad
                            else '%d rule(s) did not' % bad))
    return 1 if bad else 0


def main(argv):
    if '--selftest' in argv:
        return selftest()
    count_only = '--count' in argv
    known = known_addresses()
    ranges = known_ranges()
    if known is None:
        print('no reverse/exports/ASD-Rapture.exe - address checks skipped')
    total, by_kind, nfiles = 0, {}, 0

    for path in c_files():
        rel, hits = check(path, known, ranges)
        if not hits:
            continue
        nfiles += 1
        if not count_only:
            print(rel)
        for line, kind, msg in hits:
            total += 1
            by_kind[kind] = by_kind.get(kind, 0) + 1
            if not count_only:
                print('  %4d  %-13s %s' % (line, kind, msg))

    print()
    print('%d finding(s) in %d file(s)' % (total, nfiles))
    for kind in sorted(by_kind):
        print('   %-13s %d' % (kind, by_kind[kind]))
    if total:
        print()
        print('A comment must sit against the thing it describes, and an')
        print('ORIGIN address must still name something - rule 18.')
    return 1 if total else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
