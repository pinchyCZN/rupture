#!/usr/bin/env python
"""Space indentation -> TABS for indent, SPACES for alignment.

    py -3.10 tools/tabify.py                 convert every C file
    py -3.10 tools/tabify.py src/a.c src/b.h convert just those
    py -3.10 tools/tabify.py --check         report what still has
                                             space indentation
    py -3.10 tools/tabify.py --dry-run       say what would change
    py -3.10 tools/tabify.py --audit         prove the result still
                                             aligns at any tab width

Only src/ and only .c and .h. src/third_party is somebody else's code
and is left alone.

THE NAIVE CONVERSION IS WRONG AND LOOKS RIGHT. Turning every four
leading spaces into a tab also converts the ALIGNMENT of a wrapped
line, so

    z = Rast_TexTri(f, sx, sy,
                    u, v, tex, rows);

becomes five tabs and three spaces instead of one tab and twenty
spaces. At a tab width of four both render identically, which is why
the mistake ships silently and only appears in someone else's editor.

So a line is split in two: TABS to its statement's nesting depth,
then SPACES to reach the column it had. A line is a continuation of
the statement above it - and so keeps that statement's depth - when
it is inside a block comment, when the line above ended in a
backslash, when a parenthesis is still open, or when the line above
did not end a statement and was not an if, while, for, switch, else
or do header.

A PREPROCESSOR DIRECTIVE IS INVISIBLE TO ALL OF THAT. `#ifdef X' ends
no statement and opens no block, so counting it would make the line
after it look like a continuation of it and leave real code space
indented. Its parentheses do not belong to any statement either. So
a `#' line and its backslash continuations update neither the
statement depth nor the paren count, and the C code either side of
the directive joins up across it.

A COMMENT-ONLY LINE AFTER A DIRECTIVE IS PART OF THE DIRECTIVE'S
BLOCK. A run of aligned `#define's whose right-hand comment spills
onto a line of its own puts that comment 39 columns in at file scope
with no statement anywhere near it, and reading its lead as a depth
makes it nine tabs - identical at four, shattered at eight. So it
takes the statement depth, which is zero at file scope and the
enclosing depth inside a function.

BRACE NESTING IS NOT THAT CEILING, and must not be used as one: a
braceless `if (src)' body indents a level without opening a brace, so
clamping to the brace count steals a tab from every line inside it.

INDENTATION SHALLOWER THAN A TAB STAYS SPACES, which is the judgement
call this project makes for big repeating data tables: SHIP_PARTS[] in
ship.c is one-space indented with its columns hand aligned, and a
space indent is enough for a table like that. A one-space lead is zero
tabs by arithmetic, so nothing special is needed to keep it - but
neither should anyone "fix" ship.c to four and re-run this.

LINE ENDINGS ARE PRESERVED PER FILE, because src/ is not uniform -
most of it is LF and a few files are CRLF - and an indentation pass
must not be the thing that changes one. Git normalises either way, so
a rewrite leaves no trace in a diff and the inconsistency is found
later by something else. A file containing any CRLF is written back
entirely CRLF.

THE CHECK THAT MAKES THIS SAFE is that expanding the result at four
columns must reproduce the input byte for byte; nothing is written if
it does not. That catches anything moving. It cannot catch a wrong
split between the tab part and the space part, because both expand
the same - that is what --audit is for.
"""

import os
import sys

TAB = 4
ROOTS = ('src',)
# cgeffects/ holds the release's Cg, not ours
SKIP = ('third_party', 'cgeffects')
EXT = ('.c', '.h', '.cpp', '.glsl')
CONTROL = ('if', 'while', 'for', 'switch', 'else', 'do')


def c_files(args):
    """The files to work on: those named, or every C file we own."""
    if args:
        return list(args)
    out = []
    for root in ROOTS:
        for dirpath, dirnames, names in os.walk(root):
            dirnames[:] = [d for d in dirnames if d not in SKIP]
            for n in sorted(names):
                if n.endswith(EXT):
                    out.append(os.path.join(dirpath, n))
    return out


def read_source(path):
    """Return (text with LF endings, the ending to write back)."""
    raw = open(path, 'rb').read().decode('ascii', 'replace')
    nl = '\r\n' if '\r\n' in raw else '\n'
    return raw.replace('\r\n', '\n'), nl


def write_source(path, text, nl):
    open(path, 'wb').write(text.replace('\n', nl).encode('ascii', 'replace'))


def scan(line, in_comment):
    """Count parens and track /* */ across one line.

    Returns (paren delta, in_comment after this line, code text) with
    comment and literal bodies blanked, so neither a brace in a
    comment nor a parenthesis in a string reaches the caller.

    COMMENTS ARE TESTED BEFORE QUOTES, and that order is the whole
    reason this is one pass. English prose is full of apostrophes -
    "a triangle's fourth slot" - and treating one as a character
    literal swallows the rest of the line, the closing */ with it,
    after which every following line looks like comment.
    """
    out = []
    delta = 0
    i = 0
    n = len(line)
    while i < n:
        c = line[i]
        if in_comment:
            if line.startswith('*/', i):
                in_comment = False
                out.append('  ')
                i += 2
                continue
            out.append(' ')
            i += 1
            continue
        if line.startswith('/*', i):
            in_comment = True
            out.append('  ')
            i += 2
            continue
        if line.startswith('//', i):
            out.append(' ' * (n - i))
            break
        if c == '"' or c == "'":
            q = c
            out.append(' ')
            i += 1
            while i < n:
                if line[i] == '\\':
                    out.append('  ')
                    i += 2
                    continue
                if line[i] == q:
                    out.append(' ')
                    i += 1
                    break
                out.append(' ')
                i += 1
            continue
        if c == '(':
            delta += 1
        elif c == ')':
            delta -= 1
        out.append(c)
        i += 1
    return delta, in_comment, ''.join(out)


