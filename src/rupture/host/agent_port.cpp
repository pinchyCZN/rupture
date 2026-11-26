/*
 * OURS. The port's agent: agent_core on \\.\pipe\rupture_port, with a
 * fixed-step clock so a frame is the same frame on every run. The demo
 * clock starts at 0 and advances by dt (1/60 s unless STEP or RUN_TO
 * says dt=) per frame, the music is not loaded, and the loop starts
 * paused before the first frame.
 *
 * Its verbs, beyond the core's: PAUSE, MUTE, STEP, RUN_TO, DUMP, QUIT.
 * A STEP or RUN_TO runs its frames inside the command, on this thread,
 * and replies when they are done.
 */
#include "agent_port.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "../../agent/agent_core.h"
#include "../core/gl1.h"
#include "../core/glapi.h"
#include "host.h"

#ifndef GL_READ_FRAMEBUFFER_BINDING
#define GL_READ_FRAMEBUFFER_BINDING 0x8caa
#endif

/* RUN_TO's arrival tolerance: an exact landing can round a hair short of X. */
#define RUN_TO_EPS 1e-9

static AgentPort_Draw             g_draw;
static AgentPort_Present          g_present;
static int                        g_width, g_height;
static bool                       g_paused = true;
static bool                       g_quit;
static double                     g_now;
static double                     g_dt = 1.0 / 60.0;
static double                     g_next;           /* a shortened next step, or 0 */
static std::vector<unsigned char> g_pixels;
static bool                       g_captured;

/* OURS */
static void port_status(char *reply)
{
	const char *name;
	double      t;

	App_Part(&name, &t);
	Agent_Say(reply, " clock=%.6f paused=%d muted=%d part=%s t=%.6f dt=%.6f build=agent",
	          App_Clock(), g_paused ? 1 : 0, App_Muted() ? 1 : 0, name, t, g_dt);
}

/* OURS. dt= on a STEP or RUN_TO sets the step from then on. */
static bool take_dt(const char *line, char *reply)
{
	char   text[32];
	double dt;

	if (!Agent_Arg(line, "dt", text, sizeof text)) {
		return true;
	}
	dt = atof(text);
	if (!(dt > 0.0 && dt <= 1.0)) {
		Agent_Say(reply, "ERR dt=%s must be in (0, 1] seconds\n", text);
		return false;
	}
	g_dt = dt;
	return true;
}

/* OURS. One frame at the next step, read back if capture. */
static void one_frame(bool capture)
{
	g_draw(AgentPort_Advance());
	g_present(capture);
}

/* OURS. STEP [n=N] [dt=X]: N frames, 1 by default, then paused. */
static void cmd_step(const char *line, char *reply)
{
	unsigned long n = 1;
	unsigned long i;

	if (!take_dt(line, reply)) {
		return;
	}
	Agent_ArgU32(line, "n", &n);
	if (n > 100000) {
		Agent_Say(reply, "ERR n=%lu - at most 100000 frames per STEP\n", n);
		return;
	}
	g_paused = true;
	for (i = 0; i < n && !g_quit && !Agent_Cancelled(); i++) {
		one_frame(i + 1 == n || Agent_Cancelled());
	}
	Agent_Say(reply, i < n ? "ERR STEP cancelled after %lu frames" : "OK STEP frames=%lu", i);
	port_status(reply);
	Agent_Say(reply, "\n");
}

/* OURS. The quantity RUN_TO watches: the demo clock, or a named part's t (false: not running). */
static bool watched(const char *part, double *v)
{
	const char *name;
	double      t;

	if (part[0] == 0) {
		*v = App_Clock();
		return true;
	}
	App_Part(&name, &t);
	if (strcmp(name, part) != 0) {
		return false;
	}
	*v = t;
	return true;
}

/*
 * OURS. RUN_TO (clock=X | part=NAME t=X) [exact] [dt=X] [max=N]: frames
 * until the value reaches X. With exact, the last step is shortened so
 * it lands on X; the frames after go back to dt. Already past X is an
 * error - the clock does not run backwards; relaunch instead. A part
 * that ends before X stops the run where it ended, and so does CANCEL.
 */
static void cmd_run_to(const char *line, char *reply)
{
	char          part[32] = "";
	char          text[32];
	double        want;
	double        v;
	unsigned long max = 100000;
	unsigned long n = 0;
	bool          exact = Agent_ArgFlag(line, "exact") != 0;
	bool          seen;
	bool          wasSeen;
	bool          stop = false;
	double        last = 0.0;

	if (!take_dt(line, reply)) {
		return;
	}
	Agent_ArgU32(line, "max", &max);
	if (Agent_Arg(line, "part", part, sizeof part)) {
		if (strncmp(part, "part_", 5) != 0) {
			char full[32];

			_snprintf(full, sizeof full - 1, "part_%s", part);
			full[sizeof full - 1] = 0;
			strcpy(part, full);
		}
		if (!Agent_Arg(line, "t", text, sizeof text)) {
			Agent_Say(reply, "ERR RUN_TO part= needs t=\n");
			return;
		}
	} else if (!Agent_Arg(line, "clock", text, sizeof text)) {
		Agent_Say(reply, "ERR RUN_TO needs clock=X, or part=NAME t=X\n");
		return;
	}
	want = atof(text);
	seen = watched(part, &v);
	if (seen && v > want) {
		Agent_Say(reply, "ERR already at %.6f, past %.6f - relaunch to go back\n", v, want);
		return;
	}
	g_paused = true;
	wasSeen = seen;
	while (n < max && !g_quit && !stop && !(seen && v >= want - RUN_TO_EPS)) {
		if (exact && seen && want - v < g_dt) {
			g_next = want - v;
		}
		g_draw(AgentPort_Advance());
		n++;
		if (seen) {
			last = v;
		}
		seen = watched(part, &v);
		stop = (wasSeen && !seen) || Agent_Cancelled();
		wasSeen = wasSeen || seen;
		g_present((seen && v >= want - RUN_TO_EPS) || n == max || g_quit || stop);
	}
	if (stop && !Agent_Cancelled()) {
		Agent_Say(reply, "ERR %s ended at t=%.6f, before %.6f, after %lu frames", part, last, want, n);
		port_status(reply);
		Agent_Say(reply, "\n");
		return;
	}
	if (!(seen && v >= want - RUN_TO_EPS)) {
		Agent_Say(reply, "ERR %s after %lu frames short of %.6f", Agent_Cancelled() ? "cancelled" : "stopped",
		          n, want);
		port_status(reply);
		Agent_Say(reply, "\n");
		return;
	}
	Agent_Say(reply, "OK RUN_TO frames=%lu", n);
	port_status(reply);
	Agent_Say(reply, "\n");
}

