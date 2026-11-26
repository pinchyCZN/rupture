"""Types and globals of the port's Debug build, from its PDB, via dbghelp.

dbghelp.dll ships with Windows; this needs nothing beyond the standard
library. tools/agentctl.py uses it to turn a field path into an
address, a length and a layout:

    py -3.10 tools/pdbtypes.py Rupture_Ctx           a type's layout
    py -3.10 tools/pdbtypes.py g_demo.ctx.eye        a path's address, type
    py -3.10 tools/pdbtypes.py "g_demo.part[1]"      array elements

A path is a global (static ones too), then .field, [index] or ->field
in any order ([i] also indexes a std::vector, and [key] looks up an
integer key in a std::map, both through the live process; a READ of
a whole map lists its entries, and shows a std::string as its text);
Type@ADDR, a type laid over an address, for a this pointer a hook
logged; or Type@(path), a type laid over what the path
points to - part_motor@(g_demo.part[1]).t, the cast a base-class
pointer needs. -> needs the live process, so only agentctl.py
follows it; this script stops at the pointer.

The PDB has to be the exe's: a map or PDB older than the exe is refused,
as agentctl.py refuses a stale map.
"""
import ctypes
import ctypes.wintypes as wt
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, 'build', 'rupture', 'Debug', 'rupture.exe')

# IMAGEHLP_SYMBOL_TYPE_INFO
TI_GET_SYMTAG, TI_GET_SYMNAME, TI_GET_LENGTH, TI_GET_TYPE = 0, 1, 2, 3
TI_GET_BASETYPE, TI_FINDCHILDREN, TI_GET_DATAKIND = 5, 7, 8
TI_GET_OFFSET, TI_GET_COUNT, TI_GET_CHILDRENCOUNT = 10, 12, 13
# SymTagEnum
TAG_DATA, TAG_UDT, TAG_ENUM, TAG_POINTER = 7, 11, 12, 14
TAG_ARRAY, TAG_BASE, TAG_TYPEDEF, TAG_BASECLASS, TAG_VTABLE = 15, 16, 17, 18, 25
DATA_IS_MEMBER = 7
SYMOPT_UNDNAME, SYMOPT_DEFERRED = 0x2, 0x4

# BasicType -> name by length, and the struct format for it.
BASIC = {1: 'void', 2: 'char', 3: 'wchar_t', 6: 'int', 7: 'unsigned',
         8: 'float', 10: 'bool', 13: 'long', 14: 'unsigned long'}
SIGNED = {1: 'b', 2: 'h', 4: 'i', 8: 'q'}
UNSIGNED = {1: 'B', 2: 'H', 4: 'I', 8: 'Q'}


class SYMBOL_INFO(ctypes.Structure):
    _fields_ = [('SizeOfStruct', wt.ULONG), ('TypeIndex', wt.ULONG),
                ('Reserved', ctypes.c_ulonglong * 2), ('Index', wt.ULONG),
                ('Size', wt.ULONG), ('ModBase', ctypes.c_ulonglong),
                ('Flags', wt.ULONG), ('Value', ctypes.c_ulonglong),
                ('Address', ctypes.c_ulonglong), ('Register', wt.ULONG),
                ('Scope', wt.ULONG), ('Tag', wt.ULONG), ('NameLen', wt.ULONG),
                ('MaxNameLen', wt.ULONG), ('Name', ctypes.c_char * 512)]


class Member(object):
    def __init__(self, name, offset, tid):
        self.name = name
        self.offset = offset
        self.tid = tid


