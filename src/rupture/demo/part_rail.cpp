#include "part_rail.h"

#include <math.h>
#include <string.h>

#include "../core/gl1.h"
#include "part_truck.h"
#include "scene.h"

/* ORIGIN 0x0043caf0 FUN_0043caf0, the Camera constructor: eye, target and up all {1, 0, 0, 0}. */
static Camera *camera_construct(Camera *c)
{
	c->eye = Vec4_Make(0.0f, 0.0f, 0.0f);
	c->target = Vec4_Make(0.0f, 0.0f, 0.0f);
	c->up = Vec4_Make(0.0f, 0.0f, 0.0f);
	return c;
}

/*
 * ORIGIN 0x00446bc0 part_rail::part_rail. [open] Only the members
 * Frame, Init and DrawScene use; the ten Obj meshes Init never loads
 * and the container at +0x630 are left out.
 */
part_rail::part_rail()
{
	int i;

	Path_Construct(&path);
	vec2d4 = Vec4_Make(0.0f, 0.0f, 0.0f);
	d370 = 0.0;
	camera_construct(&cam378);
	vec1350 = Vec4_Make(0.0f, 0.0f, 0.0f);
	vec13e4 = Vec4_Make(0.0f, 0.0f, 0.0f);
	Track_Construct(&track1400);
	Track_Construct(&track);
	for (i = 0; i < 5; i++) {
		Track_Construct(&rails[i]);
	}
	t = 0.0;
	tLocal = 0.0;
	frames = 0;
	dtCopy = 0.0;
	u = 0.0;
	seqHead[0] = seqHead[1] = seqHead[2] = 0.0;
}

/* OURS. The first Camera_Set of windows 24..51, the same block inline five times in the demo. */
static void rail_shot(Camera *eye, double T)
{
	double a, x, e, B, s, W, one = 1.0;

	a = T * 0.2 - 3.4;
	x = Clamp01(((T - 24.0) + 3.0) * 0.1);
	e = Math_Ease(&x, &one);
	B = (e * 100.0 - 140.0) - 840.0;                   /* 0x004f51d8, 0x004f6580 */
	s = (T - 23.0) * 0.5;
	W = (Math_SoftRamp(&s) * 4.0 - 16.0) * 10.0;
	Cam9(eye, (sin(a) * 0.0 + 45.300000000000004) + 1386.0, (W - 63.0) - 4.0, B, 1386.0,
	     ((((W - 68.0) + 25.0) - 20.0) - 29.0) + 4.0, (15.0 - (T - 24.0) * 8.6) + B,
	     0.5, 1.0, 0.0);
}

/*
 * ORIGIN 0x0049d280 part_rail::Frame: the gate, the clock, two base
 * cameras and twelve windows tested on t, the shake from try1.sav, the
 * light camera, DrawScene and the post chain. The part hands over at
 * t = 65. Windows 38..51 construct a camera they never use, as the
 * demo does.
 */
