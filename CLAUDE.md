# PCDEMOS/rupture - Windows OpenGL demo reverse engineering

**`README.md` is what this project is and how to build it. This file is
the rules for working in it**, and it is the one document here exempt
from rule 10 - see 10.2.

These rules are adopted from the sister project `n:/DEV/PCDEMOS/relic`
(an Amiga AGA demo). Where a rule's numbering has a gap, the relic rule
had no counterpart on a PC. The numbers are kept because the tools
cite them.

## Project context

Reverse engineering RUPTURE by ASD, a 2009 Windows PC demo, in Ghidra:
naming it, typing it, and reconstructing its visual effects.

**The mission is the EFFECTS, not the whole program.** The window
setup, the loaders, the music playback, the CRT and the third-party
libraries are means to an end. Attribute them, bound them, and leave
them. What we are after is each part's rendering - its GL state, its
shaders, and the maths that drives them.

**One demo, one program.** There is no corpus here - everything in
`docs/` is about RUPTURE, so no fact needs qualifying with which demo
it belongs to.

```
reverse/ghidra/ASD-Rapture.exe   1,212,416 bytes   2009-04-10   the executable
```

The file is named `ASD-Rapture.exe`; the demo is RUPTURE. Quote the
file name as it is and do not "correct" it.

`docs/overview.txt` holds the demo's own facts - the section map, the
libraries, the part classes, the data files it names, and what is
open. **Read it before any effect note.**

### This is 32-bit MSVC C++ on Windows, and that shapes everything

- **Little-endian, flat 32-bit, based at `0x00400000` with no ASLR
  flag** [verified, PE header]. One spelling per address, and the
  Ghidra address is the live address in the running process. DLL
  addresses are NOT - they load wherever Windows puts them.
- **It is COMPILED C++, the opposite of relic's hand-written asm.**
  MSVC with RTTI and a statically linked CRT [inferred, see
  overview.txt 1]. So there IS a compiler runtime to fence off, there
  IS library code to attribute, and there ARE calling conventions the
  decompiler can lean on - `__thiscall` with `this` in `ECX`,
  `__stdcall` for Win32 and GL, `__cdecl` for the CRT.
- **RTTI names the demo's own classes.** `parts` and nine parts -
  `part_start`, `part_game`, `part_video`, `part_motor`, `part_truck`,
  `part_rail`, `part_boat`, `part_city`, `part_end` [verified]. These
  are the demo's own words, as relic.dat's chunk names were relic's.
  Each class's vtable is the map of its methods.
- **A lot of this binary is not ASD's code.** The MSVC CRT and C++
  standard library, a `.3ds` importer (`L3DS`, `LMesh`, ...), a JPEG
  decoder (`jpeg_decoder_stream`), and the import thunks for OpenGL,
  GLU, GLUT, GLEW, Cg, OpenCV 1.1 and FMOD [verified, overview.txt 4].
  **Identify a library function, name it, and move on.** Function ID
  and a source match are cheaper than reading it.
- **The effects are GPU work.** An effect here is GL state plus Cg
  shader programs plus the CPU maths that feeds their parameters - not
  a per-pixel loop. The import names give every GL and Cg call away for
  free; the work is in what the arguments are.
- **The data is outside the exe, and so are the shaders.** Textures,
  meshes, the MP3 and **the Cg shader source** (`cgeffects/`, loaded
  with `cgCreateProgramFromFile`) are plain files in the release
  [verified, overview.txt 1]. The shader text is the demo's own code -
  rule 12 treats it exactly as it treats the exe. `cgeffects/` holds
  333 files, many of them unused variants: the file a part runs is the
  one the code names, never the one whose name looks right.
- **The windowing is NeHe's framework, not GLUT.** Raw Win32 plus WGL,
  a setup dialog with a Windowed option, a PeekMessage loop calling
  `Demo_Frame`. GLUT supplies two cube calls and nothing else.
  `docs/subsystems/startup.txt`.

---

## Hard rules

### 1. Git is read-only

Enforced in `.claude/settings.json`. Read commands (`status`, `log`,
`diff`, `show`, `blame`, `ls-files`, `grep`, ...) are allowed.
Everything that mutates a repo, an index, a working tree, or a remote
is denied - including `add`, `commit`, `push`, `pull`, `fetch`,
`checkout`, `switch`, `restore`, `reset`, `rebase`, `merge`, `stash`,
`clean`, `tag`, `remote`, `config`, `init`.

**Commits, branches, and pushes are the user's job, never the agent's.**
When a change is ready to commit, say so and stop.

> This is a guardrail against accident, not a security sandbox. Do not
> route around it - no `sh -c`, no aliases, no helper script that shells
> out to git. If a git write is genuinely needed, ask for it.

A working-tree change is the only copy of itself until the user
commits it. To undo something, make targeted edits to the working
copy - never reach for a git command to do it.

### 2. Ghidra is the source of truth for the binary

Never guess at an address, offset, field, or signature. Read it out of
the database, or out of the generated maps that come from it (rule 7).
If the database is unreachable, stop and say so - do not substitute
recollection for a lookup.

The database is live and shared with the user's open Ghidra GUI.
Renames, retypes and comments are real edits to their project. Batch
them deliberately; do not churn.

```
project   rupture            reverse/ghidra/rupture in this repository
program   ASD-Rapture.exe
server    discovered by project name - see 2.1, NOT a fixed port
```

#### 2.1. THE PORT TRAP - check it before every session's first write

**A PORT NUMBER DOES NOT IDENTIFY A GHIDRA PROJECT.** More than one can
be open on a machine, and GhidraMCP takes the first free port in its
range, so the instance that launched first gets the lowest and the
others follow. rupture has been observed on 8089, which is also where
relic and other projects have been seen - launch order decides.

A read against the wrong database is a wasted call. **A WRITE AGAINST
THE WRONG DATABASE SILENTLY CORRUPTS SOMEBODY ELSE'S WORK**, and it is
the one mistake here that cannot be fixed by looking something up
again.

So, as the first MCP call of any session:

```
mcp__ghidra__list_instances
```

and read the **`project` field, never the port**. IT MUST SAY
`rupture`. Any other name means the MCP is pointed somewhere else and
is unusable here until that is fixed - do not work around it with
`connect_instance`, which the bridge can report success for while
connecting elsewhere.

`tools/gq.py` applies the same rule mechanically: it scans ports
8089-8099, picks the instance reporting project `rupture`, and refuses
to run against anything else.

