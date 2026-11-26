/*
 * The demo's top level: the part classes' base, the render context
 * every part is handed, the sequence object they relay through, and
 * Demo_Init / Demo_Frame (subsystems/parts.txt, subsystems/startup.txt).
 */
#ifndef RUPTURE_DEMO_H
#define RUPTURE_DEMO_H

#include "../core/camera.h"
#include "../core/post.h"
#include "../core/types.h"

struct Rupture_Seq;

/* The render context, g_renderCtx 0x00515608; members grow as parts need them. */
struct Rupture_Ctx {
	Gfx_Ctx      gfx;         /* the shader context, textures and targets */
	int          width;       /* maybe_g_viewWidth */
	int          height;      /* maybe_g_viewHeight */
	Rupture_Seq *seq;         /* ctx+0x14, g_sequence */
	Camera       eye;         /* ctx+0x1848 */
	Camera       light;       /* ctx+0x18a8 */
	double       d1918;       /* ctx+0x1918, part_boat writes 0 round DrawLit; reader [open] */
	Vec4         v1920;       /* ctx+0x1920, part_boat: {1, 0.2, -0.3, 1}; reader [open] */
	Vec4         v1930;       /* ctx+0x1930, part_boat: {1, 1, 1, 1}; reader [open] */
	float        litFlag;     /* ctx+0x1bd8, 0 while maybe_Scene_DrawLit runs */
	double       fovOffset;   /* ctx+0x608e38 */
	double       nearZ;       /* ctx+0x608e40 */
	double       farZ;        /* ctx+0x608e48 */
	double       lightAspect; /* ctx+0x608e50, 1 from the constructor */
	double       lookYaw;     /* OURS: the mouse look, degrees (app.cpp, Scene_LookOffset) */
	double       lookPitch;
};

/*
 * The sequence object, g_seq 0x00514840: the relay flags, and the
 * three doubles at its head that part_video::Frame copies into
 * this+0x348. What sets those and seq+0x28 is open; all were 0 in a
 * normal run [observed].
 */
struct Rupture_Seq {
	double head[3];       /* seq+0x00, +0x08, +0x10 */
	int    override28;    /* seq+0x28 */
	u8     started[9];    /* seq+0x48, g_seqPartStarted */
	u8     done[9];       /* seq+0x68, g_seqPartDone */
	int    relayMode;     /* seq+0xac, g_seqRelayMode */
	double partEnd;       /* seq+0x8b0 */
	double partClock;     /* seq+0x8b8 */
};

extern double g_partTimeCarry;   /* 0x00b1e598 */

/*
 * The base of every part class, RTTI's parts. The vtable has two slots
 * and no destructor: Frame (0) and Init (1) [verified,
 * subsystems/parts.txt 1].
 */
class parts {
public:
	virtual void Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq) = 0;
	virtual void Init(Rupture_Ctx *ctx) = 0;
};

enum { DEMO_PARTS = 9 };

struct Rupture_Demo {
	parts      *start;               /* g_partStart 0x00512794 */
	parts      *part[DEMO_PARTS];    /* g_parts 0x0051276c */
	Rupture_Ctx ctx;
	Rupture_Seq seq;
	Rupture_Seq initSeq;             /* the 0xdc8-byte one Demo_Init makes, relay mode 1 */
	double      clock;               /* g_demoClock 0x005146c8 */
	double      frameDt;             /* g_frameDt 0x005146c0 */
	double      lastTimer;           /* g_lastTimerSeconds 0x005146d0 */
	double      musicSeconds;        /* g_musicSeconds 0x00512748 */
	bool        musicSyncOn;         /* g_musicSyncOn 0x0050bbda */
	bool        music;               /* OURS: false runs on the host clock alone */
	double      fixedDt;             /* OURS: >= 0 is the next frame's step, no music pull; -1 runs free */
};

bool Demo_Init(Rupture_Demo *d, int width, int height);
bool Demo_Load(Rupture_Demo *d);
bool Demo_Frame(Rupture_Demo *d, double now);
void Demo_Jump(Rupture_Demo *d, int slot);
void Demo_Next(Rupture_Demo *d);

#endif