class Pdb(object):
    def __init__(self, exe=EXE):
        pdb = os.path.splitext(exe)[0] + '.pdb'
        if not os.path.exists(pdb):
            raise SystemExit('no %s - build rupture Debug' % pdb)
        if os.path.getmtime(pdb) + 5 < os.path.getmtime(exe):
            raise SystemExit('%s is older than the exe - rebuild Debug' % pdb)
        self.dbg = ctypes.WinDLL('dbghelp.dll')
        k32 = ctypes.WinDLL('kernel32.dll')
        k32.GetCurrentProcess.restype = wt.HANDLE
        self.proc = k32.GetCurrentProcess()
        d = self.dbg
        d.SymInitialize.argtypes = [wt.HANDLE, ctypes.c_char_p, wt.BOOL]
        d.SymLoadModuleEx.restype = ctypes.c_ulonglong
        d.SymLoadModuleEx.argtypes = [wt.HANDLE, wt.HANDLE, ctypes.c_char_p,
                                      ctypes.c_char_p, ctypes.c_ulonglong,
                                      wt.DWORD, ctypes.c_void_p, wt.DWORD]
        d.SymGetTypeFromName.argtypes = [wt.HANDLE, ctypes.c_ulonglong,
                                         ctypes.c_char_p, ctypes.c_void_p]
        d.SymFromName.argtypes = [wt.HANDLE, ctypes.c_char_p, ctypes.c_void_p]
        d.SymGetTypeInfo.argtypes = [wt.HANDLE, ctypes.c_ulonglong, wt.ULONG,
                                     ctypes.c_int, ctypes.c_void_p]
        d.SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED)
        if not d.SymInitialize(self.proc, os.path.dirname(exe).encode(),
                               False):
            raise SystemExit('dbghelp: SymInitialize failed')
        self.base = d.SymLoadModuleEx(self.proc, None, exe.encode(), None,
                                      0x400000, 0, None, 0)
        if not self.base:
            raise SystemExit('dbghelp cannot load %s' % exe)

    # ---- dbghelp's type queries -----------------------------------------

    def _info(self, tid, what, ctype=wt.DWORD):
        v = ctype()
        if not self.dbg.SymGetTypeInfo(self.proc, self.base, tid, what,
                                       ctypes.byref(v)):
            return None
        return v.value

    def tag(self, tid):
        return self._info(tid, TI_GET_SYMTAG)

    def length(self, tid):
        return self._info(tid, TI_GET_LENGTH, ctypes.c_ulonglong) or 0

    def inner(self, tid):
        """A pointer's pointee, an array's element, a typedef's type."""
        return self._info(tid, TI_GET_TYPE)

    def count(self, tid):
        return self._info(tid, TI_GET_COUNT) or 0

    def symname(self, tid):
        p = ctypes.c_wchar_p()
        if not self.dbg.SymGetTypeInfo(self.proc, self.base, tid,
                                       TI_GET_SYMNAME, ctypes.byref(p)):
            return None
        return p.value

    def strip(self, tid):
        while self.tag(tid) == TAG_TYPEDEF:
            tid = self.inner(tid)
        return tid

    def children(self, tid):
        n = self._info(tid, TI_GET_CHILDRENCOUNT) or 0
        if n == 0:
            return []

        class Find(ctypes.Structure):
            _fields_ = [('Count', wt.ULONG), ('Start', wt.ULONG),
                        ('ChildId', wt.ULONG * n)]
        f = Find(n, 0)
        if not self.dbg.SymGetTypeInfo(self.proc, self.base, tid,
                                       TI_FINDCHILDREN, ctypes.byref(f)):
            return []
        return list(f.ChildId)

    # ---- what the tools want -------------------------------------------

    def find_type(self, name):
        sym = SYMBOL_INFO()
        sym.SizeOfStruct = ctypes.sizeof(SYMBOL_INFO) - 512
        sym.MaxNameLen = 512
        if not self.dbg.SymGetTypeFromName(self.proc, self.base, name.encode(),
                                           ctypes.byref(sym)):
            raise LookupError('no type %s in the PDB' % name)
        return sym.TypeIndex

    def find_global(self, name):
        sym = SYMBOL_INFO()
        sym.SizeOfStruct = ctypes.sizeof(SYMBOL_INFO) - 512
        sym.MaxNameLen = 512
        if not self.dbg.SymFromName(self.proc, name.encode(),
                                    ctypes.byref(sym)):
            raise LookupError('no global %s in the PDB' % name)
        return sym.Address, sym.TypeIndex

    def members(self, tid):
        """Data members and base classes, by offset."""
        out = []
        for c in self.children(self.strip(tid)):
            tag = self.tag(c)
            if tag == TAG_BASECLASS:
                inner = self.inner(c)
                out.append(Member('<%s>' % (self.symname(inner) or 'base'),
                                  self._info(c, TI_GET_OFFSET) or 0, inner))
            elif tag == TAG_VTABLE:
                out.append(Member('<vfptr>', 0, None))
            elif (tag == TAG_DATA
                  and self._info(c, TI_GET_DATAKIND) == DATA_IS_MEMBER):
                out.append(Member(self.symname(c), self._info(c, TI_GET_OFFSET),
                                  self.inner(c)))
        out.sort(key=lambda m: m.offset)
        return out

    def describe(self, tid):
        """A type's name as C would write it."""
        if tid is None:
            return 'void *'
        tag = self.tag(tid)
        if tag == TAG_BASE:
            bt = self._info(tid, TI_GET_BASETYPE)
            n = self.length(tid)
            if bt == 8:
                return 'double' if n == 8 else 'float'
            if bt in (6, 13) and n == 8:
                return '__int64'
            if bt == 6 and n == 2:
                return 'short'
            if bt == 7 and n == 1:
                return 'unsigned char'
            if bt == 7 and n == 2:
                return 'unsigned short'
            return BASIC.get(bt, 'base%d' % bt)
        if tag == TAG_POINTER:
            return self.describe(self.inner(tid)) + ' *'
        if tag == TAG_ARRAY:
            return '%s[%d]' % (self.describe(self.inner(tid)), self.count(tid))
        return self.symname(tid) or '?'

    def fmt(self, tid):
        """The struct format a scalar unpacks with, or None for an aggregate."""
        tid = self.strip(tid) if tid is not None else None
        if tid is None:
            return 'I'
        tag = self.tag(tid)
        n = self.length(tid)
        if tag == TAG_POINTER:
            return 'I'
        if tag == TAG_ENUM:
            return SIGNED.get(n)
        if tag != TAG_BASE:
            return None
        bt = self._info(tid, TI_GET_BASETYPE)
        if bt == 8:
            return 'd' if n == 8 else 'f'
        if bt == 10:
            return '?'
        if bt in (6, 13) or (bt == 2 and n == 1):
            return SIGNED.get(n)
        return UNSIGNED.get(n)

    def width(self, tid):
        """The agent's operand width letter for a scalar: b w l f d."""
        f = self.fmt(tid)
        return {'d': 'd', 'f': 'f', 'b': 'b', 'B': 'b', '?': 'b', 'h': 'w',
                'H': 'w'}.get(f, 'l' if f else None)

    # ---- paths ------------------------------------------------------------

    TOKEN = re.compile(r'\.([A-Za-z_]\w*)|\[(-?(?:0x[0-9a-fA-F]+|\d+))\]'
                       r'|->([A-Za-z_]\w*)')

    CAST = re.compile(r'([A-Za-z_][\w:]*)@'
                      r'(?:\(([^()]*)\)|(?:0x)?([0-9a-fA-F]+))')

    def resolve(self, path, read=None):
        """(address, type id) for a path; read(addr, n) follows ->.

        Type@ADDR lays a type over an address; Type@(path) over what the
        path points to, or, for a path that is not a pointer, over its
        own address - the cast a base-class pointer needs:
        part_motor@(g_demo.part[1]).t."""
        m = self.CAST.match(path)
        if m:
            tid = self.find_type(m.group(1))
            if m.group(3) is not None:
                addr = int(m.group(3), 16)
            else:
                addr, inner = self.resolve(m.group(2), read)
                if self.tag(self.strip(inner)) == TAG_POINTER:
                    if read is None:
                        raise LookupError('@(pointer) needs the running'
                                          ' process')
                    addr = struct.unpack('<I', read(addr, 4))[0]
        else:
            m = re.match(r'[A-Za-z_][\w:]*', path)
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
                ptid = self.strip(tid)
                if self.tag(ptid) != TAG_POINTER:
                    raise LookupError('-> on %s, not a pointer'
                                      % self.describe(tid))
                if read is None:
                    raise LookupError('-> needs the running process')
                addr = struct.unpack('<I', read(addr, 4))[0]
                tid = self.inner(ptid)
                addr, tid = self._field(addr, tid, t.group(3), path)
            elif t.group(1) is not None:
                addr, tid = self._field(addr, tid, t.group(1), path)
            elif self.vector(tid) is not None:
                addr, tid = self._element(addr, tid, int(t.group(2), 0), read)
            elif self.tree(tid) is not None:
                addr, tid = self._map_get(addr, tid, int(t.group(2), 0), read)
            else:
                atid = self.strip(tid)
                if self.tag(atid) != TAG_ARRAY:
                    raise LookupError('[] on %s, not an array'
                                      % self.describe(tid))
                i = int(t.group(2), 0)
                if i >= self.count(atid):
                    raise LookupError('[%d] past %s' % (i, self.describe(tid)))
                tid = self.inner(atid)
                addr += i * self.length(tid)
        return addr, tid

    def vector(self, tid):
        """(first offset, last offset, element type) of a vector, or None.

        MSVC 2017 keeps _Myfirst and _Mylast under _Mypair._Myval2; this
        finds them by name at any depth rather than assuming the nesting."""
        if tid is None or not self.describe(tid).startswith('std::vector<'):
            return None
        found = {}

        def walk(t, base):
            for m in self.members(t):
                if m.name in ('_Myfirst', '_Mylast'):
                    found[m.name] = (base + m.offset, m.tid)
                elif m.tid is not None and self.fmt(m.tid) is None:
                    walk(m.tid, base + m.offset)
        walk(tid, 0)
        if '_Myfirst' not in found or '_Mylast' not in found:
            return None
        first, ptid = found['_Myfirst']
        return first, found['_Mylast'][0], self.inner(self.strip(ptid))

    def _find(self, tid, names):
        """{name: (offset, type)} for members named in names, at any depth."""
        found = {}

        def walk(t, base):
            for m in self.members(t):
                if m.name in names and m.name not in found:
                    found[m.name] = (base + m.offset, m.tid)
                elif m.tid is not None and self.fmt(m.tid) is None:
                    walk(m.tid, base + m.offset)
        walk(tid, 0)
        return found

    def tree(self, tid):
        """The offsets a std::map or std::set needs, or None: the head
        pointer and size in the container; the left, parent and right
        links, the nil flag and the value in a node; for a map, the
        value's first and second. MSVC 2017's _Tree, found by name."""
        if tid is None:
            return None
        d = self.describe(tid)
        if not (d.startswith('std::map<') or d.startswith('std::set<')):
            return None
        c = self._find(tid, ('_Myhead', '_Mysize'))
        if len(c) != 2:
            return None
        node = self.inner(self.strip(c['_Myhead'][1]))
        n = self._find(node, ('_Left', '_Parent', '_Right', '_Isnil', '_Myval'))
        if len(n) != 5:
            return None
        t = {'head': c['_Myhead'][0], 'size': c['_Mysize'][0],
             'left': n['_Left'][0], 'parent': n['_Parent'][0],
             'right': n['_Right'][0], 'isnil': n['_Isnil'][0],
             'val': n['_Myval'][0], 'valtid': n['_Myval'][1]}
        if d.startswith('std::map<'):
            p = self._find(n['_Myval'][1], ('first', 'second'))
            if len(p) != 2:
                return None
            t['key'], t['first'] = p['first'][1], p['first'][0]
            t['second'], t['sectid'] = p['second'][0], p['second'][1]
        else:
            t['key'], t['first'] = n['_Myval'][1], 0
        return t

    def map_entries(self, addr, tid, read, limit=64):
        """[(key, value address)] in key order, at most limit of them;
        None if the keys are not scalars."""
        t = self.tree(tid)
        kf = self.fmt(t['key'])
        if kf is None:
            return None
        kn = struct.calcsize('<' + kf)
        ptr = lambda a: struct.unpack('<I', read(a, 4))[0]
        nil = lambda nd: read(nd + t['isnil'], 1)[0] != 0
        head = ptr(addr + t['head'])
        out, stack, nd = [], [], ptr(head + t['parent'])
        while (stack or not nil(nd)) and len(out) < limit:
            if not nil(nd):
                stack.append(nd)
                nd = ptr(nd + t['left'])
                continue
            nd = stack.pop()
            val = nd + t['val']
            key = struct.unpack('<' + kf, read(val + t['first'], kn))[0]
            out.append((key, val + t.get('second', 0)))
            nd = ptr(nd + t['right'])
        return out

    def _map_get(self, addr, tid, key, read):
        """[key] on a std::map: the address and type of its value."""
        t = self.tree(tid)
        if read is None:
            raise LookupError('[] on a map needs the running process')
        if 'second' not in t:
            raise LookupError('[] on %s, a set' % self.describe(tid))
        kf = self.fmt(t['key'])
        if kf is None or kf in 'fd':
            raise LookupError('[] needs an integer key, not %s'
                              % self.describe(t['key']))
        kn = struct.calcsize('<' + kf)
        ptr = lambda a: struct.unpack('<I', read(a, 4))[0]
        nd = ptr(ptr(addr + t['head']) + t['parent'])
        while read(nd + t['isnil'], 1)[0] == 0:
            k = struct.unpack('<' + kf, read(nd + t['val'] + t['first'], kn))[0]
            if key == k:
                return nd + t['val'] + t['second'], t['sectid']
            nd = ptr(nd + (t['left'] if key < k else t['right']))
        raise LookupError('no key %d in %s' % (key, self.describe(tid)))

    def _element(self, addr, tid, i, read):
        first, last, el = self.vector(tid)
        if read is None:
            raise LookupError('[] on a vector needs the running process')
        lo, hi = struct.unpack('<II', read(addr + first, 4)
                               + read(addr + last, 4))
        n = (hi - lo) // max(self.length(el), 1)
        if i >= n:
            raise LookupError('[%d] past a vector of %d' % (i, n))
        return lo + i * self.length(el), el

    def _field(self, addr, tid, name, path):
        for mem in self.members(tid):
            if mem.name == name:
                return addr + mem.offset, mem.tid
        for mem in self.members(tid):
            if mem.name.startswith('<') and mem.tid is not None:
                try:
                    return self._field(addr + mem.offset, mem.tid, name, path)
                except LookupError:
                    pass
        raise LookupError('%s has no field %s (in %s)'
                          % (self.describe(tid), name, path))

    # ---- printing -----------------------------------------------------

    def layout(self, tid):
        """Lines: offset, size, type, name."""
        lines = ['%s  size 0x%x' % (self.describe(tid), self.length(tid))]
        for m in self.members(tid):
            size = self.length(m.tid) if m.tid is not None else 4
            lines.append('  +0x%04x  0x%-5x %-28s %s' % (
                m.offset, size, self.describe(m.tid), m.name))
        return lines

    def string(self, tid):
        """(buffer, size, capacity) offsets of a std::string, or None.
        MSVC 2017 keeps up to 15 characters in the 16-byte _Bx itself and
        a pointer there once _Myres reaches 16; found by name."""
        if tid is None or not self.describe(tid).startswith(
                'std::basic_string<char,'):
            return None
        s = self._find(tid, ('_Bx', '_Mysize', '_Myres'))
        if len(s) != 3:
            return None
        return s['_Bx'][0], s['_Mysize'][0], s['_Myres'][0]

    STRING_SHOWN = 200

    def _text(self, st, data, off, read):
        """A std::string's characters, quoted and escaped, or a note of
        where they are when they are on the heap and read is None."""
        size = struct.unpack_from('<I', data, off + st[1])[0]
        res = struct.unpack_from('<I', data, off + st[2])[0]
        n = min(size, self.STRING_SHOWN)
        if res < 16:
            raw = bytes(data[off + st[0]:off + st[0] + min(n, 15)])
        else:
            ptr = struct.unpack_from('<I', data, off + st[0])[0]
            if read is None:
                return 'size=%d at 0x%08x' % (size, ptr)
            try:
                raw = read(ptr, n) if n else b''
            except LookupError:
                return 'size=%d at 0x%08x, unreadable' % (size, ptr)
        text = ''.join(chr(b) if 32 <= b < 127 and b not in (34, 92)
                       else '\\x%02x' % b for b in raw)
        more = '... %d more' % (size - n) if size > n else ''
        return '"%s"%s' % (text, more)

    def value(self, tid, data, off=0, indent='  ', depth=4, limit=16,
              read=None):
        """Lines showing the bytes at data[off:] as tid; read(addr, n)
        fetches a std::string kept on the heap."""
        tid0 = tid
        tid = self.strip(tid) if tid is not None else None
        f = self.fmt(tid0)
        if f is not None:
            v = struct.unpack_from('<' + f, data, off)[0]
            if tid is None or self.tag(tid) == TAG_POINTER:
                return ['0x%08x' % v]
            return ['%.9g' % v if f in 'fd' else str(v)]
        st = self.string(tid0)
        if st is not None:
            return [self._text(st, data, off, read)]
        vec = self.vector(tid0)
        if vec is not None:
            lo = struct.unpack_from('<I', data, off + vec[0])[0]
            hi = struct.unpack_from('<I', data, off + vec[1])[0]
            n = (hi - lo) // max(self.length(vec[2]), 1)
            return ['size=%d data=0x%08x  %s' % (n, lo, self.describe(vec[2]))]
        tree = self.tree(tid0)
        if tree is not None:
            n = struct.unpack_from('<I', data, off + tree['size'])[0]
            return ['size=%d  %s' % (n, self.describe(tid0))]
        if depth == 0:
            return ['{...}']
        if self.tag(tid) == TAG_ARRAY:
            el = self.inner(tid)
            n = self.count(tid)
            size = self.length(el)
            out = []
            for i in range(min(n, limit)):
                sub = self.value(el, data, off + i * size, indent + '  ',
                                 depth - 1, limit, read)
                out += self._join('[%d]' % i, sub, indent)
            if n > limit:
                out.append('%s... %d more' % (indent, n - limit))
            return out
        out = []
        for m in self.members(tid):
            if m.name == '<vfptr>':
                sub = ['0x%08x' % struct.unpack_from('<I', data, off)[0]]
            else:
                sub = self.value(m.tid, data, off + m.offset, indent + '  ',
                                 depth - 1, limit, read)
            out += self._join(m.name, sub, indent)
        return out

    @staticmethod
    def _join(name, sub, indent):
        if len(sub) == 1 and not sub[0].startswith(' '):
            return ['%s%-14s %s' % (indent, name, sub[0])]
        return ['%s%s' % (indent, name)] + sub


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 1
    p = Pdb()
    arg = sys.argv[1]
    try:
        if re.search(r'[.\[@]|->', arg):
            addr, tid = p.resolve(arg)
            print('%s at 0x%08x, %s, 0x%x bytes'
                  % (arg, addr, p.describe(tid), p.length(tid)))
            if p.fmt(tid) is None:
                print('\n'.join(p.layout(tid)))
            return 0
        try:
            addr, tid = p.find_global(arg)
            print('%s at 0x%08x, %s' % (arg, addr, p.describe(tid)))
        except LookupError:
            tid = p.find_type(arg)
        print('\n'.join(p.layout(tid)))
    except LookupError as exc:
        print('ERR %s' % exc)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
