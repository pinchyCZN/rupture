"""Drive the agent pipe in the port (rupture's Debug build) or the shim.

The port's agent (src/rupture/host/agent_port.cpp) listens on
\\\\.\\pipe\\rupture_port when build/rupture/Debug/rupture.exe runs
with --agent; the shim's (src/shim/agent.c) on \\\\.\\pipe\\rupture_shim.
Both speak src/agent/agent_core.c's verbs; tools/README-agent.txt lists them.

    py -3.10 tools/agentctl.py launch          the port, paused at frame 0
    py -3.10 tools/agentctl.py STATUS
    py -3.10 tools/agentctl.py RUN_TO part=motor t=5.2476 exact
    py -3.10 tools/agentctl.py STEP n=10
    py -3.10 tools/agentctl.py READ addr=g_demo+0x10 len=8 -f d
    py -3.10 tools/agentctl.py HOOK add addr=part_motor::DrawScene log=ecx
    py -3.10 tools/agentctl.py HOOK add addr=Math_Ease exit log=st0,a0
    py -3.10 tools/agentctl.py TRACE
    py -3.10 tools/agentctl.py dump frame.png  the last frame, as PNG
    py -3.10 tools/agentctl.py HOOK add addr=Post_RunPass texdump=40 path=n.rtex
    py -3.10 tools/agentctl.py png n.rtex n.png  a TEXDUMP file as PNG
    py -3.10 tools/agentctl.py sym DrawScene   names containing it
    py -3.10 tools/agentctl.py type Camera     a type's layout, from the PDB
    py -3.10 tools/agentctl.py READ addr=g_demo.ctx.eye   field by field
    py -3.10 tools/agentctl.py READ "addr=part_motor@(g_demo.part[1]).t"
    py -3.10 tools/agentctl.py READ addr=part_motor@07a4fc28  a this pointer
    py -3.10 tools/agentctl.py HOOK add addr=Demo_Frame log=@g_demo.clock
    py -3.10 tools/agentctl.py CANCEL          stop a running STEP or RUN_TO
    py -3.10 tools/agentctl.py QUIT
    py -3.10 tools/agentctl.py --shim launch   the original, fixed step
    py -3.10 tools/agentctl.py --shim RUN_TO part=motor t=5.2476 exact

--shim launch starts reverse/patched/run/ASD-Rapture.exe with
RUPTURE_SHIM_FIXED=1: the dialog runs itself, the music is held and the
clock moves by a fixed step, so STEP and RUN_TO work on the original as
on the port, and a part time is the same frame in both. It refuses a
run-folder glut32.dll older than the shim's sources.

THE PORT IS ALWAYS THE DEBUG BUILD. Release has no agent and refuses
--agent. launch refuses a Debug exe older than any source under
src/rupture or src/agent, and every command refuses a map older than
the exe - rebuild Debug, then relaunch. A stale map names the wrong
addresses without any error, which is why this is checked every time.

Names: for the port, any function or global the linker map carries,
static ones included, by its C++ name (part_motor::Frame) or the
mangled one; for the shim, the Ghidra exports via tools/sym.py.
NAME+OFF works in addr= and in abs operands. In the port, addr= also
takes a field path - g_demo.ctx.eye, ptr->field, Type@ADDR, or the cast
Type@(path), as in part_motor@(g_demo.part[1]).t -
resolved through the Debug PDB by tools/pdbtypes.py; a READ of one
reads the whole object and shows it field by field. In log= and when=,
@path is the field read at the width its type has. HOOK add ... exit gets
end= from the map, the next function's address. Every reply starts OK
or ERR; the exit status is 0 for OK.

Any number of clients may connect at once: STATUS, READ and CANCEL
answer at once even while another client's STEP or RUN_TO runs, and
STATUS then says busy=RUN_TO. A RUN_TO toward a part's t stops where
that part ends, if it ends first. Failures are ERR lines on stdout.

Options:
    -x      READ: raw hex          -f FMT  READ: struct.unpack format
    --timeout S   seconds to wait for a reply (default 600)
"""
import argparse
import ctypes
import glob
import os
import re
import struct
import subprocess
import sys
import tempfile
import time
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEBUG = os.path.join(ROOT, 'build', 'rupture', 'Debug')
PORT_EXE = os.path.join(DEBUG, 'rupture.exe')
PORT_MAP = os.path.join(DEBUG, 'rupture.map')
RUN_DIR = os.path.join(ROOT, 'reverse', 'patched', 'run')
SHIM_DLL = os.path.join(RUN_DIR, 'glut32.dll')
PIPES = {'port': r'\\.\pipe\rupture_port',
         'shim': r'\\.\pipe\rupture_shim'}
