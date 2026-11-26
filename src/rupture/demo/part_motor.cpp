#include "part_motor.h"

#include <math.h>
#include <string.h>

#include "../core/camera.h"
#include "../core/file.h"
#include "../core/gl1.h"
#include "scene.h"

Obj_Mesh g_motorcycleObj;

/* ORIGIN 0x00446860 part_motor::part_motor. [open] Only the members Frame and Init need. */
part_motor::part_motor()
{
	Path_Construct(&path);
	Track_Construct(&road);
	t = 0.0;
	tLocal = 0.0;
	frames = 0;
	u = 0.0;
	seqHead[0] = seqHead[1] = seqHead[2] = 0.0;
}

/*
 * ORIGIN 0x004938a0 part_motor::Frame (effects/motor.txt 5): the gate,
 * the clock, the fourteen camera windows in order (a later one that
 * also holds overwrites an earlier), the shake from try1.sav, the
 * light camera, DrawScene and the post chain.
 */
void part_motor::Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq)
{
	Gfx_Ctx    *g = &ctx->gfx;
	Camera     *eye = &ctx->eye;
	const double h1 = seqHead[1], h2 = seqHead[2];
	double      a, r, d, dd, d3, e, e1, e4, m, s, k, w, q, p, x, one = 1.0, two = 2.0, zero = 0.0;
	double      A, K, M, S, A8, rightV, upV, alpha, c;
	float       Bf, px, py;
	Vec4        F, Fc, R, s1, s2;
	unsigned    idx;
	int         W = ctx->width, H = ctx->height;

	if (seq->relayMode == 1 && (seq->started[1] == 0 || seq->done[1] == 1)) {
		return;
	}
	Attrib_Push();
	ctx->seq = seq;
	memcpy(seqHead, seq->head, sizeof seqHead);
	dtCopy = *dt;
	tLocal += *dt;
	if (seq->override28 != 0) {
		tLocal = seqHead[0] * 4.0 - 20.0;
	}
	if (-35.01 > tLocal) {                             /* 0x004f6130 */
		tLocal = -35.01;
	}
	t = tLocal + g_partTimeCarry;
	frames++;
	seq->partClock = ((t + 35.0) + 39.0) - 1.0;
	ctx->farZ = 550.0;                                 /* 0x004f6128 */
	ctx->nearZ = 0.35;                                 /* 0x004f51b8 */
	if (t >= 29.0) {                                   /* 0x004f6120 */
		g_partTimeCarry = t - 29.0;
		Attrib_Pop();
		seq->done[1] = 1;
		seq->started[2] = 1;
		return;
	}
	gl1::PushMatrix();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Fbo_Bind(g, 0, 2);
	u = t * 25.0;                                      /* 0x004f46e8 */
	if (t < -5.0) {
		u = -275.0;                                    /* 0x004f6118 */
	}

	Cam9(eye, u + 0.0, ((10.0 * h2 - 40.0) + 70.0) + 0.0, 10.0 * h1 - 110.0,
	    50.0 + (u + 0.0), ((10.0 * h2 - 100.0) + 70.0) + 0.0, 0.0, 0.0, 1.0, 0.0);
	if (-125.0 < t && t < -5.0) {
		a = (t - -25.0) * 0.05 + h2;
		r = (0.0 * h2 + 0.5) * 15.0;
		Cam9(eye, u + sin(a) * r, -168.0, cos(a) * r, u, -170.0, 0.0, 0.0, 1.0, 1.0);
	}
	if (-35.0 < t && t < -25.0) {
		a = (t + 35.0) * -0.05 + 3.2;
		r = (0.0 * h2 + 3.5) * 15.0;
		Cam9(eye, u + sin(a) * r, -168.0, cos(a) * r, u, -170.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (-25.0 <= t && t < -20.0) {
		dd = t + 25.0;
		a = 0.05 * dd + 2.0;
		r = ((2.5 - 0.199 * dd) + 0.0 * h2) * 15.0;
		Cam9(eye, u + sin(a) * r, -168.0, cos(a) * r, u, -170.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (-20.0 <= t && t < -17.0) {
		a = (t + 20.0) * -0.08 - 0.4;
		r = ((-0.5 - (t + 25.0) * 0.199 * 0.2) + 0.0 * h2) * 15.0;
		Cam9(eye, u + sin(a) * r, -168.0, cos(a) * r, u, -170.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (-17.0 <= t && t < -14.0) {
		a = (t + 20.0) * 0.08 - 0.6;
		r = (0.6 + 0.0 * h2) * 15.0;
		Cam9(eye, u + sin(a) * r, -168.0, cos(a) * r, u, -170.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (-14.0 <= t && t < -5.0) {
		a = (t + 25.0) * 0.08 + 0.8;
		x = Clamp01((t - -15.8) * 0.1);
		e = Math_Ease(&x, &two);
		w = t > -9.0 ? (t + 9.0) * 23.0 : 0.0;
		r = (((1.0 - e) * 1.4 + 0.8) + 0.0 * h2) * 15.0;
		Cam9(eye, (sin(a) * r + 0.2 * w) + u, 0.2 * w - 168.0, cos(a) * r,
		    w + u, 0.2 * w - 170.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (-5.0 < t && t < 0.5) {
		d = t + 5.0;
		x = Clamp01(t * 0.4);
		e1 = Math_Ease(&x, &one);
		Cam9(eye, u - 60.0, 20.0, 0.0, (u - 50.0) + 0.0, 10.0, 0.0, 1.0, 0.0, 0.0);
		q = 60.0 * e1 + u;
		Cam9(eye, (q - 70.0) - 10.0 * d, (((24.0 - 30.0 * e1) + 10.0 * d) + 20.0) - 86.0,
		    10.0 - 5.0 * d, 90.0 + q, -66.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (0.5 <= t && t < 3.5) {
		d = t - 0.0;
		x = Clamp01(0.4 * t);
		e = Math_Ease(&x, &one);
		Cam9(eye, (u + 10.0) - 20.0 * d, 0.0 * h2 + 20.0, -60.0,
		    (((u - 30.0) - 20.0 * d) + 0.0) + 20.0 * e, 0.0 * h2 - 40.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (3.5 <= t && t < 6.0) {
		d = t - 3.0;
		x = Clamp01(0.4 * t);
		Math_Ease(&x, &one);
		Cam9(eye, ((u + 40.0) - 20.0 * d) - 20.0, 9.8 * d + 40.0, 9.8 * d - 20.0,
		    ((u - 0.0) - 20.0 * d) - 20.0, -20.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (6.0 <= t && t < 9.0) {
		d = t - 6.0;
		d3 = t - 3.0;
		x = Clamp01(0.4 * t);
		Math_Ease(&x, &one);
		Cam9(eye, (u + 70.0) - 25.0 * d3, 20.0 - 2.8 * d, -80.0,
		    ((u + 60.0) - 25.0 * d3) + 0.0, -40.0 - 2.8 * d, 0.0, 0.0, 1.0, 0.0);
	}
	if (9.0 <= t && t < 14.0) {
		d = t - 9.0;
		d3 = t - 3.0;
		x = Clamp01(0.4 * t);
		Math_Ease(&x, &one);
		Cam9(eye, ((30.0 * d + u) + 150.0) - 25.0 * d3, ((d + d) + 54.6) + 20.0, 0.0,
		    (((30.0 * d * 0.3 + u) + 220.0) - 25.0 * d3) + 0.0, -40.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (14.0 <= t && t < 24.5) {
		x = Clamp01((t - 9.0) * 0.4);
		Math_Ease(&x, &one);
		x = Clamp01((t - 13.0) * 0.25);
		m = Math_Ease(&x, &one) * 3.2 + 1.0;
		x = Clamp01((t - 16.0) * 0.2);
		s = Math_Ease(&x, &one) * 0.6;
		x = Clamp01((t - 18.0) * 0.15);
		e4 = Math_Ease(&x, &two);
		k = 1.0 - 0.75 * e4;
		Cam9(eye, 10.0 * e4 + u, (10.0 * m - (70.0 * e4 + 10.0)) + 0.0,
		    (((s * 50.0 - 130.0) + 20.0) + 0.0 * m) * k,
		    50.0 * k + ((10.0 * e4 + u) + 0.0), ((10.0 * m - (60.0 + 70.0 * e4)) + 0.0) + 17.6 * e4,
		    30.0 * e4, 0.0, 1.0, 0.0);
	}
	if (24.5 <= t && t < 35.0) {
		p = 13.0 - (t - 25.0) * 1.2;
		Cam9(eye, (u + 0.0) - 18.0, (0.0 * h2 - 10.0) + 0.0, 10.0 * p - 110.0,
		    (50.0 + (u + 0.0)) - 18.0, (0.0 * h2 - 60.0) + 0.0, 0.0, 0.0, 1.0, 0.0);
	}

	Vec4_Sub(&eye->target, &eye->eye, &F);
	Fc = F;
	Vec4_Normalize(&Fc);
	Vec4_Normalize(&F);
	Vec4_Cross(&F, &eye->up, &R);
	idx = (unsigned)(long)((t + 300000.0) * 35.0) % (unsigned)(path.points.size() - 1);
	A = 0.011;
	K = 0.5;
	M = 1.0;
	if (t < -24.0) {
		K = 3.0;
		M = 0.1;
	}
	if (-24.0 <= t && t < -17.0) {
		A = 0.003;
		M = 0.01;
	}
	if (t > 6.0) {
		A = 0.005;
		M = 0.07;
	}
	S = 0.0;
	if (t > -20.0) {
		S = 3.99;
	}
	if (-17.0 < t && t < -14.0) {
		S = -10.4;
	}
	px = path.points[idx].x;
	py = path.points[idx].y;
	rightV = px * K * A * 0.01 * S;
	upV = py * K * A * 0.01 * S;
	A8 = 0.8 * A;
	Bf = (float)A8;
	Vec4_Scale(&R, &px, &s1);
	Vec4_Scale(&s1, &Bf, &s2);
	Vec4_Add(&eye->target, &s2, &eye->target);
	Vec4_Scale(&eye->up, &py, &s1);
	Vec4_Scale(&s1, &Bf, &s2);
	Vec4_Add(&eye->target, &s2, &eye->target);
	eye->up.x = (float)(((sin(path.a[idx] * 1.7188733853924696 * A8) * K) * 0.3) * M + eye->up.x);
	ctx->light = *eye;
	Cam9(&ctx->light, 0.0, 0.0, 30.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);   /* 0x004f47dc */

	DrawScene(ctx, 1);
	Fbo_Resolve(g, 0, W, H, 2);
	Fbo_Unbind();
	glDisable(GL_DEPTH_TEST);
	Post_RunPass(g, passBlur, 2, 1, W / 2, H / 2, texNormal, texGlow, -1, -1);
	Post_RunPass(g, passWire, 0, 1, W, H, texNormal, texNormal, texGlow, texColour);
	Post_RunPass(g, passBlur, 2, 6, W / 2, H / 2, texNormal, texGlow, -1, -1);
	x = Clamp01((t - -30.0) * 0.4);
	alpha = (1.0 - Math_Ease(&x, &one)) * 0.3 + 0.2;
	Shader_SetParam1f(&g->shader, passBend, "alpha", &alpha);
	Shader_SetParam1f(&g->shader, passBend, "RightV", &rightV);
	Shader_SetParam1f(&g->shader, passBend, "UpV", &upV);
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
	gl1::Color3f(0.3f, 0.3f, 0.3f);                    /* 0x004f47e4 */
	Tex_Bind(g, texNormal);
	c = 1.0 * Clamp01((t + 35.0) * 0.5);
	gl1::Color3f((float)c, (float)c, (float)c);
	Gfx_DrawQuad(&zero, &one, &one, 1);
	gl1::PopMatrix();
	gl1::PopMatrix();
	Attrib_Pop();
	seq->partEnd = t + g_partTimeCarry;
	glDisable(GL_BLEND);
}

/*
 * ORIGIN 0x00493230 part_motor::Init (effects/motor.txt 1, 3.1, 4). The
 * targets are made only with seq->relayMode 0, as it is while
 * Demo_Init runs; their noise fill moves the CRT stream. pattern5, the
 * pairs 0x3ea, 0x40c, 0x40a and the passes 0x405, 0x3fd, 0x411 are
 * loaded and never bound, and no other part reads part_motor's slot
 * band (effects/motor.txt 6). bouno1.png is cvLoadImage'd as 8-bit
 * grey; here the grey is OpenCV's fixed-point weighting [inferred].
 * box.3ds is never drawn; only its material is loaded, where
 * maybe_Mesh_Draw looks [inferred].
 */
void part_motor::Init(Rupture_Ctx *ctx)
{
	Gfx_Ctx    *g = &ctx->gfx;
	Shader_Ctx *s = &g->shader;
	int         w = ctx->width, h = ctx->height, i, x, y;
	Vec4        pt, step;
	double      e, xs, two = 2.0;
	Image       img;

	t = 0.0;
	tLocal = 0.0;
	frames = 0;
	texNormal = 0;
	texColour = 0xb;
	texQuarter = 2;
	texGlow = 3;
	texBase = 1000;
	model = 1000;
	pairBlocks = 0x408;
	pairBuilding = 0x40e;
	pairHex = 0x414;
	pairRoad = 0x416;
	pairMotor = 0x418;
	pairKthrio1 = 0x41a;
	passBlur = 0x3ff;
	passBend = 0x407;
	passWire = 0x403;
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
	g->tex[texBase + 12] = Tex_Load("textures/bg08.jpg");
	g->tex[texBase + 30] = Tex_Load("textures/cloudt2.jpg");
	bounoW = bounoH = 0;
	if (Image_Load("textures/bouno1.png", img)) {
		bounoW = img.width;
		bounoH = img.height;
		bouno.resize((size_t)bounoW * bounoH);
		for (y = 0; y < bounoH; y++) {
			for (x = 0; x < bounoW; x++) {
				const u8 *p = &img.rgba[((size_t)(bounoH - 1 - y) * bounoW + x) * 4];

				bouno[(size_t)y * bounoW + x] = (u8)((p[0] * 4899 + p[1] * 9617 + p[2] * 1868 + 8192) >> 14);
			}
		}
	}
	Obj_Load(&g_motorcycleObj, gl1::GenLists(1), "meshes/motorcycle.obj");
	Obj_Load(&room, gl1::GenLists(1), "meshes/room.obj");
	Obj_Load(&hex2, gl1::GenLists(1), "meshes/hex2.obj");
	Obj_Load(&kthrio1, gl1::GenLists(1), "meshes/kthrio1.obj");

	Track_Clear(&road);
	pt.kind = 1;
	pt.x = -100.0f;                                    /* 0x004f5ff8 */
	pt.y = -50.0f;                                     /* 0x004f5db4 */
	pt.z = 0.0f;
	step.kind = 1;
	step.x = 1.0f;
	step.y = 0.0f;
	step.z = 0.0f;
	for (i = 0; i < 750; i++) {
		xs = Clamp01((double)i / 250.0);
		e = Math_Ease(&xs, &two);
		pt.y = (float)(e * 150.0 - 150.0);
		Vec4_Add(&pt, &step, &pt);
		road.points.push_back(pt);
		road.count++;
	}
	for (i = 0; i < 600; i++) {
		xs = Clamp01((double)i / 250.0);
		e = Math_Ease(&xs, &two);
		pt.y = (float)((1.0 - e) * 50.0 - 50.0);
		Vec4_Add(&pt, &step, &pt);
		Math_Random();
		road.points.push_back(pt);
		road.count++;
	}
	g->tex[(model + 1000) * 100] = Tex_Load("meshes/marmaro.jpg");
	Shader_LoadPair(s, "cgeffects/vertex_projection_anisotropic.cpp",
	                "cgeffects/fragment_projection_anisotropic.cpp", 0x3ea);
	Shader_LoadPair(s, "cgeffects/vertex_boat_test2cut.cpp", "cgeffects/fragment_boat_test2.cpp",
	                pairBlocks);
	Shader_LoadPair(s, "cgeffects/v_roadcut.cpp", "cgeffects/f_roadcut.cpp", pairRoad);
	Shader_LoadPair(s, "cgeffects/v_motor.cpp", "cgeffects/f_motor.cpp", pairMotor);
	Shader_LoadPair(s, "cgeffects/vertex_boat_test2building.cpp",
	                "cgeffects/fragment_boat_test3.cpp", pairBuilding);
	Shader_LoadPair(s, "cgeffects/vertex_boat_test2building.cpp",
	                "cgeffects/fragment_boat_testhex.cpp", pairHex);
	Shader_LoadPair(s, "cgeffects/vertex_boat_test2building.cpp", "cgeffects/f_kthrio1.cpp",
	                pairKthrio1);
	Shader_LoadPair(s, "cgeffects/vertex_boat_test_motion.cpp",
	                "cgeffects/fragment_boat_test_motion.cpp", 0x40c);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_train.cpp", 0x40a);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256.cpp", passBlur);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256FOV.cpp", 0x405);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_Blur_circular.cpp", 0x3fd);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_bend3.cpp", passBend);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_motion.cpp", 0x411);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_wireframe.cpp", passWire);
	Path_Load(&path, "meshes/try1.sav");
	t = -35.0;                                         /* 0x004f5d78 */
	tLocal = -35.0;
}

/*
 * ORIGIN 0x0048ec00 maybe_Track_DrawRoad: the road as three quad strips
 * along the track - the top 8 wide, the two sides 1.5 deep - and the
 * centre dashes as quads. It clears the track's loop byte, which later
 * Track_Eval calls see.
 */
static void track_draw_road(Track *T)
{
	Vec4 P, D, R, U, a, b, n, P2, D2, R2, U2;
	int  k;

	T->loop = 0;
	glDisable(GL_CULL_FACE);
	gl1::Begin(gl1::QUAD_STRIP);
	for (k = 1; k < T->count; k++) {
		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		gl1::Normal3f(U.x, U.y, U.z);
		a = Vec4_AddScaled(&P, &R, -4.0f);
		b = Vec4_AddScaled(&P, &R, 4.0f);
		gl1::Vertex3f(a.x, a.y, a.z);
		gl1::Vertex3f(b.x, b.y, b.z);
	}
	gl1::End();
	gl1::Begin(gl1::QUAD_STRIP);
	for (k = 1; k < T->count; k++) {
		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		gl1::Normal3f(R.x, R.y, R.z);
		a = Vec4_AddScaled(&P, &R, -4.0f);
		b = Vec4_AddScaled(&a, &U, -1.5f);
		gl1::Vertex3f(a.x, a.y, a.z);
		gl1::Vertex3f(b.x, b.y, b.z);
	}
	gl1::End();
	gl1::Begin(gl1::QUAD_STRIP);
	for (k = 1; k < T->count; k++) {
		const float neg = -1.0f;

		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		a = Vec4_AddScaled(&P, &R, 4.0f);
		b = Vec4_AddScaled(&a, &U, -1.5f);
		Vec4_Scale(&R, &neg, &n);
		gl1::Normal3f(n.x, n.y, n.z);
		gl1::Vertex3f(a.x, a.y, a.z);
		gl1::Vertex3f(b.x, b.y, b.z);
	}
	gl1::End();
	gl1::Begin(gl1::QUADS);
	for (k = 1; k < T->count - 3; k++) {
		if ((k / 2) % 5 != 1) {
			continue;
		}
		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		Scene_RoadFrame(T, k + 1, &P2, &D2, &R2, &U2);
		a = Vec4_AddScaled(&P, &R, -0.1f);
		b = Vec4_AddScaled(&P, &R, 0.1f);
		gl1::Normal3f(0.0f, -1.0f, 0.0f);
		gl1::Vertex3f(a.x, (float)(a.y + 0.1), a.z);
		gl1::Vertex3f(b.x, (float)(b.y + 0.1), b.z);
		a = Vec4_AddScaled(&P2, &R2, -0.1f);
		b = Vec4_AddScaled(&P2, &R2, 0.1f);
		gl1::Normal3f(0.0f, -1.0f, 0.0f);
		gl1::Vertex3f(b.x, (float)(b.y + 0.1), b.z);
		gl1::Vertex3f(a.x, (float)(a.y + 0.1), a.z);
	}
	gl1::End();
}

/*
 * ORIGIN 0x0048dfc0 maybe_Image_DrawHeightfield: one quad per pixel
 * cell of a grey image, height 0.03 per grey level, each corner pulled
 * 3% toward the cell's centre and the whole scaled by 7.5 - a field of
 * separate tiles. The normal it builds from the corners is overwritten
 * with (0, 1, 0) before use and is not computed here. Two Math_Random
 * draws per cell set a colour that the next glColor3f replaces.
 */
static void image_draw_heightfield(const std::vector<u8> &pix, int w, int h)
{
	const float quarter = 0.25f, pull = 0.03f, grow = 7.5f;
	Vec4        V[4], C, S, d, m;
	int         i, j, n;
	float       c1, c2;

	gl1::Begin(gl1::QUADS);
	for (j = 1; j < h - 1; j++) {
		for (i = 1; i < w - 1; i++) {
			const double k = 0.03;
			float        X = (float)(i - w / 2), Z = (float)(j - h / 2);

			V[0] = Vec4_Make(X, (float)(pix[(size_t)j * w + i] * k), Z);
			V[1] = Vec4_Make(X, (float)(pix[(size_t)(j + 1) * w + i] * k), Z + 1.0f);
			V[2] = Vec4_Make(X + 1.0f, (float)(pix[(size_t)(j + 1) * w + i + 1] * k), Z + 1.0f);
			V[3] = Vec4_Make(X + 1.0f, (float)(pix[(size_t)j * w + i + 1] * k), Z);
			Vec4_Add(&V[0], &V[1], &S);
			Vec4_Add(&S, &V[2], &S);
			Vec4_Add(&S, &V[3], &S);
			Vec4_Scale(&S, &quarter, &C);
			for (n = 0; n < 4; n++) {
				Vec4_Sub(&C, &V[n], &d);
				Vec4_Scale(&d, &pull, &m);
				Vec4_Add(&V[n], &m, &V[n]);
			}
			gl1::Normal3f(0.0f, 1.0f, 0.0f);
			for (n = 0; n < 4; n++) {
				Vec4_Scale(&V[n], &grow, &V[n]);
			}
			c1 = (float)(Math_Random() * 0.0 + 0.5);
			c2 = (float)(Math_Random() * 0.0 + 0.5);
			gl1::Color3f(0.5f, c2, c1);
			gl1::Color3f(0.21f, 0.27f, 0.3f);
			for (n = 0; n < 4; n++) {
				gl1::Vertex3f(V[n].x, V[n].y, V[n].z);
			}
		}
	}
	gl1::End();
}

/* OURS. Scene_Lit with part_motor's model, decal and light slots, and time 0. */
static void lit(Rupture_Ctx *ctx, part_motor *m, int pair)
{
	Scene_Lit(ctx, m->model, m->texBase, m->texNormal, pair, 0.0);
}

/*
 * ORIGIN 0x00490100..0x004906e0, DrawScene's sky: 2500 grey stars on the
 * upper half of a sphere of radius 400 that follows the eye in x, then
 * before t = 13 a blob41b sun and two cloudt2 glows beside it.
 */
static void motor_sky(Rupture_Ctx *ctx, part_motor *m, double t)
{
	Gfx_Ctx    *g = &ctx->gfx;
	const float k400 = 400.0f;                         /* 0x004f4638 */
	double      zero = 0.0, size35 = 35.0, size110 = 110.0, ang;
	float       gr;
	Vec4        v, P;
	int         i, k;

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Gfx_ResetState();
	Shader_DisableProfiles(&g->shader);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(65.0, (double)ctx->width / ctx->height, 1.0, 1000.0);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	glDisable(GL_DEPTH_TEST);
	Crt_Srand(0x193cedbf);
	gl1::Begin(gl1::POINTS);
	for (i = 0; i < 2500; i++) {
		Math_Random();
		Math_Random();
		gr = (float)(0.2 * (Math_Random() + 0.2));
		gl1::Color3f(gr, gr, gr);
		v = Vec4_RandomDir(true);
		Vec4_Normalize(&v);
		Vec4_Scale(&v, &k400, &v);
		gl1::Vertex3f((float)(v.x + ctx->eye.eye.x), (float)(v.y - 160.0), v.z);
	}
	gl1::End();

	if (t < 13.0) {
		Shader_DisableProfiles(&g->shader);
		gl1::PushMatrix();
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::LoadIdentity();
		gl1::MatrixMode(gl1::MODELVIEW);
		gl1::LoadIdentity();
		maybe_Camera_LookAtEye(ctx);
		Gfx_ResetState();
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		gl1::Color3f(0.4f, 0.3f, 0.3f);
		gl1::Enable(GL_TEXTURE_2D);
		Tex_Bind(g, m->texBase + 0x14);
		Crt_Srand(0x28617fa);
		P = Vec4_RandomDir(true);
		Vec4_Normalize(&P);
		Vec4_Scale(&P, &k400, &P);
		P.y = (float)(P.y - 160.0);
		P.x = (float)(P.x + ctx->eye.eye.x);
		glDepthMask(GL_FALSE);
		gl1::Translatef(P.x, P.y, P.z);
		Gfx_DrawQuad(&zero, &size35, &size35, 1);
		glDepthMask(GL_TRUE);
		gl1::Disable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		gl1::PopMatrix();
		for (k = 0; k < 2; k++) {
			gl1::PushMatrix();
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			gl1::Translatef(P.x, P.y, P.z);
			gl1::Translatef((float)(sin(t * 0.8 + k * 4.0) * 11.0 + (double)(30 * k)), 0.0f, 0.0f);
			ang = k * 40.0 - 48.0;
			if (k == 2) {
				ang = -40.0;
			}
			gl1::Color3f(0.03f, 0.05f, 0.08f);
			if (k == 2) {
				gl1::Color3f(0.05f, 0.04f, 0.04f);
			}
			gl1::Enable(GL_TEXTURE_2D);
			Tex_Bind(g, m->texBase + 0x1e);
			Gfx_DrawQuad(&ang, &size110, &size110, 1);
			glDisable(GL_BLEND);
			gl1::PopMatrix();
		}
	}
}

/*
 * ORIGIN 0x004908eb..0x00490d7a, the first row of buildings (pair
 * this+0x11ec): room.obj copies 24 apart down x, each seeded by its
 * index, tipping as the road reaches them (Math_Ease of the distance).
 */
static void buildings_near(Shader_Ctx *s, part_motor *m, double t, double u, int n)
{
	int    off = (t >= 9.0 && t < 14.0) ? 5 : 4, i, j;
	double d, c1, c2, E2, yv, Ry, alpha2, b, gr, r, two = 2.0;
	float  ax;

	for (i = n - off; i < n + 4; i++) {
		Crt_Srand(i * 8000 + 0x409bfa);
		for (j = 0; j < 100; j++) {
			Math_Random();
		}
		gl1::PushMatrix();
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::LoadIdentity();
		d = u - (double)(24 * i);
		c1 = Clamp01((d + 10.0) * 0.015);
		alpha2 = Math_Random();
		Shader_SetParam1f(s, m->pairBuilding + 1, "alpha2", &alpha2);
		c2 = Clamp01((d + 50.0) * 0.015);
		Math_Ease(&c1, &two);
		E2 = Math_Ease(&c2, &two);
		yv = (double)(3 * (i % 2)) - 20.4;
		Ry = Math_Random();
		gl1::Translatef((float)(24 * i + 12), (float)((yv - Ry * 10.0) - 10.0), -30.0f);
		gl1::Rotatef((float)((Math_Random() * 20.0 - 10.0) * E2), 0.7f, 1.0f, -0.3f);
		ax = (float)(Math_Random() * 2.0 - 1.0);
		gl1::Rotatef((float)((Math_Random() * 20.0 - 10.0) * E2), ax, 0.3f, 0.2f);
		gl1::Rotatef((float)((Math_Random() * 20.0 - 10.0) * E2 + 90.0), 0.0f, 1.0f, 0.0f);
		b = Math_Random() * 0.2 + 0.8;
		gr = Math_Random() * 0.2 + 0.8;
		r = Math_Random() * 0.2 + 0.8;
		gl1::Color4f((float)r, (float)gr, (float)b, 1.0f);
		b = Math_Random() * 0.3 + 0.9;
		gr = Math_Random() * 0.1 + 0.9;
		r = Math_Random() * 0.0 + 0.8;
		gl1::Color4f((float)r, (float)gr, (float)b, 1.0f);
		Shader_SetTextureMatrix(s, m->pairBuilding, 0xe);
		gl1::MatrixMode(gl1::MODELVIEW);
		Obj_Draw(&m->room);
		gl1::PopMatrix();
	}
}

/*
 * ORIGIN 0x00490d80..0x00491224, the road blocks (pair this+0x1180,
 * effects/motor.txt 2): ten room.obj copies 24 apart, each turning in
 * and dissolving in by Math_Ease of its distance ahead.
 */
static void road_blocks(Shader_Ctx *s, part_motor *m, double t, double u, int n)
{
	double alphaplane, alphaplane2, d, c1, lift, c3, ttime, turn, alpha2, two = 2.0;
	float  redx;
	int    i, j;

	if (!(t > -5.0)) {
		return;
	}
	alphaplane = ((u - 150.0) - 4.0) * 0.0;
	alphaplane2 = (u + 10.0) - 4.0;
	redx = (float)((u + 10.0) - 10.0);
	for (i = n - 5; i < n + 5; i++) {
		Crt_Srand(0x409bfa + i);
		for (j = 0; j < 100; j++) {
			Math_Random();
		}
		gl1::PushMatrix();
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::LoadIdentity();
		gl1::Translatef((float)(24 * i), (float)((double)(3 * (i % 2)) - 20.4), 0.0f);
		d = u - (double)(24 * i);
		c1 = Clamp01(((10.0 + d) + 20.0) * 0.015);
		lift = Clamp01(((d - 30.0) + 0.0) * 0.005);
		c3 = Clamp01((d + 100.0) * 0.015);
		ttime = Math_Ease(&c1, &two);
		turn = Math_Ease(&c3, &two);
		gl1::Rotatef((float)((Math_Random() * 40.0 - 20.0) * turn + 90.0), 0.0f, 1.0f, 0.0f);
		gl1::Color4f(0.2f, 0.22f, 0.2f, 1.0f);
		alpha2 = Math_Random() * ttime;
		Shader_SetParam1f(s, m->pairBlocks + 1, "alpha2", &alpha2);
		Shader_SetParam1f(s, m->pairBlocks + 1, "ttime", &ttime);
		Shader_SetParam1f(s, m->pairBlocks + 1, "alphaplane", &alphaplane);
		Shader_SetParam1f(s, m->pairBlocks + 1, "alphaplane2", &alphaplane2);
		gl1::Translatef(0.0f, (float)(lift * 0.0), 0.0f);
		Shader_GetParam(s, m->pairBlocks, 4, "Reduction");
		Shader_SetParam3f(s, m->pairBlocks, 4, redx, 0.0f, 0.0f);
		Shader_Bind(s, m->pairBlocks);
		Shader_Bind(s, m->pairBlocks + 1);
		Shader_SetTextureMatrix(s, m->pairBlocks, 0xe);
		gl1::MatrixMode(gl1::MODELVIEW);
		Obj_Draw(&m->room);
		gl1::PopMatrix();
	}
}

/*
 * ORIGIN 0x0049122a..0x004915ba, the far row of buildings (pair
 * this+0x11ec), every fifth index left out. n is overwritten with
 * n + 3 as the code stores the loop's end over it.
 */
static void buildings_far(Shader_Ctx *s, part_motor *m, double t, double u, int *n)
{
	int    off2 = (t >= 9.0 && t < 14.0) ? 4 : 3, i, j, start, end;
	double alpha2, c, E, two = 2.0;
	float  ax;

	if (!(t > -5.0)) {
		return;
	}
	start = *n - off2;
	end = *n + 3;
	*n = end;
	for (i = start; i < end; i++) {
		if (i % 5 == 0) {
			continue;
		}
		Crt_Srand(i * 8000 + 0x409bfa);
		for (j = 0; j < 100; j++) {
			Math_Random();
		}
		gl1::PushMatrix();
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::LoadIdentity();
		alpha2 = Math_Random();
		Shader_SetParam1f(s, m->pairBuilding + 1, "alpha2", &alpha2);
		c = Clamp01(((u - (double)(24 * i)) + 50.0) * 0.015);
		E = Math_Ease(&c, &two);
		gl1::Translatef((float)(24 * i + 12), (float)((double)(3 * (i % 2)) - 20.4), 30.0f);
		gl1::Rotatef((float)((Math_Random() * 20.0 - 10.0) * E), 0.7f, 1.0f, -0.3f);
		ax = (float)(Math_Random() * 2.0 - 1.0);
		gl1::Rotatef((float)((Math_Random() * 20.0 - 10.0) * E), ax, 0.3f, 0.2f);
		gl1::Rotatef((float)((Math_Random() * 20.0 - 10.0) * E + 90.0), 0.0f, 1.0f, 0.0f);
		gl1::Color4f((float)(Math_Random() * 0.2 + 0.8), 1.0f, 1.0f, 1.0f);
		Shader_SetTextureMatrix(s, m->pairBuilding, 0xe);
		gl1::MatrixMode(gl1::MODELVIEW);
		Obj_Draw(&m->room);
		gl1::PopMatrix();
	}
}

/*
 * ORIGIN 0x00491b00..0x00492226, the motorcycle (effects/motor.txt 3.2):
 * placed on the road track at s = max(u + 250, 5), pitched by the
 * track's slope, with a lift and tilt as it sets off and a sway after
 * t = 0. u is lowered by 3.2 around the block while t < 14 and raised
 * again after, two operations as the code has them.
 */
static void motorcycle(Rupture_Ctx *ctx, part_motor *m, double t, double *u)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const double pi = 3.141592653589793;
	double       v, sv, yaw, a, b, k, x, one = 1.0;
	float        acosArg;
	Vec4         P = Vec4_Make(0.0f, 0.0f, 0.0f), Q = P, D, R, U;
	const Vec4   up = Vec4_Make(0.0f, 1.0f, 0.0f);

	lit(ctx, m, m->pairMotor);
	glEnable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	if (t < 14.0) {
		*u = *u - 3.2;
	}
	v = (*u - 150.0) - 4.0;
	Shader_SetParam1f(s, m->pairMotor + 1, "alphaplane2", &v);
	sv = (*u + 245.0) + 5.0;
	if (sv < 5.0) {
		sv = 5.0;
	}
	Track_Eval(&m->road, (float)sv, &P);
	Track_Eval(&m->road, (float)(sv + 3.0), &Q);
	Vec4_Sub(&Q, &P, &D);
	Vec4_Normalize(&D);
	Vec4_Cross(&up, &D, &R);
	Vec4_Normalize(&R);
	Vec4_Cross(&R, &D, &U);
	Vec4_Normalize(&U);
	gl1::Translatef(P.x, P.y, P.z);
	gl1::Translatef(-180.0f, -20.0f, 0.0f);
	acosArg = D.kind == 0 ? 0.0f : D.z * 0.0f + D.x + D.y * 0.0f;
	yaw = (double)(float)acos(acosArg) * -90.0;
	x = Clamp01((t - -9.4) * 0.3);
	a = Math_Ease(&x, &one);
	x = Clamp01((t - -9.8) * 0.3);
	b = Math_Ease(&x, &one);
	if (t < -2.0) {
		gl1::Translatef((float)(sin(b * pi) * 2.8 * 2.0), 0.0f, 0.0f);
	}
	gl1::Translatef(0.0f, (float)(yaw * -4.2 * 0.01), 0.0f);
	if (t < -2.0) {
		k = Clamp01(sin(a * pi) * 1.4);
		gl1::Translatef(0.0f, (float)(k * 1.664), 0.0f);
	}
	if (0.0 < t) {
		gl1::Rotatef((float)(sin(t + t) * 20.0), 1.0f, 0.0f, 0.0f);
	}
	gl1::Rotatef(90.0f, 0.0f, 1.0f, 0.0f);
	gl1::Rotatef((float)yaw, 1.0f, 0.0f, 0.0f);
	if (t < -2.0) {
		k = Clamp01(sin(a * pi) * 1.4);
		gl1::Rotatef((float)(k * -50.0), 1.0f, 0.0f, 0.0f);
	}
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	gl1::Color3f(0.3f, 0.3f, 0.3f);
	Shader_SetTextureMatrix(s, m->pairMotor, 0xe);
	gl1::MatrixMode(gl1::MODELVIEW);
	Obj_Draw(&g_motorcycleObj);
	gl1::PopMatrix();
	if (t < 14.0) {
		*u = *u + 3.2;
	}
}

/*
 * ORIGIN 0x00492542..0x0049314b, the sparks: up to 600 untextured,
 * unblended billboards thrown back from the road 2 behind the bike,
 * each fading and shrinking over its own phase (effects/motor.txt 3.3).
 */
static void sparks(Rupture_Ctx *ctx, part_motor *m, double t, double u)
{
	const double pi = 3.141592653589793;
	const Vec4   down = Vec4_Make(0.0f, -1.0f, 0.0f), up = Vec4_Make(0.0f, 1.0f, 0.0f);
	double       w245, c, N, a, p, r, sx, sy, ang;
	float        f, fr, wv, v, b, b2, q, s30, f124, fq, fr2, k3, k4, k02 = 0.2f, nm30;
	Vec4         P2 = Vec4_Make(0.0f, 0.0f, 0.0f), Q2 = P2, D, R, U2;
	Vec4         A, B, C, E, F, G, H, I, J, K, L, M, Nn, O;
	int          i;

	(void)pi;
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	w245 = u + 245.0;
	Track_Eval(&m->road, (float)((w245 + 5.0) - 2.0), &P2);
	Track_Eval(&m->road, (float)(((w245 + 3.0) + 5.0) - 2.0), &Q2);
	Vec4_Sub(&Q2, &P2, &D);
	Vec4_Normalize(&D);
	Vec4_Cross(&up, &D, &R);
	Vec4_Normalize(&R);
	Vec4_Cross(&R, &D, &U2);
	Vec4_Normalize(&U2);
	gl1::Translatef((float)(P2.x - 180.0), (float)(P2.y - 18.0), P2.z);
	gl1::Color3f(1.0f, 0.0f, 0.0f);
	c = Clamp01((t - -8.96) * 2.6);
	N = c * 600.0;
	gl1::BatchBegin();   /* OURS: one draw for the loop, docs/subsystems/render.txt */
	for (i = 0; (double)i < N; i++) {
		gl1::PushMatrix();
		a = (Math_Random() + 1.0) * 0.6 * t;
		f = (float)(Math_Random() * 500.0 + a);
		fr = (float)(f - (float)(int)f);
		wv = (float)(fr * c);
		v = wv > 1.0f ? 1.0f : wv < 0.0f ? 0.0f : wv;
		b = (float)(3.0 * (1.0 - v));
		b2 = b * b;
		p = (double)(b2 * b2);
		q = v * v;
		r = ((1.0 - v) * 1.5) * ((1.0 - v) * 1.5);
		s30 = (float)(v * 30.0);
		gl1::Color3f((float)(p * 0.2), (float)(p * 0.05), (float)(p * 0.01));
		f124 = 12.4f;
		fq = q;
		fr2 = (float)r;
		k3 = (float)(Math_Random() * 2.0 - 1.0);
		k4 = (float)Math_Random();
		nm30 = -s30;
		Vec4_Scale(&down, &f124, &A);
		Vec4_Scale(&A, &fq, &B);
		Vec4_Scale(&down, &fr2, &C);
		Vec4_Scale(&R, &s30, &E);
		Vec4_Scale(&E, &k02, &F);
		Vec4_Scale(&F, &k3, &G);
		Vec4_Scale(&U2, &s30, &H);
		Vec4_Scale(&H, &k02, &I);
		Vec4_Scale(&I, &k4, &J);
		Vec4_Scale(&D, &nm30, &K);
		Vec4_Sub(&K, &J, &L);
		Vec4_Add(&L, &G, &M);
		Vec4_Add(&M, &C, &Nn);
		Vec4_Add(&Nn, &B, &O);
		gl1::Translatef(O.x, O.y, O.z);
		sx = r * 0.04;
		sy = r * 0.04;
		ang = Math_Random() * 360.0 + p * 60.0;
		Gfx_DrawQuad(&ang, &sx, &sy, 1);
		gl1::PopMatrix();
	}
	gl1::BatchEnd();
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x00490100 part_motor::DrawScene (effects/motor.txt 1-3): the
 * sky into the cleared target; the pass set-up and the row count n;
 * the near buildings, the road blocks, the far buildings, the road,
 * the bouno1 tile field, the motorcycle, kthrio1 before t = -5, the hex
 * pair's set-up and the sparks. Its modelview push at 0x004906f1 is
 * popped by the last call of the function. u = 27 t (0x004f4ef8) is
 * DrawScene's own, not Frame's 25 t.
 */
void part_motor::DrawScene(Rupture_Ctx *ctx, int pass)
{
	Gfx_Ctx    *g = &ctx->gfx;
	Shader_Ctx *s = &g->shader;
	double      uu = t * 27.0, Ed, a2, ap2, v, x, one = 1.0, doit;
	int         n;
	Vec4        a, b;
	static const GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1 };

	(void)pass;
	motor_sky(ctx, this, t);
	glEnable(GL_DEPTH_TEST);
	gl1::PushMatrix();
	glDrawBuffers(2, bufs);
	glViewport(0, 0, ctx->width, ctx->height);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(ctx->fovOffset + 65.0, (double)ctx->width / ctx->height, ctx->nearZ, ctx->farZ);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	Shader_DisableProfiles(s);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	a = Vec4_Make((float)(seqHead[0] * 3.0), (float)(3.0 * seqHead[1]), 0.0f);
	b = Vec4_Make((float)(seqHead[2] * 2.0), 0.0f, 0.0f);
	Vec4_Add(&a, &b, &vec1d4);
	Ed = (double)ctx->eye.eye.x;
	if (t >= 9.0) {
		Ed = ctx->eye.eye.x + 91.0;                    /* 0x004f5e10 */
	}
	Crt_Srand(0x40987e);
	glDisable(GL_CULL_FACE);
	n = (int)(Ed / 24.0 + 0.0);
	if (n < 5) {
		n = 5;
	} else if (n > 15) {
		n = 15;
	}

	lit(ctx, this, pairBuilding);
	glEnable(GL_CULL_FACE);
	buildings_near(s, this, t, uu, n);
	lit(ctx, this, pairBlocks);
	glDisable(GL_CULL_FACE);
	road_blocks(s, this, t, uu, n);
	lit(ctx, this, pairBuilding);
	glEnable(GL_CULL_FACE);
	buildings_far(s, this, t, uu, &n);

	lit(ctx, this, pairBuilding);
	glEnable(GL_CULL_FACE);
	Crt_Srand(0x108a);
	lit(ctx, this, pairRoad);
	glEnable(GL_CULL_FACE);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	a2 = Math_Random() * 0.0;
	Shader_SetParam1f(s, pairRoad + 1, "alpha2", &a2);
	ap2 = (((seqHead[2] * 0.0 - 30.0) + 10.0) + uu) - 4.0;
	Shader_SetParam1f(s, pairRoad + 1, "alphaplane2", &ap2);
	Shader_GetParam(s, pairRoad, 4, "Reduction");
	Shader_SetParam3f(s, pairRoad, 4, (float)((uu - 30.0) - 10.0), 0.0f, 0.0f);
	gl1::Translatef(-180.0f, -20.06f, 0.0f);
	gl1::Color4f(0.6f, 0.8f, 1.0f, 1.0f);
	Shader_SetTextureMatrix(s, pairRoad, 0xe);
	gl1::MatrixMode(gl1::MODELVIEW);
	track_draw_road(&road);
	gl1::PopMatrix();

	if (t > -8.5) {
		lit(ctx, this, pairHex);
		glEnable(GL_CULL_FACE);
		Crt_Srand(0x409bfa);
		gl1::PushMatrix();
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::LoadIdentity();
		gl1::Scalef(1.0f, 1.0f, 1.0f);
		Shader_SetParam1f(s, pairHex + 1, "alphaplane2", &t);
		doit = 0.0;
		Shader_SetParam1f(s, pairHex + 1, "doit", &doit);
		gl1::Translatef(50.0f, -110.0f, -50.0f);
		Shader_SetTextureMatrix(s, pairHex, 0xe);
		gl1::MatrixMode(gl1::MODELVIEW);
		gl1::Color3f(0.6f, 0.6f, 0.6f);
		if (t < 9.0) {
			image_draw_heightfield(bouno, bounoW, bounoH);
		}
		doit = 1.0;
		Shader_SetParam1f(s, pairHex + 1, "doit", &doit);
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::LoadIdentity();
		if (t >= 9.0 && t < 14.0) {
			gl1::Translatef(0.0f, 0.0f, -110.0f);
		}
		gl1::Translatef(450.0f, -110.0f, 160.0f);
		gl1::Rotatef(180.0f, 0.0f, 1.0f, 0.0f);
		gl1::MatrixMode(gl1::MODELVIEW);
		Shader_SetTextureMatrix(s, pairHex, 0xe);
		if (t >= 9.0) {
			image_draw_heightfield(bouno, bounoW, bounoH);
		}
		gl1::PopMatrix();
	}

	motorcycle(ctx, this, t, &uu);

	if (t < -5.0) {
		lit(ctx, this, pairKthrio1);
		glEnable(GL_CULL_FACE);
		Crt_Srand(0x409bfa);
		gl1::PushMatrix();
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::LoadIdentity();
		v = (uu - 150.0) - 4.0;
		Shader_SetParam1f(s, pairKthrio1 + 1, "alphaplane2", &v);
		x = Clamp01((t - -35.0) * 0.4);
		v = 1.0 - Math_Ease(&x, &one);
		Shader_SetParam1f(s, pairKthrio1 + 1, "alphaplane2", &v);
		v = 0.0;
		Shader_SetParam1f(s, pairKthrio1 + 1, "doit", &v);
		glDisable(GL_CULL_FACE);
		gl1::Translatef(-282.5f, -160.59f, 2.1f);
		gl1::Rotatef(90.0f, 0.0f, 1.0f, 0.0f);
		gl1::Scalef(5.0f, 5.0f, 5.0f);
		gl1::Color3f(0.8f, 0.9f, 1.0f);
		Shader_SetTextureMatrix(s, pairKthrio1, 0xe);
		gl1::MatrixMode(gl1::MODELVIEW);
		Obj_Draw(&kthrio1);
		gl1::PopMatrix();
	}

	lit(ctx, this, pairHex);
	glEnable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	Gfx_ResetState();
	Shader_DisableProfiles(s);
	Crt_Srand(0x409bfa);
	sparks(ctx, this, t, uu);

	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(65.0, (double)ctx->width / ctx->height, 1.0, 1000.0);
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	glEnable(GL_DEPTH_TEST);
	gl1::Begin(gl1::POINTS);
	gl1::End();
	gl1::PopMatrix();
}