```
py -3.10 tools/gq.py who                      prove which database
py -3.10 tools/gq.py find   list_             search the tool names
py -3.10 tools/gq.py sig    rename_data       what a tool takes
py -3.10 tools/gq.py call   get_metadata program=ASD-Rapture.exe
```

`DEMO_GHIDRA_URL` overrides the scan when the port is already known.

**Do not pin a port anywhere in this project.** `.mcp.json` and
`.mcp.json.example` deliberately set no `GHIDRA_MCP_URL`, because a pin
is correct only until the next time two Ghidras start in a different
order, and a stale pin fails by connecting to the wrong database rather
than by erroring. **The MCP servers for this project are defined in
this project's `.mcp.json` and nowhere else** - not in the user-scope
config - so no other project's session inherits them.

#### 2.2. Strict program routing is on - name the program every time

`GHIDRA_MCP_REQUIRE_PROGRAM_SELECTORS=all`. Every call must carry
`program="ASD-Rapture.exe"`. A call that omits it is refused rather
than silently answered from whatever the server had open.

Only one program is open here, so the refusal protects little. Obey it
anyway - it is one argument, and it is what keeps a command copied in
from somewhere else from doing something surprising.

### 3. The import: get it right once

A mis-imported program poisons every address in the project, and the
cost of noticing late is every document written in between.

**The import must be:**

```
loader     Portable Executable (PE)
file       reverse/ghidra/ASD-Rapture.exe
language   x86:LE:32:default, compiler windows (Visual Studio)
image base 0x00400000                the PE header's own [verified]
```

The program is already imported. **The expected section map is in
`docs/overview.txt` section 2**, read from the PE header. Check the
database's memory map against it and correct the document from the
database if they differ - that is the first task of the session that
first reads the database.

Analysis options worth having on, and why:

- **Windows x86 PE RTTI Analyzer** - recovers the class names and
  vtables above. Without it the part classes are anonymous.
- **Function ID** - names statically linked CRT and STL functions, and
  is the cheapest fence around code that is not ASD's.
- **Demangler Microsoft** - for any mangled name that survives.

Do not apply a PDB: there is none, and a PDB from another build would
assert names that do not belong to these addresses.

### 4. Write down what you discover

A finding that exists only in a chat transcript is lost. Anything
learned about a struct, an effect, a subsystem, or a file format goes
into `docs/` **in the same session it was found**. See `docs/README.txt`.

### 5. Separate fact from inference

Every claim in `docs/` carries a tag: `[verified]`, `[inferred]`,
`[observed]`, `[guess]`. `docs/README.txt` section 2 defines them and
says why `[observed]` outranks `[inferred]` for anything visual.

**Renaming a Ghidra symbol is an assertion. Do not assert what you have
not earned.** A name you are not sure of carries `maybe_` and a plate
comment saying what is unconfirmed.

**A type carries its tag in its Ghidra description**, whose first line
is fixed:

```
[verified] origin=004c403f  the CRT entry point
```

The exporter reads that line, so the tag and the origin address reach
`struct_map.txt` without a second copy anywhere. A type with no tag
shows up as `src=untagged` and `structmap.py check` reports it. That
report is the most common way a session ends with the work done and the
provenance lost - clear it before stopping.

### 6. Ghidra is a deliverable, not a reference

**This is the central rule of the project.** Low-level research that
turns up something the database does not have goes back INTO the
database, in the session that found it. A name read off a string, a
signature proved by a disassembly, a field whose width you just
established from a `mov dword ptr` - all of it lands in Ghidra. Leaving
it in a chat reply means the next session rediscovers it, and the
database stays the weakest copy of what we know.

So every analysis task closes with three things done, not one:

| Done | Where |
|---|---|
| **renamed** | the function, its parameters, its locals, the globals it touches |
| **retyped** | a struct applied at every site, not just the one being read |
| **documented** | a plate comment on the function; the note in `docs/` |

A function you read and understood and left as `FUN_00401a40` is work
thrown away. If the understanding is too thin to name it, say that in a
plate comment on it - that is still a deposit.

#### 6.1. Naming conventions

- **Class methods live in the class's namespace**, which RTTI recovery
  creates: `part_motor::Render`, `part_motor::Init`. The class name is
  the demo's own and is never changed; the method name is ours, as
  `Verb` or `Noun_Verb`.
- **Free functions: `Subsystem_Verb`** - `Tex_LoadJpeg`,
  `Music_Start`, `Shader_Bind`. One style, not two.
- **Use the demo's names over descriptive ones.** The RTTI part names,
  the asset file names and any string the code prints are the demo's
  own words. `part_truck` beats `TruckScene`. Where there is no name,
  describe it.
- **Unverified:** prefix `maybe_` and leave a plate comment stating what
  is unconfirmed. Dropping the prefix later is cheap; a confident wrong
  name costs a future session a wrong turn.
- **Globals:** `g_` prefix.
- **Types: a class RTTI names keeps that name exactly** (`part_motor`,
  `parts`). **A type we name ourselves is prefixed `Rupture_`**,
  because Ghidra's Windows, OpenGL and CRT archives load hundreds of
  genuine types into the same database, and an unprefixed `Vertex` or
  `Texture` of ours is indistinguishable from a real one. Anything that
  IS a real SDK type is used as-is and never redefined.
- **Never overwrite an existing meaningful name without saying why**,
  and treat an import name, a Function ID match or an RTTI name as
  meaningful - they came from the binary's own metadata.

**The Ghidra server warns against all of the above and it is wrong for
this project.** It wants `TexLoadJpeg` over `Tex_LoadJpeg`, a verb from
a fixed list as the first word, and `snake_case` labels over `g_`.
Those warnings are advisory, the write succeeds, and the conventions
above are the ones we keep. Ignore the warning; do not rename to
satisfy it, and do not re-litigate this per session.

#### 6.2. PC specifics that will cost a session otherwise

- **`ECX` at a call is usually `this`.** A function that reads `ECX`
  before writing it is a `__thiscall` method; give it the class as its
  first parameter and the decompile stops inventing `in_ECX`.
- **A virtual call is `call [reg + N]` off the first dword of an
  object.** `N / 4` is the vtable slot. Resolve it through the vtable
  RTTI recovered, and name the slot's function, before naming the call.
