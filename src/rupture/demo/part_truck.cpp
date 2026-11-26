#include "part_truck.h"

#include <math.h>
#include <string.h>

#include "../core/camera.h"
#include "../core/gl1.h"
#include "part_motor.h"
#include "scene.h"

/* ORIGIN 0x004469c0 part_truck::part_truck. [open] Only the members Frame, Init and DrawScene use. */
part_truck::part_truck()
{
	int i;

	Path_Construct(&path);
	Track_Construct(&road);
	Track_Construct(&cross);
	for (i = 0; i < 5; i++) {
		Track_Construct(&rails[i]);
	}
	vec2d4 = Vec4_Make(0.0f, 0.0f, 0.0f);
	t = 0.0;
	tLocal = 0.0;
	frames = 0;
	dtCopy = 0.0;
	u = 0.0;
	flicker = 0;
	seqHead[0] = seqHead[1] = seqHead[2] = 0.0;
}

/*
 * ORIGIN 0x004ab200 part_truck::Frame (effects/truck.txt 5): the gate,
 * the clock - held back by 2 from tLocal 63 and by 4 from 65 - the
 * base camera and fifteen windows tested on tLocal + carry, the shake
 * from try1.sav, the light camera, DrawScene and the post chain. The
 * part hands over at t = 66.
 */
