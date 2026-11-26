PCDEMOS RUPTURE                                                Tool Note


                        THE COMMENT BUDGET


Abstract

   CLAUDE.md rule 18 says what a src/ comment has to earn.  It does
   not say HOW MUCH, and that turns out to be the half that gets
   broken: a file already dense with comment reads as licence to add
   more, so each change arrives with a larger comment-to-code ratio
   than the last.  Files have reached 55 per cent comment that way -
   the fix is moving the explanation into docs/, not deleting it.

   This measures the one thing rule 18 leaves unmeasured.

Table of Contents

   1. Running it
   2. What it counts
   3. The one-to-one rule
   4. The in-body rule
   5. The reduction gate
   6. What it cannot see

1.  Running it

       py -3.10 tools/commentbudget.py            the working tree
       py -3.10 tools/commentbudget.py --reduce   a reduction pass
       py -3.10 tools/commentbudget.py --blocks   oversized blocks

   The first form reads `git diff -U0`, keeps .c/.h/.cpp/.py under
   src/ and sim/, and reports, per file
   and in total, the NET comment and code lines - added minus
   removed.  It exits non-zero when net comment exceeds net code.

   NET, NOT ADDED, and the difference matters: replacing a fourteen
   line comment with a six line one ADDS six, which read as a
   regression under the first version of this tool and punished
   exactly the edit it is meant to encourage.  A change that deletes
   more comment than it writes is a reduction and reports as one.

   The second form walks src/ and lists every run of comment lines
   longer than twelve.  It is ADVISORY and always exits zero: the
   tree is full of them already and failing on that would make the
   tool useless on day one.

2.  What it counts

   A line is comment if it starts with "/*", "*" or "//", or ends
   with "*/".  Blank lines count as neither.  That misreads a string
   containing "//" and a continuation line inside a block that
   happens to start with a letter, both rare enough not to matter.

   Only .c, .h and .cpp under src/ are counted.  docs/ and tools/
   are not - they are supposed to be prose.

3.  The one-to-one rule

   Net comment may not exceed net code.

   It is arbitrary and that is the point: it is a number, so it
   cannot be argued with the way "is this comment earning its place"
   can.  Rule 16 decides WHICH comments; this decides how many.

   The escape is the one rule 18 already names - a long explanation
   goes to docs/ and leaves a one-line pointer.  Hitting the budget
   is the signal to do that, not a reason to delete something true.

   o ONE EXEMPTION, a judgement call to state out loud in the change
     rather than to encode here: a new file's header, which is
     prose by nature.

   o "A PURE COMMENT SWEEP ADDS NONE" WAS ALSO LISTED HERE AND IT IS
     NOT TRUE. A sweep corrects comments, and a correction is usually
     longer than the wrong line it replaces - five swept files grew
     under this gate. Section 4 is the gate for a sweep.

4.  The in-body rule

   A RATIO CANNOT CATCH THE THING THAT GOES WRONG MOST. The failure is
   a three-line fix carrying a paragraph about why it was needed, and
   a ratio permits it the moment the fix is long enough. So the in-body
   rule is absolute and runs beside the budget rather than inside it:

       py -3.10 tools/commentbudget.py --inline

   A comment line INDENTED INSIDE A FUNCTION BODY is refused, however
   short, in anything under tools/ and in any src/ or sim/ function
   whose header says OURS. Where it goes instead is that function's own
   header comment, or docs/.

   TWO THINGS ARE EXEMPT, both because they cannot live anywhere else.
   A provenance marker - ORIGIN or OURS - is metadata the project
   mandates. And the body of a TRANSCRIBED function, one whose header
   carries ORIGIN, may carry notes against individual lines, because a
   remark about the original's arithmetic is worth nothing three files
   away from the arithmetic.

   WHICH FUNCTION ENCLOSES A LINE IS NOT THE NEAREST MARKER ABOVE IT.
   A transcribed body carries ORIGIN comments of its own, so the check
   walks up to the definition - column zero, not a comment, and past
   the lone brace that opens a C body - and reads the block above
   that. Getting this backwards reads every transcription as ours and
   every in-body ORIGIN note as a header.

   A HEADER'S CONTINUATION LINES LOOK INDENTED. " * more prose" begins
   with a space exactly as an in-body comment does, so indentation
   alone would flag every multi-line header. What separates them is
   where the RUN starts: a header's first line reaches column zero.

       py -3.10 tools/commentbudget.py --selftest

   proves each arm fires and each exemption holds, on a fixture whose
   comment markers are assembled rather than spelt out - written
   literally, the fixture flags itself.

5.  The reduction gate

   --reduce requires the net comment to be NEGATIVE and ignores the
   code delta entirely.

   THE DEFAULT GATE PERMITS GROWTH ON PURPOSE - net comment may not
   exceed net CODE, so a change that adds code may explain it. That
   is the wrong test for a pass whose only purpose is to shrink the
   comment, and it is why three files in a sister project each came
   out seventy-odd lines longer while being swept.

   A FILE WHOSE COMMENTS THE DIFF DOES NOT TOUCH IS NEUTRAL, so a
   reduction pass may still carry an unrelated code fix. Only a file
   with comment churn and a net of zero or more is flagged GREW.

   Use it for the second pass over a file section 3 already let
   through, and the ordinary gate for everything else.


6.  What it cannot see

   o A COMMENT THAT IS SIMPLY WRONG.  The budget is blind to truth;
     a stale comment passes it perfectly.  That is what the sweep
     and docs/comment-defects.txt are for.

   o THE SAME FACT IN TWO FILES.  Rule 14.1's one-fact-one-home is
     not a count and nothing here checks it.  Four files restating
     one fact can each sit under budget.

   o WHETHER THE CODE EARNED ITS LINES EITHER.  A change that pads
     the code passes by padding.

   o A PURE DELETION OF CODE.  Removing fifty lines of code and
     their twenty of comment nets -20 against -50 and passes, which
     is right, but so does removing fifty lines of code and adding a
     comment about having done so.

   A HEADER IS MEASURED ON THE TOTAL, NOT ON ITSELF.

   In a .c the code is the thing and a comment explains it, so
   comment exceeding code means the explanation outgrew the work.
   In a .h the code is a SIGNATURE and the comment is the CONTRACT -
   what a caller has to know to use the thing correctly. One line of
   signature legitimately carries several lines of contract, and
   holding a header to the .c ratio asks it to under-document its
   own interface.

   So a .h over its own budget is reported and not counted as a
   failure. It is still counted in the TOTAL, which is what stops a
   header becoming the place explanations go to hide.

   THIS IS A DELIBERATE LOOSENING AND IT IS THE ONLY ONE. It was
   made when src/gfx.h ran 20 lines of comment against 9 of
   declaration for Draw_Flare and the ray tables - three contracts a
   caller cannot guess: that centres are in demo units, that the ray
   value is SUBTRACTED so a negated table brightens, and that a
   colour ramp is built in the framebuffer's pixel layout. Disagree
   with it by deleting the .h branch in report_diff.

   AN ORIGIN OR OURS LINE IS METADATA, NOT COMMENTARY.

   Rule 18 requires one on every function definition in src/ and
   tools/commentcheck.py fails the build without it. A line the
   project mandates cannot also be penalised for existing, and it is
   the one comment that by definition cannot move to docs/ - its
   whole purpose is to sit against the code.

   Only the marker's own line is exempt. Prose continuing under it
   counts normally, so nothing hides behind the marker.
