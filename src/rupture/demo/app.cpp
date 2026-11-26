/*
 * OURS. The rupture program: the host's calls into Demo_Init and
 * Demo_Frame, and the controls the demo does not have - M mutes, + and
 * - step the volume, space holds the demo and . steps it a frame at a
 * time. It starts muted; the music plays on regardless, because the
 * demo clock follows it.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/gl1.h"
#include "../core/music.h"
#include "../host/host.h"
#include "demo.h"
#include "part_boat.h"
#include "part_city.h"
#include "part_end.h"
#include "part_motor.h"
#include "part_rail.h"
#include "part_truck.h"
#include "part_video.h"

static Rupture_Demo g_demo;
static char         g_title[192];
static bool         g_muted = true;
static int          g_volume = 10;   /* tenths; FMOD's default is full */

/* OURS. The mute flag and the volume, to the music. */
static void volume_apply(void)
{
	Music_SetVolume(g_muted ? 0.0 : g_volume / 10.0);
}
static bool         g_agent;
static int          g_jumpSlot = -1;
static double       g_jumpT;
static bool         g_jumpAt;

/* OURS. Before App_Init: no music, so the clock is the host's alone. */
void App_SetAgent(void)
{
	g_agent = true;
}

/*
 * OURS. Before App_Init: start at a part - "rail" or "part_rail" - and
 * with "@T" run on to its time T at the fixed step, without showing
 * the frames. False for a name no part has.
 */
bool App_SetJump(const char *spec)
{
	static const char *names[7] = { "video", "motor", "truck", "rail", "boat", "city", "end" };
	char               name[32];
	const char        *at = strchr(spec, '@');
	size_t             n = at != NULL ? (size_t)(at - spec) : strlen(spec);
	int                i;

	if (strncmp(spec, "part_", 5) == 0) {
		spec += 5;
		n -= 5;
	}
	if (n >= sizeof name) {
		return false;
	}
	memcpy(name, spec, n);
	name[n] = 0;
	for (i = 0; i < 7; i++) {
		if (strcmp(name, names[i]) == 0) {
			g_jumpSlot = i;
			g_jumpAt = at != NULL;
			g_jumpT = at != NULL ? atof(at + 1) : 0.0;
			return true;
		}
	}
	return false;
}

/* OURS. The jump App_SetJump asked for, then the frames up to its time, the last shortened to land on it. */
static void jump(void)
{
	const char *name;
	double      t, step;
	int         n, slot;

	Demo_Jump(&g_demo, g_jumpSlot);
	if (!g_jumpAt) {
		return;
	}
	for (n = 0; n < 60 * 120; n++) {
		slot = App_Part(&name, &t);
		if (slot > g_jumpSlot || (slot == g_jumpSlot && t >= g_jumpT - 1e-9)) {
			break;
		}
		step = 1.0 / 60.0;
		if (slot == g_jumpSlot && g_jumpT - t < step) {
			step = g_jumpT - t;
		}
		Demo_Frame(&g_demo, g_demo.lastTimer + step);
	}
	Music_Seek(g_demo.clock);
}

/* OURS. Where the host's clock starts: 0, or the time a jump left. */
double App_TimeBase(void)
{
	return g_demo.lastTimer;
}

/* OURS. To the loading screen, drawn; the host shows it, then calls App_Load. */
bool App_Init(int width, int height, std::string &error)
{
	volume_apply();
	g_demo.music = !g_agent;
	if (!Demo_Init(&g_demo, width, height)) {
		error = g_demo.ctx.gfx.shader.error;
		return false;
	}
	return true;
}

/* OURS. The parts and the music, then the jump if one was asked for. */
bool App_Load(std::string &error)
{
	if (!Demo_Load(&g_demo)) {
		error = g_demo.ctx.gfx.shader.error;
		return false;
	}
	if (g_jumpSlot >= 0) {
		jump();
	}
	volume_apply();
	return true;
}

/* OURS. Frames over each second of the host's clock, for the title. */
static double g_fpsStart = -1.0;
static int    g_fpsFrames;
static double g_fps;

