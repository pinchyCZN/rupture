"""
video_encode.py - test_video2/'s 838 PNG frames -> one MPEG-1 file.

    py -3.10 tools/video_encode.py [--ffmpeg PATH] [--check]

Reads src/test_video2/0001.png .. 0838.png (the frames part_video
loads; 0000.png never is - docs/effects/video.txt 1) and writes
src/video/test_video2.mpg, the file src/rupture/core/video.cpp decodes
with pl_mpeg. Video frame k is file k + 1.

ffmpeg comes from --ffmpeg, then $FFMPEG, then the PATH.

THE LUMA MUST STAY FULL RANGE. The frames are grey 0..255 and the
decoder takes the Y plane as the picture, but ffmpeg converts grey to
yuv420p as studio range (16..235) unless told otherwise, and the
mpeg1video encoder converts a full-range input back to studio range
on its own. So the scale filter keeps the range, and setparams then
labels the frames studio range so nothing downstream touches them.

--check decodes the result and compares every frame's Y plane with
its PNG, both through ffmpeg as raw bytes, so no image library is
needed. At -q:v 2 about 94% of pixels come back exact and the mean
error is about 0.1 of a grey level.
"""
import argparse
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'src', 'test_video2')
OUT = os.path.join(ROOT, 'src', 'video', 'test_video2.mpg')
FRAMES = 838
SIZE = 256 * 256


def find_ffmpeg(arg):
    for c in (arg, os.environ.get('FFMPEG'), shutil.which('ffmpeg')):
        if c and os.path.isfile(c):
            return c
    sys.exit('ffmpeg not found - pass --ffmpeg or set FFMPEG')


def run(cmd):
    r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:
        sys.exit(r.stderr.decode('ascii', 'replace'))
    return r.stdout


def encode(ff):
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    run([ff, '-v', 'error', '-y', '-framerate', '30', '-start_number', '1',
         '-i', os.path.join(SRC, '%04d.png'), '-frames:v', str(FRAMES),
         '-vf', 'scale=in_range=pc:out_range=pc,format=yuv420p,'
                'setparams=range=tv',
         '-c:v', 'mpeg1video', '-q:v', '2', '-g', '30', '-bf', '0',
         '-f', 'mpeg', OUT])
    print('wrote %s, %d bytes' % (OUT, os.path.getsize(OUT)))


def check(ff):
    yuv = run([ff, '-v', 'error', '-i', OUT, '-f', 'rawvideo',
               '-pix_fmt', 'yuv420p', '-'])
    png = run([ff, '-v', 'error', '-start_number', '1',
               '-i', os.path.join(SRC, '%04d.png'), '-frames:v', str(FRAMES),
               '-f', 'rawvideo', '-pix_fmt', 'gray', '-'])
    frame = SIZE * 3 // 2
    n = len(yuv) // frame
    if n != FRAMES or len(png) != FRAMES * SIZE:
        sys.exit('frame count: %d decoded, %d expected' % (n, FRAMES))
    exact = total = worst = 0
    for k in range(n):
        a = png[k * SIZE:(k + 1) * SIZE]
        b = yuv[k * frame:k * frame + SIZE]
        for x, y in zip(a, b):
            d = abs(x - y)
            total += d
            exact += d == 0
            worst = max(worst, d)
    cnt = n * SIZE
    print('%d frames: %.1f%% exact, mean error %.3f, worst %d'
          % (n, 100.0 * exact / cnt, total / cnt, worst))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[1])
    ap.add_argument('--ffmpeg')
    ap.add_argument('--check', action='store_true')
    a = ap.parse_args()
    ff = find_ffmpeg(a.ffmpeg)
    encode(ff)
    if a.check:
        check(ff)


if __name__ == '__main__':
    main()
