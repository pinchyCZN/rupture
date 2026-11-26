#include "part_boat.h"

#include <math.h>
#include <string.h>

#include "../core/gl1.h"
#include "scene.h"

/* ORIGIN 0x0043caf0 FUN_0043caf0, the Camera constructor: eye, target and up all {1, 0, 0, 0}. */
static Camera *camera_construct(Camera *c)
{
	c->eye = Vec4_Make(0.0f, 0.0f, 0.0f);
	c->target = Vec4_Make(0.0f, 0.0f, 0.0f);
	c->up = Vec4_Make(0.0f, 0.0f, 0.0f);
	return c;
}

/* OURS. Camera_Init with its nine doubles passed by value, as Cam9 does for Camera_Set. */
static void cam_init9(Camera *c, double ex, double ey, double ez, double tx, double ty, double tz,
                      double ux, double uy, double uz)
{
	Camera_Init(c, &ex, &ey, &ez, &tx, &ty, &tz, &ux, &uy, &uz);
}

/*
 * ORIGIN 0x00446de0 part_boat::part_boat. [open] The container at
 * +0x480 is left out.
 */
part_boat::part_boat()
{
	Path_Construct(&path);
	Track_Construct(&track);
	d330 = 0.0;
	cam338.eye = Vec4_Make(0.0f, 0.0f, 0.0f);
	cam338.target = Vec4_Make(0.0f, 0.0f, 0.0f);
	cam338.up = Vec4_Make(0.0f, 0.0f, 0.0f);
	vec11a0 = Vec4_Make(0.0f, 0.0f, 0.0f);
	vec11f4 = Vec4_Make(0.0f, 0.0f, 0.0f);
	seqPartClock = 0.0;
}

/*
 * ORIGIN 0x0046fe00 part_boat::Frame: the gate, the clock clamped
 * below at -7, two base cameras from the seq head and ten windows on
 * t, the shake from try1.sav, the light camera written by hand,
 * DrawScene and the post chain. The part hands over at t = 54. Windows
 * G..J make a Camera_Init the second one overwrites, as the demo does.
 */
