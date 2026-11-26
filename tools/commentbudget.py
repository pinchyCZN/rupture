#!/usr/bin/env python3
"""Comment budget for hand-written code. See README-commentbudget.txt.

    python tools/commentbudget.py            the working tree diff
    python tools/commentbudget.py --reduce   a REDUCTION pass: comment
                                             must go DOWN
    python tools/commentbudget.py --blocks   every oversized comment block
    python tools/commentbudget.py --inline   only the in-body rule

THE IN-BODY RULE IS ABSOLUTE AND IS NOT A BUDGET. A comment indented
inside the body of a function we WROTE - anything under tools/, and any
src/ or sim/ function whose header says OURS - is refused however short
it is and however much code sits beside it. A three-line fix carrying a
paragraph is the failure this exists to stop, and a ratio cannot catch
it: the paragraph is legal as soon as the fix is long enough.

Where the explanation goes instead is the function's OWN header comment,
or docs/. Two things are exempt, both because they cannot live anywhere
else: a provenance marker (ORIGIN, OURS), and the body of a
TRANSCRIBED function - one whose header carries ORIGIN - where a note
about the original's arithmetic is pinned to the line it describes.

THE DEFAULT GATE PERMITS GROWTH AND THAT IS DELIBERATE: net comment may
not exceed net code, so a change that adds code may explain it. It is
the wrong gate for a pass whose whole purpose is to shrink the comment -
under it a sweep can add code and spend the allowance it just earned.
--reduce requires the net to be NEGATIVE and ignores the code delta.
"""

import argparse
import os
import re
import subprocess
import sys

MAX_BLOCK = 12          # lines in one /* */ or // run
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)                       # repo root
CODE_EXT = ('.c', '.h', '.cpp', '.py')

# Hand-written code. Everything outside these roots is generated data,
# third-party, or build output, and a comment budget means nothing there.
#
# sim/ is the Python reconstruction and is under the budget for the same
# reason src/ is: it is a transcription of the binary, so a comment there
# is competing with docs/ for the same explanation. tools/ is NOT - a
# tool's docstring is its only documentation and has nowhere else to go.
SRC_ROOTS = ('src', 'sim')

# Not code we author: generated tables, vendored headers, build output.
SKIP = ('src/third_party/', 'src/generated/', 'sdk/', 'reverse/',
        'decomp/', 'tools/', 'build/')


def ours(path):
    p = path.replace('\\', '/')
    if not p.endswith(CODE_EXT):
        return False
    return not p.startswith(SKIP)


C_DIRECTIVES = ('define', 'include', 'if', 'ifdef', 'ifndef', 'else',
                'elif', 'endif', 'undef', 'pragma', 'error', 'line')


def directive(t):
    """The word after a leading '#', or '' - a C preprocessor test.

    A '#' LINE IS NOT ALWAYS A COMMENT. The rule below reads one as
    Python commentary, and in a .c file that makes every #define a
    comment - which penalises the named constant with an ORIGIN over
    it that rule 18 asks for, and credits nothing for the code it is.
    """
    w = t[1:].lstrip().split('(')[0].split()
    return w[0] if w else ''


def is_comment(line):
    """A comment line in C or Python.

    A BARE LEADING STAR IS NOT ENOUGH to call a line a comment.
    "*ou = x;" is a dereference and "*p++ = c;" is a copy loop, while
    a block comment's continuation is "* " or a star alone. Counting
    the star on its own read every pointer store in a transcribed
    function as comment.

    A Python docstring is not detected and cannot be: its lines are
    indistinguishable from a multi-line string by shape alone, and the
    budget counts lines rather than parsing. So the gate UNDER-counts
    Python comment, which --blocks partly covers by flagging long runs.
    """
    t = line.strip()
    if t.startswith('#'):
        return directive(t) not in C_DIRECTIVES
    if t.startswith(('/*', '//', '*/')) or t.endswith('*/'):
        return True
    return t == '*' or t.startswith('* ')


def is_provenance(t):
    """An ORIGIN or OURS line is METADATA, not commentary.

    Rule 18 REQUIRES one on every function definition in src/, and
    tools/commentcheck.py fails the build without it. A line the
    project mandates cannot also be penalised for existing, and it
    is the one comment that by definition cannot move to docs/ -
    its whole purpose is to sit against the code.

    Only the marker's OWN line is exempt. Prose continuing under it
    counts normally, so nothing can hide behind the marker.
    """
    return (t.startswith('/* ORIGIN') or t.startswith('/* OURS')
            or t.startswith('* ORIGIN') or t.startswith('* OURS'))


