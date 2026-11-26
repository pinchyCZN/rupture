#include "part_city.h"

#include <math.h>
#include <string.h>

#include "../core/gl1.h"
#include "scene.h"

/* ORIGIN 0x0043caf0 maybe_Camera_Construct: eye, target and up all {1, 0, 0, 0}. */
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

/* OURS. x < 0 -> 0, the inline test the code makes. */
static double Max0(double x)
{
	return x < 0.0 ? 0.0 : x;
}

/*
 * ORIGIN 0x00446f30 part_city::part_city. [open] The four Obj meshes
 * at +0x3c8..+0x47c that Init never loads and the container at +0x638
 * (maybe_StdVector_Construct) are left out.
 */
part_city::part_city()
{
	Path_Construct(&path);
	d380 = 0.0;
	camera_construct(&cam388);
	vec1358 = Vec4_Make(0.0f, 0.0f, 0.0f);
	vec13ac = Vec4_Make(0.0f, 0.0f, 0.0f);
	vec13e0 = Vec4_Make(0.0f, 0.0f, 0.0f);
}

/*
 * ORIGIN 0x00477450 part_city::Frame: the gate, the clock held at -0
 * at least, one base camera from the seq head and nine windows on t,
 * the shake from try1.sav, the light camera, DrawScene and the post
 * chain. The part hands over at t = 59. The Vec4 sums are the demo's
 * operator+ / - / * (Vec4_OpAdd and kin), float component by
 * component (effects/city.txt 3.2).
 */
