/*
 * OURS. The shim's agent: src/agent/agent_core.c on
 * \\.\pipe\rupture_shim, with the demo's clock in STATUS and PAUSE,
 * MUTE, STEP, RUN_TO, DUMP and QUIT as its own verbs. README.md lists
 * them; tools/agentctl.py --shim (or tools/shimctl.py) is the client.
 *
 * STEP and RUN_TO need the fixed-step mode (shim.c): the shim holds
 * the demo at the start of a frame, inside Timer_Seconds, and lets a
 * frame run only when agent_fixed_advance gives it a step. Those
 * verbs cannot run frames themselves - they are called from inside one
 * - so they reply later, through agent_core's deferred replies, once
 * their frames have been drawn.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../agent/agent_core.h"
#include "agent.h"
#include "shim.h"

#define DEMO_PARTS        0x0051276c   /* g_parts[0..6], part object pointers */
#define DEMO_SEQ_STARTED  0x00514888   /* g_seq+0x48, g_seqPartStarted[9] */
#define DEMO_SEQ_DONE     0x005148a8   /* g_seq+0x68, g_seqPartDone[9] */
/* RUN_TO's arrival tolerance: an exact landing can round a hair short of X. */
#define RUN_TO_EPS 1e-9

#define STEP_MAX          100000

/* A part's slot in g_parts and where its clock is; 0 where not read yet. */
static const struct {
	const char *name;
	DWORD       tOff;
} g_partInfo[] = {
	{ "part_video", 0x1b8 },   /* effects/video.txt */
	{ "part_motor", 0x210 },   /* effects/motor.txt */
	{ "part_truck", 0x310 },   /* subsystems/parts.txt 2 */
	{ "part_rail",  0x318 },
	{ "part_boat",  0x2d8 },
	{ "part_city",  0x328 },
	{ "part_end",   0x348 }
};
#define PART_COUNT (sizeof g_partInfo / sizeof g_partInfo[0])

static double        g_dt = 1.0 / 60.0;
static double        g_next;          /* a shortened next step, or 0 */
static unsigned long g_stepLeft;
static unsigned long g_stepRan;
static int           g_stepping;
static int           g_running;
static char          g_runPart[16];
static double        g_runWant;
static int           g_runExact;
static unsigned long g_runMax;
static unsigned long g_runRan;
static int           g_runSeen;       /* the watched part has run during this RUN_TO */
static double        g_runLast;
static int           g_dumping;
static char          g_dumpPath[MAX_PATH];
static LARGE_INTEGER g_lastWall;

/* OURS. The running part's slot, -1 for none; its clock if it is read. */
static int part_running(double *t, int *known)
{
	const BYTE *started = (const BYTE *)DEMO_SEQ_STARTED;
	const BYTE *done = (const BYTE *)DEMO_SEQ_DONE;
	int         i;

	*t = 0.0;
	*known = 0;
	for (i = (int)PART_COUNT - 1; i >= 0; i--) {
		if (started[i] && !done[i]) {
			BYTE *obj = *(BYTE **)(DEMO_PARTS + 4 * i);

			if (g_partInfo[i].tOff != 0 && obj != NULL) {
				*t = *(double *)(obj + g_partInfo[i].tOff);
				*known = 1;
			}
			return i;
		}
	}
	return -1;
}

/* OURS */
static void shim_status(char *reply)
{
	double t;
	int    known;
	int    part = part_running(&t, &known);

	Agent_Say(reply, " clock=%.6f paused=%d muted=%d fixed=%d part=%s", *(double *)DEMO_CLOCK,
	          shim_paused(), shim_muted(), shim_fixed(),
	          part >= 0 ? g_partInfo[part].name : "none");
	if (known) {
		Agent_Say(reply, " t=%.6f", t);
	}
	if (shim_fixed()) {
		Agent_Say(reply, " dt=%.6f held=%d", g_dt, shim_held());
	}
}

/* OURS. RUN_TO's watched value: the clock, or the named part's t; 0 if not running. */
static int watched(double *v)
{
	double t;
	int    known;
	int    part;

	if (g_runPart[0] == 0) {
		*v = *(double *)DEMO_CLOCK;
		return 1;
	}
	part = part_running(&t, &known);
	if (part < 0 || !known || strcmp(g_partInfo[part].name, g_runPart) != 0) {
		return 0;
	}
	*v = t;
	return 1;
}

