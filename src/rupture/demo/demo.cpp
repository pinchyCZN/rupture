#include "demo.h"

#include <string.h>

#include "../core/gl1.h"
#include "../core/music.h"
#include "part_boat.h"
#include "part_city.h"
#include "part_end.h"
#include "part_motor.h"
#include "part_rail.h"
#include "part_truck.h"
#include "part_start.h"
#include "part_video.h"

double g_partTimeCarry;

/*
 * ORIGIN 0x00447390 Demo_Init, to its loading screen: part_start
 * built, its Init and Frame called once; the original then swaps
 * (0x00447756), which here is the host's, and Demo_Load is the rest.
 * It constructs the seven parts into g_parts, calls Init on each and
 * starts the music. part_game, demo mode -1's, is not transcribed
 * [open]. The music is FSOUND_Sample_Load's file (0x004f3f9c) through
 * core/music, not FMOD, left out when d->music is false (the agent's
 * fixed step).
 * Through the Init calls ctx->seq is a second sequence object with
 * relay mode 1 (0x0044768f), so only part_video::Init, which has no
 * gate, makes the render targets; each Frame then points ctx->seq at
 * g_seq (subsystems/parts.txt 2). The render FBOs are made here as
 * gfx_test makes them: the demo's own creation is unread. The matrix
 * mode is left at GL_PROJECTION [inferred]: part_video::Frame's
 * matrix pushes balance only if it starts there, and the mode the
 * parts' Init calls leave is unread (effects/video.txt 5.3).
 */
bool Demo_Init(Rupture_Demo *d, int width, int height)
{
	double zero = 0.0;
	int    i;

	memset(&d->seq, 0, sizeof d->seq);
	for (i = 0; i < DEMO_PARTS; i++) {
		d->part[i] = NULL;
	}
	d->ctx.width = width;
	d->ctx.height = height;
	d->clock = 0.0;
	d->frameDt = 0.0;
	d->lastTimer = 0.0;
	d->musicSeconds = 0.0;
	d->musicSyncOn = true;
	d->fixedDt = -1.0;
	d->seq.started[0] = 1;
	memset(&d->initSeq, 0, sizeof d->initSeq);
	d->initSeq.relayMode = 1;
	d->ctx.seq = &d->initSeq;
	d->ctx.fovOffset = 0.0;
	d->ctx.nearZ = 0.0;
	d->ctx.farZ = 0.0;
	d->ctx.lightAspect = 1.0;
	d->ctx.lookYaw = 0.0;
	d->ctx.lookPitch = 0.0;
	d->ctx.litFlag = 1.0f;
	g_partTimeCarry = 0.0;
	if (!Cg_Init(&d->ctx.gfx.shader)) {
		return false;
	}
	d->ctx.gfx.texBase = 0;
	d->ctx.gfx.oned = -1;
	if (!Fbo_Create(&d->ctx.gfx, 0, width, height, 4)
	 || !Fbo_Create(&d->ctx.gfx, 1, width / 4, height / 4, 0)
	 || !Fbo_Create(&d->ctx.gfx, 2, width / 2, height / 2, 0)) {
		d->ctx.gfx.shader.error = "a render target is incomplete";
		return false;
	}
	glEnable(GL_DEPTH_TEST);           /* Demo_InitGL 0x00447ad0 */
	glDepthFunc(GL_LEQUAL);
	d->start = new part_start;
	d->start->Init(&d->ctx);
	d->start->Frame(&zero, &d->ctx, d->ctx.seq);
	return d->ctx.gfx.shader.error.empty();
}

/* OURS. Demo_Init's second half, after the loading screen is shown. */
bool Demo_Load(Rupture_Demo *d)
{
	int i;

	d->part[0] = new part_video;
	d->part[1] = new part_motor;
	d->part[2] = new part_truck;
	d->part[3] = new part_rail;
	d->part[4] = new part_boat;
	d->part[5] = new part_city;
	d->part[6] = new part_end;
	for (i = 0; i < DEMO_PARTS; i++) {
		if (d->part[i] != NULL) {
			d->part[i]->Init(&d->ctx);
		}
	}
	gl1::MatrixMode(gl1::PROJECTION);
	if (d->music) {
		Music_Load("music//aMUSiC and Leviathan - Throw Navis Off The Train.mp3");
		Music_Play(0.0);
	}
	return d->ctx.gfx.shader.error.empty();
}

