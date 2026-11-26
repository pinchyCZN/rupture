#!/usr/bin/env python3
"""
asciify.py -- enforce the project's plain-ASCII documentation rule.

Every byte in a tracked document, source file, or comment must be <= 0x7F.
Prose wraps at 80 columns; table rows may run to 120.

Usage:
    python tools/asciify.py --check [PATH ...]   report, exit 1 if violations
    python tools/asciify.py --fix   [PATH ...]   transliterate + rewrap
    python tools/asciify.py --fix --no-wrap PATH transliterate only

With no PATH, walks DEFAULT_ROOTS - the documents, the tools, and the
generated maps. Reconstructed source under src/ is opt-in by path: --fix
rewraps prose, and a comment aligned against code is not prose.

XLAT is keyed by codepoint integer, not by character. That is deliberate:
this file must itself pass --check, so it may not contain a literal
non-ASCII byte anywhere, table included.

Exempt from rewrapping (but NOT from the ASCII rule):
    - fenced code blocks
    - table rows (first non-space character is '|')
    - indented blocks of 4+ spaces
"""

import argparse
import os
import sys

PROSE_COLS = 80
TAB_WIDTH  = 4      # display width of a TAB in src/
TABLE_COLS = 120

SKIP_DIRS = {".git", "__pycache__", ".vs", ".vscode", "node_modules", "CVS",
             "Debug", "Release", "bin", "obj", "raw"}

# Skipped by RELATIVE PATH, not by basename, because "ghidra" names two
# different things: reverse/ghidra/ is the project database and must not
# be walked, while tools/ghidra/ holds scripts we author and must be.
# reverse/patched/ and reverse/extracted/ hold copies of the release -
# the demo's own files, which rule 8 does not cover.
SKIP_PATHS = ("reverse/ghidra", "reverse/patched", "reverse/extracted")

# Walked when no PATH is given. Everything else is opt-in by path.
# The two top-level markdown files are named explicitly: both are exempt
# from rule 8's no-markdown clause but not from its ASCII one, and
# leaving them off this list meant the two documents everybody reads
# were the ones nothing checked.
DEFAULT_ROOTS = ["docs", "tools", "reverse", "CLAUDE.md", "README.md"]

# Files exempt from the plain-ASCII rule. Nothing qualifies yet; add a
# name here only with a reason, the way wave race exempted its Japanese
# translation table, and never to silence a report.
EXEMPT = set()

# ASCII yes, width no. The generated maps under reverse/exports/ are DATA,
# and a 96-column row is a mangled C++ symbol the XBE actually contains -
# wrapping it would corrupt the thing the file exists to record. The width
# rule belongs to the document format; these are not documents, and each
# one says so in its own header.
DATA_DIRS = ("reverse/exports", "reverse/unpacked")
TEXT_EXT = {".md", ".txt", ".py", ".c", ".h", ".cpp", ".hpp", ".java",
            ".json", ""}