SOURCES = ('src/rupture/**/*.cpp', 'src/rupture/**/*.h', 'src/agent/*.c',
           'src/agent/*.h', 'src/rupture/rupture.vcxproj')
HINTS = {'port': 'is build/rupture/Debug/rupture.exe running with --agent?'
                 ' (agentctl.py launch)',
         'shim': 'is the shimmed demo running and past its first frame?'}


# ---- the pipe ---------------------------------------------------------

def send(target, line, timeout_s=600.0, open_wait_s=10.0):
    """One command, one reply. The header's lines=N says how many follow."""
    deadline = time.time() + open_wait_s
    while True:
        try:
            f = open(PIPES[target], 'r+b', buffering=0)
            break
        except OSError as exc:
            if time.time() > deadline:
                raise SystemExit('cannot open %s: %s - %s'
                                 % (PIPES[target], exc, HINTS[target]))
            time.sleep(0.1)
    with f:
        f.write((line.strip() + '\n').encode('ascii'))
        buf = b''
        want = None
        while want is None or buf.count(b'\n') < want + 1:
            if want is None and b'\n' in buf:
                want = 0
                for tok in buf.split(b'\n', 1)[0].split():
                    if tok.startswith(b'lines='):
                        want = int(tok[6:])
                continue
            chunk = f.read(65536)
            if not chunk:
                break
            buf += chunk
    return buf.decode('ascii', 'replace')


# ---- the port's symbols, from the linker map --------------------------

_UNDNAME_NAME_ONLY = 0x1000


def undecorate(name):
    """A C++ name from a mangled one, through dbghelp; C names lose _."""
    if not name.startswith('?'):
        return name[1:] if name.startswith('_') else name
    buf = ctypes.create_string_buffer(512)
    try:
        n = ctypes.windll.dbghelp.UnDecorateSymbolName(
            name.encode('ascii'), buf, 512, _UNDNAME_NAME_ONLY)
    except (AttributeError, OSError):
        n = 0
    return buf.value.decode('ascii', 'replace') if n else name


MAP_LINE = re.compile(r'^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+'
                      r'([0-9a-fA-F]{8})\s+(f\s+)?(i\s+)?(\S+)\s*$')


class PortMap(object):
    def __init__(self, path):
        self.syms = []      # (addr, name, raw, is_function, obj)
        with open(path, 'r', errors='replace') as f:
            for line in f:
                m = MAP_LINE.match(line)
                if not m:
                    continue
                raw, addr = m.group(1), int(m.group(2), 16)
                if addr == 0 or raw.startswith('__imp_') or '$' in raw:
                    continue
                self.syms.append((addr, undecorate(raw), raw,
                                  bool(m.group(3)), m.group(5)))
        self.syms.sort()
        self.funcs = sorted(set(s[0] for s in self.syms if s[3]))

    def find(self, name):
        hits = [s for s in self.syms if name in (s[1], s[2])]
        if not hits:
            hits = [s for s in self.syms if s[1].endswith('::' + name)]
        addrs = sorted(set(h[0] for h in hits))
        if not addrs:
            raise LookupError('%s is in no symbol of %s'
                              % (name, os.path.basename(PORT_MAP)))
        if len(addrs) > 1:
            raise LookupError('%s names %d addresses: %s' % (
                name, len(addrs),
                ', '.join('%s@%08x' % (h[1], h[0]) for h in hits[:6])))
        return addrs[0]

    def resolve(self, text):
        name, plus, off = text.partition('+')
        if re.fullmatch(r'(0x)?[0-9a-fA-F]+', name):
            base = int(name, 16)
        else:
            base = self.find(name)
        return base + (int(off, 0) if plus else 0)

    def bound(self, addr):
        """The next function's address after addr: a bound, not an end."""
        for a in self.funcs:
            if a > addr:
                return a
        raise LookupError('no function follows %08x' % addr)

    def describe(self, addr):
        best = None
        for s in self.syms:
            if s[0] > addr:
                break
            best = s
        if best is None:
            return None
        if best[0] == addr:
            return best[1]
        return '%s+0x%x' % (best[1], addr - best[0])

    def search(self, text):
        return [s for s in self.syms if text.lower() in s[1].lower()]