/* OURS. dt= on a STEP or RUN_TO sets the step from then on. */
static int take_dt(const char *line, char *reply)
{
	char   text[32];
	double dt;

	if (!Agent_Arg(line, "dt", text, sizeof text)) {
		return 1;
	}
	dt = atof(text);
	if (!(dt > 0.0 && dt <= 1.0)) {
		Agent_Say(reply, "ERR dt=%s must be in (0, 1] seconds\n", text);
		return 0;
	}
	g_dt = dt;
	return 1;
}

/* OURS. STEP [n=N] [dt=S]: N frames, then held; replies when they are drawn. */
static int cmd_step(const char *line, char *reply)
{
	unsigned long n = 1;

	if (!take_dt(line, reply)) {
		return 1;
	}
	Agent_ArgU32(line, "n", &n);
	if (n == 0 || n > STEP_MAX) {
		Agent_Say(reply, "ERR n=%lu - 1 to %d frames per STEP\n", n, STEP_MAX);
		return 1;
	}
	shim_pause(1);
	g_stepLeft = n;
	g_stepRan = 0;
	g_stepping = 1;
	return 2;
}

/* OURS. RUN_TO (clock=X | part=NAME t=X) [exact] [dt=S] [max=N], as the port's. */
static int cmd_run_to(const char *line, char *reply)
{
	char   text[32];
	double v;
	size_t i;

	if (!take_dt(line, reply)) {
		return 1;
	}
	g_runPart[0] = 0;
	g_runMax = STEP_MAX;
	Agent_ArgU32(line, "max", &g_runMax);
	if (Agent_Arg(line, "part", text, sizeof text)) {
		_snprintf(g_runPart, sizeof g_runPart - 1, "%s%s",
		          strncmp(text, "part_", 5) == 0 ? "" : "part_", text);
		for (i = 0; i < PART_COUNT && strcmp(g_partInfo[i].name, g_runPart) != 0; i++) {
		}
		if (i == PART_COUNT || g_partInfo[i].tOff == 0) {
			Agent_Say(reply, "ERR %s's clock is not known to the shim\n", g_runPart);
			return 1;
		}
		if (!Agent_Arg(line, "t", text, sizeof text)) {
			Agent_Say(reply, "ERR RUN_TO part= needs t=\n");
			return 1;
		}
	} else if (!Agent_Arg(line, "clock", text, sizeof text)) {
		Agent_Say(reply, "ERR RUN_TO needs clock=X, or part=NAME t=X\n");
		return 1;
	}
	g_runWant = atof(text);
	if (watched(&v) && v > g_runWant) {
		Agent_Say(reply, "ERR already at %.6f, past %.6f - relaunch to go back\n", v, g_runWant);
		return 1;
	}
	g_runExact = Agent_ArgFlag(line, "exact");
	g_runRan = 0;
	g_runSeen = watched(&g_runLast);
	shim_pause(1);
	g_running = 1;
	return 2;
}

/* OURS. DUMP path=FILE: the last frame drawn, or the next one if none is held. */
static int cmd_dump(const char *line, char *reply)
{
	if (!Agent_Arg(line, "path", g_dumpPath, sizeof g_dumpPath)) {
		Agent_Say(reply, "ERR DUMP needs path=FILE\n");
		return 1;
	}
	if (!shim_fixed() || !shim_captured()) {
		shim_capture_next();
	}
	g_dumping = 1;
	return 2;
}

/* OURS. PAUSE / MUTE [on|off]; with neither, toggles. */
static int cmd_switch(const char *line, char *reply, int isPause)
{
	int now = isPause ? shim_paused() : shim_muted();

	if (Agent_ArgFlag(line, "on")) {
		now = 1;
	} else if (Agent_ArgFlag(line, "off")) {
		now = 0;
	} else {
		now = !now;
	}
	if (isPause) {
		shim_pause(now);
	} else {
		shim_mute(now);
	}
	Agent_Say(reply, "OK %s %s clock=%.6f\n", isPause ? "PAUSE" : "MUTE", now ? "on" : "off",
	          *(double *)DEMO_CLOCK);
	return 1;
}

/* OURS */
static int shim_execute(const char *line, char *reply)
{
	int fixedOnly = strncmp(line, "STEP", 4) == 0 || strncmp(line, "RUN_TO", 6) == 0;

	if (fixedOnly && !shim_fixed()) {
		Agent_Say(reply, "ERR STEP and RUN_TO need the fixed-step shim - agentctl.py --shim launch\n");
		return 1;
	}
	if (strncmp(line, "PAUSE", 5) == 0) {
		return cmd_switch(line, reply, 1);
	} else if (strncmp(line, "MUTE", 4) == 0) {
		return cmd_switch(line, reply, 0);
	} else if (strncmp(line, "STEP", 4) == 0) {
		return cmd_step(line, reply);
	} else if (strncmp(line, "RUN_TO", 6) == 0) {
		return cmd_run_to(line, reply);
	} else if (strncmp(line, "DUMP", 4) == 0) {
		return cmd_dump(line, reply);
	} else if (strncmp(line, "QUIT", 4) == 0) {
		Agent_Say(reply, "OK QUIT\n");
		shim_quit_soon();
		return 1;
	}
	return 0;
}