void part_rail::Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq)
{
	Gfx_Ctx      *g = &ctx->gfx;
	Camera       *eye = &ctx->eye;
	Camera        cam2;
	const double  pi = 3.141592653589793;              /* 0x004f36e8 */
	const double  K14 = -1.4000000000000001;           /* 0x004f6590 */
	double        h1, h2, T, m, M45, M28, M75, Z, a, b, d, e, e1, f, gz, K, D, r, v, x, y, L;
	double        Bd, Bc, c, w, Ws, P, rightV, upV, alpha, T35, one = 1.0, zero = 0.0;
	double        c1, c2, c3, c4, c5, c6, c7, c8, c9;
	float         fA, fB, px, py;
	Vec4          F, Fc, R, m1, m2;
	unsigned      idx, n;
	int           W = ctx->width, H = ctx->height;

	if (seq->relayMode == 1 && (seq->started[3] == 0 || seq->done[3] == 1)) {
		return;
	}
	Attrib_Push();
	ctx->seq = seq;
	memcpy(seqHead, seq->head, sizeof seqHead);
	h1 = seqHead[1];
	h2 = seqHead[2];
	dtCopy = *dt;
	tLocal += *dt;
	if (seq->override28 != 0) {
		tLocal = seqHead[0] * 10.0 + 15.0;
	}
	if (tLocal < 0.0) {
		tLocal = 0.0;
	}
	frames++;
	t = g_partTimeCarry + tLocal;
	seq->partClock = (((tLocal + 39.0) + 64.0) + 70.0) + 7.0;
	ctx->farZ = 550.0;                                 /* 0x004f6128 */
	ctx->nearZ = 0.45;                                 /* 0x004f42e8 */
	if (t >= 65.0) {                                   /* 0x004f4938 */
		g_partTimeCarry = t - 65.0;
		Attrib_Pop();
		seq->done[3] = 1;
		seq->started[4] = 1;
		return;
	}
	gl1::PushMatrix();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Fbo_Bind(g, 0, 2);
	u = t * 27.0;                                      /* 0x004f4ef8 */
	T = t;
	m = tLocal - 0.0;
	if (m < 0.0) {
		m = 0.0;
	}
	M45 = m * 45.0;

	Z = (450.0 - M45) + 100.0;
	Cam9(eye, ((((h2 * 50.0 - 40.0) + 800.0) + 580.0) + 1.4) + 4.8, 10.0 * h1 - 64.0, Z + 10.0,
	     1386.2, -69.0, Z, 0.0, 1.0, 0.0);
	Cam9(eye, 10.0 * h2 + 1306.2, 10.0 * h1 - 42.0, Z + 10.0, 1386.2, -69.0, Z, 0.0, 1.0, 0.0);
	if (-10.0 < T && T < 6.0) {
		a = T * 0.2 - 3.4;
		x = Clamp01((T - 2.0) * 0.15);
		e = Math_Ease(&x, &one) + 0.0;
		Cam9(eye, sin(a) * 10.0 + 1386.0, e * 3.0 - 68.0,
		     (((cos(a) * 10.0 - M45) + 550.0) + 0.0) - (1.2 * e) * 10.0,
		     1386.0, -66.0, 550.0 - M45, 0.0, 1.0, e * -0.6);
	}
	if (6.0 <= T && T < 11.0) {
		a = T * 0.2 - 3.4;
		M28 = m * 28.0;
		d = T - 6.0;
		Cam9(eye, (sin(a) * 0.0 + (((d * 40.0) * 0.1) * 1.4 - 7.6)) + 1386.0, -67.0,
		     ((((cos(a) * 0.0 - M28) + 12.0) + 550.0) + 0.0) - 120.0,
		     1386.0, -68.0, (((((12.0 - M28) + 550.0) - 120.0) - 100.0) + 50.0) - d * -15.0,
		     0.0, 1.0, 0.0);
	}
	if (11.0 <= T && T < 20.0) {
		x = Clamp01((T - 10.5) * 0.25);
		r = 1.5 - Math_Ease(&x, &one) * 0.5;
		a = 0.2 + ((T * 0.2 - 1.6) - 3.4);
		Cam9(eye, sin(a) * (r * 5.0) + 1386.0, h2 * 0.0 - 68.0, (cos(a) * (r * 8.0) - M45) + 550.0,
		     1386.0, -66.0, 550.0 - M45, 0.0, 1.0, -0.3);
	}
	if (20.0 <= T && T < 24.0) {
		d = T - 20.0;
		Cam9(eye, (sin(K14) + sin(K14)) + 1386.0, d * 0.0 - 65.4, ((cos(K14) * 8.0 - M45) + 553.0) - d,
		     1386.0, -66.0, (553.0 - M45) - d * 0.7, 0.0, 1.0, -0.1);
	}
	if (24.0 <= T && T < 38.0) {
		rail_shot(eye, T);
		camera_construct(&cam2);
		b = T * -0.5 - 2.2;
		gz = (T - 20.0) * 0.0;
		c1 = sin(b) * 8.0 + 1386.0;
		c2 = -66.0;
		c3 = ((((cos(b) * 8.0 - M45) + 553.0) - gz) - 13.0) + 5.0;
		c4 = 1386.0;
		c5 = -69.0;
		c6 = (((553.0 - M45) - gz) - 13.0) + 9.0;
		c7 = 0.0;
		c8 = 1.0;
		c9 = -0.1;
		Camera_Init(&cam2, &c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9);
		x = Clamp01((T - 31.0) * 0.28);
		L = Math_Ease(&x, &one);
		Camera_Lerp(eye, &L, eye, &cam2);
	}
	if (38.0 <= T && T < 40.0) {
		rail_shot(eye, T);
		camera_construct(&cam2);
		b = 1.2 - (T * 0.1 + 2.2);
		gz = (T - 20.0) * 0.0;
		Cam9(eye, sin(b) * 8.0 + 1386.0, -68.0,
		     ((((((cos(b) * 8.0 - M45) + 553.0) - gz) - 13.0) + 5.0) - 10.0) + (T - 38.0) * 2.2,
		     1386.0, -69.0, (((553.0 - M45) - gz) - 13.0) + 9.0, 0.0, 1.0, -0.1);
	}
	if (40.0 <= T && T < 42.0) {
		rail_shot(eye, T);
		camera_construct(&cam2);
		b = 1.2 - (T * 0.1 + 2.2);
		gz = (T - 20.0) * 0.0;
		Cam9(eye, (sin(b) * 8.0 + 1386.0) - 18.0, -60.0,
		     ((((((cos(b) * 8.0 - M45) + 553.0) - gz) - 13.0) + 5.0) - 10.0)
		         + ((T - 38.0) * 2.2) * 3.2,
		     1386.0, -69.0, (((553.0 - M45) - gz) - 13.0) + 9.0, 0.0, 1.0, -0.3);
	}
	if (42.0 <= T && T < 44.0) {
		rail_shot(eye, T);
		camera_construct(&cam2);
		b = h2 - (T * 0.1 + 2.2);
		m = m + 6.0;
		K = m * 5.0;
		gz = (T - 20.0) * 0.0;
		Cam9(eye, ((sin(b) * 9.0 + 1386.0) - 18.0) + 50.0, -47.0,
		     (((((((cos(b) * 9.0 + K) + 553.0) - gz) - 13.0) + 5.0) - 10.0) - 2250.0) + 70.0,
		     1386.0, -69.0, ((((K + 553.0) - gz) - 13.0) + 9.0) - 2250.0, 0.0, 1.0, 0.3);
	}
	if (44.0 <= T && T < 51.0) {
		rail_shot(eye, T);
		camera_construct(&cam2);
		b = 444.0 - (T * 0.1 + 2.2);
		m = (h2 * 0.0 - 50.0) + m;
		K = m * 42.0;
		gz = (T - 20.0) * 0.0;
		Cam9(eye, (((sin(b) * 0.0 + 1386.0) - 18.0) - 15.0) + (((T - 44.0) * 3.2) * 5.0) * 1.8, -66.2,
		     (((((((cos(b) * 0.0 - K) + 553.0) - gz) - 13.0) + 5.0) - 10.0) - 2250.0) + 70.0,
		     1386.0, -49.0, ((((553.0 - K) - gz) - 13.0) + 9.0) - 2250.0, 0.0, 1.0, 0.3);
	}
	if (51.0 <= T && T < 54.0) {
		M75 = m * 75.0;
		d = T - 20.0;
		Cam9(eye, (sin(K14) + sin(K14)) + 1386.0, d * 0.0 - 65.4,
		     (((((cos(K14) * 8.0 - M75) + 553.0) - d) + 1470.0) + ((T - 51.0) * 1.2) * 10.0) - 6.0,
		     1386.0, -66.0, ((((553.0 - M75) - d * 0.7) + 1470.0) - 6.0) + 10.4, 0.0, 1.0, 2.2);
	}
	if (54.0 <= T && T < 58.0) {
		a = (T * 0.8 - 1.6) + 0.4;
		D = 553.0 - m * 75.0;
		e = T - 54.0;
		Cam9(eye, sin(a) * 9.0 + 1386.0, (cos(a) * 9.0 - 66.0) - 9.2,
		     (((D - (T - 20.0)) + 1470.0) - 20.0) + 20.0 * e,
		     1386.0, -66.0, (((((D + 1470.0) - 6.0) + 10.4) - 20.0) - 20.0) + e * 16.2,
		     0.6, 1.0, 2.2);
	}
	if (58.0 <= T && T < 158.0) {
		a = (T * 0.3 - 1.6) + 0.4;
		x = Clamp01((T - 58.0) * 0.5);
		e1 = Math_Ease(&x, &one) - 1.0;
		y = Clamp01((T - 60.0) * 0.2);
		v = 0.4 - Math_Ease(&y, &one);
		D = (553.0 - m * 75.0) + 1375.0;
		f = T - 58.0;
		Cam9(eye, sin(a) * 12.0 + 1386.0, (v * 10.0 - 66.0) + cos(a) * 12.0,
		     ((((D - 40.0) + f * 9.0) - 10.0) + (e1 * 4.2) * 10.0) + 7.0,
		     1386.0, -66.0, ((((D - 50.0) - -56.0) - f * 10.0) - 10.0) + 7.0, 0.0, 1.0, 0.8);
	}

	Vec4_Sub(&eye->target, &eye->eye, &F);
	Fc = F;
	Vec4_Normalize(&Fc);
	Vec4_Normalize(&F);
	Vec4_Cross(&F, &eye->up, &R);
	T35 = t * 35.0;                                    /* 0x004f47c0 */
	n = (unsigned)path.points.size() - 1;
	idx = (unsigned)(long)T35 % n;
	Bd = 0.012;                                        /* 0x004f6500 */
	Bc = 1.0;
	if (t >= 6.0) {
		Bc = 0.1;
		Bd = 0.004;                                    /* 0x004f64f8 */
	}
	if (20.0 <= t && t < 24.0) {
		Bd = 0.0;
		Bc = 0.0;
	}
	if (t >= 24.0) {
		Bc = 0.5;
		Bd = 0.004;
	}
	if (t >= 51.0) {
		Bc = 0.5;
		idx = (unsigned)(long)(T35 + 280.0) % n;
		Bd = 0.004;
	}
	c = Clamp01((t - 62.0) * 0.5);
	Bd = (1.0 - c) * Bd;
	if (24.0 <= t && t < 38.0) {
		idx = (unsigned)(long)((T35 + 280.0) + 210.0) % n;
	}
	w = Clamp01((t - 35.7) * 0.3);
	Ws = Clamp01(sin(w * pi) * 3.0) * 0.025;
	P = (Bd * 0.01) * 3.6;
	px = path.points[idx].x;
	py = path.points[idx].y;
	rightV = (P - (0.01 * Ws) * 2.6) * (px * -1.1);
	upV = (P + Ws * 0.02) * (py * -1.1);
	if (t < 42.0 && 28.0 < t) {
		rightV = rightV * 2.0;
		upV = upV * 2.0;
	}
	if (t >= 51.0) {
		rightV = rightV * 2.2;
		upV = upV * 2.2;
	}
	fA = (float)(Ws * 0.3 + Bd);
	Vec4_Scale(&R, &px, &m1);
	Vec4_Scale(&m1, &fA, &m2);
	Vec4_Add(&eye->target, &m2, &eye->target);
	fB = (float)(Ws + Bd);
	Vec4_Scale(&eye->up, &py, &m1);
	Vec4_Scale(&m1, &fB, &m2);
	Vec4_Add(&eye->target, &m2, &eye->target);
	eye->up.x = (float)(sin((path.a[idx] * 1.7188733853924696) * Bd) * (Bc * 0.3) + eye->up.x);
	ctx->light = *eye;
	Cam9(&ctx->light, 0.0, 0.0, 30.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	DrawScene(ctx, 1);
	Fbo_Resolve(g, 0, W, H, 2);
	Fbo_Unbind();
	glDisable(GL_DEPTH_TEST);
	Post_RunPass(g, passBlur, 2, 1, W / 2, H / 2, texNormal, texGlow, -1, -1);
	Post_RunPass(g, passWire, 0, 1, W, H, texNormal, texNormal, texGlow, texColour);
	Post_RunPass(g, passBlur, 2, 6, W / 2, H / 2, texNormal, texGlow, -1, -1);
	Shader_SetParam1f(&g->shader, passBend, "RightV", &rightV);
	Shader_SetParam1f(&g->shader, passBend, "UpV", &upV);
	alpha = 0.2;
	Shader_SetParam1f(&g->shader, passBend, "alpha", &alpha);
	Post_RunPass(g, passBend, 0, 1, W, H, texNormal, texNormal, texGlow, -1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	gl1::PushMatrix();
	glViewport(0, 0, W, H);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	gl1::LoadIdentity();
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(g, texGlow);
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	Tex_Bind(g, texQuarter);
	gl1::Color3f(0.3f, 0.3f, 0.3f);
	Tex_Bind(g, texNormal);
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	Gfx_DrawQuad(&zero, &one, &one, 1);
	gl1::PopMatrix();
	gl1::PopMatrix();
	Attrib_Pop();
	seq->partEnd = t;
	glDisable(GL_BLEND);
	glFlush();
}

/* ORIGIN 0x004023b0 Vec4_RotateEuler: *v = Mat_FromEuler(a, b, c) applied to *v, each angle via float. */
static void vec4_rotate_euler(Vec4 *v, const double *a, const double *b, const double *c)
{
	float m[16];

	Mat_FromEuler(m, (float)*a, (float)*b, (float)*c);
	Mat_Apply(m, v, v);
}

/*
 * ORIGIN 0x0049cb50 part_rail::Init. part_truck::Init's shape: the
 * targets only with seq->relayMode 0 and box.3ds stood in for by its material, as part_motor::Init explains.
 * The meshes' own lists are fixed ids in the demo; here they are gl1's.
 * The per-group lists 1000..1005 and 2000..2002 keep the demo's ids,
 * which DrawScene calls by number. No rand and no clock reset.
 */
void part_rail::Init(Rupture_Ctx *ctx)
{
	Gfx_Ctx    *g = &ctx->gfx;
	Shader_Ctx *s = &g->shader;
	int         w = ctx->width, h = ctx->height, id, j, kk;
	Vec4        pt, step;
	double      ang, ra, rc;

	t = 0.0;
	val13a8 = 0x70;
	tLocal = 0.0;
	val13ac = 0x70;
	frames = 0;
	texNormal = 0;
	texColour = 0xb;
	slot13b8 = 6;
	texQuarter = 2;
	texGlow = 3;
	texSlot = 1;
	texBase = 3000;
	model = 3000;
	pairAniso = 0xbba;
	passBlurCirc = 0xbcd;
	passBlur = 0xbcf;
	passMotion = 0xbe1;
	passBend = 0xbd7;
	passBlurFOV = 0xbd5;
	passWire = 0xbd3;
	slot13f8 = 0xbd8;
	slot16ac = 0xbde;
	slot16b0 = 0xbdc;
	slot16b4 = 0xbda;
	slot16b8 = 0xbe4;
	slot16bc = 0xbf0;
	slot16c0 = 0xbf6;
	slot308 = 0xbd1;
	slot13fc = 0xbe6;
	slot16a8 = 0xbe8;
	slot13a4 = 0xbea;
	slot2c8 = 0xbec;
	slot2cc = 0xc02;
	slot2d0 = 0xbee;
	pairBridge2 = 0xbf2;
	slot16a0 = 0xbfe;
	pairTrain = 0xc00;
	pairEngine = 0xc04;
	pairHand = 0xc0a;
	pairHead = 0xc0c;
	if (ctx->seq->relayMode == 0) {
		Tex_CreateTarget(g, 2, w / 4, h / 4);
		Tex_CreateTarget(g, texGlow, w / 2, h / 2);
		Tex_CreateTarget(g, texNormal, w, h);
		Tex_CreateTarget(g, texColour, w, h);
		Fbo_AttachTexture(g, 0, 0, texNormal);
		Fbo_AttachTexture(g, 0, 1, texColour);
		Fbo_AttachTexture(g, 1, 0, texQuarter);
		Fbo_AttachTexture(g, 2, 0, texGlow);
	}
	g->tex[texBase + 10] = Tex_Load("textures/pattern5.jpg");
	g->tex[texBase + 20] = Tex_LoadWithAlpha("textures/blob41b.jpg", "textures/blob41b.jpg");
	g->tex[texBase + 30] = Tex_Load("textures/cloudt2.jpg");
	g->tex[texBase + 12] = Tex_Load("textures/bg08.jpg");
	Obj_Load(&hand, gl1::GenLists(1), "meshes/hand.obj");
	Obj_Load(&g_boatObj, gl1::GenLists(1), "meshes/boat.obj");
	Obj_Load(&trainlong, gl1::GenLists(1), "meshes/trainlong.obj");
	Obj_Load(&cylinders, gl1::GenLists(1), "meshes/cylinders.obj");
	Obj_Load(&human, gl1::GenLists(1), "meshes/human.obj");

	Track_Clear(&track);
	step = Vec4_Make(0.0f, 0.0f, -0.7f);               /* 0x004f64ac */
	pt = Vec4_Make(0.0f, -50.0f, 850.0f);              /* 0x004f5db4, 0x004f64a8 */
	for (kk = 0; kk < 2950; kk++) {
		Vec4_Add(&pt, &step, &pt);
		track.points.push_back(pt);
		track.count++;
	}
	for (id = 1000; id < 1006; id++) {
		gl1::NewList(id, gl1::COMPILE);
		Obj_DrawGroup(&cylinders.groups[id - 1000]);
		gl1::EndList();
	}
	for (id = 2000; id < 2003; id++) {
		gl1::NewList(id, gl1::COMPILE);
		Obj_DrawGroup(&trainlong.groups[id - 2000]);
		gl1::EndList();
	}
	for (j = 0; j < 5; j++) {
		Track_Clear(&rails[j]);
		step = Vec4_Make(0.0f, 0.0f, 0.7f);            /* 0x004f4060 */
		pt = Vec4_Make(0.0f, -50.0f, 0.0f);
		ang = (double)(j - 2) * 0.003;                 /* 0x004f6078 */
		for (kk = 0; kk < 650; kk++) {
			Vec4_Add(&pt, &step, &pt);
			ra = -0.001;                               /* 0x004f64a0 */
			rc = 0.0;
			vec4_rotate_euler(&step, &ra, &ang, &rc);
			rails[j].points.push_back(pt);
			rails[j].count++;
		}
	}
	g->tex[(model + 1000) * 100] = Tex_Load("meshes/marmaro.jpg");
	Shader_LoadPair(s, "cgeffects/vertex_projection_anisotropic.cpp",
	                "cgeffects/fragment_projection_anisotropic.cpp", pairAniso);
	Shader_LoadPair(s, "cgeffects/v_roadcut3.cpp", "cgeffects/f_bridge2.cpp", pairBridge2);
	Shader_LoadPair(s, "cgeffects/v_roadcut3.cpp", "cgeffects/f_hand.cpp", pairHand);
	Shader_LoadPair(s, "cgeffects/v_roadcut3.cpp", "cgeffects/f_head.cpp", pairHead);
	Shader_LoadPair(s, "cgeffects/v_roadcut3.cpp", "cgeffects/f_engine.cpp", pairEngine);
	Shader_LoadPair(s, "cgeffects/vertex_train.cpp", "cgeffects/fragment_trainold2.cpp", pairTrain);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256.cpp", passBlur);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256FOV.cpp", passBlurFOV);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_Blur_circular.cpp", passBlurCirc);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_bend3.cpp", passBend);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_motion.cpp", passMotion);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_wireframe.cpp", passWire);
	Path_Load(&path, "meshes/try1.sav");
}