def port_map():
    """The map, refused when older than the exe it describes."""
    rel = os.path.relpath(PORT_MAP, ROOT)
    if not os.path.exists(PORT_MAP) or not os.path.exists(PORT_EXE):
        raise SystemExit('no %s - build rupture Debug first' % rel)
    if os.path.getmtime(PORT_MAP) + 5 < os.path.getmtime(PORT_EXE):
        raise SystemExit('%s is older than the exe - rebuild Debug' % rel)
    return PortMap(PORT_MAP)


class ShimSyms(object):
    """tools/sym.py's lookup, for the original."""
    def __init__(self):
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import sym
        self.sym = sym

    def resolve(self, text):
        return self.sym.resolve(text)

    def describe(self, addr):
        return self.sym.describe(addr)

    def bound(self, addr):
        raise LookupError('exit hooks need the port: the shim has no /GH')


# ---- the command line -------------------------------------------------

ABS = re.compile(r'(abs[bwlfd])\+([^,<>=!]+)')
RA = re.compile(r'\bra=([0-9a-fA-F]{8})\b')


PATH = re.compile(r'[.\[@]|->')
AT = re.compile(r'@([A-Za-z_][\w:.\[\]@()]*)')


class Paths(object):
    """Field paths: the port's PDB (tools/pdbtypes.py), or for the original
    the exported maps (tools/maptypes.py); -> reads the live process."""
    def __init__(self, target, timeout):
        if target == 'port':
            import pdbtypes
            self.pdb = pdbtypes.Pdb()
        else:
            import maptypes
            self.pdb = maptypes.MapTypes()
        self.target = target
        self.timeout = timeout

    def read(self, addr, n):
        return read_mem(self.target, addr, n, self.timeout)

    def resolve(self, path):
        return self.pdb.resolve(path, self.read)

    def operand(self, path):
        """@path in log= or when=: abs with the width its type has."""
        addr, tid = self.resolve(path)
        w = self.pdb.width(tid)
        if w is None:
            raise LookupError('@%s is a %s, not a scalar'
                              % (path, self.pdb.describe(tid)))
        operand = 'abs%s+%x' % (w, addr)
        names_save(self.target, {operand: '@' + path})
        return operand


def names_file(target):
    return os.path.join(tempfile.gettempdir(), 'rupture_agent_%s_names.txt'
                        % target)


def names_save(target, more):
    """Remembers what each @path became, so later TRACE lines show it."""
    names = names_load(target)
    names.update(more)
    with open(names_file(target), 'w') as f:
        for k, v in sorted(names.items()):
            f.write('%s %s\n' % (k, v))


def names_load(target):
    names = {}
    try:
        with open(names_file(target)) as f:
            for line in f:
                k, _, v = line.strip().partition(' ')
                if v:
                    names[k] = v
    except OSError:
        pass
    return names


def names_apply(target, text):
    for k, v in names_load(target).items():
        text = text.replace(k + '=', v + '=')
    return text


def read_mem(target, addr, n, timeout=600.0):
    """n bytes at addr, in READs of at most 4096."""
    out = b''
    while len(out) < n:
        k = min(4096, n - len(out))
        reply = send(target, 'READ addr=%x len=%d' % (addr + len(out), k),
                     timeout)
        lines = reply.rstrip('\n').split('\n')
        if not lines[0].startswith('OK READ') or len(lines) < 2:
            raise LookupError(lines[0])
        out += bytes.fromhex(lines[1].strip())
    return out