void part_boat::Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq)
{
	Gfx_Ctx      *g = &ctx->gfx;
	Camera       *eye = &ctx->eye;
	Camera        cam;
	const double  pi = 3.141592653589793;              /* 0x004f36e8 */
	double        h0, h1, h2, d, d8, q, Q, x, y, r, D, E, E2, Z, a, sa, ca, s, m, K, u, v;
	double        Bd, Bc, c, w, Ws, rightV, upV, alpha, k, xa, T35, f, one = 1.0, zero = 0.0;
	float         fA, fB, px, py, L;
	Vec4          F, Fc, R, m1, m2;
	unsigned      idx, n;
	int           W = ctx->width, H = ctx->height;

	if (seq->relayMode == 1 && (seq->started[4] == 0 || seq->done[4] == 1)) {
		return;
	}
	Attrib_Push();
	ctx->seq = seq;
	memcpy(seqHead, seq->head, sizeof seqHead);
	seqPartClock = seq->partClock;
	h0 = seqHead[0];
	h1 = seqHead[1];
	h2 = seqHead[2];
	dtCopy = *dt;
	tLocal += *dt;
	t = tLocal + g_partTimeCarry;
	if (seq->override28 != 0) {
		t = h0 * 8.0 - 10.0;
	}
	if (-7.0 > t) {                                    /* 0x004f4740 */
		t = -7.0;
	}
	frames++;
	seq->partClock = ((((t + 39.0) + 64.0) + 70.0) + 65.0) + 14.0;
	ctx->nearZ = 0.5;
	ctx->farZ = 460.0;                                 /* 0x004f4d68 */
	if (t >= 8.0) {
		ctx->farZ = 160.0;                             /* 0x004f4d60 */
	}
	if (t >= 46.0) {
		ctx->nearZ = 0.005;                            /* 0x004f4d58 */
	}
	if (t >= 54.0) {                                   /* 0x004f4d50 */
		g_partTimeCarry = t - 54.0;
		Attrib_Pop();
		seq->done[4] = 1;
		seq->started[5] = 1;
		return;
	}
	ctx->nearZ = ctx->nearZ - -0.2;                    /* 0x004f4900 */
	gl1::PushMatrix();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Fbo_Bind(g, 0, 2);

	ca = cos(h0);
	sa = sin(h0);
	Cam9(eye, sa * (((h2 - 1.2) + 2.0) * 15.0), h1 * 8.0, ca * ((h2 + 0.8) * 15.0),
	     0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	Cam9(eye, 0.0 + h0 * 5.0, h1, ((t * 10.0) + (h2 * 10.0)) + 8.0,
	     0.0, 0.0, (((t * 10.0) - 10.0) + (h2 * 10.0)) + 8.0, 0.0, 1.0, 0.0);
	if (-100.0 < t && t < 8.0) {                       /* 0x004f4d48 */
		d = t - -6.0;
		q = (t - -11.0) * 0.3;
		Q = 70.0 / (q * q + 1.0);
		x = Clamp01((t - -6.5) * 0.5);
		r = 1.0 - Math_Ease(&x, &one);
		Cam9(eye, ((0.6 * Q) - 10.0) - 6.6, (r * 0.6) + ((0.0 * d) + 3.5933),
		     (((((t * 10.0) + 8.0) + 28.0) - d * 3.0) - Q * 0.2) + ((r * 0.8) * 10.0),
		     0.0, 0.0 - Q * 0.1, ((((t * 10.0) - 10.0) + 8.0) - 0.2 * d) + Q,
		     0.0, 1.0, 0.0);
	}
	if (8.0 <= t && t < 12.0) {
		d8 = t - 8.0;
		Cam9(eye, 10.0 - d8 * 1.4, 4.0933, (8.0 + t * 10.0) - d8 * 0.8,
		     0.0, 0.0, (((t * 10.0) - 10.0) + 8.0) - (t - -6.0) * 0.2,
		     0.2 - d8 * 0.1, 1.0, 0.0);
	}
	if (12.0 <= t && t < 18.0) {
		Cam9(eye, 22.3125, 3.3367, (8.0 + ((t * 10.0) - 20.0)) + ((t - 12.0) * 1.8) * 2.4,
		     0.0, 0.0, (((t * 10.0) - 10.0) - 21.0) + 8.0, 0.0, 1.0, 0.2);
	}
	if (18.0 <= t && t < 22.0) {
		camera_construct(&cam);
		D = (t - 12.0) - 6.0;
		cam_init9(&cam, 0.0, D * 1.2 + 20.0, (((6.0 + t * 10.0) - 10.0) - 5.0) - D * 2.0,
		          0.0, 0.0, ((((t * 10.0) - 10.0) - 2.0) - 5.0) - D * 2.0, 0.0, 1.0, 0.0);
		f = 1.0;
		Camera_Lerp(eye, &f, eye, &cam);
	}
	if (22.0 <= t && t < 28.0) {
		camera_construct(&cam);
		x = Clamp01((t - 22.5) * 0.2);
		E = Math_Ease(&x, &one);
		cam_init9(&cam, 0.0, 5.0 - E * 28.5, (15.0 + ((((t * 10.0) + 6.0) - 10.0) - 5.0)) + 0.2,
		          0.0, 1.5, ((((t * 10.0) - 10.0) - 2.0) - 10.0) + 15.0, 0.0, 1.0, 0.0);
		f = 1.0;
		Camera_Lerp(eye, &f, eye, &cam);
	}
	if (28.0 <= t && t < 33.0) {
		camera_construct(&cam);
		x = Clamp01(((t + 2.0) - 22.5) * 0.3);
		E = Math_Ease(&x, &one);
		cam_init9(&cam, 7.0, (t - 28.0) * 1.8 - 14.0,
		          (15.0 + (((6.0 - 2.0 * (t - 0.0)) - 10.0) - 5.0)) + 360.0,
		          0.0, 1.5, (((((t - 0.0) * 10.0) - 10.0) - 2.0) - 10.0) + 15.0, 0.0, 1.0, 0.0);
		f = 1.0;
		Camera_Lerp(eye, &f, eye, &cam);
	}
	Z = (((((t - 0.0) * 10.0) - 10.0) - 2.0) - 10.0) + 15.0;
	if (33.0 <= t && t < 44.0) {
		camera_construct(&cam);
		x = Clamp01(((t + 2.0) - 22.5) * 0.3);
		E = Math_Ease(&x, &one);
		cam_init9(&cam, 17.0, 10.0 * h2, Z, 0.0, 1.5, Z, 0.0, 1.0, 0.0);
		a = ((((t - 33.0) * 0.5) + 2.6) + 0.2) - 0.4;
		sa = sin(a);
		ca = cos(a);
		cam_init9(&cam, ca * 15.0, 6.6, (sa * 15.0 + (t - 0.0) * 10.0) - 10.0,
		          0.0, 1.5, (((t - 0.0) * 10.0) - 10.0) + 10.0, 0.0, 1.0, 0.0);
		f = 1.0;
		Camera_Lerp(eye, &f, eye, &cam);
	}
	if (44.0 <= t && t < 46.0) {
		camera_construct(&cam);
		x = Clamp01(((t + 2.0) - 22.5) * 0.3);
		E = Math_Ease(&x, &one);
		cam_init9(&cam, 17.0, 10.0 * h2, Z, 0.0, 1.5, Z, 0.0, 1.0, 0.0);
		a = ((((t - 33.0) * 0.5) + 2.6) + 0.2) + h2;
		s = t - 44.0;
		m = s * -8.0;
		sa = sin(a);
		ca = cos(a);
		cam_init9(&cam, ca * 0.0, s * h2 + (m - 4.0), (((sa * 0.0) + 440.00000000000006) - 0.0) + 7.5,
		          0.0, m + 20.0, 370.00000000000006, 0.0, 1.0, 0.0);
		f = 1.0;
		Camera_Lerp(eye, &f, eye, &cam);
	}
	if (46.0 <= t && t < 49.0) {
		camera_construct(&cam);
		x = Clamp01(((t + 2.0) - 22.5) * 0.3);
		E = Math_Ease(&x, &one);
		cam_init9(&cam, 17.0, 10.0 * h2, Z, 0.0, 1.5, Z, 0.0, 1.0, 0.0);
		K = (((t - 44.0) * 3.0) + 1.0) + 5.0;
		cam_init9(&cam, 0.0, (K - 4.0) - 56.0, 433.00000000000006,
		          0.0, ((K + 120.0) - 56.0) + ((t - 46.0) * h2) * 10.0, 373.00000000000006,
		          0.0, 1.0, 0.0);
		f = 1.0;
		Camera_Lerp(eye, &f, eye, &cam);
	}
	if (49.0 <= t && t < 146.0) {                      /* 0x004f4ca8 */
		camera_construct(&cam);
		x = Clamp01((((t - 5.0) + 2.0) - 22.5) * 0.3);
		E = Math_Ease(&x, &one);
		cam_init9(&cam, 17.0, 10.0 * h2, Z, 0.0, 1.5, Z, 0.0, 1.0, 0.0);
		y = Clamp01((t - 51.0) * 0.4);
		E2 = Math_Ease(&y, &one);
		u = (t - 5.0) - 44.0;
		v = ((42.0 - u * 14.0) + 2.0) - 6.0;
		sa = sin(0.0);
		ca = cos(0.0);
		cam_init9(&cam, ca * 0.0, (v - 4.0) + u * 0.0, (((sa * 0.0) + 440.00000000000006) - 0.0) + 7.5,
		          0.0, (E2 * 7.0) * 10.0 + (v + 20.0), 370.00000000000006,
		          ((((t - 49.0) * 0.6) * 0.1) * 4.6) - 0.3, 1.0, 0.0);
		f = 1.0;
		Camera_Lerp(eye, &f, eye, &cam);
	}
	(void)E;

	t10 = t * 10.0;
	Vec4_Sub(&eye->target, &eye->eye, &F);
	Fc = F;
	Vec4_Normalize(&Fc);
	Vec4_Normalize(&F);
	Vec4_Cross(&F, &eye->up, &R);
	T35 = t * 35.0;                                    /* 0x004f47c0 */
	n = (unsigned)path.points.size() - 1;
	idx = (unsigned)(long)T35 % n;
	Bd = h2 * 0.01;
	Bc = 0.5;
	c = Clamp01(t - 0.0);
	if (t < 8.0) {
		Bc = 0.8;
		Bd = c * 0.008;                                /* 0x004f4c98 */
		idx = (unsigned)(long)(h2 * 0.0 + T35) % n;
	}
	if (t >= 8.0) {
		Bd = 0.002;                                    /* 0x004f40b0 */
	}
	if (t < 0.0) {
		idx = (unsigned)(long)(T35 + 1020.0) % n;      /* 0x004f4c90 */
	}
	w = Clamp01((t - -6.0) * 0.4);
	Ws = sin(w * pi) * 0.0;
	px = path.points[idx].x;
	py = path.points[idx].y;
	rightV = (((px * 1.1) * Bd) * 0.01) * 4.8;
	upV = (((py * 1.1) * Bd) * 0.01) * 4.8;
	fA = (float)(Bd + Ws);
	Vec4_Scale(&R, &px, &m1);
	Vec4_Scale(&m1, &fA, &m2);
	Vec4_Add(&eye->target, &m2, &eye->target);
	fB = fA;
	Vec4_Scale(&eye->up, &py, &m1);
	Vec4_Scale(&m1, &fB, &m2);
	Vec4_Add(&eye->target, &m2, &eye->target);
	eye->up.x = (float)(sin((path.a[idx] * 1.7188733853924696) * Bd) * Bc + eye->up.x);
	ctx->light = *eye;
	ctx->light.eye = Vec4_Make(0.0f, 20.0f, (float)(t * 10.0));
	ctx->light.target = Vec4_Make(0.0f, 0.0f, (float)(10.0 * t));
	ctx->light.up = Vec4_Make(1.0f, 0.0f, 0.0f);

	DrawScene(ctx, 1);
	Fbo_Resolve(g, 0, W, H, 2);
	Fbo_Unbind();
	glDisable(GL_DEPTH_TEST);
	Post_RunPass(g, passBlur, 2, 1, W / 2, H / 2, texNormal, texGlow, -1, -1);
	Post_RunPass(g, passWire, 0, 1, W, H, texNormal, texNormal, texGlow, texColour);
	Post_RunPass(g, passBlur, 2, 6, W / 2, H / 2, texNormal, texGlow, -1, -1);
	xa = Clamp01((t - -7.0) * 0.4);
	k = (1.0 - Math_Ease(&xa, &one)) * 0.6;
	Shader_SetParam1f(&g->shader, passBend, "RightV", &rightV);
	Shader_SetParam1f(&g->shader, passBend, "UpV", &upV);
	alpha = k + 0.2;
	Shader_SetParam1f(&g->shader, passBend, "alpha", &alpha);
	Shader_SetParam1f(&g->shader, passBend, "ttime", &t);
	Post_RunPass(g, passBend, 0, 1, W, H, texNormal, texNormal, texGlow, -1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	gl1::PushMatrix();
	glViewport(0, 0, W, H);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	gl1::LoadIdentity();
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(g, texGlow);
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	Tex_Bind(g, texQuarter);
	gl1::Color3f(0.2f, 0.2f, 0.7f);                    /* 0x004f3754, 0x004f4060 */
	Tex_Bind(g, texNormal);
	L = (float)(Clamp01(t - -7.0) - Clamp01(t - 53.0));
	gl1::Color3f(L, L, L);
	Gfx_DrawQuad(&zero, &one, &one, 1);
	gl1::PopMatrix();
	gl1::PopMatrix();
	Attrib_Pop();
	seq->partEnd = t;
	glDisable(GL_BLEND);
	glFlush();
}

/*
 * ORIGIN 0x0046f920 part_boat::Init: part_rail::Init's shape in slot
 * band 4000. The targets only with seq->relayMode 0, blob41b through
 * Tex_Load and box.3ds stood in for by its material, as part_motor::Init
 * explains. The meshes' own lists are fixed ids in the demo
 * (0x4c7..0x4c9, 0x4df); here they are gl1's. The per-group lists
 * 1000..1004 keep the demo's ids, which DrawScene calls by number.
 */
void part_boat::Init(Rupture_Ctx *ctx)
{
	Gfx_Ctx    *g = &ctx->gfx;
	Shader_Ctx *s = &g->shader;
	int         w = ctx->width, h = ctx->height, id, kk;
	Vec4        pt, step;

	t = 0.0;
	val11b8 = 0x70;
	tLocal = 0.0;
	val11bc = 0x70;
	frames = 0;
	texNormal = 0;
	texColour = 0xb;
	slot11c8 = 0x6f;
	texQuarter = 2;
	texGlow = 3;
	texSlot = 1;
	texBase = 4000;
	model = 4000;
	pairAnisoHex = 0xfa2;
	passBlurCirc = 0xfb5;
	passBlur = 0xfb7;
	passBend = 0xfbf;
	passBlurFOV = 0xfbd;
	passWire = 0xfbb;
	pairBoat = 0xfc0;
	pairBoat2 = 0xfca;
	pairTrain2 = 0xfc2;
	pairBridge23 = 0xff8;
	pairAnisoHex2 = 0xfc6;
	slot214 = 0xfb9;
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
	g->tex[texBase + 30] = Tex_Load("textures/cloudt2.jpg");
	g->tex[texBase + 20] = Tex_LoadWithAlpha("textures/blob41b.jpg", "textures/blob41b.jpg");
	g->tex[texBase + 12] = Tex_Load("textures/bg08.jpg");
	Obj_Load(&hex, gl1::GenLists(1), "meshes/hex.obj");
	Obj_Load(&boat, gl1::GenLists(1), "meshes/boat.obj");
	Obj_Load(&trainlong, gl1::GenLists(1), "meshes/trainlong.obj");
	Obj_Load(&cylinders, gl1::GenLists(1), "meshes/cylinders.obj");
	for (id = 1000; id < 1005; id++) {
		gl1::NewList(id, gl1::COMPILE);
		Obj_DrawGroup(&cylinders.groups[id - 1000]);
		gl1::EndList();
	}

	Track_Clear(&track);
	step = Vec4_Make(0.0f, 0.0f, 1.0f);
	pt = Vec4_Make(0.0f, 0.0f, 0.0f);
	for (kk = 0; kk < 600; kk++) {
		Vec4_Add(&pt, &step, &pt);
		track.points.push_back(pt);
		track.count++;
	}
	g->tex[(model + 1000) * 100] = Tex_Load("meshes/marmaro.jpg");
	Shader_LoadPair(s, "cgeffects/vertex_projection_anisotropic_hex.cpp",
	                "cgeffects/fragment_projection_anisotropic_hex.cpp", pairAnisoHex);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_boat.cpp", pairBoat);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_boat2.cpp", pairBoat2);
	Shader_LoadPair(s, "cgeffects/vertex_projection_anisotropic.cpp",
	                "cgeffects/fragment_projection_anisotropic_hex2.cpp", pairAnisoHex2);
	Shader_LoadPair(s, "cgeffects/v_roadcut3.cpp", "cgeffects/f_bridge23.cpp", pairBridge23);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_train2.cpp", pairTrain2);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256.cpp", passBlur);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256FOV.cpp", passBlurFOV);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_Blur_circular.cpp", passBlurCirc);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_bend3.cpp", passBend);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_wireframe.cpp", passWire);
	Path_Load(&path, "meshes/try1.sav");
	t = -7.0;                                          /* 0x004f4740 */
	tLocal = -7.0;
}