# Codepoint -> ASCII replacement. Extend deliberately; an unmapped
# character is reported, never guessed at.
XLAT = {
    0x2014: " - ",   # em dash
    0x2013: "-",     # en dash
    0x2018: "'",     # left single quotation mark
    0x2019: "'",     # right single quotation mark
    0x201C: '"',     # left double quotation mark
    0x201D: '"',     # right double quotation mark
    0x2026: "...",   # horizontal ellipsis
    0x2192: "->",    # rightwards arrow
    0x2190: "<-",    # leftwards arrow
    0x2194: "<->",   # left right arrow
    0x21D2: "=>",    # rightwards double arrow
    0x00D7: "x",     # multiplication sign
    0x00F7: "/",     # division sign
    0x2264: "<=",    # less-than or equal to
    0x2265: ">=",    # greater-than or equal to
    0x2260: "!=",    # not equal to
    0x2248: "~=",    # almost equal to
    0x00B1: "+/-",   # plus-minus sign
    # Added after a sweep wrote "?" for each of these in ot_sub.c,
    # ot_buoy.c and enmain.c. An UNMAPPED character is reported, but
    # a caller that only reads the last line of output never sees it.
    0x03C0: "pi",    # greek small letter pi
    0x00B2: "^2",    # superscript two
    0x00B3: "^3",    # superscript three
    0x2208: "in",    # element of
    0x00B0: " deg",  # degree sign
    0x00B5: "u",     # micro sign
    0x2714: "[ok]",  # heavy check mark
    0x2713: "[ok]",  # check mark
    0x2718: "[X]",   # heavy ballot x
    0x2717: "[X]",   # ballot x
    0x2022: "*",     # bullet
    0x00A0: " ",     # no-break space
    0x2011: "-",     # non-breaking hyphen
    0xFEFF: "",      # zero width no-break space (BOM)
    # box drawing -> ASCII art
    0x2500: "-", 0x2501: "-",
    0x2502: "|", 0x2503: "|",
    0x250C: "+", 0x250F: "+",
    0x2510: "+", 0x2513: "+",
    0x2514: "+", 0x2517: "+",
    0x2518: "+", 0x251B: "+",
    0x251C: "+", 0x2523: "+",
    0x2524: "+", 0x252B: "+",
    0x252C: "+", 0x2533: "+",
    0x2534: "+", 0x253B: "+",
    0x253C: "+", 0x254B: "+",
    0x2550: "=", 0x2551: "|",
    0x2588: "#", 0x2591: ".",
    0x2592: ":", 0x2593: "%",
}


def transliterate(text):
    """Return (ascii_text, unmapped) -- unmapped is a set of leftover chars."""
    out = []
    unmapped = set()
    for ch in text:
        cp = ord(ch)
        if cp <= 0x7F:
            out.append(ch)
        elif cp in XLAT:
            out.append(XLAT[cp])
        else:
            unmapped.add(ch)
            out.append("?")
    return "".join(out), unmapped


def is_table(line):
    """Table rows and RFC-style +---+ / +===+ borders get the wide limit."""
    s = line.lstrip()
    return s.startswith("|") or s.startswith("+-") or s.startswith("+=")


def splitlines_keep(text):
    """text.split on the newline, without spelling it in a literal."""
    return text.split(chr(10))



def is_border(line):
    """A +---+ / +===+ rule, as opposed to a | row."""
    s = line.strip()
    return (s.startswith("+") and s.endswith("+")
            and len(s) > 1 and set(s) <= set("+-="))


def is_row(line):
    s = line.strip()
    return s.startswith("|") and s.endswith("|") and len(s) > 1


def misaligned(text):
    """Yield (lineno, width, border_width) for every table row whose
    right edge does not line up with the border above it.

    RULE 7 SAYS TABLES ARE SPACE-ALIGNED SO THEY LINE UP IN ANY
    EDITOR, and nothing was checking it. Editing a cell without
    re-padding the row is the easiest mistake in the format to make
    and the hardest to see in a diff - 130 rows across 30 documents
    had drifted before this existed.

    A blank line ends a table, so two tables with no gap between them
    are treated as one. That is what the format means anyway.
    """
    want = None
    for i, line in enumerate(splitlines_keep(text), 1):
        if is_border(line):
            want = len(line.expandtabs(TAB_WIDTH))
        elif is_row(line):
            if want is not None:
                got = len(line.expandtabs(TAB_WIDTH))
                if got != want:
                    yield i, got, want
        elif not line.strip():
            want = None