# The in-body rule reaches further than the budget does: a tool's
# DOCSTRING has nowhere else to go and is exempt by being unindented,
# but a comment inside a tool's function is commentary like any other.
INLINE_SKIP = ('src/third_party/', 'src/generated/', 'sdk/', 'reverse/',
               'decomp/', 'build/')


def authored(path):
    p = path.replace('\\', '/')
    return p.endswith(CODE_EXT) and not p.startswith(INLINE_SKIP)


def added_with_lines():
    """[(path, line number in the new file, text)] for added lines."""
    out = subprocess.run(['git', 'diff', '-U0'], cwd=ROOT,
                         capture_output=True, text=True).stdout
    path, n, res = None, 0, []
    for line in out.splitlines():
        if line.startswith('+++ b/'):
            path = line[6:]
        elif line.startswith('@@'):
            m = re.search(r'\+(\d+)', line)
            n = int(m.group(1)) if m else 0
        elif line.startswith('+') and not line.startswith('+++'):
            if path and authored(path):
                res.append((path, n, line[1:]))
            n += 1
    return res


def opens_data(lines, idx):
    """Is the definition enclosing line idx a data one?"""
    i = min(idx, len(lines) - 1)
    while i >= 0:
        l = lines[i]
        if (l.strip() and not l[:1].isspace()
                and not l.lstrip().startswith(('/*', '*', '//', '#'))):
            return is_data_definition(l)
        i -= 1
    return False


def enclosing_marker(lines, idx):
    """'ORIGIN' or 'OURS' for the function containing line idx, else None.

    IT HAS TO FIND THE DEFINITION FIRST. Rule 18 puts the marker on
    the function header, but a transcribed body may carry ORIGIN
    comments of its own against individual lines, so the nearest
    marker ABOVE a line is often one of those rather than the
    function's. Walking up to the definition - column zero, not a
    comment - and reading the block above THAT is what distinguishes
    a function we wrote from one we transcribed.

    The walk lands on the lone { that opens a C body in this house
    style, so the signature is one line further up again.
    """
    i = min(idx, len(lines) - 1)
    while i >= 0:
        l = lines[i]
        if (l.strip() and not l[:1].isspace()
                and not l.lstrip().startswith(('/*', '*', '//', '#'))):
            break
        i -= 1
    while i >= 0 and lines[i].strip() == '{':
        i -= 1
    j = i - 1
    while j >= 0 and not lines[j].strip():
        j -= 1
    marker = None
    while j >= 0 and is_comment(lines[j]):
        t = lines[j].strip()
        for mark in ('ORIGIN', 'OURS'):
            if t.startswith(('/* ' + mark, '* ' + mark, '# ' + mark)):
                marker = mark
        j -= 1
    return marker


def in_header_block(lines, idx):
    """Is this comment line part of a block that starts at column 0?

    A C header block opens at column 0 and every line after it begins
    " *", one space in, so indentation alone cannot tell a header's
    continuation from a comment inside a body. The run's FIRST line
    can: if it reaches column 0 the whole run is a header.
    """
    i = min(idx, len(lines) - 1)
    while i >= 0 and is_comment(lines[i]):
        if lines[i].strip() and not lines[i][:1].isspace():
            return True
        i -= 1
    return False


def is_data_definition(line):
    """Does this column-zero line open DATA rather than a function?

    The rule is about function BODIES. A struct's field comments and a
    table's row comments sit at the same indentation as a statement
    and read identically to one, but they document a layout and have
    nowhere else to live - moving them to a header would separate a
    field from its meaning.

    A table's opening line is spotted by its initialiser: "static
    const Thing NAME[] = {" has an = before any bracket, where a
    function definition has its parameter list there instead.
    """
    t = line.strip()
    if t.startswith(('typedef', 'struct', 'union', 'enum')):
        return True
    return '=' in t.split('(')[0]


def report_inline():
    bad = 0
    cache = {}
    for path, n, text in added_with_lines():
        if not is_comment(text) or not text[:1].isspace():
            continue
        if is_provenance(text.strip()):
            continue
        if path not in cache:
            full = os.path.join(ROOT, path)
            try:
                with open(full, encoding='utf-8', errors='replace') as fh:
                    cache[path] = fh.read().splitlines()
            except OSError:
                cache[path] = []
        if in_header_block(cache[path], n - 1):
            continue
        if opens_data(cache[path], n - 1):
            continue
        if enclosing_marker(cache[path], n - 1) == 'ORIGIN':
            continue
        print('%s:%d  in-body comment: %s' % (path, n, text.strip()))
        bad += 1
    if bad:
        print('\n%d in-body comment line(s) in code we wrote. Put it in '
              'the function header or in docs/, or delete it.' % bad)
    return bad