- **Constructors are where sizes come from.** The `operator new` size
  in the caller is the object's size; the constructor writes the
  vtable pointer first. That is the strongest struct evidence there is.
- **Floats go through x87 or SSE and the decompiler misreads both.**
  `fld`/`fstp` stacks and `movss` spills produce phantom locals and
  wrong types. Check the disassembly before trusting a float
  expression's shape, and keep `float` and `double` as the code has
  them.
- **GL extension calls are indirect.** GLEW reaches extension entry
  points through pointers it fills in at `glewInit`, so they show up as
  calls through a data pointer rather than as named imports. Establish
  which entry point a pointer holds and name it after that entry point.
- **Cg programs are files.** Each one is named by a
  `cgCreateProgramFromFile` call; read the file name and the entry
  point out of that call, then read that file in `cgeffects/`. Record
  the profile (`Cg_Init` picks it, and switches it on ATI). The shader
  is part of the effect, not an asset.
- **Code and data in the CRT and library regions are not ASD's.** Do
  not plate-comment a CRT function's internals; name it and leave it.

#### 6.3. Check the maps AND the comments before naming anything

Grep `reverse/exports/` and `docs/` for an address before assigning a
name, and search the plate comments too. A global's identity is often
already written in the comment of a function nobody has read yet. In
relic that cost three renames of one symbol across three sessions -
once in the wrong direction - while the answer sat in a plate comment
the whole time.

Searching for an address is cheap and belongs *before* reasoning about
what something means, not after.

#### 6.4. Save the database when the work lands

**Edits are live but UNSAVED. Call `save_program` after any batch of
real changes** - a struct created or retyped, functions renamed, plate
comments written, a script applied. Not after every single call; after
the batch that constitutes the finding.

An unsaved database is one closed window away from losing a session's
work, and the loss is silent: Ghidra reopens looking exactly as it did
before, so there is nothing to notice. Nothing else in this project is a
copy of it - `reverse/exports/` is generated FROM the database, so an
export is not a backup.

**Save before re-exporting the maps**, so the text files can never
describe a state the database would not come back as.

The one caution: saving commits whatever the user's own GUI session has
in flight along with ours. That is normally what they want, but if a
batch turns out to be wrong, say so rather than saving over it.

### 7. Struct layout comes from the maps, never from arithmetic

`reverse/exports/ASD-Rapture.exe/` holds three files generated from the
database by `tools/ghidra/demo_export_maps.java`:

| File | What it holds |
|---|---|
| `struct_map.txt` | every struct, union, enum and typedef we defined |
| `symbol_map.txt` | named data globals - address, type, count, size |
| `function_map.txt` | every function carrying a name somebody chose |

They exist because reading a decompiled function means answering "what
is at `+0x18` of this thing" and "what is `0x0050c120`" dozens of times,
and a flat text file answers both with one grep instead of one round
trip apiece.

```
py -3.10 tools/structmap/structmap.py -p ASD-Rapture.exe whatis 0x0050c120
py -3.10 tools/structmap/structmap.py -p ASD-Rapture.exe off  part_motor.camera
py -3.10 tools/structmap/structmap.py -p ASD-Rapture.exe at   part_motor 0x18
py -3.10 tools/structmap/structmap.py -p ASD-Rapture.exe sym  Tex_LoadJpeg
py -3.10 tools/structmap/structmap.py -p ASD-Rapture.exe check
```

- **Consult the maps first.** Grep by hex offset, field name, symbol or
  address.
- **Do NOT calculate offsets by hand** when the type is in the map.
  `off` and `at` print their working, so the arithmetic can be checked
  against the instruction in front of you.
- **Do not hand-edit the maps.** They are generated; the database is the
  source of truth and the next export discards the edit.
- **Re-export after any session that applied a type or a name**, and run
  `structmap.py check`. A map older than the database is the one way
  this system lies to you.

```
py -3.10 tools/export_maps.py                    every known program
py -3.10 tools/export_maps.py ASD-Rapture.exe    just one
```

#### 7.1. THE PROGRAM SELECTOR DOES NOT REACH A GHIDRA SCRIPT

Rule 2.2 makes `program=` mandatory on every call, so the habit is to
add it and trust it. **`run_ghidra_script` accepts it and ignores it.**
A script's `currentProgram` is whatever the Ghidra *tool* has active,
which is normally whatever was imported or clicked last - so the
selector names one program while the export describes another.

It is silent. The export succeeds, prints a plausible count, and the
only clue is the `program:` line the script itself prints.

So: `switch_program` first, then run the script, then check that printed
line against what you asked for. `tools/export_maps.py` does all three
and fails loudly when they disagree. Use it rather than calling
`run_ghidra_script` directly, and assume the same trap for any future
script under `tools/ghidra/`.

With one program open this trap is dormant. It wakes up the moment a
second program is imported - a DLL, a memory dump - so keep using the
wrapper.

#### 7.2. ONE ADDRESS OR OFFSET HAS EXACTLY ONE OWNING SCRIPT

Apply scripts under `tools/ghidra/` **EDIT, they never REBUILD.** Two
ways of getting this wrong have cost real work in relic:

- **A script that constructs a fresh `StructureDataType` and adds it
  with `REPLACE_HANDLER` deletes every field the other scripts
  defined.** Re-running one such script silently destroyed 24 recovered
  fields. Look up the existing type and edit it; only construct when it
  does not exist.
- **Two scripts naming the same address fight.** Re-running an older one
  reverted a confirmed name back to its `maybe_` form. When a later
  script takes over an address or an offset, DELETE the entry from the
  earlier one and leave a comment saying who owns it now.

The symptom is silent: the script reports success and the count it
prints is its own, not the total. Print both - "14 from here, 38 named
in total" is what caught it.

**A missing struct is a stop, not a guess.** An honest `undefined4`
beats a confident wrong field. Map what the evidence supports, leave the
rest undefined, and say at the end of the task which types are missing.

### 8. Plain ASCII in everything we write

Every byte in a document, script, or comment we author must be `<= 0x7F`.
No em dashes, curly quotes, arrows, box-drawing, or ellipsis glyphs.
Write `->`, `-`, `...`, `x`, and `+`/`-`/`|` for diagrams.

```
py -3.10 tools/asciify.py --check    ASCII, width, table alignment
py -3.10 tools/asciify.py --fix      transliterate and rewrap
```

The demo's own strings are not covered by this - they are data, read out
of the binary, and get quoted as they are. Neither is anything under
`reverse/`.