def resolve_words(words, syms, paths):
    """addr= and abs operands become hex, from the map or, for a field
    path, the PDB; @path operands get their width; HOOK add ... exit
    gets end=. Returns the words and the type id of a path addr=."""
    out = []
    addr = None
    tid = None
    for w in words:
        key, eq, val = w.partition('=')
        if eq and key == 'addr':
            if PATH.search(val):
                addr, tid = paths().resolve(val)
            else:
                addr = syms.resolve(val)
            w = 'addr=%x' % addr
        elif eq and key in ('log', 'when', 'texdump'):
            val = AT.sub(lambda m: paths().operand(m.group(1)), val)
            w = key + '=' + ABS.sub(
                lambda m: '%s+%x' % (m.group(1), syms.resolve(m.group(2))),
                val)
        elif eq and key == 'path':
            w = 'path=' + os.path.abspath(val).replace('\\', '/')
        out.append(w)
    if (out[:2] == ['HOOK', 'add'] and 'exit' in out and addr is not None
            and not any(w.startswith('end=') for w in out)):
        out.append('end=%x' % syms.bound(addr))
    named = [w[5:] for w in words if w.startswith('addr=')]
    if (out[:2] == ['HOOK', 'add'] and named and not any(
            w.startswith('tag=') for w in out)):
        out.append('tag=' + named[0].split('::')[-1][:23])
    return out, tid


def read_path(words, tid, paths, args):
    """READ of a field path: the whole object unless len= says otherwise,
    shown field by field (-x, -f as for any READ)."""
    pdb = paths().pdb
    addr = int([w for w in words if w.startswith('addr=')][0][5:], 16)
    lens = [w for w in words if w.startswith('len=')]
    n = int(lens[0][4:], 0) if lens else pdb.length(tid) or 4
    data = read_mem(paths().target, addr, n, args.timeout)
    print('OK READ addr=0x%08x len=%d type=%s' % (addr, n, pdb.describe(tid)))
    if args.f:
        vals = struct.unpack_from('<' + args.f, data)
        print('  ' + '  '.join(repr(v) for v in vals))
    elif args.x:
        print(data.hex())
    elif n < pdb.length(tid):
        print(hexdump(data, addr))
    else:
        read = lambda a, k: read_mem(paths().target, a, k, args.timeout)
        print('\n'.join(pdb.value(tid, data, read=read)))
        tree = pdb.tree(tid)
        if tree is not None:
            entries = pdb.map_entries(addr, tid, read)
            if entries is None:
                print('  (entries not listed: the keys are %s)'
                      % pdb.describe(tree['key']))
                entries = []
            for key, at in entries:
                if 'sectid' not in tree:
                    print('  %s' % key)
                    continue
                vt = tree['sectid']
                sub = pdb.value(vt, read(at, pdb.length(vt) or 4), read=read)
                if len(sub) == 1:
                    print('  [%s] %s' % (key, sub[0]))
                else:
                    print('  [%s]' % key)
                    print('\n'.join('  ' + s for s in sub))
    return 0


def name_callers(text, syms):
    def one(m):
        where = syms.describe(int(m.group(1), 16))
        return m.group(0) + ('(%s)' % where if where else '')
    return RA.sub(one, text)


def hexdump(data, base):
    out = []
    for off in range(0, len(data), 16):
        row = data[off:off + 16]
        hx = ' '.join('%02x' % c for c in row)
        txt = ''.join(chr(c) if 32 <= c < 127 else '.' for c in row)
        out.append('%08x  %-47s  %s' % (base + off, hx, txt))
    return '\n'.join(out)


def png_chunk(kind, body):
    crc = struct.pack('>I', zlib.crc32(kind + body))
    return struct.pack('>I', len(body)) + kind + body + crc


def rtex_to_png(src, dst):
    """RTEX (w, h, RGBA rows bottom up) to a PNG, top row first."""
    with open(src, 'rb') as f:
        data = f.read()
    if data[:4] != b'RTEX':
        raise SystemExit('%s is not an RTEX file' % src)
    w, h = struct.unpack_from('<ii', data, 4)
    px = data[12:]
    stride = w * 4
    rows = [b'\x00' + px[(h - 1 - y) * stride:(h - y) * stride]
            for y in range(h)]
    head = struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)
    with open(dst, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + png_chunk(b'IHDR', head)
                + png_chunk(b'IDAT', zlib.compress(b''.join(rows), 6))
                + png_chunk(b'IEND', b''))
    return w, h