/* OURS. Space and the step key: held, steps asked for, and when a step's music tick ends. */
static bool   g_paused;
static int    g_steps;
static double g_tickEnd = -1.0;

/* OURS. The mouse look. LOOK_RETURN is how fast it eases back: the offset shrinks by e^-LOOK_RETURN a second. */
static const double LOOK_DEGREES = 0.25, LOOK_RETURN = 4.0;
static bool         g_lookHeld;
static bool         g_lookHold;   /* the C key: running, the look stays as it does paused */
static double       g_lookYaw, g_lookPitch, g_lookLast = -1.0;

/*
 * OURS. The wheel's zoom: a notch is ZOOM_NOTCH steps of ZOOM_STEP, or
 * ZOOM_NOTCH_FINE with Ctrl held. ZOOM_GLIDE is how fast the lens
 * follows a notch, in the same way as LOOK_RETURN.
 */
static const double ZOOM_STEP = 1.1, ZOOM_IN = 70.0, ZOOM_OUT = 1.0 / 10.0, ZOOM_GLIDE = 14.0;
static const int    ZOOM_NOTCH = 4, ZOOM_NOTCH_FINE = 1;
static double       g_zoom = 1.0, g_zoomTarget = 1.0;

/*
 * OURS. Each frame, by the host's time: the look eases back once the
 * button is up, but not while paused, stepping included, so a paused
 * frame can be looked over at leisure, nor while the C key holds it;
 * under a hundredth of a degree it is 0 again. The zoom glides to its
 * target, paused or not.
 */
static void look_ease(double seconds)
{
	double dt = g_lookLast < 0.0 ? 0.0 : seconds - g_lookLast, f;

	g_lookLast = seconds;
	if (dt < 0.0 || dt > 0.25) {
		dt = 0.0;
	}
	if (!g_lookHeld && !g_paused && !g_lookHold) {
		f = exp(-LOOK_RETURN * dt);
		g_lookYaw *= f;
		g_lookPitch *= f;
		if (fabs(g_lookYaw) < 0.01 && fabs(g_lookPitch) < 0.01) {
			g_lookYaw = 0.0;
			g_lookPitch = 0.0;
		}
	}
	g_demo.ctx.lookYaw = g_lookYaw;
	g_demo.ctx.lookPitch = g_lookPitch;
	g_zoom += (g_zoomTarget - g_zoom) * (1.0 - exp(-ZOOM_GLIDE * dt));
	if (fabs(g_zoomTarget - g_zoom) < 0.0005) {
		g_zoom = g_zoomTarget;
	}
	gl1::SetZoom(g_zoom);
}

/* OURS. The F key: on, each frame steps FAST_SPEED times the host's time, and the music is re-seeked every FAST_SEEK. */
static const double FAST_SPEED = 4.0, FAST_SEEK = 0.25;
static bool         g_fast;
static double       g_fastSeek;

/*
 * OURS. Held, the frame is drawn again with no step and the music
 * paused. A step advances the clock by one 60th and plays that much of
 * the music from the new position, then pauses it again. False when
 * the demo has ended (Demo_Frame), the music then stopped: the host
 * closes, or in the browser parks.
 */