/* OURS. DUMP path=FILE: the last frame as drawn, RTEX as TEXDUMP writes it. */
static void cmd_dump(const char *line, char *reply)
{
	char path[MAX_PATH];

	if (!Agent_Arg(line, "path", path, sizeof path)) {
		Agent_Say(reply, "ERR DUMP needs path=FILE\n");
		return;
	}
	if (!g_captured) {
		Agent_Say(reply, "ERR no frame drawn yet - STEP first\n");
		return;
	}
	if (!Agent_WriteRtex(path, g_width, g_height, &g_pixels[0])) {
		Agent_Say(reply, "ERR cannot write %s\n", path);
		return;
	}
	Agent_Say(reply, "OK DUMP width=%d height=%d frame=%u path=%s\n", g_width, g_height,
	          Agent_Frames(), path);
}

/* OURS. PAUSE / MUTE [on|off]; with neither, toggles. MUTE is the app's M. */
static void cmd_switch(const char *line, char *reply, bool isPause)
{
	bool now = isPause ? g_paused : App_Muted();

	if (Agent_ArgFlag(line, "on")) {
		now = true;
	} else if (Agent_ArgFlag(line, "off")) {
		now = false;
	} else {
		now = !now;
	}
	if (isPause) {
		g_paused = now;
	} else if (now != App_Muted()) {
		App_Key('M');
	}
	Agent_Say(reply, "OK %s %s clock=%.6f\n", isPause ? "PAUSE" : "MUTE", now ? "on" : "off",
	          App_Clock());
}

/* OURS */
static int port_execute(const char *line, char *reply)
{
	if (strncmp(line, "PAUSE", 5) == 0) {
		cmd_switch(line, reply, true);
	} else if (strncmp(line, "MUTE", 4) == 0) {
		cmd_switch(line, reply, false);
	} else if (strncmp(line, "STEP", 4) == 0) {
		cmd_step(line, reply);
	} else if (strncmp(line, "RUN_TO", 6) == 0) {
		cmd_run_to(line, reply);
	} else if (strncmp(line, "DUMP", 4) == 0) {
		cmd_dump(line, reply);
	} else if (strncmp(line, "QUIT", 4) == 0) {
		g_quit = true;
		Agent_Say(reply, "OK QUIT\n");
	} else {
		return 0;
	}
	return 1;
}

/* OURS. The gl1 stacks are the port's matrices; GL's own are unused. */
static int port_matrix(int which, float m[16])
{
	static const int modes[3] = { gl1::TEXTURE, gl1::MODELVIEW, gl1::PROJECTION };

	memcpy(m, gl1::Matrix(modes[which]), 16 * sizeof(float));
	return 1;
}

/* OURS */
static void port_log(const char *text)
{
	OutputDebugStringA(text);
	OutputDebugStringA("\n");
}

static const Agent_Host g_portHost = {
	"\\\\.\\pipe\\rupture_port", port_status, port_execute, NULL, port_matrix, port_log,
	"PAUSE MUTE STEP RUN_TO DUMP QUIT"
};

/* OURS */
bool AgentPort_Start(AgentPort_Draw draw, AgentPort_Present present, int width, int height, double now)
{
	g_now = now;
	g_draw = draw;
	g_present = present;
	g_width = width;
	g_height = height;
	g_pixels.resize((size_t)width * height * 4);
	return Agent_Start(&g_portHost) != 0;
}

/* OURS */
void AgentPort_Pass(void)
{
	Agent_Service();
}

/* OURS */
bool AgentPort_Paused(void)
{
	return g_paused;
}

/* OURS */
bool AgentPort_Quit(void)
{
	return g_quit;
}

/* OURS */
double AgentPort_Step(void)
{
	return g_dt;
}

/* OURS. The clock for the next frame: one step on, or a shortened one. */
double AgentPort_Advance(void)
{
	g_now += g_next > 0.0 ? g_next : g_dt;
	g_next = 0.0;
	return g_now;
}

/* OURS. Counts a frame drawn. */
void AgentPort_FrameDone(void)
{
	Agent_FrameDone();
}

/* OURS. The finished frame from the back buffer, before the swap. */
void AgentPort_Capture(void)
{
	GLint was = 0;

	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &was);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadBuffer(GL_BACK);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, g_width, g_height, GL_RGBA, GL_UNSIGNED_BYTE, &g_pixels[0]);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)was);
	g_captured = true;
}