def stale_sources():
    """Sources newer than the Debug exe."""
    exe_t = os.path.getmtime(PORT_EXE)
    newer = []
    for pat in SOURCES:
        for p in glob.glob(os.path.join(ROOT, pat), recursive=True):
            if os.path.getmtime(p) > exe_t:
                newer.append(os.path.relpath(p, ROOT))
    return newer


def stale_shim():
    """Shim sources newer than the glut32.dll in the run folder."""
    dll_t = os.path.getmtime(SHIM_DLL)
    newer = []
    for pat in ('src/shim/*.c', 'src/shim/*.h', 'src/agent/*.c',
                'src/agent/*.h'):
        for p in glob.glob(os.path.join(ROOT, pat)):
            if os.path.getmtime(p) > dll_t:
                newer.append(os.path.relpath(p, ROOT))
    return newer


def pipe_up(target):
    try:
        open(PIPES[target], 'r+b', buffering=0).close()
    except OSError:
        return False
    return True


def launch_shim(args):
    """The original from the run folder, shimmed, in fixed-step mode."""
    if not os.path.exists(SHIM_DLL):
        raise SystemExit('no %s - set up the run folder (README.md)' % SHIM_DLL)
    newer = stale_shim()
    if newer and not args.stale_ok:
        raise SystemExit('the run folder\'s glut32.dll is older than %s'
                         ' - build the shim (Release copies it in)'
                         % newer[0])
    if pipe_up('shim'):
        raise SystemExit('a shim is already listening - QUIT it first')
    env = dict(os.environ, RUPTURE_SHIM_FIXED='1')
    if os.path.exists(names_file('shim')):
        os.remove(names_file('shim'))
    subprocess.Popen([os.path.join(RUN_DIR, 'ASD-Rapture.exe')],
                     cwd=RUN_DIR, env=env)
    deadline = time.time() + args.wait
    reply = send('shim', 'STATUS', open_wait_s=args.wait)
    while ' held=1' not in reply and time.time() < deadline:
        time.sleep(0.25)
        reply = send('shim', 'STATUS')
    print(reply.rstrip())
    if ' held=1' not in reply:
        print('ERR the demo never reached its first frame')
        return 1
    return 0


def launch(args):
    if not os.path.exists(PORT_EXE):
        raise SystemExit('no Debug build - build src/rupture.sln Debug|x86')
    newer = stale_sources()
    if newer and not args.stale_ok:
        raise SystemExit('the Debug exe is older than %d source(s), e.g. %s'
                         ' - rebuild Debug (or --stale-ok)'
                         % (len(newer), newer[0]))
    port_map()
    if pipe_up('port'):
        raise SystemExit('an agent is already listening on %s - QUIT it'
                         % PIPES['port'])
    argv = [PORT_EXE, '--agent']
    if args.size:
        argv.append('--size=' + args.size)
    if args.nobatch:
        argv.append('--nobatch')
    if args.part:
        argv.append('--part=' + args.part)
    if os.path.exists(names_file('port')):
        os.remove(names_file('port'))
    subprocess.Popen(argv, cwd=DEBUG)
    reply = send('port', 'STATUS', open_wait_s=args.wait)
    print(reply.rstrip())
    return 0 if reply.startswith('OK') else 1


def dump(target, path, timeout):
    tmp = os.path.join(tempfile.gettempdir(),
                       'rupture_dump_%d.rtex' % os.getpid())
    reply = send(target, 'DUMP path=%s' % tmp, timeout)
    print(reply.rstrip())
    if not reply.startswith('OK'):
        return 1
    w, h = rtex_to_png(tmp, path)
    os.remove(tmp)
    print('wrote %s %dx%d' % (path, w, h))
    return 0


def type_cmd(arg, paths):
    """type NAME: a type's layout, or a path's address, type and layout."""
    pdb = paths().pdb
    if PATH.search(arg):
        addr, tid = paths().resolve(arg)
        print('%s at 0x%08x, %s, 0x%x bytes'
              % (arg, addr, pdb.describe(tid), pdb.length(tid)))
        if pdb.fmt(tid) is None:
            print('\n'.join(pdb.layout(tid)))
        return 0
    try:
        addr, tid = pdb.find_global(arg)
        print('%s at 0x%08x, %s' % (arg, addr, pdb.describe(tid)))
    except LookupError:
        tid = pdb.find_type(arg)
    print('\n'.join(pdb.layout(tid)))
    return 0