void part_city::Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq)
{
	Gfx_Ctx  *g = &ctx->gfx;
	Camera   *eye = &ctx->eye;
	Camera    cam;
	double    h0, h1, h2, r, a, x, f, u, R10, Bd, Bc, k, K16, c, rightV, upV, alpha, L;
	double    one = 1.0, two = 2.0, zero = 0.0;
	float     s, fB, px, py, Lf, k14 = -14.0f;
	Vec4      A, B, C, E, V, V0, Vc, Wv, D, S, F, Fc, R, m1, m2;
	unsigned  idx, n;
	int       W = ctx->width, H = ctx->height;

	if (seq->relayMode == 1 && (seq->started[5] == 0 || seq->done[5] == 1)) {
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
		t = h0 * 5.0;
	}
	if (-0.0 > t) {                                    /* 0x004f5010 */
		t = -0.0;
	}
	frames++;
	seq->partClock = (((((t + 39.0) + 64.0) + 70.0) + 65.0) + 61.0) + 7.0;
	ctx->nearZ = h2 * 10.0 + 1.5;
	if (32.0 <= t && t < 42.0) {
		ctx->nearZ = 29.5;                             /* 0x004f52b8 */
	}
	ctx->farZ = 2300.0;                                /* 0x004f52b0 */
	if (t >= 59.0) {                                   /* 0x004f52a8 */
		g_partTimeCarry = t - 59.0;
		Attrib_Pop();
		seq->done[5] = 1;
		seq->started[6] = 1;
		return;
	}
	ctx->nearZ = ctx->nearZ + 0.4;
	if (15.0 <= t && t < 22.0) {
		ctx->nearZ = ctx->nearZ - 0.3;
	}
	if (t >= 32.0) {
		ctx->nearZ = ctx->nearZ + 6.0;
	}
	gl1::PushMatrix();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Fbo_Bind(g, 0, 2);

	r = h2 * 80.0 + 10.0;
	Cam9(eye, sin(h0) * r, h1 * 128.0, cos(h0) * r, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	if (-10.0 <= t && t < 3.0) {
		a = (t * 0.8 + 3.0) + 1.2;
		Cam9(eye, sin(a) * 5.0, (((1332.0 - t * 94.0) + 10.0) - 4.0) - 2.0, cos(a) * 5.0,
		     0.0, 1340.0 - t * 105.0, -t, 0.0, 1.0, 0.02);
	}
	Wv = Vec4_Make(129.0f, 269.0f, -73.0f);
	if (3.0 <= t && t < 5.0) {
		a = (t * 0.1 + 3.0) + 4.4;
		r = 80.0 - (t - 3.0) * 5.0;
		V = Vec4_Make((float)(sin(a) * r), 16.0f, (float)(cos(a) * r));
		Vec4_Add(&V, &Wv, &V);
		Cam9(eye, V.x, V.y, V.z, 129.0, 269.0, -90.0, 0.0, 1.0, 0.0);
	}
	if (5.0 <= t && t < 15.0) {
		a = (t * 0.1 + 3.0) + 1.98;
		V = Vec4_Make((float)(sin(a) * 10.0), 6.8f, (float)(cos(a) * 10.0));
		Vec4_Add(&V, &Wv, &V);
		Cam9(eye, V.x, V.y, V.z, 129.0, 269.0, -73.0, 0.0, 1.0, 0.0);
		V = Vec4_Make(92.0f, 420.0f, (float)((t - 5.0) * 20.0 - 70.0));
		camera_construct(&cam);
		cam_init9(&cam, V.x, V.y, V.z, 129.0, 69.0, -73.0, 0.1, 1.0, 0.0);
		x = Clamp01((t - 5.0) * 0.08);
		f = Math_Ease(&x, &two);
		Camera_Lerp(eye, &f, eye, &cam);
	}
	if (15.0 <= t && t < 22.0) {
		a = ((t * 0.2 + 3.0) + 1.98) + 2.0;
		B = Vec4_Make((float)(sin(a) * 6.0), 0.0f, (float)(cos(a) * 6.0));
		A = Vec4_Make(-10.0f, 96.8f, (float)((((t - 15.0) * 0.5 - 8.0) * 100.0 + 657.0) - 6.0));
		C = Vec4_Make(0.0f, (float)(((t - 15.0) * 0.8 - 0.6000000000000001) + 2.0), 0.0f);
		Vec4_Add(&A, &B, &m1);
		Vec4_Add(&m1, &C, &E);
		Cam9(eye, E.x, 1.0 + E.y, E.z, A.x, A.y + 5.0, A.z, 0.0, 1.0, -0.4);
	}
	if (22.0 <= t && t < 32.0) {
		a = ((3.0 - t * 0.2) + 1.98) + 0.2;
		B = Vec4_Make((float)(sin(a) * 6.0), 0.0f, (float)(cos(a) * 6.0));
		A = Vec4_Make(-10.0f, 96.8f, (float)((((t - 15.0) * 0.5 - 8.0) * 100.0 - 157.0) - 6.0));
		C = Vec4_Make(0.0f, 3.166f, 0.0f);                 /* 0x004f5218 */
		Vec4_Add(&A, &B, &m1);
		Vec4_Add(&m1, &C, &E);
		x = Clamp01((t - 28.0) * 0.25);
		E.y = (float)(E.y - Math_Ease(&x, &one) * 1.6);
		Cam9(eye, E.x, E.y, (2.0 + E.z) + 0.8, A.x, A.y + 2.0, A.z, 0.0, 1.0, 0.6);
	}
	if (32.0 <= t && t < 36.0) {
		V0 = Vec4_Make(-96.0f, 150.0f, -408.0f);
		Wv = Vec4_Make(-353.0f, (float)(h2 * 0.0 + 255.0), -152.0f);
		s = (float)(Clamp01((t - 32.0) * 0.05) + 1.7);
		Vec4_Sub(&V0, &Wv, &D);
		Vec4_Scale(&D, &s, &S);
		Vec4_Add(&Wv, &S, &V);
		V.x = (float)(V.x + 22.0);
		V.z = (float)(((t - 32.0) * 5.6) * 10.0 + V.z);
		V.y = (float)(V.y + 50.0);
		Cam9(eye, V.x, V.y, V.z, V0.x, V0.y, V0.z, 0.0, 1.0, 0.0);
	}
	if (36.0 <= t && t < 42.0) {
		a = 0.8 - t * 0.2;
		Cam9(eye, sin(a) * 490.0, 260.0, cos(a) * 490.0, 0.0, 150.0, 0.0, 0.0, 1.0, 0.4);
	}
	if (42.0 <= t && t < 45.0) {
		a = t * 0.2 - 3.2;
		V = Vec4_Make(26.0f, 155.0f, 150.0f);
		B = Vec4_Make((float)(sin(a) * 140.0), -110.0f, (float)(cos(a) * 140.0));
		Vec4_Add(&V, &B, &V);
		Cam9(eye, V.x, V.y, V.z, 26.0, 155.0, 150.0, 0.2, 1.0, 0.0);
	}
	if (45.0 <= t && t < 145.0) {                      /* 0x004f51c8 */
		u = Max0(t - 47.0) * 0.5;
		R10 = Math_SoftRamp(&u) * 10.0;
		V = Vec4_Make(176.0f, 155.0f, (float)(((t - 45.0) * 20.0 + 150.0) - 300.0));
		Vc = V;
		m1 = Vec4_Make(100.0f, 0.0f, 0.0f);
		Vec4_Add(&V, &m1, &V);
		if (t >= 54.0) {
			V.z = (float)(V.z + 60.0);
			V.y = (float)(V.y + 12.0);
			Vc.x = (float)(Vc.x + 60.0);
		}
		Cam9(eye, 0.0 * h2 + V.x, (V.y + 50.0) - 50.0, V.z + R10, Vc.x, Vc.y, Vc.z + 60.0,
		     0.2, 1.0, 0.0);
	}

	Vec4_Sub(&eye->target, &eye->eye, &F);
	Fc = F;
	Vec4_Normalize(&Fc);
	Vec4_Normalize(&F);
	Vec4_Cross(&F, &eye->up, &R);
	n = (unsigned)path.points.size() - 1;
	idx = (unsigned)(long)(t * 35.0) % n;              /* 0x004f47c0 */
	if (t >= 54.0) {
		idx = (unsigned)(long)((t + 3.0) * 35.0) % n;
	}
	Bd = 0.008;                                        /* 0x004f4c98 */
	Bc = 1.0;
	k = 1.0;
	if (t > 34.0) {                                    /* 0x004f4ee0 */
		Bd = 0.08;
		k = 0.14;                                      /* 0x004f4778 */
	}
	if (t >= 36.0) {
		Bd = Bd * 0.5;
		Bc = 0.0;
	}
	if (t < 45.0 && t >= 42.0) {
		Bd = Bd * 5.0;
		Bc = 1.2;
	}
	if (t >= 49.0) {
		c = Clamp01(t - 49.0);
		Bd = (1.0 + (c + c)) * Bd;
	}
	K16 = k * 1.6;
	px = path.points[idx].x;
	py = path.points[idx].y;
	rightV = (((((px * 1.1) * Bd) * 0.01) * 4.8) * 0.4) * K16;
	upV = (((((py * 1.1) * Bd) * 0.01) * 4.8) * 0.4) * K16;
	fB = (float)Bd;
	Vec4_Scale(&R, &px, &m1);
	Vec4_Scale(&m1, &fB, &m2);
	Vec4_Add(&eye->target, &m2, &eye->target);
	Vec4_Scale(&eye->up, &py, &m1);
	Vec4_Scale(&m1, &fB, &m2);
	Vec4_Add(&eye->target, &m2, &eye->target);
	eye->up.x = (float)((sin((path.a[idx] * 1.7188733853924696) * Bd) * 0.05) * Bc + eye->up.x);
	t10 = t * 10.0;
	ctx->light = *eye;
	m1 = Vec4_Make(30.0f, 0.0f, 0.0f);                 /* 0x004f47dc, 0x004f4f74 */
	Vec4_Scale(&m1, &k14, &m2);
	Vec4_Add(&ctx->light.eye, &m2, &ctx->light.eye);

	DrawScene(ctx, 1);
	Fbo_Resolve(g, 0, W, H, 2);
	Fbo_Unbind();
	glDisable(GL_DEPTH_TEST);
	Post_RunPass(g, passBlur, 2, 1, W / 2, H / 2, texNormal, texGlow, -1, -1);
	Post_RunPass(g, passWire, 0, 1, W, H, texNormal, texNormal, texGlow, texColour);
	Post_RunPass(g, passBlur, 2, 6, W / 2, H / 2, texNormal, texGlow, -1, -1);
	L = Clamp01(t - 0.1) - Clamp01((t - 51.0) * 0.5);
	if (t >= 54.0) {
		L = Clamp01(t - 54.0) * 0.35 - Clamp01((t - 56.0) * 0.5) * 0.35;
		rightV = rightV * 2.0;
		upV = upV * 2.0;
	}
	x = Clamp01(t - 0.1);
	alpha = (1.0 - Math_Ease(&x, &one)) * 0.4 + 0.15;
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
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	gl1::LoadIdentity();
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(g, texGlow);
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	Tex_Bind(g, texQuarter);
	gl1::Color3f(0.15f, 0.15f, 0.15f);                 /* 0x004f3760 */
	Tex_Bind(g, texNormal);
	Lf = (float)L;
	gl1::Color3f(Lf, Lf, Lf);
	Gfx_DrawQuad(&zero, &one, &one, 1);
	gl1::PopMatrix();
	gl1::PopMatrix();
	Attrib_Pop();
	seq->partEnd = t;
	glDisable(GL_BLEND);
	glFlush();
}

/*
 * ORIGIN 0x00476630 part_city::Init: part_boat::Init's shape in slot
 * band 0. The targets only with seq->relayMode 0, blob41b through
 * Tex_LoadWithAlpha, box.3ds stood in for by its material as
 * part_motor::Init explains. The meshes' own lists are fixed ids in
 * the demo (0x5c..0x77); here they are gl1's. citycrash.obj and
 * Hellidist.obj are broken into shards and flown at load, each shard
 * thrown by its own randoms; the first v each loop builds is
 * overwritten before use, as in the demo (effects/city.txt 2).
 */
void part_city::Init(Rupture_Ctx *ctx)
{
	Gfx_Ctx    *g = &ctx->gfx;
	Shader_Ctx *s = &g->shader;
	int         w = ctx->width, h = ctx->height, i;
	const float k05 = 0.5f, k10 = 10.0f, k02 = 0.2f;   /* 0x004f3720, 0x004f3bbc, 0x004f3754 */
	Vec4        off = Vec4_Make(-3.71f, 1.81f, 0.0f); /* 0x004f50e8, 0x004f50e4 */
	Vec4        up3 = Vec4_Make(0.0f, 3.0f, 0.0f);    /* 0x004f3854 */
	Vec4        d, v, m;
	double      r, D, P;
	float       fP, a1, a2, a3, b1, b2;
	size_t      j;

	t = 0.0;
	tLocal = 0.0;
	frames = 0;
	texBase = 0;
	val1370 = 0x70;
	val1374 = 0x70;
	texNormal = 0;
	texColour = 0xb;
	slot1380 = 0x6f;
	texQuarter = 2;
	texGlow = 3;
	texSlot = 1;
	model = 0;
	pairBoat = 0x30;
	pairAniso2h = 2;
	passBlurCirc = 0x15;
	passBlur = 0x17;
	passBend = 0x1f;
	passBlurFOV = 0x1d;
	passWire = 0x1b;
	slot13c0 = 0x20;
	pairTrain = 0x22;
	pairHeli = 0x26;
	pairCity = 0x2a;
	pairCity2 = 0x56;
	slot314 = 0x19;
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
	g->tex[texBase + 0x5a] = Tex_Load("textures/bg08.jpg");
	g->tex[texBase + 0x14] = Tex_LoadWithAlpha("textures/blob41b.jpg", "textures/blob41b.jpg");
	Obj_Load(&boat, gl1::GenLists(1), "meshes/boat.obj");
	Obj_Load(&helicopter, gl1::GenLists(1), "meshes/helicopter.obj");
	Obj_Load(&hellicrash2, gl1::GenLists(1), "meshes/hellicrash2.obj");
	Obj_Load(&helix, gl1::GenLists(1), "meshes/helix.obj");
	Obj_Load(&hellidist, gl1::GenLists(1), "meshes/Hellidist.obj");
	Obj_Load(&city1, gl1::GenLists(1), "meshes/city1.obj");
	Obj_Load(&cityLess, gl1::GenLists(1), "meshes/city_less.obj");
	Obj_Load(&citywater, gl1::GenLists(1), "meshes/citywater.obj");
	Obj_Load(&citycrash, gl1::GenLists(1), "meshes/citycrash.obj");
	Obj_Load(&tablet, gl1::GenLists(1), "meshes/tablet.obj");

	Crt_Srand(0x2861f2eu);
	for (i = 0; i < 0x6a; i++) {
		Crt_Rand();
	}
	Obj_MakeShards(&citycrash, &citycrashShards, 0.12);   /* 0x004f4600 */
	for (j = 0; j < citycrashShards.size(); j++) {
		Shard *sh = &citycrashShards[j];

		r = Math_Random();
		D = pow_int(r, 3) * 6.0;
		P = pow_int(Math_Random() * 1.2, 17) + 2.0;
		fP = (float)P;
		Vec4_Sub(&sh->rest, &off, &d);
		Vec4_Scale(&d, &fP, &v);
		Vec4_Add(&v, &up3, &v);
		a1 = (float)(Math_Random() * 8.0 - 4.0);
		a2 = (float)(Math_Random() * 9.0 - 2.0);
		a3 = (float)((Math_Random() * 4.0 - 2.0) - D);
		v = Vec4_Make(a3, a2, a1);
		b1 = (float)(Math_Random() * 5.0 - 3.0);
		r = Math_Random();
		b2 = (float)((r + r) - 1.0);
		d = Vec4_Make(b2, b1, (float)(P * 4.0));
		Vec4_Scale(&d, &k05, &m);
		Vec4_Scale(&m, &k10, &v);
		sh->a = Vec4_Make(0.0f, -66.0f, 0.0f);         /* 0x004f50e0 */
		sh->v = v;
	}
	Obj_BakeShards(&citycrashShards, 6, 20);
	Obj_MakeShards(&hellidist, &hellidistShards, 5.0);
	for (j = 0; j < hellidistShards.size(); j++) {
		Shard *sh = &hellidistShards[j];

		r = Math_Random();
		D = pow_int(r, 3) * 6.0;
		P = pow_int(Math_Random() * 1.2, 17) + 2.0;
		fP = (float)P;
		Vec4_Sub(&sh->rest, &off, &d);
		Vec4_Scale(&d, &fP, &v);
		Vec4_Add(&v, &up3, &v);
		a1 = (float)(Math_Random() * 8.0 - 4.0);
		a2 = (float)(Math_Random() * 9.0 - 2.0);
		a3 = (float)((Math_Random() * 4.0 - 2.0) - D);
		v = Vec4_Make(a3, a2, a1);
		a1 = (float)(Math_Random() * 8.0 - 4.0);
		a2 = (float)(Math_Random() * 9.0 - 2.0);
		a3 = (float)((Math_Random() * 4.0 - 2.0) - D);
		d = Vec4_Make(a3, a2, a1);
		Vec4_Scale(&d, &k02, &v);
		sh->a = Vec4_Make(10.0f, -6.0f, 0.0f);         /* 0x004f50dc */
		sh->v = v;
	}
	Obj_BakeShards(&hellidistShards, 6, 20);

	val13f0 = 200;
	g->tex[(model + 1000) * 100] = Tex_Load("meshes/marmaro.jpg");
	Shader_LoadPair(s, "cgeffects/vertex_projection_anisotropic.cpp",
	                "cgeffects/fragment_projection_anisotropic2h.cpp", pairAniso2h);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_boat.cpp", pairBoat);
	Shader_LoadPair(s, "cgeffects/vertex_boat2.cpp", "cgeffects/fragment_city.cpp", pairCity);
	Shader_LoadPair(s, "cgeffects/vertex_boat2.cpp", "cgeffects/fragment_city2.cpp", pairCity2);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_heli.cpp", pairHeli);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_train.cpp", pairTrain);
	g->tex[texBase + 0x1e] = Tex_Load("textures/cloudt2.jpg");
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256.cpp", passBlur);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256FOV.cpp", passBlurFOV);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_Blur_circular.cpp", passBlurCirc);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_bend3.cpp", passBend);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_wireframe.cpp", passWire);
	Path_Load(&path, "meshes/try1.sav");
	tLocal = t;
}
