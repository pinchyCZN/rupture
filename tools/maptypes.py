"""Types and globals of the ORIGINAL, from the exported maps.

The shim's side of tools/pdbtypes.py: the same paths and the same
printing, with struct_map.txt and symbol_map.txt standing in for a PDB
(the original has none). Only what somebody typed in Ghidra is known,
so most paths start from Type@ADDR:

    py -3.10 tools/maptypes.py Rupture_Track
    py -3.10 tools/maptypes.py Rupture_Vec4@00516e50.x

A type is its Ghidra name; a pointer is "NAME *", and a member with a
count is an array of it. Regenerate the maps (tools/export_maps.py)
after typing something in Ghidra.
"""
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXPORTS = os.environ.get('DEMO_EXPORTS',
                         os.path.join(ROOT, 'reverse', 'exports'))
PROGRAM = 'ASD-Rapture.exe'

# Ghidra base types: (size, struct format)
BASE = {
    'undefined': (1, 'B'), 'undefined1': (1, 'B'), 'byte': (1, 'B'),
    'uchar': (1, 'B'), 'char': (1, 'b'), 'bool': (1, '?'),
    'undefined2': (2, 'H'), 'word': (2, 'H'), 'ushort': (2, 'H'),
    'short': (2, 'h'), 'undefined4': (4, 'I'), 'dword': (4, 'I'),
    'uint': (4, 'I'), 'ulong': (4, 'I'), 'int': (4, 'i'), 'long': (4, 'i'),
    'float': (4, 'f'), 'double': (8, 'd'), 'undefined8': (8, 'Q'),
    'qword': (8, 'Q'), 'longlong': (8, 'q'), 'ulonglong': (8, 'Q'),
}


class Member(object):
    def __init__(self, name, offset, tid):
        self.name = name
        self.offset = offset
        self.tid = tid


