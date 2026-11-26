"""portprof.py - a sampling profiler for the port's Debug build.

    py -3.10 tools/portprof.py rail 40           300 frames from rail t=40
    py -3.10 tools/portprof.py city 50 --frames 600 --top 30
    py -3.10 tools/portprof.py --release 20      the running Release exe

Needs the port running under the agent (agentctl.py launch). It seeks
with RUN_TO, then while one STEP runs the frames it suspends the main
thread about every millisecond and reads its stack. The call stack is
the EBP chain (Debug keeps frame pointers), each link checked to return
just past a call instruction in rupture.exe, and each return address
names a function from build/rupture/Debug/rupture.map.

A sample in a DLL - the GL driver, the CRT - may have EBP in use as a
plain register. Then the first return address above ESP is taken as
the port function that called into the DLL, and the chain is walked
from the first frame above it that leads back to App_Frame. Such a
sample is charged as "[in DLL] under" that function. That function can
be wrong: a dead return address left in a local can sit nearer ESP
than the live one, so a leaf such as gl1::multiply or an Init-only
function may be named. The inclusive rows above it, from the walked
chain, are the ones to trust.

A sample counts once for every distinct function on its stack
(inclusive) and once for the innermost one (self).

--release samples a Release rupture.exe that is already running (start
it with --part=NAME@T to reach the stretch) for the seconds given, in
real time. Release inlines and keeps no frame pointers, so it reports
self time only: the function the thread was in, with time inside a DLL
charged, as above, to the first return address above ESP.

Debug is optimised but never inlines, and every function pays the
agent's /Gh /GH entry and exit hooks (_penter, _pexit), so small
functions weigh more here than in Release. GPU time shows up only where
the CPU waits for the driver.
"""
import argparse
import bisect
import ctypes
import os
import sys
import threading
import time
from collections import Counter, defaultdict
from ctypes import wintypes as W

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import agentctl  # noqa: E402

k32 = ctypes.WinDLL('kernel32', use_last_error=True)

TH32CS_SNAPPROCESS, TH32CS_SNAPTHREAD = 0x2, 0x4
PROCESS_READ = 0x0010 | 0x0400            # VM_READ | QUERY_INFORMATION
THREAD_ALL = 0x0002 | 0x0008 | 0x0010 | 0x0040   # SUSPEND, CONTEXT, QUERY
WOW64_CONTEXT_CONTROL = 0x00010001
D = W.DWORD


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [('dwSize', D), ('cntUsage', D), ('th32ProcessID', D),
                ('th32DefaultHeapID', ctypes.c_void_p),
                ('th32ModuleID', D), ('cntThreads', D),
                ('th32ParentProcessID', D), ('pcPriClassBase', W.LONG),
                ('dwFlags', D), ('szExeFile', W.WCHAR * 260)]


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', D), ('cntUsage', D), ('th32ThreadID', D),
                ('th32OwnerProcessID', D), ('tpBasePri', W.LONG),
                ('tpDeltaPri', W.LONG), ('dwFlags', D)]


class WOW64_CONTEXT(ctypes.Structure):
    _fields_ = [('ContextFlags', D), ('Dr', D * 6),
                ('FloatSave', ctypes.c_byte * 112),
                ('SegGs', D), ('SegFs', D), ('SegEs', D), ('SegDs', D),
                ('Edi', D), ('Esi', D), ('Ebx', D), ('Edx', D),
                ('Ecx', D), ('Eax', D), ('Ebp', D), ('Eip', D),
                ('SegCs', D), ('EFlags', D), ('Esp', D), ('SegSs', D),
                ('Ext', ctypes.c_byte * 512)]


k32.CreateToolhelp32Snapshot.restype = W.HANDLE
k32.OpenProcess.restype = W.HANDLE
k32.OpenThread.restype = W.HANDLE


