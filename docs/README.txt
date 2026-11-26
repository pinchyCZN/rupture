PCDEMOS RUPTURE                                            Analysis Note


                       DOCUMENTATION CONVENTIONS


Abstract

   The format and the evidence rules for everything under docs/.
   CLAUDE.md rules 4, 5, 7, 8 and 9 are the short form; this is the
   long form, with the templates.

   Read it before writing a new document. The checks in section 9 are
   not optional and they are cheap.

Table of Contents

   1. The format
   2. Evidence tagging
   3. Addresses
   4. One fact, one home
   5. The effect note
   6. The struct note
   7. The subsystem and format notes
   8. Record facts, not the journey
   9. Before calling a document done

1.  The format

   Documents are .txt, plain ASCII, laid out like an old RFC. CLAUDE.md
   is the single exception, because Claude Code requires that name and
   the harness renders markdown.

   What that means concretely:

   o No markdown syntax in a .txt file. No hash headings, no asterisks
     around a word for emphasis, no backtick spans, no bracket-paren
     links, no pipe-delimited markdown tables. If it needs a renderer
     to look right, it is wrong.
   o Numbered sections, 1. and 1.1., with a Table of Contents.
   o Body text indented three spaces and wrapped at 72 columns.
   o Tables drawn with +---+ borders and space-aligned columns, +===+
     under the header row. Every | row must be exactly as wide as the
     border above it. Tables may run to 120 columns including the
     indent.
   o Blocks indented four or more spaces are preformatted - address
     maps, disassembly, diagrams - and are never rewrapped.
   o Every byte <= 0x7F. Write ->, -, ..., x, and +/-/| for diagrams.
     No em dashes, curly quotes, arrows or box-drawing glyphs.

   The banner is the project name left, the document class right, to
   column 72:

    PCDEMOS RUPTURE                                            Analysis Note

   Two tools enforce the format:

    py -3.10 tools/asciify.py --check    ASCII, width, table alignment
    py -3.10 tools/asciify.py --fix      transliterate and rewrap
    py -3.10 tools/rfcfmt.py  --fix P    render a markdown draft as RFC

   rfcfmt.py is a one-way converter, not a round-trip formatter. It
   refuses a file already in RFC form. Author new documents in RFC
   form directly; use rfcfmt only to convert a draft, and read the
   result - it strips inline markup, so a passage ABOUT markdown comes
   out mangled.

   The table-alignment check exists because editing a cell without
   repadding the row is the easiest mistake in this format to make and
   the hardest to see in a diff. --fix does not repair those; widen or
   rewrap the cell by hand.

   The demo's own strings are not covered by the ASCII rule. They are
   data, read out of the binary, and get quoted exactly as they are.

2.  Evidence tagging

   Every claim carries a tag. Reverse engineering is mostly inference,
   and a document that does not separate what was read from what was
   reasoned is a document nobody can audit later.

   +------------+----------------------------------------------------------+
   | Tag        | What it means                                            |
   +============+==========================================================+
   | [verified] | read directly out of the binary or the Ghidra DB         |
   +------------+----------------------------------------------------------+
   | [inferred] | reasoned from verified evidence. State the evidence      |
   +------------+----------------------------------------------------------+
   | [observed] | seen running, in a capture, or read live from the        |
   |            | process                                                  |
   +------------+----------------------------------------------------------+
   | [guess]    | a hypothesis worth testing. Never build on one silently  |
   +------------+----------------------------------------------------------+

   [observed] is empirical but weaker than [verified]: a screenshot
   says what it looked like, not what the code does. It is stronger
   than [inferred] for anything visual, because no amount of static
   reading tells you which way a camera turns or how a blend reads on
   screen.

   The thing [observed] is FOR is deciding what to go and read, and for
   catching a wrong colour. It is never a licence to implement the
   observation directly - see CLAUDE.md rule 12.

   A rename in Ghidra is an assertion at [inferred] or better. Where it
   is not, the name carries maybe_ and the plate comment says what is
   unconfirmed.

   A string in the binary is [verified] as a STRING, not as a
   description of the code that uses it. "textures/caust00.jpg" is a
   verified string and a [guess] about which part loads it until the
   reference is read.