### 9. Documents are RFC-style plain text, not markdown

Documents are `.txt`, all under `docs/`: project-level documents at the
top, effect reports in `effects/`, subsystem reports in `subsystems/`,
struct notes in `structs/`, file formats in `formats/`.

**A cross-reference is relative to `docs/`** - `effects/motor.txt`,
`subsystems/music.txt` - never to the file's own directory.
`tools/checkrefs.py` resolves all of them and is in the closing
checklist. This file is the one exception - Claude Code requires the
name `CLAUDE.md` and the harness renders it.

```
PCDEMOS RUPTURE                                            Analysis Note


                            THE PART REGISTRY


Abstract

   Body text indented three spaces, wrapped at 72 columns.

Table of Contents

   1. First Section
      1.1. A Subsection

1.  First Section

   +--------------+-------+
   | Column       | Count |
   +==============+=======+
   | aligned with | 2542  |
   +--------------+-------+
```

What follows from that:

- **No markdown syntax in a `.txt` file.** No `#` headings, no
  `**bold**`, no backticks, no `*emphasis*`, no `[text](link)`, no
  `|`-delimited markdown tables. If it needs a renderer to look right,
  it is wrong.
- **Tables are drawn with `+---+` borders and space-aligned columns**,
  `+===+` under the header row. Every `|` row must be exactly as wide as
  the border above it.
- **Numbered sections** (`1.`, `1.1.`) with a Table of Contents.
- **Body indented three spaces, wrapped at 72 columns.** Tables may run
  to 120 including the indent, and so may a deeply indented listing.
- Blocks indented four or more spaces are preformatted - address maps,
  disassembly, diagrams - and are never rewrapped.
- Keep an **Open questions** section in every document. An honest gap is
  more useful than a confident guess.

`py -3.10 tools/rfcfmt.py --fix P` renders a markdown-ish draft as RFC
text. It is a one-way converter, not a round-trip formatter - it refuses
a file already in RFC form. Author new documents in RFC form directly,
and if you do use it, **read the output**: it strips inline markup, so a
passage about markdown comes out mangled.

The full conventions, the evidence tags and the templates are
`docs/README.txt`.

### 10. Record facts, not the journey - and record each one once

Documentation says **what the code does and why**. It does not narrate
the search that found it. Do not write "we first thought X, then found
Y" - write Y. No "this took three sessions", no restating what a
previous section established, no summary of the summary.

The one exception is a marked trap: if an approach looks obviously right
and is actually wrong, say so in a sentence. "`ECX` is live at entry, so
this is a method, not a free function" is a fact about the problem. "I
typed it as `__cdecl` first" is a diary entry.

**One fact, one home:**

| What | Where it lives |
|---|---|
| addresses, instruction-level evidence | the Ghidra plate comment |
| struct offsets, strides, sizes | the Ghidra type -> `struct_map.txt` |
| a type's evidence tag and origin address | the Ghidra type description |
| what an effect does, and its maths | `docs/effects/` |
| what a subsystem does | `docs/subsystems/` |
| a file or asset format | `docs/formats/` |
| the demo's own facts, status, open list | `docs/overview.txt` |
| why the C is shaped this way; divergences | the `src/` comment |
| project rules | this file |

Everything else **points at** that home - `see effects/motor.txt 3.2` -
and does not restate it.

**Correct in place; never append.** When a fact changes, edit the
sentence that is now wrong. Leaving it standing with a correction beside
it produces two statements, one of them false, and the reader cannot
tell which.

**AN ANSWERED OPEN QUESTION IS DELETED, not annotated.** Move its fact
into the body and remove the bullet. `ANSWERED:` and `RESOLVED:`
prefixes in an Open questions list are the most common way this rule
gets broken.

#### 10.1. The test, and the check

This rule gets broken anyway, so it has a tool rather than another
paragraph.

```
py -3.10 tools/journalcheck.py          every hit, with its category
py -3.10 tools/journalcheck.py --count  just the totals
```

Every pattern in it was measured against relic's whole `docs/` before
being added. Ones that fired on legitimate prose were dropped rather
than kept with exceptions - a check with known false positives stops
being read.

#### 10.2. The journey has a home, so it stops leaking

**`notes/` is the only place the search may be written down.** It is
gitignored, no check reads it, and it may be deleted at any time.

It exists because the journey previously had nowhere to go. Rule 10
bans it from `docs/` and rule 18 bans it from the code, so in relic it
went into whichever was being written - and once `commentbudget.py`
started biting on source, the leak moved into `docs/`. A note with a
home does not leak.

**A fact in `notes/` is a fact lost.** `checkrefs` does not resolve
its links and nobody greps it. The test is still rule 10's: the
BINARY goes to `docs/`, the history of our work on it goes here.

#### 10.3. Comments must sit against what they describe

```
py -3.10 tools/commentcheck.py             every finding
py -3.10 tools/commentcheck.py --selftest  prove each rule fires
```

`journalcheck.py` polices what a comment SAYS. This polices where it
SITS and whether it still points at anything:

| Finding | What it means |
|---|---|
| **orphaned** | a block comment with nothing under it - something got inserted between it and what it described |
| **stacked** | two block comments in a row - a second explanation bolted beside the first, not replacing it |
| **dead-origin** | an `ORIGIN` address no export carries any more |
| **bare-address** | an address in a comment that is in no export, so a reader cannot check the claim |

**An orphaned comment is the characteristic AI failure here** and it
accrues silently: each insertion is locally reasonable and the
wreckage only shows up when someone reads the file top to bottom.

The address checks are the stronger half. An `ORIGIN` is the only
part of a comment that survives a rename, so a stale one is worse
than no comment - and a bare address in no export means rule 6 was
skipped: the finding was written into a comment and never into the
database.

**Run `--selftest` after touching the tool.** A linter that has never
been seen to fire is not a linter.

#### 10.4. Why there is no journal database

A store of comments keyed by file and function, with the code left
bare and a sweep at the end, has been considered and rejected. It
would make things worse in four ways:

- **Distance causes staleness; it does not cure it.** A comment
  beside the code is at least seen when the code changes. One in a
  database never is.
- **It breaks `ORIGIN`.** The address is the link back to the
  binary and has to be AT the code - rule 18.
- **It deletes the forcing function.** `commentbudget.py` works
  because comments cost lines in a file. A database is unbounded,
  which is the condition under which more gets written, not less.