bool App_Frame(double seconds)
{
	double real;
	bool   running;

	look_ease(seconds);
	if (g_tickEnd >= 0.0 && seconds >= g_tickEnd) {
		Music_Pause(true);
		g_tickEnd = -1.0;
	}
	if (g_paused && g_steps > 0) {
		g_steps--;
		g_demo.fixedDt = 1.0 / 60.0;
		running = Demo_Frame(&g_demo, seconds);
		g_demo.fixedDt = 0.0;
		Music_Seek(g_demo.clock);
		Music_Pause(false);
		g_tickEnd = seconds + 1.0 / 60.0;
	} else if (g_fast && !g_paused) {
		real = seconds - g_demo.lastTimer;
		g_demo.fixedDt = FAST_SPEED * (real < 0.0 ? 0.0 : (real > 0.1 ? 0.1 : real));
		running = Demo_Frame(&g_demo, seconds);
		g_demo.fixedDt = -1.0;
		if (g_demo.clock < g_fastSeek || g_demo.clock - g_fastSeek >= FAST_SEEK) {
			Music_Seek(g_demo.clock);
			g_fastSeek = g_demo.clock;
		}
	} else {
		running = Demo_Frame(&g_demo, seconds);
	}
	if (!running) {
		Music_Pause(true);
	}
	if (g_fpsStart < 0.0 || seconds < g_fpsStart) {
		g_fpsStart = seconds;
		g_fpsFrames = 0;
	}
	g_fpsFrames++;
	if (seconds - g_fpsStart >= 1.0) {
		g_fps = g_fpsFrames / (seconds - g_fpsStart);
		g_fpsStart = seconds;
		g_fpsFrames = 0;
	}
	return running;
}

/* OURS. Space holds or releases the demo; the music picks up at the clock. */
static void pause_toggle(void)
{
	g_paused = !g_paused;
	g_steps = 0;
	g_tickEnd = -1.0;
	if (g_paused) {
		g_demo.fixedDt = 0.0;
		Music_Pause(true);
	} else {
		g_demo.fixedDt = -1.0;
		Music_Seek(g_demo.clock);
		Music_Pause(false);
	}
}

/* OURS. The agent drives its own clock, so it takes none of the clock keys. */
void App_Key(int key)
{
	if (key == ' ' && !g_agent) {
		pause_toggle();
	} else if (key == '.' && !g_agent) {
		if (!g_paused) {
			pause_toggle();
		}
		g_steps++;
	} else if (key == 'F' && !g_agent) {
		g_fast = !g_fast;
		g_fastSeek = -1.0;
		if (!g_fast && !g_paused) {
			Music_Seek(g_demo.clock);
		}
	} else if (key == ']' && !g_agent) {
		Demo_Next(&g_demo);
		g_lookYaw = 0.0;
		g_lookPitch = 0.0;
	} else if (key == 'C') {
		g_lookHold = !g_lookHold;
	} else if (key == 'M') {
		g_muted = !g_muted;
	} else if ((key == '+' || key == '=') && g_volume < 10) {
		g_volume++;
	} else if ((key == '-' || key == '_') && g_volume > 0) {
		g_volume--;
	}
	volume_apply();
}

/*
 * OURS. A drag moves the picture with the mouse, as a panorama viewer
 * does: right looks left, up looks down. The turn per pixel shrinks
 * with the zoom, so the picture keeps pace with the mouse at any zoom.
 * The yaw goes all the way round, kept within half a turn so the ease
 * back takes the short way.
 */
void App_Drag(int dx, int dy)
{
	g_lookHeld = true;
	g_lookYaw -= dx * LOOK_DEGREES / g_zoom;
	g_lookPitch -= dy * LOOK_DEGREES / g_zoom;
	while (g_lookYaw > 180.0) {
		g_lookYaw -= 360.0;
	}
	while (g_lookYaw < -180.0) {
		g_lookYaw += 360.0;
	}
	if (g_lookPitch > 85.0) {
		g_lookPitch = 85.0;
	} else if (g_lookPitch < -85.0) {
		g_lookPitch = -85.0;
	}
}

/* OURS. The button let go: the view eases back to the demo's. */
void App_DragEnd(void)
{
	g_lookHeld = false;
}

/* OURS. A wheel notch away from the user zooms in, towards zooms out, finer with Ctrl; the lens glides there. */
void App_Wheel(int notches, bool fine)
{
	g_zoomTarget *= pow(ZOOM_STEP, (double)(notches * (fine ? ZOOM_NOTCH_FINE : ZOOM_NOTCH)));
	if (g_zoomTarget > ZOOM_IN) {
		g_zoomTarget = ZOOM_IN;
	} else if (g_zoomTarget < ZOOM_OUT) {
		g_zoomTarget = ZOOM_OUT;
	}
}