def doubled_rules(text):
    """Yield the lineno of every +---+ rule sitting directly on top of a
    +===+ rule of the same shape.

    A GENERATOR EMITTED 38 OF THESE ACROSS SIX DOCUMENTS before anyone
    noticed. The pattern is a table helper that closes the header row
    with an ordinary rule and then writes the +===+ separator, so the
    header ends up underlined twice. It reads as a formatting tic
    rather than an error, which is exactly why it survived: every row
    still lines up, so the misaligned() check above passes it.

    Only the identical shape is reported, so a genuine +---+ that
    happens to precede a differently-columned +===+ - two tables run
    together - is left alone.
    """
    lines = splitlines_keep(text)
    for i, line in enumerate(lines):
        nxt = lines[i + 1] if i + 1 < len(lines) else ""
        if (is_border(line) and is_border(nxt)
                and "-" in line and "=" in nxt
                and line.replace("-", "=") == nxt):
            yield i + 1


def wrappable(line):
    s = line.strip()
    if not s or is_table(line):
        return False
    if line.startswith("    ") or line.startswith("\t"):
        return False
    if s.startswith(("#", "===", "---", "***", "|")):
        return False
    return True


def hanging_indent(line):
    """Indent that continuation lines of this line should get."""
    stripped = line.lstrip()
    indent = line[: len(line) - len(stripped)]
    # A blockquote marker must be repeated on every continuation line,
    # unlike a list bullet which becomes plain indentation.
    if stripped.startswith("> "):
        return indent, indent + "> "
    for marker in ("- ", "* ", "+ "):
        if stripped.startswith(marker):
            return indent, indent + "  "
    i = 0
    while i < len(stripped) and stripped[i].isdigit():
        i += 1
    if i and stripped[i : i + 2] == ". ":
        return indent, indent + " " * (i + 2)
    return indent, indent


def wrap_one(line, width):
    """Wrap one logical line, preserving indent and list hanging indent."""
    indent, hang = hanging_indent(line)
    words = line.split()
    if not words:
        return [line]
    lines = []
    cur = indent + words[0]
    for w in words[1:]:
        if len(cur) + 1 + len(w) > width:
            lines.append(cur)
            cur = hang + w
        else:
            cur = cur + " " + w
    lines.append(cur)
    return lines


def rewrap(text, width=PROSE_COLS):
    """Rewrap prose; leave code fences, tables and indented blocks alone."""
    out = []
    in_fence = False
    para = []

    def flush():
        if not para:
            return
        indent, _ = hanging_indent(para[0])
        joined = " ".join(p.strip() for p in para)
        out.extend(wrap_one(indent + joined, width))
        del para[:]

    for line in text.split("\n"):
        if line.lstrip().startswith("```"):
            flush()
            in_fence = not in_fence
            out.append(line)
            continue
        if in_fence or not wrappable(line):
            flush()
            out.append(line)
            continue
        s = line.lstrip()
        if para and (s.startswith(("- ", "* ", "+ ", "> ")) or s[:1].isdigit()):
            flush()
        para.append(line)
    flush()
    return "\n".join(out)


def iter_files(paths, expanded=False):
    """Explicit file arguments are yielded whatever their extension; files
    found by expanding a directory are filtered to TEXT_EXT, so pointing at
    a tree never tries to rewrap a .bin or a .tex."""
    # The expanded guard matters: an empty directory recurses with an
    # empty list, and without it that falls back to DEFAULT_ROOTS and
    # walks the whole tree again, forever.
    if not paths and not expanded:
        paths = [p for p in DEFAULT_ROOTS if os.path.exists(p)]
    for p in paths:
        if os.path.basename(p) in EXEMPT:
            continue
        if os.path.isdir(p):
            if os.path.basename(p) in SKIP_DIRS:
                continue
            if any(s in p.replace("\\", "/") for s in SKIP_PATHS):
                continue
            kids = [os.path.join(p, x) for x in sorted(os.listdir(p))]
            for f in iter_files(kids, expanded=True):
                yield f
        elif os.path.isfile(p):
            if expanded and os.path.splitext(p)[1].lower() not in TEXT_EXT:
                continue
            yield p


