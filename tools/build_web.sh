#!/bin/sh
# PCDEMOS rupture - the emscripten build.
#
#     sh tools/build_web.sh [rupture] [outdir]
#     sh tools/build_web.sh rupture build/web
#
# The browser build is the full demo only. gfx_test has no page; its
# sources are still compiled here, checked and thrown away, because this
# is their C++98 check too.
#
# RUN THIS EVEN WHEN YOU ARE NOT SHIPPING A WEB BUILD. emcc is clang
# with -std=c++98 -pedantic, which is far stricter than MSVC - and
# MSVC has no C++98 switch at all, so this is THE ONLY REAL C++98
# CHECK (CLAUDE.md rule 16).
#
# The release files a program reads are PRELOADED at the same relative
# paths the Windows build opens, so nothing under src/ knows which host
# it is under. --use-preload-plugins makes the browser decode the
# JPEGs and PNGs while it preloads them; core/file.cpp takes the pixels.
# -s LZ4=1 compresses the package as it is built; the loader decompresses
# each file as it is read, and before a plugin decodes it, so no code
# here sees the difference.
#
# emcc writes only PROG.js, PROG.wasm and PROG.data; the page is a plain,
# tracked HTML file copied beside them: src/rupture/host/index.html as
# index.html. The page and
# host_web.cpp are one host; the web_* functions host_web.cpp exports
# (EMSCRIPTEN_KEEPALIVE) are what the page calls.

set -e

ROOT=$(cd "$(dirname "$0")/.." && pwd)
PROG=${1:-rupture}
OUT=${2:-$ROOT/build/web}

cd "$ROOT"

# The Windows emsdk ships emcc.bat and no extensionless script, and a
# POSIX shell will not resolve one from the other; the .bat is not
# marked executable either, so command -v may not report it. Calling
# it by its full path works, so the last resort is to go and look.
# EMCC=/path/to/emcc overrides the lot.
if [ -z "$EMCC" ]; then
    if command -v emcc >/dev/null 2>&1; then
        EMCC=emcc
    elif command -v emcc.bat >/dev/null 2>&1; then
        EMCC=emcc.bat
    else
        OLDIFS=$IFS
        IFS=:
        for d in $PATH; do
            if [ -f "$d/emcc.bat" ]; then EMCC="$d/emcc.bat"; break; fi
            if [ -f "$d/emcc" ]; then EMCC="$d/emcc"; break; fi
        done
        IFS=$OLDIFS
    fi
fi
if [ -z "$EMCC" ]; then
    echo "emcc not found - install emsdk and run its env script" >&2
    exit 1
fi

# The core and the browser host go into every program; host_win32.cpp
# never does.
SRCS="src/rupture/host/host_web.cpp $(ls src/rupture/core/*.cpp)"
PRELOAD="--preload-file src/rupture/shaders@shaders"

case "$PROG" in
    rupture)
        PAGE=src/rupture/host/index.html
        PAGE_OUT=index.html
        SRCS="$SRCS $(ls src/rupture/demo/*.cpp)"
        for f in meshes/hex4.obj meshes/walls2.obj meshes/walls.obj meshes/room2.obj \
                 meshes/try1.sav meshes/marmaro.jpg textures/bg08.jpg textures/blob41b.jpg \
                 textures/cloudt2.jpg textures/bouno1.png meshes/motorcycle.obj meshes/room.obj \
                 meshes/hex2.obj meshes/kthrio1.obj textures/pattern5.jpg \
                 meshes/hex3.obj meshes/sign.obj meshes/tholos.obj meshes/tholos2.obj \
                 meshes/truck.obj meshes/truck_empty2.obj meshes/truckdestruction.obj \
                 meshes/trucklights.obj meshes/boat.obj meshes/trainlong.obj \
                 meshes/hand.obj meshes/cylinders.obj meshes/human.obj meshes/hex.obj \
                 meshes/helicopter.obj meshes/hellicrash2.obj meshes/helix.obj meshes/Hellidist.obj \
                 meshes/city1.obj meshes/city_less.obj meshes/citywater.obj meshes/citycrash.obj \
                 meshes/tablet.obj meshes/outputobj.obj textures/R0.jpg textures/R1.jpg \
                 textures/theloader.jpg; do
            if [ ! -f "src/$f" ]; then
                echo "missing src/$f - copy it from the release" >&2
                exit 1
            fi
            PRELOAD="$PRELOAD --preload-file src/$f@$f"
        done
        if [ -f src/video/test_video2.mpg ]; then
            PRELOAD="$PRELOAD --preload-file src/video/test_video2.mpg@video/test_video2.mpg"
        fi
        # The MP3's name has spaces, so its folder is preloaded whole.
        if [ ! -d src/music ]; then
            echo "missing src/music - copy the release's music/ folder" >&2
            exit 1
        fi
        PRELOAD="$PRELOAD --preload-file src/music@music"
        ;;
    *)
        echo "no such program: $PROG (rupture)" >&2
        exit 1
        ;;
esac

mkdir -p "$OUT" "$ROOT/build"
# The page shows the loading screen while the data downloads, so the
# image goes beside it as well as into the package.
if [ -f src/textures/theloader.jpg ]; then
    cp src/textures/theloader.jpg "$OUT/theloader.jpg"
fi
cp "$PAGE" "$OUT/$PAGE_OUT"
# The page emcc wrote here before the page was our own file.
rm -f "$OUT/$PROG.html"

# pl_mpeg is C99, so it is compiled on its own, as C; jpgd is C++11,
# so it too gets its own line, with its SSE2 path off.
OBJ="$ROOT/build/web_obj"
mkdir -p "$OBJ"
$EMCC -c src/third_party/pl_mpeg/pl_mpeg.c -std=c99 -O2 -o "$OBJ/pl_mpeg.o"
$EMCC -c src/third_party/jpgd/jpgd.cpp -std=c++11 -O2 -DJPGD_USE_SSE2=0 -o "$OBJ/jpgd.o"

$EMCC $SRCS "$OBJ/pl_mpeg.o" "$OBJ/jpgd.o" \
    -std=c++98 -pedantic -Wall -Wextra \
    -O2 \
    -s USE_WEBGL2=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s EXIT_RUNTIME=0 \
    --use-preload-plugins \
    -s LZ4=1 \
    --js-library src/rupture/core/music.js \
    $PRELOAD \
    -o "$OUT/$PROG.js"

echo "wrote $OUT/$PAGE_OUT and $PROG.js, .wasm, .data"

# gfx_test's C++98 check: compiled, not linked, nothing written.
$EMCC -fsyntax-only -std=c++98 -pedantic -Wall -Wextra src/gfx_test/*.cpp
echo "gfx_test: C++98 check passed"
echo "serve it - a file:// page cannot read the preloaded data:"
echo "  cd $OUT && py -3.10 -m http.server 8000"
