#!/usr/bin/env python3
"""Export the three maps for every program, one directory each.

    py -3.10 tools/export_maps.py                  all known programs
    py -3.10 tools/export_maps.py ASD-Rapture.exe  just one

Wraps tools/ghidra/demo_export_maps.java because driving it by hand gets
two things wrong, and both are silent.

THE PROGRAM SELECTOR DOES NOT REACH A GHIDRA SCRIPT. Every other call in
this project must carry program=NAME and is refused without it, so the
habit is to add it and trust it. run_ghidra_script accepts it and ignores
it: the script's currentProgram is whatever the Ghidra TOOL has active,
so the selector names one program while the export describes another, and
the only clue is the `program:` line the script prints. switch_program
first; that is what this does.

THE THREE FILENAMES ARE FIXED, so one output directory per program -
otherwise the last export silently overwrites the others, and the packed
and depacked images share no addresses at all (CLAUDE.md rule 3).

structmap.py reads these; see its resolve_exports for how it picks one.
"""

import json
import os
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
GQ = [sys.executable, os.path.join(ROOT, 'tools', 'gq.py')]
SCRIPT = 'demo_export_maps.java'
EXPORTS = os.path.join(ROOT, 'reverse', 'exports')

PROGRAMS = ['ASD-Rapture.exe']


def gq(*args):
    r = subprocess.run(GQ + list(args), capture_output=True, text=True)
    return (r.stdout or r.stderr).strip()


def open_programs():
    out = gq('who')
    return [l.split()[1] for l in out.splitlines() if l.startswith('program')]


def install_script():
    """Ghidra resolves scripts from its own directories, not from ours."""
    home = os.environ.get('USERPROFILE') or os.path.expanduser('~')
    dest = os.path.join(home, 'ghidra_scripts')
    if not os.path.isdir(dest):
        os.makedirs(dest)
    src = os.path.join(ROOT, 'tools', 'ghidra', SCRIPT)
    with open(src, 'rb') as f:
        data = f.read()
    with open(os.path.join(dest, SCRIPT), 'wb') as f:
        f.write(data)
    return os.path.join(dest, SCRIPT)


def export(prog):
    outdir = os.path.join(EXPORTS, prog)
    if not os.path.isdir(outdir):
        os.makedirs(outdir)

    gq('call', 'switch_program', 'program=%s' % prog)
    raw = gq('call', 'run_ghidra_script', 'script_name=%s' % SCRIPT,
             'args=%s' % outdir, 'program=%s' % prog)
    try:
        d = json.loads(raw)
    except ValueError:
        return prog, False, raw[:200]
    if not d.get('success'):
        return prog, False, raw[:200]

    body = d.get('console_output', '')
    if '--- SCRIPT OUTPUT ---' in body:
        body = body.split('--- SCRIPT OUTPUT ---')[1].split('===')[0]
    lines = [l.strip() for l in body.strip().splitlines() if l.strip()]

    # The script prints which program it actually ran against. That line
    # is the only check on the selector trap in the module docstring.
    said = ''
    for l in lines:
        if l.startswith('program:'):
            said = l.split(':', 1)[1].strip()
    if said != prog:
        return prog, False, ('ran against %r, not %r - switch_program '
                             'did not take' % (said, prog))
    return prog, True, '  '.join(l for l in lines if ':' not in l[:9])


def main(argv):
    want = [a for a in argv if not a.startswith('-')] or PROGRAMS
    have = open_programs()
    install_script()

    bad = 0
    for prog in want:
        if prog not in have:
            print('%-16s skipped - not open in Ghidra' % prog)
            continue
        name, ok, msg = export(prog)
        print('%-16s %s  %s' % (name, 'ok  ' if ok else 'FAIL', msg))
        bad += 0 if ok else 1
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