def check(paths, ascii_only=False):
    """ascii_only skips the width and table checks.

    Those enforce the document format (rule 3). A translated source file
    must be plain ASCII, but reflowing the original's data tables to 80
    columns would be editing the game code, which rule 7 forbids - so the
    completion test for a source file is this flag, not a full check.
    """
    bad = 0
    for p in iter_files(paths):
        try:
            text = open(p, encoding="utf-8").read()
        except (UnicodeDecodeError, OSError):
            continue
        rel = p.replace("\\", "/")
        wide = ascii_only or any(d in rel for d in DATA_DIRS)
        for i, line in enumerate(text.split("\n"), 1):
            odd = sorted({c for c in line if ord(c) > 0x7F})
            if odd:
                cps = " ".join("U+%04X" % ord(c) for c in odd)
                print("%s:%d: non-ASCII %s" % (rel, i, cps))
                bad += 1
            # Control characters are <= 0x7F and so pass the rule
            # above, but they corrupt a document just as thoroughly.
            # A stray backspace once survived a full --check run.
            ctrl = sorted({c for c in line
                           if ord(c) < 0x20 and ord(c) != 9})
            if ctrl:
                cps = " ".join("U+%04X" % ord(c) for c in ctrl)
                print("%s:%d: control character %s" % (rel, i, cps))
                bad += 1
            # WIDTH IS MEASURED IN DISPLAY COLUMNS, not characters, so
            # a tab counts as the four it occupies. Source files indent
            # with tabs; counting them as one
            # character each would let a deeply nested line run to 110
            # real columns and still pass an 80-column check.
            limit = TABLE_COLS if is_table(line) else PROSE_COLS
            width = len(line.expandtabs(TAB_WIDTH))
            if width > limit and not wide:
                print("%s:%d: %d cols (limit %d)" % (rel, i, width, limit))
                bad += 1
        if wide:
            continue
        for i, got, want in misaligned(text):
            print("%s:%d: table row is %d cols, its border is %d"
                  % (rel, i, got, want))
            bad += 1
        for i in doubled_rules(text):
            print("%s:%d: +---+ rule directly above the +===+ header rule"
                  % (rel, i))
            bad += 1
    print("\n%d violation(s)." % bad)
    return 1 if bad else 0


def fix(paths, wrap=True):
    changed = 0
    for p in iter_files(paths):
        try:
            # newline="" keeps the file's own line endings in the string
            # instead of translating them to \n. Writing with newline="\n"
            # after a universal-newlines read silently converted CRLF
            # source files to LF, turning a few character substitutions
            # into a whole-file diff on 11 of them.
            with open(p, encoding="utf-8", newline="") as fh:
                text = fh.read()
        except (UnicodeDecodeError, OSError):
            continue
        new, unmapped = transliterate(text)
        if unmapped:
            cps = " ".join("U+%04X" % ord(c) for c in sorted(unmapped))
            print("%s: UNMAPPED %s -- wrote '?', add to XLAT" % (p, cps))
        if wrap and os.path.splitext(p)[1].lower() in (".md", ".txt"):
            new = rewrap(new)
        if new != text:
            with open(p, "w", encoding="ascii", newline="") as fh:
                fh.write(new)
            print("fixed %s" % p.replace("\\", "/"))
            changed += 1
    print("\n%d file(s) changed." % changed)
    return 0


def main():
    ap = argparse.ArgumentParser(description="Enforce plain-ASCII docs.")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--fix", action="store_true")
    ap.add_argument("--no-wrap", action="store_true")
    ap.add_argument("--ascii-only", action="store_true",
                    help="bytes only: skip the width and table checks")
    ap.add_argument("paths", nargs="*")
    a = ap.parse_args()
    if a.fix:
        return fix(a.paths, wrap=not a.no_wrap)
    return check(a.paths, ascii_only=a.ascii_only)


if __name__ == "__main__":
    sys.exit(main())