- **Dangling rows are the orphan problem, moved somewhere nobody
  looks.**

The staging instinct behind it is sound, and `commentbudget.py
--reduce` is where it lives: a sweep that happens continuously
instead of one deferred to an end that never arrives.

**THE TEST A SENTENCE MUST PASS: does it describe the BINARY, or the
history of our work on the binary?** A reader who has never seen a
previous session should not be able to tell that there were any.

| Banned | Write instead |
|---|---|
| "this was called `Scene_Draw` for several sessions" | "`part_boat::Render` draws the water pass" |
| "ANSWERED: `caust00.jpg` is a caustics frame" | delete the bullet; state it in the body |
| "the first string sweep missed them" | ".rdata holds 38 asset file names" |
| "that cost four wrong names" | nothing - delete it |

Both columns carry the same fact. The left also carries us.

**CLAUDE.md itself is exempt and nothing else is.** A rule needs its
rationale, and "this cost three renames of one symbol" is what stops
the next person undoing it. That licence does not extend to
`docs/`.

### 11. Running the demo

**The agent MAY run RUPTURE**, and should want to - this is a visual
artifact and `[observed]` evidence is the only kind that settles what
an effect looks like. It is a native Windows program, so it runs
directly; there is no emulator in the way.

**The full release is at `e:/PCgames/PCDEMOS/ASD/rupture/`** - the exe
(byte-identical to ours), its DLLs and its data folders. Only the exe is
in `reverse/ghidra/` so far. The demo loads its data by relative path,
so it runs from a folder holding the whole release.

**It runs in a window without patching**: tick "Windowed" in its setup
dialog. Fullscreen is the default and calls `ChangeDisplaySettingsA`,
which is what rearranges the desktop. `docs/subsystems/startup.txt` 6.

What the permission to run does not extend to:

- **THE ORIGINAL FILES ARE STOCK AND STAY STOCK.** Never write to
  anything under `reverse/ghidra/` - not to patch, not to "fix" a
  header, not to drop a DLL in. A tool that transforms an original
  writes to `reverse/patched/`, `reverse/extracted/` or `%TEMP%`,
  never over its input.
- **A run produces `[observed]` evidence, not `[verified]`.** A
  screenshot says what it looked like; it does not say what the code
  does.
- A prediction that would need a run to confirm stays tagged as a
  prediction until the run happens. Do not quietly promote it.
- Say what you ran and how, so the result can be reproduced.

#### 11.1. Live reads and the debugger

**Reads need no debugger.** `OpenProcess` plus `ReadProcessMemory`
works on a running process with nothing attached, cannot stop a
thread, and costs the demo nothing. With no ASLR, a Ghidra address is
the address to read. JSRF's `n:/DEV/JSRF/tools/jsrf_snap.py` is the
pattern: a per-title tool that resolves a pointer chain and dumps or
diffs a struct, annotating offsets from `struct_map.txt`.

**The debugger is pybag/dbgeng, and it is NOT in this project.** It
lives in `n:/DEV/xbdbg/` with its own git repository: a long-lived
HTTP server on 8099. Start it with `sh n:/DEV/xbdbg/debugger/restart.sh`.
The ghidra MCP bridge carries the `debugger_*` tools, which proxy to
it via `GHIDRA_DEBUGGER_URL` in this project's `.mcp.json`; they are
inert unless the server is running. There is no separate MCP server
for it. Attach explicitly, by name:

```
POST /debugger/attach   {"target": "ASD-Rapture.exe"}
```

**Read `n:/DEV/xbdbg/docs/debugger.txt` section 4 before arming
anything.** Most of its rules are dbgeng's and apply here unchanged:
killing an attached server kills the target (4.8), `attached: false`
is not proof a kill is safe (4.12), do not attach in order to read
(4.13). Stop it with `POST /debugger/shutdown` or `restart.sh`, never
`Stop-Process`.

**THE CXBX-SPECIFIC RULES ARE A TRAP ON A NATIVE TARGET.** xbdbg
ignores access violations in the engine (`sxi av`, its 4.5) because
Cxbx faults thousands of times by design. RUPTURE does not: an access
violation in a native process is a real fault, and ignoring it hides
the one event worth breaking on. Until xbdbg can attach without that
setting, treat a debugger session here as unable to catch a crash.

**Fixes belong UPSTREAM in `n:/DEV/xbdbg`, never in a copy here**, and
changing that repository is the user's call - ask first.

**A demo runs on a clock.** Suspending it at a breakpoint will move
the music and the timeline relative to each other [inferred]. Arm as
few things as possible, and get the value out of a plain read first.

#### 11.2. PROBING THE PORT: THE DEBUG BUILD, THROUGH THE AGENT

**Every live probe of the reconstruction goes through the agent, and
the agent is ONLY in the Debug build** - `build/rupture/Debug/rupture.exe
--agent`. Release has no agent and refuses the flag. Always start it
with:

```
py -3.10 tools/agentctl.py launch
```

which refuses an exe older than its sources and a map older than the
exe. **Rebuild Debug, not just Release, after every change to
`src/`**, or `launch` stops you - and if you find yourself reading
screenshots of the Release build to judge a change, stop: that is the
wall-clock path the agent replaced. The reason this rule is loud: in
the 64k project an agent spent a long stretch probing a stale build
without noticing.

In agent mode the clock is a fixed step and the music is off, so
`RUN_TO part=motor t=5.2476 exact` lands on the same frame every run,
and `dump F.png` is that frame. Use hooks and `READ` instead of
compiling temporary printfs or harnesses into the project - `READ
addr=g_demo.ctx.eye` prints a struct field by field from the PDB, so
no offset is worked out by hand. The
original answers the same verbs through the shim (`--shim`), so the
two can be read side by side at the same part time.
`tools/README-agent.txt` is the reference.

### 12. Never fake the look

**Match the original's CODE, not a capture of it.** When a reconstructed
effect looks wrong next to the real demo, the fix is to find the code
that produces the right result - never to add something that produces
the right pixels.

**WE DO NOT INVENT CODE TO MATCH VISUALS.** Banned, however tempting:
hardcoding a constant, colour or timing that "looks right" instead of
reading it out of the binary; writing a shader that approximates what
the screen shows instead of transcribing the Cg program the demo
binds; inventing a blend mode, a fog colour or a camera path to stand
in for one we have not located; tuning a magic number until the output
matches a screenshot.