void part_truck::Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq)
{
	Gfx_Ctx      *g = &ctx->gfx;
	Camera       *eye = &ctx->eye;
	const double  pi = 3.141592653589793;              /* 0x004f36e8 */
	double        h1, h2, T, a, k, d, e, q, s, w, x, y, r, P, X, Y, v, Wz, ex, tz, Q, Up;
	double        A, B, c, s1, s2, rightV, upV, alpha, T35, one = 1.0, two = 2.0, zero = 0.0;
	float         fA, px, py;
	Vec4          F, Fc, R, m1, m2;
	unsigned      idx, n;
	int           W = ctx->width, H = ctx->height;

	if (seq->relayMode == 1 && (seq->started[2] == 0 || seq->done[2] == 1)) {
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
		tLocal = seqHead[0] * 8.0 - 0.0;
	}
	if (-8.0 > tLocal) {                               /* 0x004f4718 */
		tLocal = -8.0;
	}
	t = g_partTimeCarry + tLocal;
	if (tLocal >= 63.0) {                              /* 0x004f6570 */
		t = (tLocal - 2.0) + g_partTimeCarry;
	}
	if (tLocal >= 65.0) {                              /* 0x004f4938 */
		t = (tLocal - 4.0) + g_partTimeCarry;
	}
	frames++;
	ctx->nearZ = 0.05;                                 /* 0x004f48d8 */
	ctx->farZ = 550.0;                                 /* 0x004f6128 */
	seq->partClock = ((t + 39.0) + 64.0) - -7.0;
	if (59.0 <= t && t < 63.0) {
		ctx->nearZ += 2.1;                             /* 0x004f5468 */
	}
	if (t < 63.0) {
		ctx->nearZ += 0.5;
	}
	ctx->nearZ += h2;
	if (t >= 66.0) {                                   /* 0x004f6520 */
		g_partTimeCarry = t - 66.0;
		Attrib_Pop();
		seq->done[2] = 1;
		seq->started[3] = 1;
		return;
	}
	gl1::PushMatrix();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Fbo_Bind(g, 0, 2);
	u = t * 27.0;                                      /* 0x004f4ef8 */

	Cam9(eye, u - 20.0, 10.0 * h2 - 50.0, -10.0, u - 70.0, (10.0 * h2 - 10.0) - 50.0, 0.0,
	     0.0, 1.0, 0.0);
	T = tLocal + g_partTimeCarry;
	if (-10.0 < T && T < -5.0) {
		x = Clamp01((t - -9.8) * 0.25);
		e = Math_Ease(&x, &one);
		q = (1.0 - e) * -14.0;
		X = u + 40.0;
		Cam9(eye, (X - 50.0) - (tLocal - -10.0) * 15.0, q - 60.0, (tLocal - -8.5) - 14.0,
		     20.0 + X, q - 100.0, 0.0, 0.1 * q, 1.0, 0.0);
	}
	if (-5.0 <= T && T < 1.0) {
		a = u - 25.0;
		k = tLocal * 6.5;
		Cam9(eye, a - k, -68.0, -4.0, (a - 100.0) - k, -88.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (1.0 <= T && T < 5.0) {
		Cam9(eye, (u - 20.0) - tLocal * 5.0, -32.0, (tLocal - 1.0) * 20.0 * 0.2 - 140.0,
		     (u - 30.0) - tLocal * 6.0, -102.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (5.0 <= T && T < 8.0) {
		ex = ((h2 * 20.0 + u) - 60.0) - tLocal * 5.0;
		Cam9(eye, ex, h1 - 62.0, 0.0, 250.0 + ex, h1 - 132.0, 0.0, 0.0, 1.0, 0.0);
		d = tLocal - 5.0;
		Cam9(eye, 124.0 - d * 19.0, -69.35, -3.0, 24.0 - d * 19.0, -69.35, d * 13.0 + 0.0,
		     0.0, 1.0, 0.1 * d);
	}
	if (8.0 <= T && T < 17.0) {
		s = (tLocal - 15.0) * 0.5;
		if (s < 0.0) {
			s = 0.0;
		}
		w = Math_SoftRamp(&s) * h2;
		x = Clamp01(((tLocal - 8.0) + 0.2) * 0.5 * 0.4);
		e = Math_Ease(&x, &two);
		q = (1.0 - e) * 1.6 - 1.8;
		Up = u - 11.8;                                 /* 0x004f6ad8 */
		Q = q * 21.0;
		Cam9(eye, ((Up - 20.0) + Q + tLocal) + w, w - 68.0, ((q + 0.0) - (tLocal - 8.0) * 0.0) - w,
		     ((Up - 70.0) + Q) + tLocal, -70.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (17.0 <= T && T < 25.0) {
		s = (tLocal - 17.0) * 0.5;
		if (s < 0.0) {
			s = 0.0;
		}
		Math_SoftRamp(&s);
		x = Clamp01(((tLocal - 8.0) + 0.2) * 0.5 * 0.4);
		e = Math_Ease(&x, &two);
		q = (1.0 - e) * 1.6 - 1.8;
		P = (((u - 11.8) + 47.0) + q * 21.0) - tLocal * 3.0;
		Cam9(eye, P - 4.0, -68.2, -3.0, P, -68.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (25.0 <= T && T < 28.0) {
		X = (u - 11.8) - 40.0;
		Cam9(eye, X + 0.0, -68.0, (tLocal - 25.0) * 0.5, 40.0 + X, -68.0, 0.0, 0.0, 1.0, 0.0);
	}
	if (28.0 <= T && T < 36.0) {
		d = tLocal - 30.0;
		X = (u - 11.8) - 40.0;
		Cam9(eye, d * 4.0 + ((X + 30.0) - 30.0), d - 68.0, 9.0 - d * 2.6, (X - 40.0) + 30.0,
		     d - 71.0, 0.0, 0.0, 1.0, 0.0);
	}
	Y = (((((u - 11.8) - 40.0) + 30.0) - 30.0) - 10.0);
	if (36.0 <= T && T < 46.0) {
		a = 1.0 - tLocal * 0.45;
		x = Clamp01(tLocal - 40.0);
		e = Math_Ease(&x, &one) * -4.4;
		Cam9(eye, (((((((((sin(a) * 35.0 + e) - 9.2) - 2.6) + u) - 40.0) + 30.0) - 30.0) - 10.0) - 2.6),
		     -58.0, cos(a) * 35.0, Y, -68.0, 0.0, 0.2, 1.0, 0.0);
	}
	if (46.0 <= T && T < 56.0) {
		a = tLocal * 0.15 - 0.2;
		Cam9(eye, ((((((((sin(a) * 41.0 - 9.2) - 2.6) + u) - 40.0) + 30.0) - 30.0) - 10.0) - 2.6) - 20.0,
		     -58.0, cos(a) * 41.0, Y - 20.0, -68.0, 0.0, 0.2, 1.0, 0.0);
	}
	if (56.0 <= T && T < 59.0) {
		a = tLocal * -0.15 - 1.2;
		Cam9(eye, ((sin(a) * 15.0 + 0.0) + 1420.0) - 25.0, -66.0, cos(a) * 15.0, 1395.0, -68.0, 0.0,
		     0.0, 1.0, 0.0);
	}
	if (59.0 <= T && T < 63.0) {
		v = tLocal - 53.0;
		if (v < 0.0) {
			v = 0.0;
		}
		Cam9(eye, 1396.2, (tLocal - 59.0) * 0.3 - 65.0, (450.0 - (v * 20.0 + 208.0)) - 3.0, 1386.2,
		     -69.0, 450.0 - v * 50.0, 0.0, 1.0, 0.0);
	}
	if (63.0 <= T && T < 65.0) {
		a = tLocal * -0.15 - 1.2;
		tz = 0.0;
		if (63.5 < tLocal) {
			y = Clamp01(((tLocal - 63.5) + 0.0) * 0.5);
			r = 1.0 - y;
			r = r * (r * r);
			tz = (1.0 - r) * -16.0;
		}
		Cam9(eye, ((sin(a) * 15.0 + 0.0) + 1420.0) - 25.0, -66.0, cos(a) * 15.0, 1395.0, -68.0, tz,
		     0.0, 1.0, 0.0);
	}
	if (65.0 <= T && T < 67.0) {
		v = tLocal - 55.0;
		if (v < 0.0) {
			v = 0.0;
		}
		Wz = 450.0 - v * 50.0;
		Cam9(eye, 1376.2, -64.0, (Wz + 60.0) - (tLocal - 65.0) * -10.0, 1386.2, -69.0, 100.0 + Wz,
		     0.0, 1.0, 0.0);
	}
	if (67.0 <= T && T < 165.0) {
		v = tLocal - 55.0;
		if (v < 0.0) {
			v = 0.0;
		}
		Wz = (450.0 - v * 0.4 * 50.0) - 250.0;
		Cam9(eye, 1388.2, -68.0, Wz, 1386.2, -63.0, Wz - 40.0, 0.0, 1.0, 0.0);
	}

	Vec4_Sub(&eye->target, &eye->eye, &F);
	Fc = F;
	Vec4_Normalize(&Fc);
	Vec4_Normalize(&F);
	Vec4_Cross(&F, &eye->up, &R);
	T35 = t * 35.0;
	n = (unsigned)path.points.size() - 1;
	idx = (unsigned)(long)T35 % n;
	A = h2 * 0.01;
	B = 0.0;
	if (t < -4.0) {
		A = 0.002;
	}
	if (t >= -4.0) {
		A = Clamp01(t - -2.0) * 0.03;
		B = 0.05;
	}
	if (t >= 8.0) {
		A = Clamp01((t - 11.0) * 0.5) * 0.02 + 0.012;
		B = 0.05;
	}
	if (t >= 17.0) {
		A = 0.002;
		B = 0.05;
	}
	if (t >= 25.0) {
		idx = (unsigned)(long)(T35 - -245.0) % n;
		A = 0.04 - Clamp01((t - 36.0) * 0.2) * 0.036000000000000004;   /* 0x004f6a38 */
		B = 0.09;
	}
	if (t >= 46.0) {
		idx = (unsigned)(long)T35 % n;
		A = (1.0 - Clamp01((t - 46.0) * 0.3)) * 0.01 * 3.0 + 0.01;
		B = 0.09;
	}
	if (tLocal > 63.8) {
		c = Clamp01((t - 61.8) * 10.0);
		A = (c + c) * A;
		B = 0.1;
	}
	px = path.points[idx].x;
	py = path.points[idx].y;
	rightV = px * -1.1 * A * 0.01;
	upV = (A * (py * -1.1)) * 0.01;
	if (tLocal > 63.8) {
		k = 4.0 - Clamp01((t - 61.8) * 3.0) * 3.0;
		rightV = k * rightV;
		upV = k * upV;
	}
	if (tLocal > 57.0) {
		k = 5.0 - Clamp01((t - 57.0) * 0.5) * 4.0;
		rightV = k * rightV;
		upV = k * upV;
	}
	s1 = sin(Clamp01((t - 8.5) * 0.7) * pi);
	A = (s1 + 1.0) * A;
	B = (s1 * 60.0 + 1.0) * B;
	s2 = sin(Clamp01((t - 59.0) * 0.4) * pi);
	rightV = s2 * 0.04000000000000001 + rightV;        /* 0x004f6a18 */
	fA = (float)A;
	Vec4_Scale(&R, &px, &m1);
	Vec4_Scale(&m1, &fA, &m2);
	Vec4_Add(&eye->target, &m2, &eye->target);
	Vec4_Scale(&eye->up, &py, &m1);
	Vec4_Scale(&m1, &fA, &m2);
	Vec4_Add(&eye->target, &m2, &eye->target);
	eye->up.x = (float)(sin(path.a[idx] * 1.7188733853924696 * A) * B + eye->up.x);
	ctx->light = *eye;
	Cam9(&ctx->light, 0.0, 0.0, 30.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	DrawScene(ctx, 1);
	Fbo_Resolve(g, 0, W, H, 2);
	Fbo_Unbind();
	glDisable(GL_DEPTH_TEST);
	Post_RunPass(g, passBlur, 2, 1, W / 2, H / 2, texNormal, texGlow, -1, -1);
	Post_RunPass(g, passWire, 0, 1, W, H, texNormal, texNormal, texGlow, texColour);
	Post_RunPass(g, passBlur, 2, 6, W / 2, H / 2, texNormal, texGlow, -1, -1);
	alpha = 0.2;
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
 * ORIGIN 0x004aa5e0 part_truck::Init (effects/truck.txt 1, 2). The
 * targets are made only with seq->relayMode 0, as in part_motor::Init,
 * and their noise moves the CRT stream before the shards draw from it.
 * box.3ds is stood in for by its material, as part_motor::Init
 * explains. The meshes' display lists are fixed ids in the demo
 * (0x385..0x3a3); here they are gl1's.
 */
void part_truck::Init(Rupture_Ctx *ctx)
{
	Gfx_Ctx    *g = &ctx->gfx;
	Shader_Ctx *s = &g->shader;
	int         w = ctx->width, h = ctx->height, j, kk;
	size_t      i;
	Vec4        pt, step, A, Bv, Cv, Dv;
	double      r1, a, ang, zx, zz;
	float       vz, vy, af, Ax, Ay, Az;
	const float zero = 0.0f, k16 = 1.6f;               /* 0x004f4598 */

	t = 0.0;
	tLocal = 0.0;
	frames = 0;
	texNormal = 0;
	texColour = 0xb;
	texQuarter = 2;
	texGlow = 3;
	texBase = 2000;
	model = 2000;
	passBlur = 0x7e7;
	passBend = 0x7ef;
	passWire = 0x7eb;
	pairS2 = 0x7f0;
	pairS1 = 0x7f6;
	pairMotion = 0x7f4;
	pairHex2 = 0x7fc;
	pairHex22 = 0x808;
	pairHex222 = 0x80e;
	pairRoad = 0x7fe;
	pairMotor = 0x800;
	pairTruck = 0x804;
	pairDestruction = 0x81a;
	pairLight = 0x806;
	pairBridge = 0x80a;
	pairBoat = 0x816;
	pairTrain = 0x818;
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
	Obj_Load(&g_motorcycleObj, gl1::GenLists(1), "meshes/motorcycle.obj");
	Obj_Load(&hex2, gl1::GenLists(1), "meshes/hex2.obj");
	Obj_Load(&destruction, gl1::GenLists(1), "meshes/truckdestruction.obj");
	Obj_Load(&sign, gl1::GenLists(1), "meshes/sign.obj");
	Obj_Load(&hex3, gl1::GenLists(1), "meshes/hex3.obj");
	Obj_Load(&g_truckObj, gl1::GenLists(1), "meshes/truck.obj");
	Obj_Load(&tholos, gl1::GenLists(1), "meshes/tholos.obj");
	Obj_Load(&tholos2, gl1::GenLists(1), "meshes/tholos2.obj");
	Obj_Load(&lights, gl1::GenLists(1), "meshes/trucklights.obj");
	Obj_Load(&empty2, gl1::GenLists(1), "meshes/truck_empty2.obj");
	Obj_Load(&g_boatObj, gl1::GenLists(1), "meshes/boat.obj");
	Obj_Load(&trainlong, gl1::GenLists(1), "meshes/trainlong.obj");

	Obj_MakeShards(&destruction, &shards, 1.0);
	for (i = 0; i < shards.size(); i++) {
		r1 = Math_Random();
		a = (r1 * (r1 * r1)) * 6.0;                    /* 0x004f3780 */
		vz = (float)-(Math_Random() * 8.0 + a * 3.0);
		vy = (float)(Math_Random() * 8.0 - 3.0);
		af = (float)a;
		Az = (float)(Math_Random() * 8.0 - 4.0);
		Ay = (float)(((Math_Random() * 2.0) - 1.0) * 0.0);
		Ax = (float)(Math_Random() * 9.0);
		A = Vec4_Make(Ax, Ay, Az);
		Vec4_Scale(&A, &af, &Bv);
		Vec4_Scale(&Bv, &k16, &Cv);
		Vec4_Scale(&Cv, &zero, &Dv);
		shards[i].a = Dv;
		shards[i].v = Vec4_Make(0.0f, vy, vz);
	}
	Obj_BakeShards(&shards, 8, 20);

	Track_Clear(&road);
	step = Vec4_Make(1.0f, 0.0f, 0.0f);
	pt = Vec4_Make(-100.0f, -50.0f, 0.0f);             /* 0x004f5ff8, 0x004f5db4 */
	for (kk = 0; kk < 1680; kk++) {
		Vec4_Add(&pt, &step, &pt);
		road.points.push_back(pt);
		road.count++;
	}
	Track_Clear(&cross);
	step = Vec4_Make(0.0f, 0.0f, -0.7f);               /* 0x004f64ac */
	pt = Vec4_Make(0.0f, -50.0f, 450.0f);              /* 0x004f5dac */
	for (kk = 0; kk < 950; kk++) {
		Vec4_Add(&pt, &step, &pt);
		cross.points.push_back(pt);
		cross.count++;
	}
	for (j = 0; j < 5; j++) {
		Track_Clear(&rails[j]);
		step = Vec4_Make(0.0f, 0.0f, 0.7f);            /* 0x004f4060 */
		pt = Vec4_Make(0.0f, -50.0f, 0.0f);
		ang = (double)(j - 2) * 0.003;                 /* 0x004f6078 */
		for (kk = 0; kk < 650; kk++) {
			Vec4_Add(&pt, &step, &pt);
			zx = 0.0;
			zz = 0.0;
			vec4_rotate_euler(&step, &zx, &ang, &zz);
			rails[j].points.push_back(pt);
			rails[j].count++;
		}
	}
	g->tex[(model + 1000) * 100] = Tex_Load("meshes/marmaro.jpg");
	Shader_LoadPair(s, "cgeffects/vertex_projection_anisotropic.cpp",
	                "cgeffects/fragment_projection_anisotropic.cpp", 0x7d2);
	Shader_LoadPair(s, "cgeffects/v_roadcut2.cpp", "cgeffects/f_roadcut.cpp", pairRoad);
	Shader_LoadPair(s, "cgeffects/v_roadcut2.cpp", "cgeffects/f_bridge.cpp", pairBridge);
	Shader_LoadPair(s, "cgeffects/v_motor.cpp", "cgeffects/f_motor.cpp", pairMotor);
	Shader_LoadPair(s, "cgeffects/v_truck.cpp", "cgeffects/f_truck.cpp", pairTruck);
	Shader_LoadPair(s, "cgeffects/v_truck_destruction.cpp", "cgeffects/f_truck_destruction.cpp",
	                pairDestruction);
	Shader_LoadPair(s, "cgeffects/v_truck.cpp", "cgeffects/f_truck_light.cpp", pairLight);
	Shader_LoadPair(s, "cgeffects/vertex_boat_test2building2.cpp",
	                "cgeffects/fragment_boat_testhex2.cpp", pairHex2);
	Shader_LoadPair(s, "cgeffects/vertex_boat_test2building2.cpp",
	                "cgeffects/fragment_boat_testhex22.cpp", pairHex22);
	Shader_LoadPair(s, "cgeffects/vertex_boat_test2building2.cpp",
	                "cgeffects/fragment_boat_testhex222.cpp", pairHex222);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_boatold.cpp", pairBoat);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_trainold.cpp", pairTrain);
	Shader_LoadPair(s, "cgeffects/vertex_boat_test_motion.cpp",
	                "cgeffects/fragment_boat_test_motion.cpp", pairMotion);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256.cpp", passBlur);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256FOV.cpp", 0x7ed);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_Blur_circular.cpp", 0x7e5);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_bend3.cpp", passBend);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_motion.cpp", 0x7f9);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_wireframe.cpp", passWire);
	Path_Load(&path, "meshes/try1.sav");
	t = -8.0;                                          /* 0x004f4718 */
	tLocal = -8.0;
}
