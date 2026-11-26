PCDEMOS RUPTURE                                                Tool Note


                     THE AGENT PIPE AND THE PROBES


Abstract

   The original (through the shim) and the port (its Debug build)
   carry the same agent, src/agent/agent_core.c, on a named pipe. It
   steps either program on a fixed clock, dumps frames and textures,
   reads and writes memory, and hooks functions, so the two can be
   compared at the same part time. This note is its reference, with
   the port's part jumps and profiling tools.

Table of Contents

   1. The pipes and the clients
   2. Commands
   3. Addresses and field paths
   4. The port's agent
   5. The original's fixed-step mode
   6. Hooks
   7. Starting the port at a part
   8. Profiling and checking

1.  The pipes and the clients

   The shimmed original serves \\.\pipe\rupture_shim from its first
   frame; the port serves \\.\pipe\rupture_port when run with
   --agent. tools/agentctl.py sends one command and prints the reply;
   --shim (or tools/shimctl.py) talks to the original.

    agentctl.py launch                the port, paused at frame 0
    agentctl.py RUN_TO part=motor t=5.2476 exact
    agentctl.py HOOK add addr=part_motor::DrawScene log=ra,ecx,a0
    agentctl.py STEP
    agentctl.py TRACE
    agentctl.py dump frame.png
    agentctl.py READ addr=g_demo.ctx.eye
    agentctl.py READ "addr=g_demo.ctx.gfx.tex[11]"
    agentctl.py HOOK add addr=Post_RunPass texdump=40 path=n#.rtex
    agentctl.py png n1.rtex n1.png
    agentctl.py READ "addr=part_motor@(g_demo.part[1]).road.count"
    agentctl.py type Rupture_Ctx
    shimctl.py READ addr=g_demoClock len=8 -f d
    shimctl.py HOOK add addr=Demo_Frame cap=5 log=ra,absd+g_demoClock
    agentctl.py --shim launch         the original, fixed step, held
    agentctl.py --shim RUN_TO part=motor t=5.2476 exact

   Each line is run as py -3.10 tools/agentctl.py ... (or
   tools/shimctl.py) from the repository root.

   Any number of clients may connect at once. STATUS, READ and CANCEL
   answer at once, even while another client's STEP or RUN_TO runs,
   and STATUS then ends busy=RUN_TO. The other commands run on the
   host's thread between frames, one client's at a time - a STEP or
   RUN_TO replies when its frames are done. Every failure is an ERR
   line on stdout with exit status 1.

   launch --size 800x450 gives the port the shimmed original's view:
   800x450 is maybe_g_viewWidth/Height, the size every gluPerspective
   aspect and render target is made from, even though the original's
   client area is only 794x421, its bottom-left corner of that view.
   Matching the client area instead stretches the port's aspect by 6%.

2.  Commands

   +-------------------------------------+--------------------------------------------------+
   | Command                             | Does                                             |
   +=====================================+==================================================+
   | STATUS                              | frame, clock, pause, mute, hooks, trace lines;   |
   |                                     | the port adds the part, its t, the step          |
   +-------------------------------------+--------------------------------------------------+
   | READ addr=X len=N                   | up to 4096 bytes as hex; -f FMT unpacks, -x      |
   |                                     | prints raw                                       |
   +-------------------------------------+--------------------------------------------------+
   | WRITE addr=X value=HEX              | up to 128 bytes, code or data; replies was= for  |
   |                                     | the undo                                         |
   +-------------------------------------+--------------------------------------------------+
   | HOOK add addr=X [exit] [len=N]      | arm one of 16 hooks (section 6)                  |
   | [tag=T] [cap=N] [when=...] [log=...]|                                                  |
   | [texdump=OP path=FILE]              |                                                  |
   +-------------------------------------+--------------------------------------------------+
   | HOOK del idx=N, HOOK clear,         |                                                  |
   | HOOK list                           |                                                  |
   +-------------------------------------+--------------------------------------------------+
   | TRACE [keep]                        | the hook hits since the last drain, oldest first |
   +-------------------------------------+--------------------------------------------------+
   | PAUSE [on|off], MUTE [on|off]       | as Space and M; no argument toggles              |
   +-------------------------------------+--------------------------------------------------+
   | TEXDUMP tex=N [path=FILE]           | a GL texture's size and internal format; with    |
   |                                     | path= its pixels                                 |
   +-------------------------------------+--------------------------------------------------+
   | STEP [n=N] [dt=S]                   | N frames at the fixed step, then held            |
   +-------------------------------------+--------------------------------------------------+
   | RUN_TO clock=X                      | frames until there; exact lands on X by          |
   | RUN_TO part=NAME t=X [exact] [dt=S] | shortening the last step. A RUN_TO part= stops   |
   |                                     | where that part ends, if it ends before t=       |
   +-------------------------------------+--------------------------------------------------+
   | DUMP path=FILE                      | the last frame drawn; agentctl.py dump F.png     |
   |                                     | writes it as PNG                                 |
   +-------------------------------------+--------------------------------------------------+
   | QUIT                                | exits                                            |
   +-------------------------------------+--------------------------------------------------+
   | CANCEL                              | stops the running STEP or RUN_TO where it is;    |
   |                                     | it replies ERR ... cancelled                     |
   +-------------------------------------+--------------------------------------------------+

   A RUN_TO cannot go back - relaunch. The demo seeds rand from its
   clock, so a run that stops at different points takes different
   steps and draws different particles: compare runs that took the
   same steps.