3.  Addresses

   ASD-Rapture.exe is a flat 32-bit little-endian PE image based at
   0x00400000 with no ASLR flag [verified, the PE header], so a Ghidra
   address is also the live address in the running process.

   o Write the Ghidra address as the database spells it: 0x004c403f.
     Eight hex digits, lower case, 0x prefix.
   o Name the section when it helps a reader place something -
     ".rdata" or ".data" - but the flat address is the primary form
     and the one a grep has to find.
   o A FILE offset is not an address. Where a document gives one, say
     so: "file 0x0f1800". The section table in overview.txt holds the
     mapping.
   o An address inside a DLL is not an image address. Name the module
     - "opengl32!glBegin", "cg.dll+0x1234" - and never write it bare.
     DLLs can load anywhere; only the exe is fixed.

   An offset into a data file (a .3ds mesh, a texture) is a third kind
   of number again. Write it with the file name - "R0.jpg+0x40" - and
   never as a bare hex value.

4.  One fact, one home

   The same fact in two files means one of them goes stale and only the
   other gets corrected.

   +------------------------------------------------------+---------------------------------------------+
   | What                                                 | Where it lives                              |
   +======================================================+=============================================+
   | addresses, instruction-level evidence, byte findings | the Ghidra plate comment                    |
   +------------------------------------------------------+---------------------------------------------+
   | struct offsets, sizes, strides                       | the Ghidra type, exported to struct_map.txt |
   +------------------------------------------------------+---------------------------------------------+
   | the evidence tag and origin address for a type       | the Ghidra type description                 |
   +------------------------------------------------------+---------------------------------------------+
   | what an effect does, and its maths                   | docs/effects/                               |
   +------------------------------------------------------+---------------------------------------------+
   | what a subsystem does                                | docs/subsystems/                            |
   +------------------------------------------------------+---------------------------------------------+
   | a file or asset format                               | docs/formats/                               |
   +------------------------------------------------------+---------------------------------------------+
   | the demo's own facts, section map, status, open list | docs/overview.txt                           |
   +------------------------------------------------------+---------------------------------------------+
   | why the C is shaped this way; divergences            | the src/ comment                            |
   +------------------------------------------------------+---------------------------------------------+
   | project rules                                        | CLAUDE.md                                   |
   +------------------------------------------------------+---------------------------------------------+

   Everything else points at that home - see effects/<name>.txt 3.2 -
   and does not restate it. A constant the code multiplies by keeps its
   number and its address inline; it does not also need a table in
   docs/ repeating it.

   Correct in place; never append. When a fact changes, edit the
   sentence that is now wrong. Leaving it standing with a correction
   beside it produces two statements, one of them false, and the reader
   cannot tell which.

5.  The effect note

   docs/effects/<name>.txt, one per visual effect. This is the
   project's product and the only document type whose length is
   allowed to be generous, because the maths is the point.

   Name the file after the demo's own name for the part. The binary's
   RTTI names nine part classes - part_start, part_game, part_video,
   part_motor, part_truck, part_rail, part_boat, part_city, part_end
   [verified, RTTI type descriptors] - so the motor part's note is
   motor.txt, not bikes.txt. Describe only where there is no name.

   What it must answer, in this order: what you see, how the frame is
   drawn, what the inner computation is, where the constants came from,
   and what is still missing.

    PCDEMOS RUPTURE                                            Analysis Note


                                 <PART>


    Abstract

       part_<name> vtable at 0x00xxxxxx, render method at 0x00xxxxxx.
       Assets loaded: the file names, with the address of each string.

       One paragraph on what the viewer sees.

    Table of Contents

       1. What it draws
       2. The GL state
       3. The inner computation
       4. Tables and constants
       5. Reconstruction notes
       6. Open questions

    2.  The GL state

       The passes, the render targets, the blend and depth state, the
       Cg programs bound and their parameters - each with the address
       of the call that sets it. An effect here is mostly GL state and
       shader maths, not a pixel loop; record both halves.

    3.  The inner computation

       +-------+-------------+----------+--------------------------------+
       | Thing | Where       | Evidence | Notes                          |
       +=======+=============+==========+================================+
       | 0.35  | 0x00xxxxxx  | verified | the float the camera speed is  |
       |       |             |          | multiplied by                  |
       +-------+-------------+----------+--------------------------------+

    4.  Tables and constants

       Every magic number gets a row and an address. A constant with
       no address is a rule 12 violation waiting to happen: it is the
       thing somebody will later tune to match a screenshot.

   The Evidence column is why this document exists rather than just the
   code. Keep it doing that work.

