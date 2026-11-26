"""Compile and link the port's GLSL on this machine's GL driver.

There is no standalone GLSL validator here, so this makes a hidden
window and a GL context (ctypes, WGL), and compiles every shader the
way core/shader.cpp does: its own #version header first - 330 core for
the desktop and the 300 es one for the browser, both read out of
core/shader.cpp so they cannot drift - then the file.

    py -3.10 tools/glcheck.py                 every pair the port loads
    py -3.10 tools/glcheck.py VERT+FRAG ...   these pairs (paths or names)
    py -3.10 tools/glcheck.py -v              the logs of passing ones too

With no arguments the pairs come from the Shader_LoadPair and
Shader_LoadFragmentOnly calls under src/rupture and src/gfx_test,
"cgeffects/x.cpp" naming src/rupture/shaders/x.glsl as the loader does.
A fragment-only post pass links with shaders/ff.vert, the vertex stage
the port binds when the vertex profile is off. A name alone means
src/rupture/shaders/<name>.glsl.

THE 300 es CHECK IS THE DESKTOP DRIVER'S, NOT A BROWSER'S. A driver that
accepts "#version 300 es" through its ES compatibility is stricter than
nothing and looser than WebGL2; the web build in a browser is the real
test. Exit status 1 if anything fails.
"""
import ctypes
import glob
import os
import re
import sys
from ctypes import wintypes as W

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHADERS = os.path.join(ROOT, 'src', 'rupture', 'shaders')
LOADER = os.path.join(ROOT, 'src', 'rupture', 'core', 'shader.cpp')
FF_VERT = 'ff.vert'

PAIR = re.compile(r'Shader_LoadPair\s*\(\s*\w+\s*,\s*"([^"]+)"\s*,'
                  r'\s*"([^"]+)"', re.S)
FRAG = re.compile(r'Shader_LoadFragmentOnly\s*\(\s*\w+\s*,\s*"([^"]+)"', re.S)
GL_FRAGMENT_SHADER, GL_VERTEX_SHADER = 0x8B30, 0x8B31
GL_COMPILE_STATUS, GL_LINK_STATUS = 0x8B81, 0x8B82


def headers():
    """The two #version headers stage_compile prepends, from its source."""
    text = open(LOADER).read()
    m = re.search(r'#ifdef __EMSCRIPTEN__\s*static const char \*head = (.*?);'
                  r'\s*#else\s*static const char \*head = (.*?);', text, re.S)
    if not m:
        sys.exit('ERR cannot find the #version headers in %s' % LOADER)

    def c_string(expr):
        parts = re.findall(r'"((?:[^"\\]|\\.)*)"', expr)
        return ''.join(parts).encode().decode('unicode_escape').encode()
    return [('330', c_string(m.group(2))), ('es', c_string(m.group(1)))]


def glsl_path(name):
    """A demo path or a bare name -> the transcription's file."""
    if os.path.isfile(name):
        return name
    base = os.path.basename(name)
    if base.endswith('.cpp'):
        base = base[:-4]
    if not os.path.splitext(base)[1]:
        base += '.glsl'
    return os.path.join(SHADERS, base)


def loaded_pairs():
    """Every (vertex, fragment) the port's sources load, deduplicated."""
    pairs = []
    srcs = (glob.glob(os.path.join(ROOT, 'src', 'rupture', '**', '*.cpp'),
                      recursive=True)
            + glob.glob(os.path.join(ROOT, 'src', 'gfx_test', '*.cpp')))
    for path in sorted(srcs):
        text = open(path, errors='replace').read()
        for v, f in PAIR.findall(text):
            pairs.append((glsl_path(v), glsl_path(f)))
        for f in FRAG.findall(text):
            pairs.append((glsl_path(FF_VERT), glsl_path(f)))
    seen, out = set(), []
    for p in pairs:
        if p not in seen:
            seen.add(p)
            out.append(p)
    return out


def pixel_format():
    """PIXELFORMATDESCRIPTOR, with the fields this does not set as bytes."""
    fields = ([('nSize', W.WORD), ('nVersion', W.WORD), ('dwFlags', W.DWORD),
               ('iPixelType', ctypes.c_ubyte), ('cColorBits', ctypes.c_ubyte)]
              + [('f%d' % i, ctypes.c_ubyte) for i in range(13)]
              + [('iLayerType', ctypes.c_ubyte), ('bReserved', ctypes.c_ubyte),
                 ('dwLayerMask', W.DWORD), ('dwVisibleMask', W.DWORD),
                 ('dwDamageMask', W.DWORD)])
    return type('PFD', (ctypes.Structure,), {'_fields_': fields})