This rule carries particular weight here because **the assets are
ordinary files** - JPEGs, `.3ds` meshes, MP3s - which makes the
temptation specific: it is easy to load a texture onto a mesh with a
plausible blend and call the part understood. How the demo uses the
file is what the code says it is. Read the code.

`[observed]` evidence is for deciding what to go and read, and for
catching a wrong colour. It is never a licence to implement the
observation directly.

### 13. Python is `py -3.10`

Run scripts through the launcher: `py -3.10`, which finds the
interpreter wherever it is installed. Do not assume `python` is on
PATH. Ask before assuming a pip install; nothing beyond the standard
library is required by anything in `tools/` today. (`pybag` is
installed for `py -3.10` and is used only by `n:/DEV/xbdbg`.)

### 14. Scratch files go to the temp directory

Test exports, one-off scripts, and debug output go to the session
scratch directory or `%TEMP%`, resolved via the environment variable -
never a folder literally named `%TEMP%` inside the repo. Repo paths are
for persistent artifacts: `reverse/exports/*.txt`,
`reverse/extracted/*`, `docs/*.txt`, `src/`.

`structmap.py` honours `DEMO_EXPORTS` so a trial export can be checked
in a scratch directory before it replaces the real one.

---

## The reconstruction

**`src/` IS THE RECONSTRUCTION. `tools/` IS THE ARCHAEOLOGY BENCH.**
They are split by the KIND of work, not by stage, and neither feeds
the other. Rules 15 to 20 apply to everything in `src/`, including
test programs.

`src/rupture.sln` is hand written and tracked. **`rupture` is the
reconstruction**: C++98 on OpenGL 3.3 core and WebGL2, built for
Windows by the solution and for the browser by `tools/build_web.sh`.
Its `core/` is shared by every program that draws.

`gfx_test` is the teaching program: the demo's basic effects and look
on single meshes (a cube, the motorcycle, one room mesh), with
switches, for someone who wants to see how the style is made. It shows
effects, never whole scenes. It runs on `rupture/core` and the GLSL
transcriptions, as the demo does.

`src/meshes/`, `src/textures/`, `src/music/` and `src/test_video2/`
hold files copied from the release and are gitignored; the post-build
steps copy them beside the executables. `src/video/` is made from
`test_video2/` by `tools/video_encode.py` and is gitignored for the
same reason. `src/cgeffects/`, also gitignored, is the demo's Cg
source to read the transcriptions against; no build reads it.
`src/third_party/` holds libraries copied in unmodified (pl_mpeg,
dr_mp3, jpgd); the tools skip it.

The second project is `shim`, a stand-in `glut32.dll` that patches the
ORIGINAL demo as it loads - the live-control tool (pause, mute, a
console, a clean exit, an 800x600 windowed default, an agent pipe
for reads, writes and hooks, and in time seeking). It
runs only from `reverse/patched/run/`, a copy of the release with the
real DLL renamed `glut32_orig.dll`; never from the release or
`reverse/ghidra/`. It verifies the exe's bytes before it patches. The
addresses it uses are `docs/subsystems/clock.txt` 5 and
`docs/subsystems/startup.txt` 2 and 5.

**EFFECTS GO STRAIGHT INTO `src/`. There is no Python prototype
stage.** Python in `tools/` is for cracking formats and reading the
live process - the work Python is best at - and its results land in
`src/` and `docs/`.

**The reconstruction is C++98 plus OpenGL 3.3 core / WebGL2.** The
original is MSVC 2005 C++ (msvcr80, msvcp80): classes, virtual
methods, RTTI, std::vector and std::string, C stdio. The port is
written in that period's C++ and nothing later. There is no SDL: a
host is a single file - `host_win32.cpp` makes a window and a GL 3.3
core context, `host_web.cpp` a WebGL2 canvas - and calls the core.

**This is a REIMAGINING, not an emulation.** Running the original
already reproduces RUPTURE exactly and there is no reason to write
another. The judgement calls are the point - but the line is narrow:
**enhance the sampling of the demo's own data, never substitute data
of our own.** Rule 12 still bans inventing a colour, a shader or a
constant.

Decisions taken by the user:

- **The Cg programs are transcribed to GLSL**, one `.glsl` file per
  `cgeffects/` file, version 330 core on the desktop and 300 es in
  the browser from the same text. The Cg runtime cannot run in a
  browser. Each file names the Cg file it came from.
- **The fixed-function GL the original uses** - glBegin/glEnd,
  display lists, the matrix stacks, glColor, the texture matrix as an
  object matrix - **goes through `core/`'s compatibility layer**,
  whose calls keep the original's names and order, so a transcribed
  routine still reads like the listing.
- **Music: the release's MP3 decoded by dr_mp3 and played through
  WinMM on Windows, Web Audio in the browser** (`core/music`, one
  file split by `__EMSCRIPTEN__`, with `core/music.js` as the
  browser half). The demo clock follows it as the original's follows
  FMOD. It starts MUTED - M unmutes, + and - step the volume - so a
  test run is silent.
- **The video (test_video2) is one MPEG-1 file decoded by pl_mpeg**
  into one texture as the frame changes, instead of 838 PNGs as 838
  textures (`core/video`). The luma plane is the frame; the encoder
  keeps it full range. Nothing loads it unless a program asks.

### 15. A transcription, not an impression

Rule 12 applies hardest of all here, because a window is exactly where
faking it is easiest and most tempting.

- **One function per Ghidra function, carrying the database's name.**
  A method stays a method of the class RTTI names - `part_motor::Frame`
  is `part_motor::Frame`; a free function keeps its `Subsystem_Verb`
  name.
- **Every routine's header comment opens with `ORIGIN` and the
  address**, so any number in the file can be checked against the
  listing.
- **Transcribe the form the code uses, not the form the maths reduces
  to.** If the code computes a rotation as three separate sin/cos
  products, `src/` writes those products and `docs/` records the
  identity. The identity is a description, not a licence to
  substitute.
- **Keep the numeric types.** `float` stays `float` and `double` stays
  `double`; an `int` truncation from `__ftol` stays a truncation; a
  `short` that wraps keeps wrapping. MSVC's x87 code can compute at
  higher precision than the declared type - where that shows, note it.
- **GL calls stay GL calls**, with the same arguments in the same
  order, and with `GL_` enum names rather than raw numbers. A state
  change the original makes is made, even if it looks redundant.