class MapTypes(object):
    def __init__(self):
        d = os.path.join(EXPORTS, PROGRAM)
        self.types = {}         # name -> (size, [Member])
        self.globals = {}       # name -> (addr, type)
        cur = None
        with open(os.path.join(d, 'struct_map.txt')) as f:
            for line in f:
                if line.startswith('#') or not line.strip():
                    continue
                if line.startswith('type,'):
                    p = line.rstrip('\n').split(',')
                    cur = []
                    self.types[p[1]] = (int(p[3], 16), cur, p[2])
                elif line.startswith('\t') and cur is not None:
                    p = line.strip().split(',')
                    if len(p) < 5:
                        continue            # an enum member: name,value
                    tid = p[1] + ('[%d]' % int(p[3]) if p[3] else '')
                    if p[1] == 'undefined' and int(p[4], 16) > 1:
                        tid = 'undefined1[%d]' % int(p[4], 16)
                    cur.append(Member(p[0], int(p[2], 16), tid))
        with open(os.path.join(d, 'symbol_map.txt')) as f:
            for line in f:
                if line.startswith('#') or not line.strip():
                    continue
                p = line.rstrip('\n').split(',')
                if len(p) >= 5:
                    typ = p[2] + ('[%d]' % int(p[3]) if p[3] else '')
                    self.globals[p[1]] = (int(p[0], 16), typ)

    # ---- the type queries pdbtypes.Pdb also answers ---------------------

    def _array(self, tid):
        m = re.match(r'(.*)\[(\d+)\]$', tid or '')
        return (m.group(1), int(m.group(2))) if m else (None, 0)

    def length(self, tid):
        el, n = self._array(tid)
        if el is not None:
            return self.length(el) * n
        if tid.endswith('*'):
            return 4
        if tid in BASE:
            return BASE[tid][0]
        if tid in self.types:
            return self.types[tid][0]
        return 0

    def describe(self, tid):
        return tid

    def fmt(self, tid):
        if tid is None or tid.endswith('*'):
            return 'I'
        if tid in BASE:
            return BASE[tid][1]
        if tid in self.types and self.types[tid][2] == 'enum':
            return 'i'
        return None

    def width(self, tid):
        f = self.fmt(tid)
        return {'d': 'd', 'f': 'f', 'b': 'b', 'B': 'b', '?': 'b', 'h': 'w',
                'H': 'w'}.get(f, 'l' if f else None)

    def members(self, tid):
        t = self.types.get(tid)
        return t[1] if t and t[2] in ('struct', 'union') else []

    def find_type(self, name):
        if name not in self.types and name not in BASE:
            raise LookupError('no type %s in struct_map.txt' % name)
        return name

    def find_global(self, name):
        if name not in self.globals:
            raise LookupError('no global %s in symbol_map.txt' % name)
        return self.globals[name]

    # ---- paths, as pdbtypes.Pdb.resolve --------------------------------

    TOKEN = re.compile(r'\.([A-Za-z_(][\w()]*)|\[(\d+)\]|->([A-Za-z_]\w*)')
    CAST = re.compile(r'([A-Za-z_][\w:]*)@'
                      r'(?:\(([^()]*)\)|(?:0x)?([0-9a-fA-F]+))')

    def resolve(self, path, read=None):
        m = self.CAST.match(path)
        if m:
            tid = self.find_type(m.group(1))
            if m.group(3) is not None:
                addr = int(m.group(3), 16)
            else:
                addr, inner = self.resolve(m.group(2), read)
                if inner.endswith('*'):
                    if read is None:
                        raise LookupError('@(pointer) needs the live'
                                          ' process')
                    addr = struct.unpack('<I', read(addr, 4))[0]
        else:
            m = re.match(r'[A-Za-z_]\w*', path)
            if not m:
                raise LookupError('%s is not a path' % path)
            addr, tid = self.find_global(m.group(0))
        rest = path[m.end():]
        while rest:
            t = self.TOKEN.match(rest)
            if not t:
                raise LookupError('cannot read %r in %s' % (rest, path))
            rest = rest[t.end():]
            if t.group(3) is not None:
                if not tid.endswith('*') or read is None:
                    raise LookupError('-> needs a pointer and the'
                                      ' live process')
                addr = struct.unpack('<I', read(addr, 4))[0]
                tid = tid[:-1].strip()
                addr, tid = self._field(addr, tid, t.group(3), path)
            elif t.group(1) is not None:
                addr, tid = self._field(addr, tid, t.group(1), path)
            else:
                el, n = self._array(tid)
                i = int(t.group(2))
                if el is None or i >= n:
                    raise LookupError('[%d] on %s' % (i, tid))
                addr += i * self.length(el)
                tid = el
        return addr, tid

    def _field(self, addr, tid, name, path):
        for mem in self.members(tid):
            if mem.name == name:
                return addr + mem.offset, mem.tid
        raise LookupError('%s has no field %s (in %s)' % (tid, name, path))

    # ---- printing, as pdbtypes.Pdb -------------------------------------

    def layout(self, tid):
        lines = ['%s  size 0x%x' % (tid, self.length(tid))]
        for m in self.members(tid):
            lines.append('  +0x%04x  0x%-5x %-28s %s' % (
                m.offset, self.length(m.tid), m.tid, m.name))
        return lines

    def value(self, tid, data, off=0, indent='  ', depth=4, limit=16):
        f = self.fmt(tid)
        if f is not None:
            v = struct.unpack_from('<' + f, data, off)[0]
            if tid.endswith('*'):
                return ['0x%08x' % v]
            return ['%.9g' % v if f in 'fd' else str(v)]
        if depth == 0:
            return ['{...}']
        el, n = self._array(tid)
        out = []
        if el is not None:
            for i in range(min(n, limit)):
                out += self._join('[%d]' % i, self.value(
                    el, data, off + i * self.length(el), indent + '  ',
                    depth - 1, limit), indent)
            return out
        for m in self.members(tid):
            if m.name == '(pad)':
                continue
            out += self._join(m.name, self.value(
                m.tid, data, off + m.offset, indent + '  ', depth - 1, limit),
                indent)
        return out or ['(%d bytes, no layout)' % self.length(tid)]

    @staticmethod
    def _join(name, sub, indent):
        if len(sub) == 1 and not sub[0].startswith(' '):
            return ['%s%-14s %s' % (indent, name, sub[0])]
        return ['%s%s' % (indent, name)] + sub


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 1
    m = MapTypes()
    try:
        if re.search(r'[.\[@]|->', sys.argv[1]):
            addr, tid = m.resolve(sys.argv[1])
            print('%s at 0x%08x, %s, 0x%x bytes'
                  % (sys.argv[1], addr, tid, m.length(tid)))
            if m.fmt(tid) is None:
                print('\n'.join(m.layout(tid)))
        else:
            print('\n'.join(m.layout(m.find_type(sys.argv[1]))))
    except LookupError as exc:
        print('ERR %s' % exc)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