def diff_lines():
    """{path: (added, removed)} for hand-written code in the working tree."""
    out = subprocess.run(['git', 'diff', '-U0'], cwd=ROOT,
                         capture_output=True, text=True).stdout
    path, d = None, {}
    for line in out.splitlines():
        if line.startswith('+++ b/'):
            path = line[6:] if ours(line[6:]) else None
            if path:
                d.setdefault(path, ([], []))
        elif not path:
            continue
        elif line.startswith('+') and not line.startswith('+++'):
            d[path][0].append(line[1:])
        elif line.startswith('-') and not line.startswith('---'):
            d[path][1].append(line[1:])
    return d


def blocks(text):
    """Yield (start_line, length) for each run of comment lines."""
    run = 0
    start = 0
    for n, line in enumerate(text.splitlines(), 1):
        if is_comment(line):
            if not run:
                start = n
            run += 1
        else:
            if run:
                yield start, run
            run = 0
    if run:
        yield start, run


def counts(lines):
    """Comment and code line counts.

    A PROVENANCE MARKER IS NEITHER. Counting it as comment penalises
    a line rule 18 requires; counting it as code would let someone
    buy comment budget by adding markers. It is metadata and sits
    outside the ledger entirely.
    """
    body = [l for l in lines if not is_provenance(l.strip())]
    c = sum(1 for l in body if is_comment(l))
    k = sum(1 for l in body if l.strip() and not is_comment(l))
    return c, k


def report_diff(reduce_mode=False):
    bad = 0
    tc = tk = 0
    touched = 0            # files whose comments the diff actually moves
    for path, (add, rem) in sorted(diff_lines().items()):
        if not path.endswith(CODE_EXT):
            continue
        ac, ak = counts(add)
        rc, rk = counts(rem)
        # NET, so replacing a long comment with a short one counts as
        # the reduction it is rather than the addition it contains.
        c, k = ac - rc, ak - rk
        tc += c
        tk += k
        if not (ac or ak or rc or rk):
            continue
        if ac or rc:
            touched += 1
        if reduce_mode:
            # A file whose comments were not touched at all is neutral -
            # a reduction pass is allowed to carry an unrelated code fix.
            over = (ac or rc) and c >= 0
            flag = '  GREW' if over else ''
        else:
            over = c > max(k, 0)
            flag = '  OVER' if over else ''
        print('%-18s comment %+4d  code %+4d%s' % (path, c, k, flag))
        if over:
            # A HEADER IS MEASURED ON THE TOTAL, NOT ON ITSELF.
            #
            # In a .c the code is the thing and the comment explains
            # it, so comment > code means the explanation outgrew the
            # work. In a .h the code is a SIGNATURE and the comment is
            # the CONTRACT - what a caller must know to use the thing
            # correctly. One line of signature legitimately carries
            # several of contract, and holding a header to the .c
            # ratio asks it to under-document its own interface.
            #
            # The total check below still counts headers, so one
            # cannot become a dumping ground.
            if path.endswith('.h'):
                flag = '  over (header, advisory)'
                print('%-18s   ^ counted in the total only' % '')
            else:
                bad += 1
    if tc or tk:
        print('%-18s comment %+4d  code %+4d' % ('TOTAL', tc, tk))
    if reduce_mode:
        if touched and tc >= 0:
            print('\na reduction pass must REMOVE comment and this one is '
                  '%+d - shorten a block or move it to docs/' % tc)
            bad += 1
    elif tc > max(tk, 0):
        print('\nthis change adds more comment than code - move the '
              'explanation to docs/ and leave a one-line pointer')
        bad += 1
    return bad


def report_blocks():
    n = 0
    for sub in SRC_ROOTS:
        base = os.path.normpath(os.path.join(ROOT, sub))
        for f in sorted(os.listdir(base)) if os.path.isdir(base) else []:
            p = os.path.join(base, f)
            if not os.path.isfile(p):
                continue
            rel = os.path.relpath(p, ROOT).replace('\\', '/')
            if not ours(rel):
                continue
            with open(p, encoding='utf-8', errors='replace') as fh:
                text = fh.read()
            for start, length in blocks(text):
                if length > MAX_BLOCK:
                    print('%s:%d  %d-line block' % (rel, start, length))
                    n += 1
    if n:
        print('\n%d block(s) over %d lines' % (n, MAX_BLOCK))
    return 0            # advisory: the tree is full of them already