def first_word(text):
    t = text.strip()
    i = 0
    while i < len(t) and (t[i].isalpha() or t[i] == '_'):
        i += 1
    return t[:i]


def convert(text):
    """Return (converted text, list of lines needing a human eye)."""
    lines = text.split('\n')
    out = []
    odd = []
    paren = 0
    in_comment = False
    stmt_depth = 0
    comment_depth = 0
    slash_depth = 0
    prev_code = ''
    slash = False
    dir_run = False
    dir_prev = False
    for num, line in enumerate(lines, 1):
        body = line.lstrip(' ')
        lead = len(line) - len(body)
        was_comment = in_comment
        delta, in_comment, code = scan(line, in_comment)

        if not body.strip():
            out.append(line)
            slash = False
            dir_run = False
            dir_prev = False
            continue

        directive = not was_comment and body[:1] == '#'
        if directive:
            dir_run = True
        elif not slash:
            dir_run = False

        prev_open = bool(prev_code) and not prev_code.rstrip().endswith(
            (';', '{', '}', ':'))
        prev_ctl = first_word(prev_code) in CONTROL
        closer = body[:1] in ('}', ')')
        comment_only = not code.strip()
        cont = not closer and (paren > 0
                               or (prev_open and not prev_ctl)
                               or (dir_prev and comment_only))

        if was_comment:
            depth = comment_depth
        elif slash:
            depth = slash_depth
        elif directive:
            depth = lead // TAB
        elif lead == 0:
            depth = 0
            stmt_depth = 0
        elif cont:
            depth = stmt_depth
        else:
            depth = lead // TAB
            stmt_depth = depth
        if in_comment and not was_comment:
            comment_depth = depth

        pad = lead - depth * TAB
        if pad < 0:
            depth = lead // TAB
            pad = lead % TAB
            odd.append((num, line))
        out.append('\t' * depth + ' ' * pad + body)

        slash = code.rstrip().endswith('\\')
        if slash:
            slash_depth = depth
        dir_prev = dir_run
        if not dir_run:
            paren += delta
            if paren < 0:
                paren = 0
            if code.strip():
                prev_code = code
    return '\n'.join(out), odd


def verify(original, converted):
    """Every line must expand back to exactly what came in."""
    a = original.split('\n')
    b = converted.split('\n')
    if len(a) != len(b):
        return 'line count changed'
    for i, (x, y) in enumerate(zip(a, b), 1):
        if y.expandtabs(TAB) != x:
            return 'line %d does not round-trip:\n  in  %r\n  out %r' % (
                i, x, y)
    return None


def needs_work(src):
    """True when converting would change the file.

    A LEADING SPACE IS NOT ENOUGH TO GO ON: alignment is spaces by
    design, and so is the one before a block comment's star, so a
    file can be perfectly converted and still have thousands of
    lines that begin with a space. Running the conversion and asking
    whether anything moved is the only honest test.
    """
    text = '\n'.join(l.expandtabs(TAB) for l in src.split('\n'))
    converted, odd = convert(text)
    return verify(text, converted) is not None or converted != src


def audit(path):
    """Does every alignment line still align at a different tab width?

    A line indented TABS + SPACES claims "I am at my statement's
    depth, then padded out". If its tab count differs from that of
    the line it is padding out from, the two separate the moment the
    editor's tab width is not four. Round-tripping cannot see this;
    both forms expand to the same columns at four.
    """
    rows = open(path, 'rb').read().decode('ascii', 'replace')
    rows = rows.replace('\r\n', '\n').split('\n')
    counts = [0, 0, 0]
    skew = []
    prev_tabs = 0
    prev_num = 0
    for num, line in enumerate(rows, 1):
        if not line.strip():
            continue
        t = 0
        while t < len(line) and line[t] == '\t':
            t += 1
        rest = line[t:]
        pad = len(rest) - len(rest.lstrip(' '))
        body = rest.lstrip(' ')
        if pad and body[:1] == '*':
            counts[0] += 1
        elif pad and t == prev_tabs:
            counts[1] += 1
        elif pad:
            counts[2] += 1
            skew.append((num, prev_num, prev_tabs, t, body[:48]))
        prev_tabs = t
        prev_num = num
    return counts, skew


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    check = '--check' in sys.argv
    dry = '--dry-run' in sys.argv
    show = '--audit' in sys.argv
    bad = 0
    total = [0, 0, 0]
    for path in c_files(args):
        if show:
            counts, skew = audit(path)
            for i in range(3):
                total[i] += counts[i]
            for num, pnum, pt, t, body in skew:
                print('%s:%d  %d tabs here, %d on line %d: %s' % (
                    path, num, t, pt, pnum, body))
                bad += 1
            continue
        src, nl = read_source(path)
        text = '\n'.join(l.expandtabs(TAB) for l in src.split('\n'))
        if check:
            if needs_work(src):
                print('%s: not tab-indented' % path)
                bad += 1
            continue
        converted, odd = convert(text)
        why = verify(text, converted)
        if why:
            print('%s: REFUSED - %s' % (path, why))
            bad += 1
            continue
        if converted == src:
            continue
        for num, line in odd:
            print('%s:%d  look at this one: %r' % (path, num, line[:60]))
        if dry:
            print('%s: would convert' % path)
            continue
        write_source(path, converted, nl)
        print('%s: converted' % path)
    if show:
        print('block comment continuations : %5d' % total[0])
        print('alignment lines, anchored   : %5d' % total[1])
        print('alignment lines, SKEWED     : %5d' % total[2])
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