3.  Addresses and field paths

   Addresses are hex, or a name with an optional +OFF, in addr= and in
   abs operands. For the shim the names come from the Ghidra exports;
   for the port from build/rupture/Debug/rupture.map, static functions
   included, by their C++ name (part_motor::Frame).

   In addr=, either side also takes a field path: the port's resolved
   through its Debug PDB by tools/pdbtypes.py, the original's through
   the exported maps by tools/maptypes.py (so only what is typed in
   Ghidra). A path is a global, then .field, [i] (arrays, and
   std::vectors through the live process), [key] (an integer key of a
   std::map, through the live process) and ->field; Type@ADDR for an
   object at an address, such as a hooked ecx; and the cast
   Type@(path) for what a base-class pointer points to.

   A READ of a path reads the whole object and prints it field by
   field, a std::map's entries in key order, and a std::string as
   quoted text. type NAME prints a layout. The client refuses a name
   it finds twice, and trace lines get the caller's name after ra=.

   tools/sym.py is the same lookup on its own, from the named symbols
   of the last tools/export_maps.py:

       py -3.10 tools/sym.py Timer_Seconds       name -> address, size, type
       py -3.10 tools/sym.py -q g_demoClock+8    the address only
       py -3.10 tools/sym.py -s part_motor       every name containing it
       py -3.10 tools/sym.py 0x00448dd8          address -> WinMain+0x118

4.  The port's agent

   The agent is in the Debug build only, which is built for it: /Gh
   /GH entry and exit hooks, optimised but with no inlining, a map and
   a fixed base address. Release has no agent and refuses --agent.

   agentctl.py launch starts build/rupture/Debug/rupture.exe --agent,
   and refuses one older than any source under src/rupture or
   src/agent; every command refuses a map older than the exe. In agent
   mode the music is not loaded and the clock is a fixed step, 1/60 s
   unless dt= changes it, so a frame number or a part time is the same
   frame on every run; the loop starts paused before frame 0.

5.  The original's fixed-step mode

   agentctl.py --shim launch starts reverse/patched/run/ASD-Rapture.exe
   with RUPTURE_SHIM_FIXED=1, and refuses a run-folder glut32.dll
   older than the shim's sources. The setup dialog runs itself, the
   music is held, g_musicSyncOn is held at 0, and Timer_Seconds
   advances by exactly one step per frame - so a part time is the same
   frame in the original and the port, and RUN_TO part=motor t=5.2476
   exact lands both on frame 4700.

   The shim holds the demo at the start of each frame, inside
   Timer_Seconds, serving the pipe and the window until it is given a
   step; launch returns once it is held before frame 1 (held=1 in
   STATUS). Its STEP, RUN_TO and DUMP reply once their frames are
   drawn, so a DUMP before the first STEP waits for a frame that never
   comes.