class Gl(object):
    """A legacy WGL context on a hidden window - enough to compile."""
    def __init__(self):
        u32, g32 = ctypes.windll.user32, ctypes.windll.gdi32
        self.gl = gl = ctypes.windll.opengl32
        u32.CreateWindowExA.restype = W.HWND
        u32.CreateWindowExA.argtypes = [
            W.DWORD, ctypes.c_char_p, ctypes.c_char_p, W.DWORD, ctypes.c_int,
            ctypes.c_int, ctypes.c_int, ctypes.c_int, W.HWND, W.HMENU,
            W.HINSTANCE, W.LPVOID]
        u32.GetDC.restype = W.HDC
        u32.GetDC.argtypes = [W.HWND]
        PFD = pixel_format()
        hwnd = u32.CreateWindowExA(0, b'STATIC', b'glcheck', 0, 0, 0, 16, 16,
                                   None, None, None, None)
        hdc = u32.GetDC(hwnd)
        pfd = PFD()
        pfd.nSize = ctypes.sizeof(PFD)
        pfd.nVersion = 1
        pfd.dwFlags = 0x25              # draw to window, OpenGL, double buffer
        pfd.cColorBits = 32
        g32.ChoosePixelFormat.argtypes = [W.HDC, ctypes.POINTER(PFD)]
        g32.SetPixelFormat.argtypes = [W.HDC, ctypes.c_int, ctypes.POINTER(PFD)]
        fmt = g32.ChoosePixelFormat(hdc, ctypes.byref(pfd))
        g32.SetPixelFormat(hdc, fmt, ctypes.byref(pfd))
        gl.wglCreateContext.restype = ctypes.c_void_p
        gl.wglCreateContext.argtypes = [W.HDC]
        gl.wglMakeCurrent.argtypes = [W.HDC, ctypes.c_void_p]
        if not gl.wglMakeCurrent(hdc, gl.wglCreateContext(hdc)):
            sys.exit('ERR no GL context')
        gl.wglGetProcAddress.restype = ctypes.c_void_p
        gl.wglGetProcAddress.argtypes = [ctypes.c_char_p]
        gl.glGetString.restype = ctypes.c_char_p
        U, I, P = ctypes.c_uint, ctypes.c_int, ctypes.POINTER
        V, S = ctypes.c_void_p, ctypes.c_char_p
        self.CreateShader = self.fn(b'glCreateShader', U, U)
        self.ShaderSource = self.fn(b'glShaderSource', None, U, I, P(S), V)
        self.CompileShader = self.fn(b'glCompileShader', None, U)
        self.GetShaderiv = self.fn(b'glGetShaderiv', None, U, U, P(I))
        self.GetShaderInfoLog = self.fn(b'glGetShaderInfoLog', None, U, I, V, S)
        self.DeleteShader = self.fn(b'glDeleteShader', None, U)
        self.CreateProgram = self.fn(b'glCreateProgram', U)
        self.AttachShader = self.fn(b'glAttachShader', None, U, U)
        self.LinkProgram = self.fn(b'glLinkProgram', None, U)
        self.GetProgramiv = self.fn(b'glGetProgramiv', None, U, U, P(I))
        self.GetProgramInfoLog = self.fn(b'glGetProgramInfoLog', None,
                                         U, I, V, S)
        self.DeleteProgram = self.fn(b'glDeleteProgram', None, U)

    def fn(self, name, res, *args):
        addr = self.gl.wglGetProcAddress(name)
        if not addr:
            sys.exit('ERR the driver has no %s' % name.decode())
        return ctypes.WINFUNCTYPE(res, *args)(addr)

    def version(self):
        return '%s, %s' % (self.gl.glGetString(0x1F01).decode(),
                           self.gl.glGetString(0x1F02).decode())

    def compile(self, head, path, stage):
        sh = self.CreateShader(stage)
        text = (ctypes.c_char_p * 2)(head, open(path, 'rb').read())
        self.ShaderSource(sh, 2, text, None)
        self.CompileShader(sh)
        ok = ctypes.c_int(0)
        self.GetShaderiv(sh, GL_COMPILE_STATUS, ctypes.byref(ok))
        log = ctypes.create_string_buffer(8192)
        self.GetShaderInfoLog(sh, 8192, None, log)
        return sh, bool(ok.value), log.value.decode(errors='replace').strip()

    def link(self, vs, fs):
        p = self.CreateProgram()
        self.AttachShader(p, vs)
        self.AttachShader(p, fs)
        self.LinkProgram(p)
        ok = ctypes.c_int(0)
        self.GetProgramiv(p, GL_LINK_STATUS, ctypes.byref(ok))
        log = ctypes.create_string_buffer(8192)
        self.GetProgramInfoLog(p, 8192, None, log)
        self.DeleteProgram(p)
        return bool(ok.value), log.value.decode(errors='replace').strip()


def main(argv):
    verbose = '-v' in argv
    args = [a for a in argv if a != '-v']
    if args:
        pairs = []
        for a in args:
            if '+' not in a:
                sys.exit('ERR %s is not VERT+FRAG' % a)
            v, f = a.split('+', 1)
            pairs.append((glsl_path(v), glsl_path(f)))
    else:
        pairs = loaded_pairs()
    missing = [p for pair in pairs for p in pair if not os.path.isfile(p)]
    gl = Gl()
    print('GL %s' % gl.version())
    fails = 0
    for path in sorted(set(missing)):
        print('MISSING %s' % os.path.relpath(path, ROOT))
        fails += 1
    for v, f in pairs:
        if v in missing or f in missing:
            continue
        for name, head in headers():
            vs, vok, vlog = gl.compile(head, v, GL_VERTEX_SHADER)
            fs, fok, flog = gl.compile(head, f, GL_FRAGMENT_SHADER)
            lok, llog = gl.link(vs, fs) if vok and fok else (False, '')
            gl.DeleteShader(vs)
            gl.DeleteShader(fs)
            ok = vok and fok and lok
            fails += not ok
            if ok and not verbose:
                continue
            print('%-4s %-3s %s + %s' % ('OK' if ok else 'FAIL', name,
                                         os.path.basename(v),
                                         os.path.basename(f)))
            for label, text in (('vertex', vlog), ('fragment', flog),
                                ('link', llog)):
                if text:
                    print('     %s: %s' % (label,
                                           text.replace('\n', '\n     ')))
    print('%d pair(s), 2 dialects each, %d failure(s)' % (len(pairs), fails))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
