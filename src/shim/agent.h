/*
 * OURS. The shim's agent: a named pipe, \\.\pipe\rupture_shim, that
 * tools/agentctl.py --shim drives. README.md lists its commands.
 */
#ifndef AGENT_H
#define AGENT_H

/* Starts the pipe thread. Call once, from the demo's own thread. */
void   agent_start(void);

/* Runs the pending command, if any; only ever on the demo's thread. */
void   agent_service(void);

/* Counts a frame the demo draws. */
void   agent_frame(void);

/* Fixed-step mode: the step to let the held frame run with, or 0 to keep holding. */
double agent_fixed_advance(void);

#endif
