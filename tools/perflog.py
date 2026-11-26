"""perflog.py - summarise or record a Release rupture.exe --perflog file.

    py -3.10 tools/perflog.py LOG                  summarise LOG
    py -3.10 tools/perflog.py LOG --run end 20     run --part=end for 20 s
                                                   into LOG, then summarise
    py -3.10 tools/perflog.py LOG --run city@40 15 --size 1920x1080

The host writes one line per frame (host_win32.cpp perf_open): the part,
its time, then dt, cpu, swap and gpu in ms. The summary is per 2 s of
part time: the mean and the worst of each, and how many frames came
more than 1.5 refreshes (at 60 Hz) after the one before, the frames a
fps readout loses. With vsync on, swap is the wait for the display, so
cpu well under 16.7 with a small swap is not a margin: the frame is
ready early and waits. gpu counts idle gaps too; it is the GPU's own
cost only while the CPU is ahead of it.
"""
import argparse
import collections
import os
import subprocess
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, 'build', 'rupture', 'Release', 'rupture.exe')
LATE = 1.5 * 1000.0 / 60.0


def record(log, part, seconds, size):
    p = subprocess.Popen([EXE, '--size=' + size, '--part=' + part,
                          '--perflog=' + os.path.abspath(log)],
                         cwd=os.path.dirname(EXE))
    time.sleep(seconds)
    p.kill()
    p.wait()


def summarise(log):
    rows = collections.OrderedDict()
    for line in open(log):
        f = line.split()
        if len(f) != 10:
            continue                   # the last line of a killed run
        key = (f[0], int(float(f[1]) // 2) * 2)
        rows.setdefault(key, []).append([float(f[i]) for i in (3, 5, 7, 9)])
    print('%-10s %4s %4s  %-11s %-11s %-11s %-11s %s'
          % ('part', 't', 'n', 'dt', 'cpu', 'swap', 'gpu', 'late'))
    for (part, t), v in rows.items():
        cols = ''
        for i in range(4):
            xs = [r[i] for r in v]
            cols += ' %5.2f %5.2f' % (sum(xs) / len(xs), max(xs))
        late = sum(1 for r in v if r[0] > LATE)
        print('%-10s %4d %4d %s %4d' % (part, t, len(v), cols, late))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('log')
    ap.add_argument('--run', nargs=2, metavar=('PART', 'SECONDS'))
    ap.add_argument('--size', default='1280x720')
    args = ap.parse_args()
    if args.run:
        record(args.log, args.run[0], float(args.run[1]), args.size)
    summarise(args.log)


if __name__ == '__main__':
    main()
