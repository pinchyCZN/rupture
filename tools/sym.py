"""Symbol name -> address, and address -> symbol, from the exported maps.

Reads reverse/exports/ASD-Rapture.exe/function_map.txt and
symbol_map.txt, the maps tools/export_maps.py writes from the database
(CLAUDE.md rule 7). The demo has no relocations, so a map address is
the live address in the running process.

    py -3.10 tools/sym.py Timer_Seconds          one name, exactly
    py -3.10 tools/sym.py g_demoClock+0x8        a name plus an offset
    py -3.10 tools/sym.py -q Demo_Frame          only the address, for scripts
    py -3.10 tools/sym.py -s part_motor          every name containing it
    py -3.10 tools/sym.py 0x00448dd8             the symbol holding it

An exact name is tried case-sensitive first, then case-insensitive. A
name the maps carry more than once - Function ID's FID_conflict names
are - is listed and refused rather than picked. Exit status: 0 found,
1 not found, 2 ambiguous.

The maps hold only what somebody named: an address in a FUN_ is
reported as in no named symbol. A stale map answers confidently, so
re-export after renaming. DEMO_EXPORTS overrides the map directory.

tools/shimctl.py imports resolve() so addr= can take a name.
"""
import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROGRAM = 'ASD-Rapture.exe'


class Sym(object):
    def __init__(self, addr, size, name, kind, typ, src):
        self.addr = addr
        self.size = size
        self.name = name
        self.kind = kind        # 'function' or 'global'
        self.typ = typ
        self.src = src

    def line(self):
        extra = 'size=0x%x' % self.size if self.size else 'size=?'
        if self.typ:
            extra += ' type=%s' % self.typ
        return '0x%08x  %-8s  %-40s %s %s' % (self.addr, self.kind, self.name,
                                             extra, self.src)


_SYMS = None


def exports_dir():
    return os.environ.get('DEMO_EXPORTS') or \
        os.path.join(ROOT, 'reverse', 'exports', PROGRAM)


def _rows(path):
    with open(path, encoding='ascii') as f:
        for raw in f:
            line = raw.strip()
            if line and not line.startswith('#'):
                yield line.split(',')


def load():
    """Every named function and global, sorted by address."""
    global _SYMS
    if _SYMS is not None:
        return _SYMS
    d = exports_dir()
    out = []
    funcs = os.path.join(d, 'function_map.txt')
    globs = os.path.join(d, 'symbol_map.txt')
    if not os.path.exists(funcs):
        raise SystemExit('no %s - run py -3.10 tools/export_maps.py' % funcs)
    for p in _rows(funcs):
        if len(p) >= 4:
            out.append(Sym(int(p[0], 16), int(p[1], 16), p[2], 'function',
                           '', p[3]))
    if os.path.exists(globs):
        for p in _rows(globs):
            if len(p) >= 6:
                out.append(Sym(int(p[0], 16), int(p[4], 16), p[1], 'global',
                               p[2], p[5]))
    out.sort(key=lambda s: (s.addr, s.kind))
    _SYMS = out
    return out


def find(name):
    """The symbols called name: exact case first, then any case."""
    syms = load()
    hits = [s for s in syms if s.name == name]
    if not hits:
        low = name.lower()
        hits = [s for s in syms if s.name.lower() == low]
    return hits


HEX = re.compile(r'^(0x)?[0-9a-fA-F]+$')
EXPR = re.compile(r'^(.+?)([+-](0x)?[0-9a-fA-F]+)?$')


def resolve(text):
    """An address from hex, NAME or NAME+OFF (hex). Raises LookupError.

    Bare hex is an address. A name that is also valid hex would be
    misread, so that case is refused, not guessed.
    """
    if HEX.match(text):
        if not text.lower().startswith('0x') and find(text):
            raise LookupError('%s is both hex and a symbol name - write '
                              '0x%s or the name with +0' % (text, text))
        return int(text, 16)
    m = EXPR.match(text)
    name, off = m.group(1), m.group(2)
    hits = find(name)
    if not hits:
        raise LookupError('no symbol %s in the maps (try sym.py -s)' % name)
    if len({s.addr for s in hits}) > 1:
        raise LookupError('%s names %d addresses: %s' % (
            name, len(hits), ' '.join('0x%08x' % s.addr for s in hits)))
    return hits[0].addr + (int(off, 16) if off else 0)


def containing(addr):
    """(symbol, offset) of the named function or global holding addr."""
    best = None
    for s in load():
        if s.addr > addr:
            break
        if addr < s.addr + max(s.size, 1):
            best = s
    if best is None:
        return None, 0
    return best, addr - best.addr


def describe(addr):
    """name+0xOFF for addr, or '' if no named symbol holds it."""
    s, off = containing(addr)
    if s is None:
        return ''
    return s.name + ('+0x%x' % off if off else '')


def main():
    ap = argparse.ArgumentParser(description='Look up a symbol in the maps.')
    ap.add_argument('what', help='NAME, NAME+OFF, or an address with 0x')
    ap.add_argument('-q', action='store_true', help='print only the address')
    ap.add_argument('-s', action='store_true', help='substring search')
    a = ap.parse_args()

    if a.s:
        low = a.what.lower()
        hits = [s for s in load() if low in s.name.lower()]
        for s in hits:
            print(s.line())
        return 0 if hits else 1

    if a.what.lower().startswith('0x'):
        addr = int(a.what, 16)
        s, off = containing(addr)
        if s is None:
            print('0x%08x is in no named function or global' % addr)
            return 1
        print('0x%08x  %s+0x%x' % (addr, s.name, off) if off else
              '0x%08x  %s' % (addr, s.name))
        if not a.q:
            print(s.line())
        return 0

    m = EXPR.match(a.what)
    hits = find(m.group(1))
    if len({s.addr for s in hits}) > 1:
        print('%s is ambiguous:' % m.group(1))
        for s in hits:
            print(s.line())
        return 2
    try:
        addr = resolve(a.what)
    except LookupError as exc:
        print(exc)
        return 1
    if a.q:
        print('0x%08x' % addr)
    else:
        print(hits[0].line() if addr == hits[0].addr else
              '0x%08x  = %s' % (addr, a.what))
    return 0


if __name__ == '__main__':
    sys.exit(main())