def find_pid():
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    e = PROCESSENTRY32W()
    e.dwSize = ctypes.sizeof(e)
    pids = []
    ok = k32.Process32FirstW(snap, ctypes.byref(e))
    while ok:
        if e.szExeFile.lower() == 'rupture.exe':
            pids.append(e.th32ProcessID)
        ok = k32.Process32NextW(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    if len(pids) != 1:
        raise SystemExit('want exactly one rupture.exe running, found %d'
                         % len(pids))
    return pids[0]


def threads_of(pid):
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
    e = THREADENTRY32()
    e.dwSize = ctypes.sizeof(e)
    out = []
    ok = k32.Thread32First(snap, ctypes.byref(e))
    while ok:
        if e.th32OwnerProcessID == pid:
            out.append(e.th32ThreadID)
        ok = k32.Thread32Next(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


class Target(object):
    """The process, its .text section, and the call-instruction test."""

    def __init__(self, pid):
        self.h = k32.OpenProcess(PROCESS_READ, False, pid)
        if not self.h:
            raise SystemExit('OpenProcess failed: %d'
                             % ctypes.get_last_error())
        hdr = self.read(0x400000, 0x400)
        pe = int.from_bytes(hdr[0x3c:0x40], 'little')
        nsec = int.from_bytes(hdr[pe + 6:pe + 8], 'little')
        opt = int.from_bytes(hdr[pe + 20:pe + 22], 'little')
        sec = pe + 24 + opt
        self.text = None
        for i in range(nsec):
            s = hdr[sec + 40 * i:sec + 40 * (i + 1)]
            if s[:5] == b'.text':
                va = 0x400000 + int.from_bytes(s[12:16], 'little')
                size = int.from_bytes(s[8:12], 'little')
                self.text = (va, va + size)
                self.code = self.read(va, size)
        if self.text is None:
            raise SystemExit('no .text in the exe image')

    def read(self, addr, n):
        buf = ctypes.create_string_buffer(n)
        got = ctypes.c_size_t()
        k32.ReadProcessMemory(self.h, ctypes.c_void_p(addr), buf, n,
                              ctypes.byref(got))
        return buf.raw[:got.value]

    def read_upto(self, addr, n):
        """Up to n bytes from addr, stopping at the first unreadable page
        (a read across the top of a stack fails whole)."""
        out = b''
        while len(out) < n:
            step = min(0x1000 - (addr + len(out)) % 0x1000, n - len(out))
            got = self.read(addr + len(out), step)
            out += got
            if len(got) < step:
                break
        return out

    def in_text(self, a):
        return self.text[0] <= a < self.text[1]

    # call r/m32, FF /2: (length, ModRM test)
    FORMS = ((2, lambda m: 0xD0 <= m <= 0xD7),                  # reg
             (2, lambda m: m & 0xF8 == 0x10 and m not in (0x14, 0x15)),
             (3, lambda m: m == 0x14),                          # [sib]
             (3, lambda m: m & 0xF8 == 0x50 and m != 0x54),     # [reg+d8]
             (4, lambda m: m == 0x54),                          # [sib+d8]
             (6, lambda m: m == 0x15),                          # [abs32]
             (6, lambda m: m & 0xF8 == 0x90 and m != 0x94),     # [reg+d32]
             (7, lambda m: m == 0x94))                          # [sib+d32]

    def after_call(self, a, indirect=False):
        """a is a return address: a call instruction ends just before it.
        indirect: only a call through a register or memory, the only way
        the exe calls into a DLL."""
        o = a - self.text[0]
        if o < 7 or not self.in_text(a):
            return False
        c = self.code
        if c[o - 5] == 0xE8 and not indirect:                    # rel32
            return True
        for n, test in self.FORMS:
            if c[o - n] == 0xFF and test(c[o - n + 1]):
                return True
        return False


class Stack(object):
    """One sample: the registers and the stack above ESP."""

    def __init__(self, tgt, eip, esp, ebp, data):
        self.tgt, self.eip, self.esp, self.ebp = tgt, eip, esp, ebp
        self.data = data
        self.top = esp + len(data)

    def word(self, a):
        o = a - self.esp
        return int.from_bytes(self.data[o:o + 4], 'little')

    def chain(self, ebp):
        out = []
        while self.esp <= ebp < self.top - 8 and len(out) < 200:
            ret = self.word(ebp + 4)
            if not self.tgt.after_call(ret):
                break
            out.append(ret)
            nxt = self.word(ebp)
            if nxt <= ebp:
                break
            ebp = nxt
        return out

    def first_return(self, indirect=False):
        """The first return address above ESP, and where it sits."""
        for a in range(self.esp, self.top - 3, 4):
            if self.tgt.after_call(self.word(a), indirect):
                return self.word(a), a
        return None, None


def sample(tgt, tid, ctx):
    th = k32.OpenThread(THREAD_ALL, False, tid)
    if not th:
        return None
    try:
        if k32.Wow64SuspendThread(th) == 0xFFFFFFFF:
            return None
        try:
            ctx.ContextFlags = WOW64_CONTEXT_CONTROL
            if not k32.Wow64GetThreadContext(th, ctypes.byref(ctx)):
                return None
            eip, esp, ebp = ctx.Eip, ctx.Esp, ctx.Ebp
            data = tgt.read_upto(esp, 0x10000)
        finally:
            k32.ResumeThread(th)
    finally:
        k32.CloseHandle(th)
    return Stack(tgt, eip, esp, ebp, data)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('part', nargs='?')
    ap.add_argument('t', type=float, nargs='?')
    ap.add_argument('--release', type=float, metavar='SECONDS')
    ap.add_argument('--frames', type=int, default=300)
    ap.add_argument('--top', type=int, default=25)
    ap.add_argument('--interval', type=float, default=0.001)
    args = ap.parse_args()

    if args.release:
        pmap = agentctl.PortMap(os.path.join(agentctl.ROOT, 'build',
                                             'rupture', 'Release',
                                             'rupture.map'))
    elif args.part is None or args.t is None:
        raise SystemExit('give PART T, or --release SECONDS')
    else:
        pmap = agentctl.port_map()
    starts = sorted(set(s[0] for s in pmap.syms if s[3]))
    names = {}
    for s in pmap.syms:
        if s[3]:
            names.setdefault(s[0], s[1])

    def func_of(a):
        i = bisect.bisect_right(starts, a) - 1
        return names[starts[i]] if i >= 0 else '?'

    def to_frame(chain):
        fs = [func_of(a) for a in chain]
        return fs if 'App_Frame' in fs else None

    def walk(st):
        """The functions on the stack, innermost first, or None."""
        if st.tgt.in_text(st.eip):
            fs = to_frame(st.chain(st.ebp))
            return ([func_of(st.eip)] + fs) if fs else None
        ret, at = st.first_return()
        if ret is None:
            return None
        caller = '[in DLL] under ' + func_of(ret)
        fs = to_frame(st.chain(st.ebp))
        if fs:
            return [caller, func_of(ret)] + fs
        for q in range(at + 4, min(at + 0x2000, st.top - 8), 4):
            if st.word(q) > q and st.tgt.after_call(st.word(q + 4)):
                fs = to_frame(st.chain(q))
                if fs:
                    return [caller, func_of(ret)] + fs
        return None

    if args.release:
        return release(args, func_of)
    r = agentctl.send('port', 'RUN_TO part=%s t=%g exact'
                      % (args.part, args.t), 1800)
    if not r.startswith('OK'):
        raise SystemExit(r)
    pid = find_pid()
    tgt = Target(pid)
    done = threading.Event()
    timing = {}

    def stepper():
        t0 = time.time()
        agentctl.send('port', 'STEP n=%d' % args.frames, 3600)
        timing['s'] = time.time() - t0
        done.set()

    th = threading.Thread(target=stepper)
    th.start()
    ctx = WOW64_CONTEXT()
    incl, self_ = Counter(), Counter()
    n = dll = lost = 0
    main_tid = None
    while not done.is_set():
        for tid in ([main_tid] if main_tid else threads_of(pid)):
            st = sample(tgt, tid, ctx)
            if st is None:
                continue
            fs = walk(st)
            if fs is None:
                if main_tid:
                    lost += 1
                continue
            main_tid = tid
            n += 1
            if fs[0].startswith('[in DLL]'):
                dll += 1
            for f in set(fs):
                incl[f] += 1
            self_[fs[0]] += 1
        t0 = time.perf_counter()
        while time.perf_counter() - t0 < args.interval:
            pass
    th.join()
    if n == 0:
        raise SystemExit('no samples landed in App_Frame')
    ms = timing['s'] * 1000.0 / args.frames
    print('%s t=%g: %d frames, %.1f ms per frame, %d samples'
          ' (%d not walked), %.0f%% in DLLs'
          % (args.part, args.t, args.frames, ms, n, lost, 100.0 * dll / n))
    for title, table in (('inclusive', incl), ('self', self_)):
        print('\n  %-46s %5s  %8s' % (title, '%', 'ms/frame'))
        for f, c in table.most_common(args.top):
            print('  %-46s %5.1f  %8.2f'
                  % (f[:46], 100.0 * c / n, ms * c / n))


def release(args, func_of):
    """Self time of the main thread for args.release seconds. The main
    thread is the one found most often in rupture.exe's own code over
    the first 200 passes."""
    pid = find_pid()
    tgt = Target(pid)
    ctx = WOW64_CONTEXT()

    def where(st):
        if tgt.in_text(st.eip):
            f = func_of(st.eip)
            ret, _ = st.first_return()
            callers[f][func_of(ret) if ret else '?'] += 1
            return f
        ret, _ = st.first_return(True)
        return '[in DLL] under ' + (func_of(ret) if ret else '?')

    callers = defaultdict(Counter)

    ours = Counter()
    for i in range(200):
        for tid in threads_of(pid):
            st = sample(tgt, tid, ctx)
            if st is not None and tgt.in_text(st.eip):
                ours[tid] += 1
    if not ours:
        raise SystemExit('no thread is running rupture.exe code')
    tid = ours.most_common(1)[0][0]
    table = Counter()
    end = time.time() + args.release
    while time.time() < end:
        st = sample(tgt, tid, ctx)
        if st is not None:
            table[where(st)] += 1
        t0 = time.perf_counter()
        while time.perf_counter() - t0 < args.interval:
            pass
    n = sum(table.values())
    print('Release, %g s, thread %d: %d samples'
          % (args.release, tid, n))
    print('\n  %-46s %5s' % ('self', '%'))
    for f, c in table.most_common(args.top):
        print('  %-46s %5.1f' % (f[:46], 100.0 * c / n))
        if f in callers and c * 50 >= n:
            for g, k in callers[f].most_common(3):
                print('      called from %-34s %5.1f'
                      % (g[:34], 100.0 * k / n))


if __name__ == '__main__':
    main()