6.  Hooks

   In the port, a hook on a function's first byte is an entry hook and
   exit makes it an exit hook (end= comes from the map), served by
   _penter and _pexit. Both see ra and a0... as at the function's
   entry; at exit eax and st0 hold the return value. gl1.cpp, the
   agent and the third-party decoders are not instrumented, so they
   take no entry or exit hook. Any other address takes a patch hook,
   as in the shim.

   A patch hook can go at any address where the instructions covering
   the first 5 bytes are ones the stub can run elsewhere - no branch,
   call or return. The agent decodes them and refuses anything else;
   len= makes it also check the count against the one expected. A
   patched address takes one hook. NOTHING MAY JUMP INTO THE MIDDLE OF
   THE BYTES IT REPLACES - the agent cannot see that, so check the
   site's cross-references in Ghidra. The stub saves every register,
   the flags and the x87/SSE state.

   log= and when= take operands:

   +---------------------------------+------------------------------------------------+
   | Operand                         | Is                                             |
   +=================================+================================================+
   | eax ... edi, efl                | a register at the hooked instruction           |
   +---------------------------------+------------------------------------------------+
   | a0 ... a9                       | the dword at [esp+4+4N] - the arguments, at a  |
   |                                 | function entry                                 |
   +---------------------------------+------------------------------------------------+
   | ra                              | the dword at [esp] - the caller, at an entry   |
   +---------------------------------+------------------------------------------------+
   | a1f                             | an operand's bits read as a float              |
   +---------------------------------+------------------------------------------------+
   | ecxf+10, a0l+8, absd+5146c8     | b w l f d at operand+offset (abs is 0)         |
   +---------------------------------+------------------------------------------------+
   | a2s, a2s+4                      | the string at operand+offset, up to 31 chars   |
   +---------------------------------+------------------------------------------------+
   | tex0 ... tex7                   | the GL 2D texture bound on that unit           |
   +---------------------------------+------------------------------------------------+
   | vp0 ... vp3                     | the GL viewport: x, y, width, height           |
   +---------------------------------+------------------------------------------------+
   | mtex0..mtex15, mmvN, mprojN     | element N, column major, of that matrix; in    |
   |                                 | the port, gl1's                                |
   +---------------------------------+------------------------------------------------+
   | st0                             | port entry and exit hooks: the x87 top of      |
   |                                 | stack, a float or double return at exit        |
   +---------------------------------+------------------------------------------------+
   | @path                           | the field at its type's width (@g_demo.clock)  |
   +---------------------------------+------------------------------------------------+

   A plain number is an operand too: texdump=40. In the port,
   texdump=@g_demo.ctx.gfx.tex[0] names the texture behind a slot.

   when= is OPERAND, meaning non-zero, or OPERAND=N, !=, <, >.
   texdump=OPERAND path=FILE dumps the GL texture the operand names at
   every recorded hit - inside the frame, where TEXDUMP only runs
   between frames. A # in the path becomes the hit number; the trace
   line ends with the texture's size and format. agentctl.py png
   F.rtex F.png converts a dump. cap= stops recording after that many
   hits, 200 by default and 0 for none. A hook with no tag= is tagged
   with the name in addr=. The trace keeps the last 256 lines.

7.  Starting the port at a part

   rupture.exe --part=rail starts the demo at part_rail, and
   --part=rail@31 at part_rail's t = 31. The names are the parts' own
   without part_: video, motor, truck, rail, boat, city, end. Every
   part's Init still runs first, so the textures, meshes and render
   targets are those of a run from the start. The parts before the
   chosen one are marked done, and the demo clock, the host's clock,
   the music and the time carried into the part are set to the values
   a run at the agent's 1/60 step has there; with @T the frames up to
   T then run at that step, unshown, the last one shortened to land on
   T. agentctl.py launch --part=rail passes it to the port.

   In the agent this is exact: at a part time on the same 1/60 grid, a
   jumped run's frame is pixel-identical to a run from the start, and
   --part=NAME@T matches --part=NAME followed by RUN_TO ... exact. A
   frame can depend on the steps before it - part_truck carries image
   history - so compare runs that took the same steps. In a real-time
   run the clock differs from run to run anyway, and a jumped run is
   as close to a full one as two full runs are.

8.  Profiling and checking

   py -3.10 tools/portprof.py rail 40 profiles the running agent from
   part_rail t = 40: where each frame's time goes, by function,
   sampled from the main thread's stack. py -3.10 tools/portprof.py
   --release 15 samples a Release rupture.exe already running (started
   with --part=NAME@T) for 15 seconds, by function, self time only.

   rupture.exe --perflog=FILE (Release, or Debug without the agent)
   writes a line per frame: the time since the last frame, the time in
   the demo's frame, in SwapBuffers and on the GPU.
   py -3.10 tools/perflog.py FILE --run city@40 15 runs Release from
   part_city t = 40 for 15 seconds into FILE and summarises it per 2
   seconds, with the frames that came late. --nobatch turns gl1's draw
   batching off, to compare against.

   py -3.10 tools/glcheck.py compiles and links every shader pair the
   port loads on this machine's GL driver, with the loader's own
   #version 330 core and 300 es headers. The 300 es half is the
   desktop driver's ES compatibility, not WebGL2, so the browser is
   still the final test.
