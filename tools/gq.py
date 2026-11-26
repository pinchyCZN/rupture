#!/usr/bin/env python3
"""Talk to this project's Ghidra server directly over HTTP.

    py -3.10 tools/gq.py who
    py -3.10 tools/gq.py call get_metadata program=ASD-Rapture.exe
    py -3.10 tools/gq.py call list_segments program=ASD-Rapture.exe
    py -3.10 tools/gq.py call disassemble_bytes address=0x21f000 length=64 \
                               program=ASD-Rapture.exe
    py -3.10 tools/gq.py find list_          search the tool names
    py -3.10 tools/gq.py sig  rename_data    what params a tool takes

Why this exists, because it duplicates the ghidra MCP and that is
normally the wrong move:

The MCP bridge honours GHIDRA_MCP_URL above everything else, and a
user-scope config may set it to a port some other project is on. Inside
a session that inherited it, connect_instance("rupture") reports success
and connects to that port anyway. A write issued then lands in the
WRONG DATABASE, which is the one failure here that cannot be undone by
re-reading something.

So this script addresses the server by port, which no environment
variable can redirect, and it refuses to run if the port does not
answer as this project. Use the MCP when `who` says the MCP is pointed
here; use this when it is not, and for anything scripted over many
addresses.

The port is read from DEMO_GHIDRA_URL if set, so a second instance can
be driven without editing this file.
"""

import json
import os
import sys
import urllib.error
import urllib.parse
import urllib.request

# PORT IS NOT AN IDENTITY. GhidraMCP takes the first free port in its
# range, so whichever Ghidra instance launched first gets 8089 and the
# rest follow. Two projects open together swap ports on launch order
# alone. So this scans the range and picks the instance that reports
# EXPECT_PROJECT, which is the only stable handle.
#
# DEMO_GHIDRA_URL overrides the scan when a port is already known.
PORT_RANGE = range(8089, 8100)
URL = os.environ.get('DEMO_GHIDRA_URL', '').rstrip('/')

# The server's own tool descriptions contain non-ASCII - arrows in the
# prose - and a Windows console defaults to cp1252, so printing one
# raised UnicodeEncodeError and took `sig` down. Our own output is ASCII
# by rule 8; this is for everything that comes back from the server.
for _s in (sys.stdout, sys.stderr):
    try:
        _s.reconfigure(encoding='utf-8', errors='replace')
    except (AttributeError, ValueError):
        pass

# The guard. This script is only ever allowed to talk to the rupture
# project, so a stale port, a relaunched Ghidra, or a copied config
# cannot point it at somebody else's database.
EXPECT_PROJECT = 'rupture'

TIMEOUT = 120


def _get(path):
    req = urllib.request.Request(URL + path, method='GET')
    return urllib.request.urlopen(req, timeout=TIMEOUT).read().decode(
        'utf-8', 'replace')


def info():
    return json.loads(_get('/mcp/instance_info'))


def _probe(url):
    try:
        req = urllib.request.Request(url + '/mcp/instance_info', method='GET')
        raw = urllib.request.urlopen(req, timeout=2).read().decode(
            'utf-8', 'replace')
        return json.loads(raw)
    except Exception:
        return None


def check():
    """Find this project's Ghidra and point URL at it. Exits if absent."""
    global URL
    if URL:
        d = _probe(URL)
        if d is None:
            sys.exit('%s does not answer. Is Ghidra running with the MCP '
                     'plugin?' % URL)
        if d.get('project') != EXPECT_PROJECT:
            sys.exit('%s is project %r, expected %r. REFUSING - a write '
                     'here would land in the wrong database.'
                     % (URL, d.get('project'), EXPECT_PROJECT))
        return d

    seen = []
    for port in PORT_RANGE:
        u = 'http://127.0.0.1:%d' % port
        d = _probe(u)
        if d is None:
            continue
        seen.append((port, d.get('project')))
        if d.get('project') == EXPECT_PROJECT:
            URL = u
            return d

    if seen:
        sys.exit('no Ghidra serving project %r. Found: %s\nOpen the rupture '
                 'project in Ghidra, or set DEMO_GHIDRA_URL.'
                 % (EXPECT_PROJECT,
                    ', '.join('%d=%s' % s for s in seen)))
    sys.exit('no Ghidra MCP server answered on ports %d-%d.'
             % (PORT_RANGE[0], PORT_RANGE[-1]))


def schema():
    d = json.loads(_get('/mcp/schema'))
    return {t['path'].lstrip('/'): t for t in d['tools']}


def call(name, params):
    """POST a tool. Query params go in the URL, body params in the JSON."""
    spec = schema().get(name)
    if spec is None:
        sys.exit('no such tool: %s   (try: gq.py find %s)' % (name, name))

    where = {p['name']: p.get('source', 'body') for p in spec['params']}
    unknown = [k for k in params if k not in where]
    if unknown:
        sys.exit('%s does not take %s\nit takes: %s'
                 % (name, ', '.join(unknown), ', '.join(sorted(where))))

    query = {k: v for k, v in params.items() if where[k] == 'query'}
    body = {k: v for k, v in params.items() if where[k] != 'query'}

    url = URL + '/' + name
    if query:
        url += '?' + urllib.parse.urlencode(query)

    data = json.dumps(body).encode('utf-8') if body else b'{}'
    req = urllib.request.Request(
        url, data=data, method=spec.get('method', 'POST'),
        headers={'Content-Type': 'application/json'})
    try:
        return urllib.request.urlopen(req, timeout=TIMEOUT).read().decode(
            'utf-8', 'replace')
    except urllib.error.HTTPError as e:
        return '[HTTP %s] %s' % (e.code, e.read().decode('utf-8', 'replace'))


def main(argv):
    if not argv:
        sys.exit(__doc__)
    cmd, rest = argv[0], argv[1:]

    if cmd == 'who':
        d = check()
        print('url      %s' % URL)
        print('project  %s' % d['project'])
        print('path     %s' % d['project_path'])
        print('pid      %s' % d['pid'])
        for p in d.get('programs', []):
            print('program  %-20s open=%s' % (p['name'], p.get('open')))
        return

    if cmd == 'find':
        check()
        pat = (rest[0] if rest else '').lower()
        for n in sorted(schema()):
            if pat in n.lower():
                print(n)
        return

    if cmd == 'sig':
        check()
        spec = schema().get(rest[0]) if rest else None
        if spec is None:
            sys.exit('no such tool')
        print('%s  [%s]' % (rest[0], spec.get('category', '')))
        print(spec.get('description', ''))
        for p in spec['params']:
            print('  %-22s %-8s %-6s %s'
                  % (p['name'], p.get('type', ''), p.get('source', ''),
                     'required' if p.get('required') else ''))
        return

    if cmd == 'call':
        check()
        if not rest:
            sys.exit('call needs a tool name')
        params = {}
        for kv in rest[1:]:
            if '=' not in kv:
                sys.exit('params are key=value, got %r' % kv)
            k, v = kv.split('=', 1)
            params[k] = v
        print(call(rest[0], params))
        return

    sys.exit('unknown command %r' % cmd)


if __name__ == '__main__':
    main(sys.argv[1:])
