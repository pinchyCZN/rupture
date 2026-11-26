# RUPTURE

**[RUPTURE](https://www.pouet.net/prod.php?which=52931)** by **ASD** is a
Windows PC demo, drawn with OpenGL and Cg shaders and scored with MP3s through
FMOD. It took first place in the PC demo competition at The Gathering 2009.

This repository is the reverse engineering of it in Ghidra, and a
reconstruction of the whole demo in C++98 on OpenGL 3.3 and WebGL2 - not an
emulator, and not a capture. Every part is a transcription of the original's
own code, and its Cg shaders are transcribed to GLSL.

## Reading it

Start with **`docs/overview.txt`** - the demo's own facts, the section map, the
libraries, the nine part classes its RTTI names, and what is open. Then:

```
docs/effects/        one file per part's effect - the heart of it
docs/subsystems/     loaders, timing, music, the part registry
docs/formats/        the data files it reads
docs/README.txt      the documentation conventions
```

Documents are RFC-style plain text. Every claim carries its evidence:
`[verified]` from the code, `[observed]` from the running demo, `[inferred]`,
`[guess]`. An honest gap is recorded as an open question rather than filled in.

## The projects

`src/rupture.sln` (Visual Studio 2017, v141, x86) holds three:

- **rupture** - the reconstruction: the whole demo, all seven parts with the
  music, on Windows and in the browser. It opens with the original's setup
  dialog, and adds a free look, zoom, pause and fast-forward of our own.
- **gfx_test** - the demo's look on one mesh at a time (a cube, the
  motorcycle, the room and its shatter), with switches for each shader and
  the post chain, for studying how the style is made.
- **shim** - a stand-in `glut32.dll` for the ORIGINAL demo: it adds pause,
  mute, a windowed default and a console, and the agent pipe the port is
  compared through.

## Building

You need the original release; it is **not** in this repository. Copy these
from its folder into `src/` (none of them are committed):

```
meshes/       ->  src/meshes/      boat city1 citycrash citywater city_less
                                   cylinders hand helicopter helix hellicrash2
                                   Hellidist hex hex2 hex3 hex4 human kthrio1
                                   motorcycle outputobj room room2 sign tablet
                                   tholos tholos2 trainlong truck
                                   truckdestruction trucklights truck_empty2
                                   walls walls2 (all .obj), try1.sav,
                                   marmaro.jpg
textures/     ->  src/textures/    all of it (7 .jpg and bouno1.png)
music/        ->  src/music/       the .mp3
test_video2/  ->  src/test_video2/ the .png frames
```

The release's other meshes are never loaded. The shaders need nothing: the
GLSL transcriptions are in the repository. The one thing to build is the TV
video - the release's 838 PNG frames become one MPEG-1 file, which needs
[ffmpeg](https://ffmpeg.org) once:

```
py -3.10 tools/video_encode.py --ffmpeg C:/path/to/ffmpeg.exe --check
```

It writes `src/video/test_video2.mpg` and checks every frame against its PNG.
`--ffmpeg` may be left out if `ffmpeg` is on the PATH or `FFMPEG` names it.
The tools are Python 3.10 through the `py` launcher, standard library only.

Then build `src/rupture.sln`, Release. The post-build steps copy the shaders
and the data beside each exe; run `build/rupture/Release/rupture.exe`.

**The browser build** needs [emsdk](https://emscripten.org): run its
`emsdk_env` so `emcc` is on the PATH, or set `EMCC` to its full path. Then,
from a POSIX shell (Git Bash):

```
sh tools/build_web.sh
cd build/web && py -3.10 -m http.server 8000
```

and open `http://localhost:8000`. It preloads the same `src/` files, so the
video and the release data must be in place first. The page has to be served
over http; opened as a file it cannot read its data.

## Running

`rupture.exe` with no arguments opens the setup dialog: resolution, Windowed
(unticked is borderless fullscreen on that monitor), Force 16:9 and Muted.
F1 there lists the keys. Any argument skips it: `--size=WxH`, `--fullscreen`,
`--part=NAME[@T]` to start at a part (`--part=rail@31`). The music starts
muted.

| Key / mouse | Does |
|-------------|------|
| Space | pause and resume |
| `.` | step one frame (held: keeps stepping) |
| F | four times speed, on and off |
| `]` | skip to the next part |
| M, + - | mute and unmute, volume |
| drag | look around; it eases back when let go (paused: it stays) |
| C | hold the look while running too |
| wheel | zoom (Ctrl: finer); middle or right click resets view and zoom |
| Esc | quit (in the browser: leave fullscreen) |

The browser page starts on a click, has its own Muted box, volume slider,
size menu and fullscreen button (or Enter, or a double-click on the picture),
and lists the same keys.

## The shim

The shim runs the original from a copy of the release, never the original
folder:

```
reverse/patched/run/          a copy of the whole release folder
  glut32_orig.dll             the release's glut32.dll, renamed
  glut32.dll                  build/shim/<config>/glut32.dll
```

Rename the copy's `glut32.dll` to `glut32_orig.dll`; from then on every shim
build copies itself in. Run `ASD-Rapture.exe` from that folder: the dialog
opens windowed at 800x600, Space pauses and resumes (the clock and the music
together), M unmutes, and a console carries the demo's output.

The agent pipe - stepping the port and the original on a fixed clock,
dumping frames, reading memory, hooking functions - is documented in
`tools/README-agent.txt`. Working on the reverse engineering itself (Ghidra,
the maps, the rules) is `CLAUDE.md`.

## Licence

Our code and documents are under the MIT licence - see `LICENSE`. The
libraries in `src/third_party/` keep their own: pl_mpeg (MIT), dr_mp3
(public domain or MIT-0) and jpgd (public domain or Apache 2.0).

A personal interoperability and preservation effort on a 2009 demoscene
release. **The original executable and its data are not redistributed** - you
supply your own copy, and the files built from it (the video) are not ours to
share either.