/* OURS. The deferred replies: a STEP or RUN_TO once its frames are drawn, a DUMP once captured. */
static int shim_poll(char *reply)
{
	const unsigned char *px;
	double               v;
	int                  w;
	int                  h;

	if ((g_stepping || g_running) && Agent_Cancelled()) {
		Agent_Say(reply, "ERR %s cancelled after %lu frames", g_stepping ? "STEP" : "RUN_TO",
		          g_stepping ? g_stepRan : g_runRan);
		g_stepping = 0;
		g_running = 0;
		shim_status(reply);
		Agent_Say(reply, "\n");
		return 1;
	}
	if (g_stepping && g_stepLeft == 0) {
		g_stepping = 0;
		Agent_Say(reply, "OK STEP frames=%lu", g_stepRan);
		shim_status(reply);
		Agent_Say(reply, "\n");
		return 1;
	}
	if (g_running && watched(&v)) {
		g_runSeen = 1;
		g_runLast = v;
	} else if (g_running && g_runSeen) {
		g_running = 0;
		Agent_Say(reply, "ERR %s ended at t=%.6f, before %.6f, after %lu frames", g_runPart,
		          g_runLast, g_runWant, g_runRan);
		shim_status(reply);
		Agent_Say(reply, "\n");
		return 1;
	}
	if (g_running && watched(&v) && v >= g_runWant - RUN_TO_EPS) {
		g_running = 0;
		Agent_Say(reply, "OK RUN_TO frames=%lu", g_runRan);
		shim_status(reply);
		Agent_Say(reply, "\n");
		return 1;
	}
	if (g_running && g_runRan >= g_runMax) {
		g_running = 0;
		Agent_Say(reply, "ERR stopped after %lu frames short of %.6f\n", g_runRan, g_runWant);
		return 1;
	}
	if (g_dumping && shim_captured()) {
		g_dumping = 0;
		shim_capture_get(&px, &w, &h);
		if (!Agent_WriteRtex(g_dumpPath, w, h, px)) {
			Agent_Say(reply, "ERR cannot write %s\n", g_dumpPath);
		} else {
			Agent_Say(reply, "OK DUMP width=%d height=%d frame=%u path=%s\n", w, h,
			          Agent_Frames(), g_dumpPath);
		}
		return 1;
	}
	return 0;
}

/* OURS */
static void shim_agent_log(const char *text)
{
	shim_log("%s", text);
}

static const Agent_Host g_shimHost = {
	"\\\\.\\pipe\\rupture_shim", shim_status, shim_execute, shim_poll, NULL, shim_agent_log,
	"PAUSE MUTE STEP RUN_TO DUMP QUIT"
};

/* OURS */
void agent_start(void)
{
	Agent_Start(&g_shimHost);
	QueryPerformanceCounter(&g_lastWall);
}

/* OURS. Runs the pending command, if any. */
void agent_service(void)
{
	Agent_Service();
}

/* OURS. Counts a frame the demo draws. */
void agent_frame(void)
{
	Agent_FrameDone();
}

/* OURS. The next step, or 0 to keep the demo held: a STEP's, a RUN_TO's, or free running paced to dt. */
double agent_fixed_advance(void)
{
	LARGE_INTEGER now;
	LARGE_INTEGER freq;
	double        v;
	double        step;

	if (g_stepping && g_stepLeft > 0) {
		g_stepLeft--;
		g_stepRan++;
	} else if (g_running) {
		if (watched(&v) && v >= g_runWant - RUN_TO_EPS) {
			return 0.0;
		}
		if (g_runExact && watched(&v) && g_runWant - v < g_dt) {
			g_next = g_runWant - v;
		}
		g_runRan++;
	} else if (!shim_paused()) {
		QueryPerformanceCounter(&now);
		QueryPerformanceFrequency(&freq);
		if ((double)(now.QuadPart - g_lastWall.QuadPart) / (double)freq.QuadPart < g_dt) {
			return 0.0;
		}
		g_lastWall = now;
	} else {
		return 0.0;
	}
	step = g_next > 0.0 ? g_next : g_dt;
	g_next = 0.0;
	return step;
}