/*
 * OURS, the entry to a part (README.md, "Starting at a part"): the
 * parts before slot marked done, and the clock and the carry set to a
 * 1/60-step run's own values there, to the last bit [observed], since
 * the rand seed and the cuts depend on them. part_motor is entered a
 * frame early, at part_video's last t, so that part_video's hand-over
 * gives it its scaled first delta (effects/video.txt 5).
 */
static void enter(Rupture_Demo *d, int slot)
{
	static const double clock[7] = { 0.0, 38.083333333332455, 102.06666666666216, 180.05000000003542,
	                                  245.03333333342047, 306.016666666717, 364.9999999999967 };
	static const double carry[7] = { 0.0, 0.0, 0.00999999999466894,
	                                  0.010000000034608547, 0.010000000086336058,
	                                  0.010000000049529945, 0.009999999995869757 };
	int                 i;

	for (i = 0; i < slot; i++) {
		d->seq.started[i] = 1;
		d->seq.done[i] = 1;
	}
	d->seq.started[slot] = 1;
	if (slot == 1) {
		d->seq.done[0] = 0;
		d->seq.started[0] = 1;
		((part_video *)d->part[0])->t = 38.989999999998304;
	}
	g_partTimeCarry = carry[slot];
	d->clock = clock[slot];
	if (d->music) {
		Music_Seek(d->clock);
	}
}

/* OURS. --part: the entry, with the host time set to the clock as a run from the start has it. */
void Demo_Jump(Rupture_Demo *d, int slot)
{
	if (slot < 0 || slot >= 7) {
		return;
	}
	enter(d, slot);
	d->lastTimer = d->clock;
}

/*
 * OURS. The ] key: the next part entered as Demo_Jump enters it, with
 * the host's clock left running. The last part has no next.
 */
void Demo_Next(Rupture_Demo *d)
{
	int i;

	for (i = 0; i < 6; i++) {
		if (d->seq.started[i] && !d->seq.done[i]) {
			enter(d, i + 1);
			return;
		}
	}
}

/*
 * ORIGIN 0x00447b40 Demo_Frame (subsystems/clock.txt 1-3): the frame
 * delta from the host's clock; _srand((int)(clock * 1e6)) and fifty
 * _rand() draws before any part runs; then, until 230 s, a tenth of
 * the gap to the music's position added to the delta, the pull
 * latching off 5 s before the music ends; then Frame on every part,
 * each gating itself on the sequence flags. The music is taken as on,
 * as the release has it. Divergence: a browser may only start audio
 * after a key or a click, so until the music plays it is retried here
 * at the current clock, and the clock runs on the host alone. Ours:
 * d->fixedDt >= 0, set by the pause, step and fast keys (app.cpp), is the
 * frame's step instead, with no music pull. False once the clock has
 * passed 401 (k_dbl_401 0x004f4070), tested after the parts have drawn,
 * where the original closes the music and the window and exits
 * (0x0044854c); here the host does that.
 */
bool Demo_Frame(Rupture_Demo *d, double now)
{
	double pull;
	int    i;

	d->frameDt = now - d->lastTimer;
	d->lastTimer = now;
	if (d->frameDt < 0.0) {
		d->frameDt = 0.0;
	}
	if (d->fixedDt >= 0.0) {
		d->frameDt = d->fixedDt;
	}
	d->seq.relayMode = 1;
	Crt_Srand((unsigned)(long)(d->clock * 1000000.0));
	for (i = 0; i < 50; i++) {
		Crt_Rand();
	}
	if (!Music_Playing() && d->fixedDt < 0.0) {
		Music_Play(d->clock);
	}
	if (Music_Playing() && d->fixedDt < 0.0) {
		d->musicSeconds = Music_Seconds();
		if (d->musicSeconds > Music_Length() - 5.0) {          /* 0x004f40c8 */
			d->musicSyncOn = false;
		}
		pull = (d->musicSeconds - d->clock) * 0.1;              /* 0x004f3cb0 */
		if (d->clock > 230.0) {                                 /* 0x004f40c0 */
			pull = 0.0;
		}
		if (d->musicSyncOn) {
			d->frameDt += pull;
		}
	}
	d->clock += d->frameDt;
	glViewport(0, 0, d->ctx.width, d->ctx.height);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for (i = 0; i < DEMO_PARTS; i++) {
		if (d->part[i] != NULL) {
			d->part[i]->Frame(&d->frameDt, &d->ctx, &d->seq);
		}
	}
	return !(401.0 < d->clock);
}