/* OURS. A middle or right click: the demo's own view and lens again, at once. */
void App_ViewReset(void)
{
	g_zoom = 1.0;
	g_zoomTarget = 1.0;
	gl1::SetZoom(1.0);
	g_lookYaw = 0.0;
	g_lookPitch = 0.0;
	g_demo.ctx.lookYaw = 0.0;
	g_demo.ctx.lookPitch = 0.0;
}

/* OURS. The demo clock. */
double App_Clock(void)
{
	return g_demo.clock;
}

/* OURS. The running part's slot, name and clock; -1 before the first or after the last. */
int App_Part(const char **name, double *t)
{
	if (g_demo.seq.started[6] && !g_demo.seq.done[6]) {
		*name = "part_end";
		*t = ((part_end *)g_demo.part[6])->t;
		return 6;
	}
	if (g_demo.seq.started[5] && !g_demo.seq.done[5]) {
		*name = "part_city";
		*t = ((part_city *)g_demo.part[5])->t;
		return 5;
	}
	if (g_demo.seq.started[4] && !g_demo.seq.done[4]) {
		*name = "part_boat";
		*t = ((part_boat *)g_demo.part[4])->t;
		return 4;
	}
	if (g_demo.seq.started[3] && !g_demo.seq.done[3]) {
		*name = "part_rail";
		*t = ((part_rail *)g_demo.part[3])->t;
		return 3;
	}
	if (g_demo.seq.started[2] && !g_demo.seq.done[2]) {
		*name = "part_truck";
		*t = ((part_truck *)g_demo.part[2])->t;
		return 2;
	}
	if (g_demo.seq.started[1] && !g_demo.seq.done[1]) {
		*name = "part_motor";
		*t = ((part_motor *)g_demo.part[1])->t;
		return 1;
	}
	if (g_demo.seq.started[0] && !g_demo.seq.done[0]) {
		*name = "part_video";
		*t = ((part_video *)g_demo.part[0])->t;
		return 0;
	}
	*name = "none";
	*t = 0.0;
	return -1;
}

/* OURS. The host's mute: the PC dialog's before App_Init, the web page's at any time. */
void App_SetMuted(bool muted)
{
	g_muted = muted;
	volume_apply();
}

/* OURS. The volume in tenths, 0 to 10, as + and - step it. */
int App_Volume(void)
{
	return g_volume;
}

/* OURS */
void App_SetVolume(int tenths)
{
	g_volume = tenths < 0 ? 0 : (tenths > 10 ? 10 : tenths);
	volume_apply();
}

/* OURS */
bool App_Muted(void)
{
	return g_muted;
}

/* OURS. The title, with the running part's clock. */
const char *App_Title(void)
{
	const char *name;
	double      t;

	App_Part(&name, &t);
	sprintf(g_title, "RUPTURE - ASD 2009 (reconstruction)   %s t %.2f", name, t);
	if (g_zoom != 1.0) {
		sprintf(g_title + strlen(g_title), "   zoom %.1fx", g_zoom);
	}
	if (g_lookHold) {
		strcat(g_title, "   [CAM HOLD]");
	}
	if (g_paused) {
		strcat(g_title, "   [PAUSED]");
	} else if (g_fast) {
		strcat(g_title, "   [FAST x4]");
	} else if (g_fps > 0.0) {
		sprintf(g_title + strlen(g_title), "   %.0f fps (%.1f ms)", g_fps, 1000.0 / g_fps);
	}
	if (Music_Playing()) {
		sprintf(g_title + strlen(g_title), "   music %.1f", Music_Seconds());
	}
	if (g_muted) {
		strcat(g_title, "   [M] muted");
	} else {
		sprintf(g_title + strlen(g_title), "   volume %d%%", g_volume * 10);
	}
	if (!g_demo.ctx.gfx.shader.error.empty()) {
		sprintf(g_title + strlen(g_title), "   shader error: %.60s", g_demo.ctx.gfx.shader.error.c_str());
	}
	return g_title;
}