6.  The struct note

   docs/structs/<name>.txt, one per reconstructed type. The Ghidra type
   is the layout's home, so this document is for what the layout does
   not say: what the thing IS, who allocates it, how its fields relate.

    PCDEMOS RUPTURE                                            Analysis Note


                              RUPTURE_<TYPE>


    Abstract

       Size: 0x24 [verified] - from the operator new at 0x00xxxxxx.
       Ghidra type applied: yes.

       One paragraph on what the thing is.

    1.  The layout

       +--------+------+----------+------------+-------------------------+
       | Offset | Size | Type     | Confidence | Notes                   |
       +========+======+==========+============+=========================+
       | +0x00  |   4  | void **  | verified   | vtable; RTTI names the  |
       |        |      |          |            | class                   |
       +--------+------+----------+------------+-------------------------+

    2.  Open questions

   The layout table repeats what struct_map.txt already holds, and that
   is the one sanctioned duplication in this project: the offsets in
   the document carry the per-field EVIDENCE, which the type cannot.
   Keep the Notes column doing that work. A row that just restates the
   field name is a row to delete.

   For a C++ class, say where the size came from - the operator new
   argument in the constructor's caller is the strongest evidence
   there is - and which vtable slot each method sits in.

   Anything that IS a genuine Windows, OpenGL, Cg or CRT type is used
   from Ghidra's archives and gets no note here. Document ours.

7.  The subsystem and format notes

   docs/subsystems/<name>.txt: what it does, the entry points with
   addresses, the types it owns, how it talks to the rest of the demo,
   and what is still not understood.

   docs/formats/<name>.txt is the same shape for a file or asset
   format, with the record layout and the size formulas.

   Proportion matters, and it cuts the other way from section 5. A
   subsystem note should be shorter than the code it describes. If a
   section is longer than the function it documents, it is too long.
   Prefer a table to a paragraph, a formula to a description of a
   formula, and silence to a restatement.

   Third-party code is the clearest case. The MSVC runtime, the C++
   standard library, the L3DS mesh importer, the JPEG decoder, FMOD,
   Cg and OpenCV are somebody else's work, not ASD's and not an
   effect. A note on one records which library it is, where its entry
   points are, and stops.

8.  Record facts, not the journey

   Documentation says what the code does and why. It does not narrate
   the search that found it.

   Do not write "we first thought X, then found Y" - write Y. No "this
   took three sessions", no "an earlier note said", no restating what a
   previous section established, no summary of the summary, no mention
   of which tool was used.

   The one exception is a marked trap. If an approach looks obviously
   right and is actually wrong, say so in a sentence and say why.

    py -3.10 tools/journalcheck.py          every hit, with its category

   Keep an Open questions section in every document. An honest gap is
   more useful than a confident guess.

9.  Before calling a document done

    py -3.10 tools/asciify.py --check
    py -3.10 tools/checkrefs.py
    py -3.10 tools/journalcheck.py
    py -3.10 tools/structmap/structmap.py -p ASD-Rapture.exe check

   The first is the format, the second is the cross-references, the
   third is section 8. The last is the maps, and it reports a type
   whose Ghidra description carries no evidence tag - which is the most
   common way a session ends with the work done and the provenance
   lost.

   Then three things only a reader can check:

   o Does every claim carry a tag (section 2)?
   o Is every number's kind unambiguous - image address, file offset,
     DLL address or data-file offset (section 3)?
   o Does every magic constant carry the address it was read from
     (section 5)?