- **Where a value is not known, refuse.** No placeholder colour that
  happens to look like the capture. Draw it visibly wrong and say so in
  the window title. A plausible value would be a rule 12 violation
  wearing a proof-of-concept hat.
- **Anything ours is labelled ours.** A viewer, a free camera or a
  scene skip that is not in the demo says so.
- **Load and decode once, at startup.** A frame must touch no file.

### 16. C++98, and the target is readable code

**C++98, as MSVC 2005 wrote it.** Classes, virtual methods, the STL
containers and C stdio. Nothing from C++11 or later: no `auto`,
`nullptr`, lambdas, `override`, range-for, `<cstdint>`, `std::array`,
smart pointers or brace initialisation. Fixed-width types come from a
project header. MSVC has no C++98 switch, so **`tools/build_web.sh`
IS THE ONLY REAL C++98 CHECK** - emcc with `-std=c++98 -pedantic` -
and it is run even when nobody is shipping a web build. The C
programs (`shim`, `src/agent`) stay C89.

**TABS INDENT, SPACES ALIGN** in every `.c`, `.cpp`, `.h` and `.glsl`.
One tab per nesting level, then spaces out to the column a wrapped
parameter list or continued statement lines up under, so it holds at
any tab width. `py -3.10 tools/tabify.py` checks and fixes it.

The goal is **code that reads like the effect**, not code that reads
like a decompiler. The decompiler's output is the starting point and
no more:

- Reconstruct the maths. A reciprocal multiply is a divide and should be
  written as one, with the original's form noted if the rounding
  differs.
- **A class the demo has stays a class**, with the RTTI name and the
  virtual methods in the original's vtable slot order. A field keeps
  the offset it has in the Ghidra type where the layout matters, and
  says it in a comment where it does not.
- **A Cg program is part of the transcription.** Its GLSL file says
  which `cgeffects/` file it is, keeps the live statements in their
  order, and drops only what the Cg file overwrites before use - each
  drop noted once at the top.
- Keep the original's numeric widths and overflow behaviour where the
  effect depends on them.

### 17. Import whole functions, not slices

**One C function per Ghidra function, carrying the database's name.**
Start from the decompiler's C and fix it up. Do not transcribe only the
part of a function the current task happens to need - a half-imported
function looks finished and is the hardest kind of gap to find later.

Inlined library code - an STL container's growth, a `std::string`
copy - is not ASD's function. Replace it with the plain C it amounts
to, and say so where it matters.

### 18. Comments: `docs/` is the default, `src/` is the exception

**The failure mode this rule exists to stop: a one-line fix carrying
five lines of comment.** That is not documentation, it is a changelog
written into the code.

Comment **whole functions**, not individual edits. A function gets a
header comment saying what it does. A three-line fix inside it gets
**zero** comments unless it is one of these:

| Keep | Why |
|---|---|
| **`ORIGIN` + the address** | address first, name second - names drift, addresses do not |
| **`OURS`** | a routine with no counterpart in the demo |
| **a landmine** | looks obviously right, is wrong. State the PROBLEM as a fact, never the story |
| **a deliberate divergence** | so nobody "fixes" it back |
| **`[open]` / not implemented** | what is missing and what it would take |
| **a magic constant's provenance** | the address it was read from, because rule 12 depends on being able to check it |
| **a non-obvious WHY** | something reading the code cannot give you |

Everything else goes to `docs/` with a one-line pointer, or nowhere. In
particular, delete on sight: the dev journal ("we first thought X",
"fixed the crash by"); restatement of the code; our own debug toggles;
inline disassembly listings, since the `ORIGIN` address IS the listing;
and anything stale, which is the highest priority of all because a
stale comment actively misleads where a missing one merely omits.

**EVERY FUNCTION DEFINITION IN `src/` SAYS WHICH IT IS.** A
transcription opens with `ORIGIN` and the address it came from; a
routine of ours opens with `OURS`. There is no third case, and
`tools/commentcheck.py` fails on one.

The point is not tidiness. Without it a reader cannot tell whether a
piece of arithmetic is the demo's or an invention, and rule 12 turns
on exactly that distinction. `ORIGIN` goes on the DEFINITION, not
only on the declaration in a header - someone reading the `.c` needs
the provenance in front of them.

**`FUN_` and `DAT_` in a `src/` comment are a reference, not a stale
name.** Do not "fix" `FUN_00401a40` to whatever the database now calls
that address. The name carries the address, and the address is the only
thing that survives - lose the `.rep` and a friendly name points at
nothing while `FUN_00401a40` still points at the binary. Writing both is
fine; writing the friendly name *instead* is the loss.

The gate, because the criteria above are a judgement and judgement is
generous toward one's own fresh work:

```
py -3.10 tools/commentbudget.py            net comment <= net code
py -3.10 tools/commentbudget.py --reduce   a sweep: comment must DROP
py -3.10 tools/commentbudget.py --blocks   runs over 12 lines
py -3.10 tools/commentbudget.py --inline   no in-body comments in OURS code
```

Run the first before calling any change done. Going over it is the
signal to move the explanation to `docs/`, never to delete something
true. See `tools/README-commentbudget.txt` for what it is blind to.
It reads `git diff`, so it measures against the last commit.

> **The surrounding file is not the standard.** A dense neighbour reads
> as house style; it is evidence of the problem, not licence.

### 19. Discoveries flow BOTH ways

**The database and `src/` describe the same program. A fact learned on
one side is recorded on the other, in the same session.**

The failure this prevents is easy to walk into: work moves to the port,
a struct in `src/` turns out to be wrong, the agent goes back to Ghidra,
finds the missing field, fixes the C and not the database - and from
then on the two disagree with nothing saying which is right.

| Learned while... | Also required |
|---|---|
| fixing `src/` against the binary | apply the field, name or type in Ghidra |
| reading Ghidra for a port question | write it into `docs/` AND the code |
| a struct gains or loses a member | update the Ghidra struct to match |
| a function's real name is settled | rename it in the database too |

This does not mean `src/` must mirror the database's layouts. The
reconstruction may hold a different, simpler struct, as long as the
difference is deliberate and written down. What is banned is the two
drifting apart by accident.

### 20. The core knows nothing about a host

`src/` issues GL calls into a context it is handed and does nothing
else - no window calls, no input polling, no file paths outside one
loader module. A HOST owns the window, the GL context, the input and
the clock, and calls the core once per frame. `host_win32.cpp` and
`host_web.cpp` are the two hosts.

