/*
 * OURS. The agent pipe shared by the shim (the original demo) and the
 * port's Debug build: one client at a time on a named pipe, a line of
 * text in and a reply out. The verbs and operands are README.md's
 * "The agent pipe"; tools/agentctl.py is the client.
 *
 * STATUS and READ are answered on the pipe thread. Every other verb is
 * parked in a mailbox that Agent_Service runs on the host's thread,
 * between frames, so nothing it changes lands half way through one.
 *
 * C89, so the shim (C) and the port (C++98) both build it.
 */
#ifndef AGENT_CORE_H
#define AGENT_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * execute returns 0 for a verb that is not the host's, 1 when it has
 * replied, and 2 to reply later: the client waits, and every
 * Agent_Service calls poll until it returns 1 with the reply written.
 * That is how a host whose agent runs inside its own frame - the shim -
 * answers a STEP once the frames are drawn.
 */
typedef struct Agent_Host {
	const char *pipe;                                     /* \\.\pipe\...            */
	void      (*status)(char *reply);                     /* " clock=.. paused=.." */
	int       (*execute)(const char *line, char *reply);  /* 0, 1 or 2, as above    */
	int       (*poll)(char *reply);                       /* a deferred reply, or 0 */
	int       (*matrix)(int which, float m[16]);          /* 0 tex, 1 mv, 2 proj    */
	void      (*log)(const char *text);
	const char *verbs;                                    /* the host's, for ERR    */
} Agent_Host;

/* Starts the pipe thread. Call once, from the host's own thread. */
int      Agent_Start(const Agent_Host *host);

/* Runs the pending command, if any; call once per pass of the host loop. */
void     Agent_Service(void);

/* True once a client has sent CANCEL during the running command: stop it. */
int      Agent_Cancelled(void);

/* Counts a frame; the trace stamps hits with it. */
void     Agent_FrameDone(void);
unsigned Agent_Frames(void);

/* For host verbs: append to a reply, read key=value and lone words. */
void     Agent_Say(char *reply, const char *fmt, ...);
void     Agent_SayHex(char *reply, const unsigned char *p, int n);
int      Agent_Arg(const char *line, const char *key, char *out, int size);
int      Agent_ArgU32(const char *line, const char *key, unsigned long *out);
int      Agent_ArgFlag(const char *line, const char *word);

/* RGBA8 rows bottom up as "RTEX", w, h, pixels - TEXDUMP's and DUMP's file. */
int      Agent_WriteRtex(const char *path, int w, int h, const unsigned char *pixels);

#ifdef __cplusplus
}
#endif

#endif