def main(argv=None, default_target='port'):
    """Every failure is an ERR line on stdout and exit status 1, so a
    pipe cannot hide it."""
    try:
        return run(argv, default_target)
    except SystemExit as exc:
        if isinstance(exc.code, str):
            print(exc.code if exc.code.startswith('ERR') else
                  'ERR ' + exc.code)
            return 1
        raise
    except LookupError as exc:
        print('ERR %s' % exc)
        return 1


def run(argv=None, default_target='port'):
    ap = argparse.ArgumentParser(
        description='Drive the agent pipe.', epilog=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('words', nargs='+',
                    help='a command line, or launch / dump PNG / sym TEXT')
    ap.add_argument('--shim', action='store_true',
                    help='the shimmed original, not the port')
    ap.add_argument('-x', action='store_true', help='READ: raw hex')
    ap.add_argument('-f', metavar='FMT', help='READ: struct.unpack format')
    ap.add_argument('--timeout', type=float, default=600.0)
    ap.add_argument('--wait', type=float, default=120.0,
                    help='launch: seconds to wait for the pipe')
    ap.add_argument('--stale-ok', action='store_true',
                    help='launch: run an out-of-date exe')
    ap.add_argument('--part', metavar='NAME[@T]',
                    help='launch: start at a part, or at time T in it')
    ap.add_argument('--nobatch', action='store_true',
                    help='launch: draw every End on its own, as before the'
                         ' hot-spot batching, to compare against it')
    ap.add_argument('--size', metavar='WxH',
                    help='launch: the view, e.g. 800x450 as the shimmed'
                         ' original has it (its client area is 794x421)')
    args = ap.parse_args(argv)
    target = 'shim' if args.shim or default_target == 'shim' else 'port'
    words = args.words

    if words[0] == 'launch':
        return launch(args) if target == 'port' else launch_shim(args)
    if words[0] == 'dump' and len(words) == 2:
        return dump(target, words[1], args.timeout)
    if words[0] == 'png' and len(words) == 3:
        w, h = rtex_to_png(words[1], words[2])
        print('wrote %s %dx%d' % (words[2], w, h))
        return 0
    syms = port_map() if target == 'port' else ShimSyms()
    made = []

    def paths():
        if not made:
            made.append(Paths(target, args.timeout))
        return made[0]

    if words[0] == 'type' and len(words) == 2:
        try:
            return type_cmd(words[1], paths)
        except LookupError as exc:
            print('ERR %s' % exc)
            return 1
    if words[0] == 'sym':
        if target != 'port':
            sym_py = os.path.join(ROOT, 'tools', 'sym.py')
            return subprocess.call([sys.executable, sym_py] + words[1:])
        for s in syms.search(words[1]):
            print('%08x %s %-50s %s' % (s[0], 'f' if s[3] else 'd', s[1],
                                        s[4]))
        return 0

    try:
        words, tid = resolve_words(words, syms, paths)
        if words[0] == 'READ' and tid is not None:
            return read_path(words, tid, paths, args)
    except LookupError as exc:
        print('ERR %s - nothing sent' % exc)
        return 1
    reply = send(target, ' '.join(words), args.timeout)
    lines = reply.rstrip('\n').split('\n')
    head = lines[0] if lines else ''
    if not head.startswith('OK READ') or len(lines) < 2:
        print(names_apply(target, name_callers(reply.rstrip(), syms)))
        return 0 if head.startswith('OK') else 1
    print(head)
    data = bytes.fromhex(lines[1].strip())
    addr = int([t for t in head.split() if t.startswith('addr=')][0][5:], 16)
    if args.f:
        vals = struct.unpack_from('<' + args.f, data)
        print('  ' + '  '.join(repr(v) for v in vals))
    elif args.x:
        print(lines[1].strip())
    else:
        print(hexdump(data, addr))
    return 0


if __name__ == '__main__':
    sys.exit(main())