**Shape every part as `frame(part, tick)`, never a loop.** The original
already is: `WinMain` loops and calls `Demo_Frame` once per idle pass,
which calls slot 0 of each part [verified]. Keep that shape. Only the
host loops.

**EFFECTS ARE RESOLUTION-PARAMETRIC, and the rule that keeps that
honest is: scale the pixel geometry, never the table domains.** Where
the demo indexes a table or a texture by a screen-derived value,
divide by the scale BEFORE the lookup, so every index stays in the
range the demo uses. A render-target size the demo hard-codes is a
constant with an address like any other.

The demo's own resolution stays exact and is the reference every other
size is checked against.

---

## Repository layout

```
rupture/
  README.md              what this is, and how to build it
  CLAUDE.md              this file - the rules, for an agent working here
  .mcp.json              this project's MCP servers  [not committed]
  .mcp.json.example      the same, with paths as placeholders
  .claude/settings.json  permissions (read-only git)
  notes/                 the journey - gitignored, rule 10.2
  docs/                  everything we discover - RFC-style plain ASCII .txt
    README.txt             doc conventions, evidence tags, templates
    overview.txt           THE DEMO'S facts, sections, status, open list
    effects/               one file per part's effect  <- the mission
    subsystems/            one file per subsystem
    structs/               one file per reconstructed type
    formats/               file and asset formats
  reverse/
    ghidra/                Ghidra project and the originals  [not committed]
      ASD-Rapture.exe
      rupture.gpr / rupture.rep/
    patched/               derived copies  [not committed]
    extracted/             carved data  [not committed]
    exports/               GENERATED from the database - see rule 7
      ASD-Rapture.exe/
        struct_map.txt / symbol_map.txt / function_map.txt
  tools/
    gq.py                  find and talk to the right Ghidra - rule 2.1
    agentctl.py            the agent pipe's client: the port, or --shim
    pdbtypes.py            the port's types and field paths, from its PDB
    maptypes.py            the original's, from the exported maps
    shimctl.py             agentctl.py --shim
    sym.py                 symbol <-> address, from the exported maps
    export_maps.py         the maps - rule 7.1
    checkrefs.py           every doc cross-reference resolves
    journalcheck.py        no dev-journal writing in docs  [rule 10]
    commentcheck.py        comments anchored, addresses live [rule 10.3]
    asciify.py             ASCII and width rule: --check / --fix
    tabify.py              tab indent, space alignment - .c/.cpp/.h/.glsl
    video_encode.py        test_video2/*.png -> src/video/test_video2.mpg
    rfcfmt.py              markdown draft -> RFC text
    commentbudget.py       no more comment than code  [rule 18]
    README-commentbudget.txt
    README-agent.txt       the agent pipe, part jumps, profiling
    ghidra/
      demo_export_maps.java  the database -> the three maps
    structmap/
      structmap.py           query and validate the maps
  src/                   THE RECONSTRUCTION - C++98 + OpenGL 3.3 / WebGL2
    rupture.sln            hand written, tracked; VS2017 (v141), Win32
    rupture/               THE RECONSTRUCTION, C++98
      core/                  shared by every program that draws
        glapi                  GL 3.3 entry points (Windows) / GLES3 (web)
        gl1                    the fixed-function GL the original calls
        shader                 the demo's Cg layer, carried on GLSL
        post                   render targets, the post chain, Math_*
        obj                    .obj loader, face normals, Vec4 and matrices
        track                  the key track and Track_Eval
        shard                  walls.obj's shards: make, bake, replay
        camera                 the camera object: set, init, lerp
        path                   the recorded path a .sav file holds
        file                   the one file and image loader
        video                  the TV's video: one MPEG-1, one texture
        music                  the MP3: dr_mp3 + WinMM, or Web Audio
      demo/                  Demo_Init, Demo_Frame, the parts base, part_video,
                             maybe_Scene_DrawLit
      host/                  host_win32.cpp, host_web.cpp, host.h,
                             agent_port.cpp - the port's agent (Debug),
                             index.html - the browser page, rupture.rc
      shaders/               GLSL transcriptions of cgeffects/, ff.*
    gfx_test/              the teaching program, C++, on rupture/core
    shim/                  glut32.dll stand-in that patches the original
      shim.c                 OURS - forwards, byte check, timer hook, keys
      agent.c                OURS - the shim's verbs on src/agent's pipe
    agent/                 agent_core.c - the pipe, hooks and verbs the
                           shim and the port share (rule 11.2)
    cgeffects/             the Cg sources the GLSL transcribes  [not committed]
    meshes/                the demo's meshes the port loads  [not committed]
    textures/              the demo's textures in use  [not committed]
    test_video2/           the demo's video frames  [not committed]
    music/                 the demo's MP3  [not committed]
    video/                 test_video2.mpg, made from them  [not committed]
    third_party/           pl_mpeg, dr_mp3 and jpgd, unmodified, and the
                           files that build them
  build/                 MSBuild output  [not committed]
```

Outside this repository, and used by it:

```
n:/DEV/xbdbg/            the pybag/dbgeng debugger server - rule 11.1
n:/DEV/PCDEMOS/relic/    the sister project these rules came from
```

## Working method

Anything run more than twice becomes a script in `tools/`, with a header
comment saying what it does and how to run it. Ghidra scripts go in
`tools/ghidra/` and are run as described in rule 7.

### Closing a session

```
py -3.10 tools/gq.py who                        rule 2.1 - the right DB
py -3.10 tools/asciify.py --check
py -3.10 tools/checkrefs.py                     doc refs still resolve
py -3.10 tools/journalcheck.py                  rule 10 - facts, not us
py -3.10 tools/commentcheck.py                  rule 10.3 - anchored, live
save_program program=ASD-Rapture.exe            rule 6.4 - FIRST
py -3.10 tools/export_maps.py
py -3.10 tools/structmap/structmap.py -p ASD-Rapture.exe check
py -3.10 tools/commentbudget.py                 once src/ has code
```

The save comes before the export, so the maps can never describe a state
the database would not come back as.

Then say what was renamed, retyped and documented, and what is still
open. If `check` reports an untagged type, the session is not finished.

## Legal note

A personal interoperability and preservation effort on a 2009
demoscene release. The original executable and its data are not
redistributed. Notes and any reconstructed source are our own work
product.