# THE FIXTURE'S COMMENT MARKERS ARE ASSEMBLED, NOT WRITTEN. Spelt out,
# its four comment-shaped lines are indistinguishable from real ones in
# a diff and the rule flags this tool's own test data.
_C, _E = '/' + '*', '*' + '/'
TEST_C = '\n'.join((
    '%s OURS. A thing we wrote. %s' % (_C, _E),
    'static void ours_fn(void)',
    '{',
    '    %s an explanation that should not be here %s' % (_C, _E),
    '    x = 1;',
    '}',
    '',
    '%s ORIGIN 0x00123456 Some_DemoRoutine - what the original does,' % _C,
    ' * at length, because this is a transcription. %s' % _E,
    'static void transcribed_fn(void)',
    '{',
    '    %s the original shifts first, which matters %s' % (_C, _E),
    '    y = 2;',
    '}',
    ''))


def selftest():
    """Prove each arm of the in-body rule fires, and each exemption.

    The rule turns on two judgements that are easy to get backwards -
    where a comment run STARTS, and which function encloses it - so
    both are asserted here rather than trusted.

    A header line has no enclosing marker and is never asked for one:
    it sits above its function, so the walk finds the previous one or
    nothing, and in_header_block settles it first.
    """
    lines = TEST_C.split('\n')
    bad = 0
    cases = (
        (3, 'OURS', False, 'body of a function we wrote'),
        (11, 'ORIGIN', False, 'body of a transcription'),
        (0, None, True, 'the OURS header itself'),
        (8, None, True, 'continuation of an ORIGIN header'),
    )
    for idx, want_mark, want_hdr, what in cases:
        mark = enclosing_marker(lines, idx)
        hdr = in_header_block(lines, idx)
        ok = (mark == want_mark and hdr == want_hdr)
        print('%-36s marker=%-6s header=%-5s %s'
              % (what, mark, hdr, 'ok' if ok else 'FAILED'))
        if not ok:
            bad += 1
    data = ['typedef struct {', '    s32 x;      /* a field */', '} T;']
    if opens_data(data, 1):
        print('%-36s %s' % ('a struct field comment is spared', 'ok'))
    else:
        print('%-36s %s' % ('a struct field comment is spared', 'FAILED'))
        bad += 1
    pre = (('#define EA_HOLD 200L', False), ('#include "rupture.h"', False),
           ('#endif', False), ('# a python comment', True), ('#', True))
    for t, want in pre:
        if is_comment(t) != want:
            print('%-36s %r  FAILED' % ('preprocessor vs comment', t))
            bad += 1
            break
    else:
        print('%-36s %s' % ('a #define is code, # is comment', 'ok'))
    deref = ['    *ou = (sn * py + c * px) >> 10;', '    *p++ = c;']
    for d in deref:
        if is_comment(d):
            print('%-36s %s' % ('a dereference read as comment', 'FAILED'))
            bad += 1
            break
    else:
        print('%-36s %s' % ('a dereference is not a comment', 'ok'))
    flagged = [i for i in range(len(lines))
               if is_comment(lines[i]) and lines[i][:1].isspace()
               and not in_header_block(lines, i)
               and enclosing_marker(lines, i) != 'ORIGIN'
               and not is_provenance(lines[i].strip())]
    if flagged != [3]:
        print('rule flagged %s, expected [3]  FAILED' % flagged)
        bad += 1
    else:
        print('%-36s %s' % ('rule flags exactly the one line', 'ok'))
    print('\n%s' % ('selftest FAILED' if bad else 'selftest ok'))
    return bad


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--blocks', action='store_true')
    ap.add_argument('--reduce', action='store_true',
                    help='net comment must be NEGATIVE')
    ap.add_argument('--inline', action='store_true',
                    help='only the in-body rule')
    ap.add_argument('--selftest', action='store_true',
                    help='prove the in-body rule fires')
    a = ap.parse_args()
    if a.selftest:
        sys.exit(selftest())
    if a.blocks:
        sys.exit(report_blocks())
    if a.inline:
        sys.exit(report_inline())
    sys.exit(report_diff(a.reduce) + report_inline())


if __name__ == '__main__':
    main()
