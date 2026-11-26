#include "part_truck.h"

#include <math.h>

#include "../core/gl1.h"
#include "part_motor.h"
#include "scene.h"

Obj_Mesh g_truckObj;
Obj_Mesh g_boatObj;

/*
 * ORIGIN 0x0040b4b0 FUN_0040b4b0, maybe_Math_Flicker: 1 - (1 - x + H)^2
 * on x clamped to [0, 1], H = 1 while sin(24.4 x^2 + the 20-term sum)
 * is >= 0. The sum does not depend on x and is kept as the loop.
 */
static double maybe_Math_Flicker(double x)
{
	double acc, s, h, b, r;
	int    i;

	if (x > 1.0) {
		x = 1.0;
	} else if (x < 0.0) {
		x = 0.0;
	}
	acc = x * x * 24.4;                                /* 0x004f3790 */
	for (i = 0; i < 20; i++) {
		acc = acc + sin((double)i * 13.2) * 6.0;       /* 0x004f3788, 0x004f3780 */
	}
	s = sin(acc);
	h = s >= 0.0 ? 1.0 : 0.0;
	b = 1.0 - (x - h);
	r = b * b;
	return 1.0 - r;
}

/* OURS. x < 0 -> 0, the inline test the code repeats. */
static double Max0(double x)
{
	return x < 0.0 ? 0.0 : x;
}

/*
 * ORIGIN 0x004a1710 FUN_004a1710, maybe_Track_DrawRoadDepth:
 * maybe_Track_DrawRoad (0x0048ec00) with the two side strips dropped by
 * (U * 1.5) * depth instead of U * -1.5. ctx is not read.
 */
static void maybe_Track_DrawRoadDepth(Rupture_Ctx *ctx, Track *T, double depth)
{
	const float k15 = 1.5f, neg = -1.0f;
	Vec4        P, D, R, U, a, b, c, m1, m2, P2, D2, R2, U2;
	float       depthf;
	int         k;

	(void)ctx;
	glDisable(GL_CULL_FACE);
	T->loop = 0;
	gl1::Begin(gl1::QUAD_STRIP);
	for (k = 1; k < T->count; k++) {
		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		a = Vec4_AddScaled(&P, &R, -4.0f);
		b = Vec4_AddScaled(&P, &R, 4.0f);
		gl1::Normal3f(U.x, U.y, U.z);
		gl1::Vertex3f(a.x, a.y, a.z);
		gl1::Vertex3f(b.x, b.y, b.z);
	}
	gl1::End();
	gl1::Begin(gl1::QUAD_STRIP);
	depthf = (float)depth;
	for (k = 1; k < T->count; k++) {
		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		a = Vec4_AddScaled(&P, &R, -4.0f);
		Vec4_Scale(&U, &k15, &m1);
		Vec4_Scale(&m1, &depthf, &m2);
		c = Vec4_AddScaled(&P, &R, -4.0f);
		Vec4_Sub(&c, &m2, &b);
		gl1::Normal3f(R.x, R.y, R.z);
		gl1::Vertex3f(a.x, a.y, a.z);
		gl1::Vertex3f(b.x, b.y, b.z);
	}
	gl1::End();
	gl1::Begin(gl1::QUAD_STRIP);
	depthf = (float)depth;
	for (k = 1; k < T->count; k++) {
		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		a = Vec4_AddScaled(&P, &R, 4.0f);
		Vec4_Scale(&U, &k15, &m1);
		Vec4_Scale(&m1, &depthf, &m2);
		c = Vec4_AddScaled(&P, &R, 4.0f);
		Vec4_Sub(&c, &m2, &b);
		Vec4_Scale(&R, &neg, &R);
		gl1::Normal3f(R.x, R.y, R.z);
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
		a = Vec4_AddScaled(&P, &R, -0.1f);
		b = Vec4_AddScaled(&P, &R, 0.1f);
		gl1::Normal3f(0.0f, -1.0f, 0.0f);
		gl1::Vertex3f(a.x, (float)(a.y + 0.1), a.z);
		gl1::Vertex3f(b.x, (float)(b.y + 0.1), b.z);
		Scene_RoadFrame(T, k + 1, &P2, &D2, &R2, &U2);
		a = Vec4_AddScaled(&P2, &R2, -0.1f);
		b = Vec4_AddScaled(&P2, &R2, 0.1f);
		gl1::Normal3f(0.0f, -1.0f, 0.0f);
		gl1::Vertex3f(b.x, (float)(b.y + 0.1), b.z);
		gl1::Vertex3f(a.x, (float)(a.y + 0.1), a.z);
	}
	gl1::End();
}

/* OURS. One rail wall of maybe_Track_DrawRails: R at w, dropped (U * 1.5) * depth, normal -R. */
static void rail_wall(Track *T, float w, double depth)
{
	const float k15 = 1.5f, neg = -1.0f;
	Vec4        P, D, R, U, a, b, c, m1, m2;
	float       depthf;
	int         k;

	gl1::Begin(gl1::QUAD_STRIP);
	depthf = (float)depth;
	for (k = 1; k < T->count; k++) {
		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		a = Vec4_AddScaled(&P, &R, w);
		Vec4_Scale(&U, &k15, &m1);
		Vec4_Scale(&m1, &depthf, &m2);
		c = Vec4_AddScaled(&P, &R, w);
		Vec4_Sub(&c, &m2, &b);
		Vec4_Scale(&R, &neg, &R);
		gl1::Normal3f(R.x, R.y, R.z);
		gl1::Vertex3f(a.x, a.y, a.z);
		gl1::Vertex3f(b.x, b.y, b.z);
	}
	gl1::End();
}

/* OURS. One rail top of maybe_Track_DrawRails: R from w0 to w1, normal U. */
static void rail_top(Track *T, float w0, float w1)
{
	Vec4 P, D, R, U, a, b;
	int  k;

	gl1::Begin(gl1::QUAD_STRIP);
	for (k = 1; k < T->count; k++) {
		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		a = Vec4_AddScaled(&P, &R, w0);
		b = Vec4_AddScaled(&P, &R, w1);
		gl1::Normal3f(U.x, U.y, U.z);
		gl1::Vertex3f(a.x, a.y, a.z);
		gl1::Vertex3f(b.x, b.y, b.z);
	}
	gl1::End();
}

/*
 * ORIGIN 0x0049f9b0 FUN_0049f9b0, maybe_Track_DrawRails: two rails
 * (tops and inner walls) and a five-faced sleeper box 4.4 wide on
 * every fourth point, its face normals world axes. ctx is not read.
 * The far corners of the top swap y (C's x, z with E's y and back), as
 * the code does.
 */
void maybe_Track_DrawRails(Rupture_Ctx *ctx, Track *T, double depth)
{
	Vec4  P, D, R, U, A, B, P2, D2, R2, U2, C, E;
	float Ay4, By4, Cy4, Ey4, Ay1, By1, Cy1, Ey1;
	int   c;

	(void)ctx;
	glDisable(GL_CULL_FACE);
	T->loop = 0;
	rail_top(T, -1.4f, -1.2f);
	rail_wall(T, -1.4f, depth);
	gl1::Begin(gl1::QUAD_STRIP);
	gl1::End();
	rail_top(T, 1.4f, 1.6f);
	rail_wall(T, 1.4f, depth);
	gl1::Begin(gl1::QUAD_STRIP);
	gl1::End();
	gl1::Begin(gl1::QUADS);
	for (c = 1; c < T->count - 3; c++) {
		c++;
		if ((c / 2) % 2 != 1) {
			continue;
		}
		Scene_RoadFrame(T, c, &P, &D, &R, &U);
		A = Vec4_AddScaled(&P, &R, -2.2f);
		B = Vec4_AddScaled(&P, &R, 2.2f);
		gl1::Normal3f(0.0f, 1.0f, 0.0f);
		Ay4 = (float)(A.y - 0.4);
		gl1::Vertex3f(A.x, Ay4, A.z);
		By4 = (float)(B.y - 0.4);
		gl1::Vertex3f(B.x, By4, B.z);
		Scene_RoadFrame(T, c + 1, &P2, &D2, &R2, &U2);
		C = Vec4_AddScaled(&P2, &R2, -2.2f);
		E = Vec4_AddScaled(&P2, &R2, 2.2f);
		gl1::Normal3f(0.0f, 1.0f, 0.0f);
		Cy4 = (float)(C.y - 0.4);
		gl1::Vertex3f(E.x, Cy4, E.z);
		Ey4 = (float)(E.y - 0.4);
		gl1::Vertex3f(C.x, Ey4, C.z);

		gl1::Normal3f(-1.0f, 0.0f, 0.0f);
		gl1::Vertex3f(A.x, Ay4, A.z);
		gl1::Vertex3f(B.x, By4, B.z);
		By1 = (float)(B.y - 1.0);
		gl1::Vertex3f(B.x, By1, B.z);
		Ay1 = (float)(A.y - 1.0);
		gl1::Vertex3f(A.x, Ay1, A.z);

		gl1::Normal3f(1.0f, 0.0f, 0.0f);
		gl1::Vertex3f(C.x, Cy4, C.z);
		gl1::Vertex3f(E.x, Ey4, E.z);
		Ey1 = (float)(E.y - 1.0);
		gl1::Vertex3f(E.x, Ey1, E.z);
		Cy1 = (float)(C.y - 1.0);
		gl1::Vertex3f(C.x, Cy1, C.z);

		gl1::Normal3f(0.0f, 0.0f, 1.0f);
		gl1::Vertex3f(A.x, Ay4, A.z);
		gl1::Vertex3f(C.x, Cy4, C.z);
		gl1::Vertex3f(C.x, Cy1, C.z);
		gl1::Vertex3f(A.x, Ay1, A.z);

		gl1::Normal3f(0.0f, 0.0f, -1.0f);
		gl1::Vertex3f(B.x, By4, B.z);
		gl1::Vertex3f(E.x, Ey4, E.z);
		gl1::Vertex3f(E.x, Ey1, E.z);
		gl1::Vertex3f(B.x, By1, B.z);
	}
	gl1::End();
}

/*
 * OURS. Scene_Lit with part_truck's slots. The demo passes
 * *(double*)(ctx+0x19a0) as the time; maybe_Scene_DrawLit ignores it,
 * so 0.0 goes here.
 */
static void lit(Rupture_Ctx *ctx, part_truck *p, int pair)
{
	Scene_Lit(ctx, p->model, p->texBase, p->texNormal, pair, 0.0);
}

/* OURS. The object-matrix opening every mesh gets: push, TEXTURE, identity. */
static void obj_begin(void)
{
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
}

/* OURS. The texture matrix handed to the pair as ModelViewObject, back to MODELVIEW. */
static void obj_end(Shader_Ctx *s, int pair)
{
	Shader_SetTextureMatrix(s, pair, 0xe);
	gl1::MatrixMode(gl1::MODELVIEW);
}

/* OURS. The two Math_Ease calls of the motorcycle placement whose results are never read. */
static void eases_unread(double t)
{
	double x, one = 1.0;

	x = Clamp01((t - -9.4) * 0.3);
	Math_Ease(&x, &one);
	x = Clamp01((t - -9.8) * 0.3);
	Math_Ease(&x, &one);
}

/*
 * OURS. The truck's place on the road track, as sections A, B, C, the
 * lights and the headlights each compute it: P = track(k); the basis
 * from track(k + 3) is computed and not read.
 */
static void truck_on_road(part_truck *p, double t, Vec4 *P)
{
	double     y, sr, k;
	Vec4       Q = Vec4_Make(0.0f, 0.0f, 0.0f), D, R, U;
	const Vec4 up = Vec4_Make(0.0f, 1.0f, 0.0f);

	y = Max0(t - 53.2);
	sr = Math_SoftRamp(&y) * -22.0;
	*P = Vec4_Make(0.0f, 0.0f, 0.0f);
	k = 15.0 * (t + 13.0) + 50.0;
	if (8.0 < t) {
		k = ((t + 13.0) * 27.0 - 130.0) + sr;
	}
	Track_Eval(&p->road, (float)k, P);
	Track_Eval(&p->road, (float)(k + 3.0), &Q);
	Vec4_Sub(&Q, P, &D);
	Vec4_Normalize(&D);
	Vec4_Cross(&up, &D, &R);
	Vec4_Normalize(&R);
	Vec4_Cross(&R, &D, &U);
	Vec4_Normalize(&U);
}

/*
 * ORIGIN 0x004a2d0f..0x004a2e00, the flag this+0x2c0: on when
 * seqHead[2] > 0.1 or the flicker of t - 6 is lit; after t = 8 the
 * flicker of t - 13 alone; after t = 58 the inverse of the flicker of
 * 2 (t - 58).
 */
static void truck_flag(part_truck *p, double t)
{
	double d;

	p->flicker = 0;
	if (0.1 < p->seqHead[2]) {
		p->flicker = 1;
	}
	if (maybe_Math_Flicker(t - 6.0) > 0.5) {
		p->flicker = 1;
	}
	if (t > 8.0) {
		p->flicker = maybe_Math_Flicker(t - 13.0) > 0.5 ? 1 : 0;
	}
	if (t > 58.0) {
		d = t - 58.0;
		p->flicker = maybe_Math_Flicker(d + d) < 0.5 ? 1 : 0;
	}
}

/*
 * ORIGIN 0x004a2e01..0x004a326a, the sky: motor_sky's 2500 stars, then
 * always a blob41b sun and three cloudt2 glows at (eye.x - 200, 0, -60),
 * the glows pushed along z.
 */
static void truck_sky(Rupture_Ctx *ctx, part_truck *p, double t)
{
	Gfx_Ctx    *g = &ctx->gfx;
	const float k400 = 400.0f;                         /* 0x004f4638 */
	double      zero = 0.0, s25 = 25.0, s90a = 90.0, s90b = 90.0, ang, dk;
	float       gr;
	Vec4        v;
	int         i, k;
	static const GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1 };

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawBuffers(2, bufs);
	Gfx_ResetState();
	Shader_DisableProfiles(&g->shader);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(65.0, (double)ctx->width / ctx->height, 1.0, 1000.0);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	glDisable(GL_DEPTH_TEST);
	gl1::Begin(gl1::POINTS);
	for (i = 0; i < 2500; i++) {
		Math_Random();
		Math_Random();
		gr = (float)((Math_Random() + 0.2) * 0.2);
		gl1::Color3f(gr, gr, gr);
		v = Vec4_RandomDir(true);
		Vec4_Normalize(&v);
		Vec4_Scale(&v, &k400, &v);
		gl1::Vertex3f(ctx->eye.eye.x + v.x, (float)(v.y - 160.0), v.z);
	}
	gl1::End();

	gl1::PushMatrix();
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	gl1::Translatef((float)(ctx->eye.eye.x - 200.0), 0.0f, -60.0f);
	gl1::Color3f(0.4f, 0.3f, 0.3f);
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(g, p->texBase + 0x14);
	Gfx_DrawQuad(&zero, &s25, &s25, 1);
	glDisable(GL_BLEND);
	gl1::PopMatrix();
	for (k = 0; k < 3; k++) {
		gl1::PushMatrix();
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		gl1::Translatef((float)(ctx->eye.eye.x - 200.0), 0.0f, -60.0f);
		dk = (double)k;
		gl1::Translatef(0.0f, 0.0f, (float)(sin(t * 0.8 + dk * 4.0) * 11.0 + (double)(30 * k)));
		ang = dk * 40.0 - 48.0;
		if (k == 2) {
			ang = -40.0;
		}
		gl1::Color3f(0.03f, 0.05f, 0.08f);
		if (k == 2) {
			gl1::Color3f(0.05f, 0.04f, 0.04f);
		}
		gl1::Enable(GL_TEXTURE_2D);
		Tex_Bind(g, p->texBase + 0x1e);
		Gfx_DrawQuad(&ang, &s90a, &s90b, 1);
		glDisable(GL_BLEND);
		gl1::PopMatrix();
	}
}

/*
 * OURS. The pairRoad block's parameters, the same in S3, S4 and S5:
 * one draw for alpha2, alphaplane2, then 0 for it after t = 5, and the
 * Reduction, (0, 0, 0) after t = 5.
 */
static void road_params(Shader_Ctx *s, part_truck *p, int P, double t, double uu)
{
	double a2, ap2, z = 0.0;
	float  red;

	a2 = Math_Random() * 0.0;
	Shader_SetParam1f(s, P + 1, "alpha2", &a2);
	ap2 = (((p->seqHead[2] * 0.0 - 30.0) + 10.0) + uu) - 4.0;
	Shader_SetParam1f(s, P + 1, "alphaplane2", &ap2);
	if (t > 5.0) {
		Shader_SetParam1f(s, P + 1, "alphaplane2", &z);
	}
	Shader_GetParam(s, P, 4, "Reduction");
	red = (float)((uu - 30.0) - 10.0);
	Shader_SetParam3f(s, P, 4, red, 0.0f, 0.0f);
	if (t > 5.0) {
		Shader_SetParam3f(s, P, 4, 0.0f, 0.0f, 0.0f);
	}
}

/* OURS. The road pair: pairRoad, or pairBridge after t = 28. */
static int road_pair(part_truck *p, double t)
{
	return t > 28.0 ? p->pairBridge : p->pairRoad;
}

/*
 * ORIGIN 0x004a3594..0x004a38ec, S3: the road along this+0x1378, its
 * sides 1.5 v deep - v falls from 100 to 5 over t = 31..33 and rises
 * by 95 again over 46..47. Not drawn in [36, 39.3) and [43, 46.6).
 */
static void truck_road(Rupture_Ctx *ctx, part_truck *p, double t, double uu)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	int         P;
	double      v, c1, c2;
	bool        ok;

	Crt_Srand(0x108a);
	P = road_pair(p, t);
	lit(ctx, p, P);
	glEnable(GL_CULL_FACE);
	obj_begin();
	road_params(s, p, P, t, uu);
	gl1::Translatef(-180.0f, -20.06f, 0.0f);
	gl1::Color4f(0.6f, 0.8f, 1.0f, 1.0f);
	obj_end(s, P);
	v = 1.0;
	if (t > 28.0) {
		c1 = Clamp01((t - 31.0) * 0.5);
		v = 100.0 - c1 * 95.0;
		c2 = Clamp01(t - 46.0);
		v = v + 95.0 * c2;
	}
	ok = true;
	if (t >= 36.0 && t < 39.3) {
		ok = false;
	}
	if ((t < 43.0 || t >= 46.6) && ok) {
		maybe_Track_DrawRoadDepth(ctx, &p->road, v);
	}
	gl1::PopMatrix();
}

/* ORIGIN 0x004a38f2..0x004a3b3a, S4: after t = 53 the rails along this+0x13d8. */
static void truck_cross(Rupture_Ctx *ctx, part_truck *p, double t, double uu)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	int         P;

	Crt_Srand(0x108a);
	if (!(t > 53.0)) {
		return;
	}
	P = road_pair(p, t);
	lit(ctx, p, P);
	glEnable(GL_CULL_FACE);
	obj_begin();
	road_params(s, p, P, t, uu);
	gl1::Translatef(1386.2f, -19.74f, 0.0f);
	gl1::Color4f(0.6f, 0.8f, 1.0f, 1.0f);
	obj_end(s, P);
	Scene_DrawRailsCached(ctx, &p->cross, 0.5, maybe_Track_DrawRails);
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x004a3b40..0x004a3eab, S5: after t = 50.4 the rails of
 * tracks this+0x1498 and this+0x1558 (the index and the pointer step
 * twice a pass), each drawn again turned 188 degrees about y, rising
 * from 10 below over one second.
 */
static void truck_rails(Rupture_Ctx *ctx, part_truck *p, double t, double uu)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	int         P, i;
	Track      *tr;
	double      c, drop;

	Crt_Srand(0x108a);
	if (!(t > 50.4)) {
		return;
	}
	i = 0;
	tr = &p->rails[0];
	do {
		c = Clamp01(t - 50.4);
		i++;
		tr++;
		drop = (1.0 - c) * -10.0;
		P = road_pair(p, t);
		lit(ctx, p, P);
		glEnable(GL_CULL_FACE);
		obj_begin();
		road_params(s, p, P, t, uu);
		gl1::Translatef(1386.2f, (float)((((drop - 1.6) + 2.0) - 20.0) - 0.06), 2.0f);
		gl1::Color4f(0.7f, 0.8f, 0.8f, 1.0f);
		obj_end(s, P);
		Scene_DrawRailsCached(ctx, tr, 0.5, maybe_Track_DrawRails);
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::Rotatef(188.0f, 0.0f, 1.0f, 0.0f);
		gl1::Color4f(0.7f, 0.8f, 0.8f, 1.0f);
		obj_end(s, P);
		Scene_DrawRailsCached(ctx, tr, 0.5, maybe_Track_DrawRails);
		gl1::PopMatrix();
		i++;
		tr++;
	} while (i < 4);
}

/*
 * ORIGIN 0x004a3eb1..0x004a446c, S6: before t = 17 the motorcycle on
 * the road track at s = max(u + 250, 5), pitched by the track's slope,
 * swaying after t = 0. u is lowered by 3.2 around the block.
 */
static void truck_motorcycle(Rupture_Ctx *ctx, part_truck *p, double t, double *uu)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	double      ap2, k, yaw;
	float       dot;
	Vec4        P0 = Vec4_Make(0.0f, 0.0f, 0.0f), P1 = P0, D, R, U = Vec4_Make(0.0f, 1.0f, 0.0f);

	if (!(t < 17.0)) {
		return;
	}
	lit(ctx, p, p->pairMotor);
	glEnable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	*uu = *uu - 3.2;
	obj_begin();
	ap2 = (*uu - 150.0) - 4.0;
	Shader_SetParam1f(s, p->pairMotor + 1, "alphaplane2", &ap2);
	k = (*uu + 245.0) + 5.0;
	if (k < 5.0) {
		k = 5.0;
	}
	Track_Eval(&p->road, (float)k, &P0);
	Track_Eval(&p->road, (float)(k + 3.0), &P1);
	Vec4_Sub(&P1, &P0, &D);
	Vec4_Normalize(&D);
	Vec4_Cross(&U, &D, &R);
	Vec4_Normalize(&R);
	Vec4_Cross(&R, &D, &U);
	Vec4_Normalize(&U);
	gl1::Translatef(P0.x, P0.y, P0.z);
	gl1::Translatef(-180.0f, -20.0f, 0.0f);
	dot = D.kind != 0 ? (D.y * 0.0f + D.x) + D.z * 0.0f : 0.0f;
	yaw = (double)(float)acos((double)dot) * -90.0;
	eases_unread(t);
	gl1::Translatef(0.0f, (float)(yaw * -4.2 * 0.01), 0.0f);
	if (t > 0.0) {
		gl1::Rotatef((float)(sin(t + t) * 20.0), 1.0f, 0.0f, 0.0f);
	}
	gl1::Rotatef(90.0f, 0.0f, 1.0f, 0.0f);
	gl1::Rotatef((float)yaw, 1.0f, 0.0f, 0.0f);
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	gl1::Color3f(0.3f, 0.3f, 0.3f);
	obj_end(s, p->pairMotor);
	Obj_Draw(&g_motorcycleObj);
	gl1::PopMatrix();
	*uu = *uu + 3.2;
}

/*
 * OURS. The lit bracket and opening of sections A, B and C: the pair,
 * cull off then on, srand(0x409bfa), the object matrix opened.
 */
static void truck_open(Rupture_Ctx *ctx, part_truck *p, int pair)
{
	lit(ctx, p, pair);
	glDisable(GL_CULL_FACE);
	glEnable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	obj_begin();
}

/*
 * ORIGIN 0x004a446d..0x004a4d33, section A: before t = 59 the whole
 * truck.obj on the road track, rolled in after t = 61.6 and jolted on
 * one frame-count in three.
 */
static void truck_whole(Rupture_Ctx *ctx, part_truck *p, double t, double uu, double red)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const double pi = 3.141592653589793, d528 = p->seqHead[2];
	double       x, A, a2, ap2, e, w, a, c, b, pw, v, f, S, two = 2.0;
	Vec4         pos;

	truck_open(ctx, p, p->pairTruck);
	x = Max0((t - 9.0) * 0.5);
	A = Math_SoftRamp(&x) * -0.8 * 10.0;
	if (9.0 < t) {
		A = A - (d528 * 0.0 + 20.0);
	}
	if (59.0 <= t) {
		A = A - 100.0;
	}
	a2 = Math_Random() * 0.0;
	Shader_SetParam1f(s, p->pairTruck + 1, "alpha2", &a2);
	ap2 = ((((((d528 * 0.0 + 2.0) - 30.0) + 10.0) + uu) - 4.0) - 4.0) + A;
	Shader_SetParam1f(s, p->pairTruck + 1, "alphaplane2", &ap2);
	Shader_GetParam(s, p->pairTruck, 4, "Reduction");
	Shader_SetParam3f(s, p->pairTruck, 4, (float)(red + A), 0.0f, 0.0f);
	truck_on_road(p, t, &pos);
	x = Clamp01((t - 56.7) * 2.5);
	e = Math_Ease(&x, &two);
	w = 0.18 - e * 0.18;
	if (56.0 <= t) {
		pos.y = (float)(w * 1.25 + pos.y);
	}
	gl1::Translatef(pos.x, pos.y, pos.z);
	a = t - 61.6;
	c = Clamp01(a * 6.0);
	b = 1.0 - c;
	pw = (b * b) * (b * b);
	v = (1.0 - pw) * 0.5 + Max0(a * 3.8);
	if (v > 1.0) {
		v = 1.0;
	}
	gl1::Translatef(-180.0f, -20.0f, (float)(0.0 * (v * -0.3)));
	gl1::Rotatef((float)(v * d528 * 10.0), 0.0f, 1.0f, 0.0f);
	gl1::Rotatef((float)(v * -3.0), 0.0f, 1.0f, 0.0f);
	eases_unread(t);
	f = t * 6.0 * 5.0;
	S = (int)f % 3 == 0 ? sin(f * pi) : 0.0;
	if (28.0 <= t) {
		S = S * ((1.0 - Clamp01(t - 57.0)) * 2.5);
	}
	gl1::Rotatef((float)(S * 0.3), 1.0f, 0.0f, 0.0f);
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	gl1::Color3f(0.3f, 0.3f, 0.3f);
	obj_end(s, p->pairTruck);
	Obj_Draw(&g_truckObj);
	gl1::PopMatrix();
}

/* OURS. The late lift of sections B and C: 0.05 down to 0 over t = 56.7 .. 57.03, after t = 55. */
static void truck_lift(double t, Vec4 *pos)
{
	double x, e, two = 2.0;

	if (55.0 < t) {
		x = Clamp01((t - 56.7) * 3.0);
		e = Math_Ease(&x, &two);
		pos->y = (float)((0.05 - e * 0.05) + pos->y);
	}
}

/*
 * ORIGIN 0x004a4d38..0x004a557b, section B: from t = 59 the two groups
 * of truck_empty2.obj, tipping over after t = 61.55, group 1 by a fifth
 * as much.
 */
static void truck_empty(Rupture_Ctx *ctx, part_truck *p, double t, double uu, double red)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const double d528 = p->seqHead[2];
	double       x, AB, a2, ap2, c, q2, h, ang;
	Vec4         pos;
	size_t       i;

	for (i = 0; i < 2; i++) {
		truck_open(ctx, p, p->pairTruck);
		x = Max0((t - 9.0) * 0.5);
		AB = Math_SoftRamp(&x) * -0.8 * 10.0;
		if (9.0 < t) {
			AB = AB - 1020.0;
		}
		a2 = Math_Random() * 0.0;
		Shader_SetParam1f(s, p->pairTruck + 1, "alpha2", &a2);
		ap2 = ((((((d528 * 0.0 + 2.0) - 30.0) + 10.0) + uu) - 4.0) - 4.0) + AB;
		Shader_SetParam1f(s, p->pairTruck + 1, "alphaplane2", &ap2);
		Shader_GetParam(s, p->pairTruck, 4, "Reduction");
		Shader_SetParam3f(s, p->pairTruck, 4, (float)(AB + red), 0.0f, 0.0f);
		truck_on_road(p, t, &pos);
		truck_lift(t, &pos);
		gl1::Translatef(pos.x, pos.y, pos.z);
		c = Clamp01((t - 61.55) + (t - 61.55));
		q2 = 1.0 - (1.0 - c) * (1.0 - c);
		if (q2 > 1.0) {
			q2 = 1.0;
		}
		gl1::Translatef(-180.0f, -20.0f, (float)(0.0 * (q2 * -0.3 * 3.6)));
		h = Clamp01(t - 61.9) + q2;
		gl1::Translatef(0.0f, (float)(h * 0.2 - 0.3), 0.0f);
		ang = h * -1.5 * 10.0 * 0.6 * 1.2;
		if (i != 0) {
			ang = ang * 0.2;
		}
		gl1::Rotatef((float)ang, 1.0f, 2.6f, 0.0f);
		gl1::Rotatef((float)(h * -3.0), 0.0f, 1.0f, 0.0f);
		eases_unread(t);
		gl1::Scalef(1.0f, 1.0f, 1.0f);
		gl1::Color3f(0.3f, 0.3f, 0.3f);
		obj_end(s, p->pairTruck);
		if (i < p->empty2.groups.size()) {
			Obj_DrawGroup(&p->empty2.groups[i]);
		}
		gl1::PopMatrix();
	}
}

/*
 * OURS. A shard's rest-centre distance from P0, as the code inlines it:
 * the squares rounded to float, summed once, rooted, rounded to float.
 */
static float shard_len(const Vec4 *d)
{
	float xx = d->x * d->x, yy = d->y * d->y, zz = d->z * d->z;

	return (float)sqrt((double)(float)(((double)xx + yy) + zz));
}

/*
 * ORIGIN 0x004a5581..0x004a6ebf, section C: from t = 59 the shards of
 * truckdestruction.obj. Each is seeked to s from its rest distance to
 * P0 = (-14, 3.1, -1.32): beyond 3.5 (unless the draw is under 0.07) it
 * flies off at 10 + 0.8 W, nearer it follows R. Drawn as four
 * triangles each, greyer the nearer it is.
 */
static void truck_destruction(Rupture_Ctx *ctx, part_truck *p, double t)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const double d528 = p->seqHead[2];
	const Vec4   P0 = Vec4_Make(-14.0f, 3.1f, -1.32f), zero = Vec4_Make(0.0f, 0.0f, 0.0f);
	const Vec4   half3 = Vec4_Make(0.5f, 0.5f, 0.5f);
	const float  one = 1.0f, half = 0.5f, fd = (float)d528;
	double       x, c3, q3, G, h, U, R, W, sk, v, L98, r;
	float        len, c;
	Vec4         pos, d, t0, t1, t2, V[4], pp, pq, Ld8;
	size_t       i, n;
	int          k;
	bool         isFar;
	static const GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1 };

	truck_open(ctx, p, p->pairDestruction);
	x = Max0((t - 9.0) * 0.5);
	Math_SoftRamp(&x);
	truck_on_road(p, t, &pos);
	truck_lift(t, &pos);
	gl1::Translatef(pos.x, pos.y, pos.z);
	gl1::Translatef(-180.0f, -20.0f, (float)(-0.3 * 0.0));
	gl1::Rotatef((float)(0.0 * -3.0), 0.0f, 1.0f, 0.0f);
	eases_unread(t);
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	gl1::Color3f(0.3f, 0.3f, 0.3f);
	c3 = Clamp01(((t - 61.7) + 0.08) * 3.0);
	q3 = 1.0 - (1.0 - c3) * (1.0 - c3);
	if (q3 > 1.0) {
		q3 = 1.0;
	}
	G = Max0(t - 62.0);
	gl1::Translatef(0.0f, (float)((q3 - 3.0 * G) * 0.05), (float)(-0.3 * q3 * 3.6));
	h = Clamp01(t - 61.9) + q3;
	gl1::Translatef(0.0f, (float)(h * 0.2 - 0.3), 0.0f);
	gl1::Rotatef((float)(h * -1.5 * 10.0 * 0.6), 1.0f, 0.0f, 0.0f);
	gl1::Rotatef((float)(h * -3.0), 0.0f, 1.0f, 0.0f);
	obj_end(s, p->pairDestruction);
	glDisable(GL_CULL_FACE);

	n = p->shards.size();
	for (i = 0; i < n; i++) {
		Shard *sh = &p->shards[i];

		d = sh->rest;
		Vec4_Sub(&d, &P0, &d);
		U = Max0((t - 61.6) + 0.2);
		R = U * 4.0 * 10.0;
		W = (t - 61.7) + (t - 61.7);
		W = W > 4.0 ? 4.0 : (W < 0.0 ? 0.0 : W);
		isFar = false;
		len = shard_len(&d);
		if (len > 3.5) {
			isFar = true;
		}
		r = Math_Random();
		if (!(r < 0.07) && isFar) {
			sk = W * 0.8 + 10.0;
		} else {
			sk = Clamp01(fabs(shard_len(&d) - 3.5) * 1.6) * R;
		}
		Shard_Seek(sh, sk);
	}
	gl1::BatchBegin();   /* OURS: one draw for the loop, docs/subsystems/render.txt */
	for (i = 0; i < n; i++) {
		Shard *sh = &p->shards[i];

		d = sh->rest;
		Vec4_Sub(&d, &P0, &d);
		R = Max0((t - 61.6) + 0.2) * 4.0 * 10.0;
		len = shard_len(&d);
		if (len > 3.5) {
			v = 0.0;
		} else {
			v = Clamp01(fabs(len - 3.5) * 100.6) * R;
		}
		L98 = v * v * 0.6 * 0.001;
		c = (float)(L98 + 0.3);
		gl1::Color3f(c, c, c);
		for (k = 0; k < 4; k++) {
			Vec4_Scale(&sh->turned[k], &one, &t0);
			Vec4_Add(&t0, &sh->pos, &t1);
			Vec4_Add(&t1, &zero, &t2);
			Vec4_Scale(&t2, &one, &V[k]);
		}
		Vec4_Sub(&sh->pos, &zero, &pp);
		Vec4_Scale(&pp, &half, &pq);
		Vec4_Scale(&pq, &fd, &pp);
		Vec4_Add(&half3, &pp, &Ld8);
		gl1::Begin(gl1::TRIANGLES);
		gl1::Normal3f(sh->turnedN[0].x, sh->turnedN[0].y, sh->turnedN[0].z);
		gl1::Vertex3f(V[0].x, V[0].y, V[0].z);
		gl1::Vertex3f(V[1].x, V[1].y, V[1].z);
		gl1::Vertex3f(V[2].x, V[2].y, V[2].z);
		gl1::Normal3f(sh->turnedN[1].x, sh->turnedN[1].y, sh->turnedN[1].z);
		gl1::Vertex3f(V[0].x, V[0].y, V[0].z);
		gl1::Vertex3f(V[1].x, V[1].y, V[1].z);
		gl1::Vertex3f(V[3].x, V[3].y, V[3].z);
		gl1::Normal3f(sh->turnedN[2].x, sh->turnedN[2].y, sh->turnedN[2].z);
		gl1::Vertex3f(V[0].x, V[0].y, V[0].z);
		gl1::Vertex3f(V[2].x, V[2].y, V[2].z);
		gl1::Vertex3f(V[3].x, V[3].y, V[3].z);
		gl1::Normal3f(sh->turnedN[3].x, sh->turnedN[3].y, sh->turnedN[3].z);
		gl1::Vertex3f(V[1].x, V[1].y, V[1].z);
		gl1::Vertex3f(V[2].x, V[2].y, V[2].z);
		gl1::Vertex3f(V[3].x, V[3].y, V[3].z);
		gl1::End();
	}
	gl1::BatchEnd();
	glDrawBuffers(2, bufs);
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x004a6ebf..0x004a731e, sections 1 and 2: after t = 53 the long
 * train and after t = 60.5 the boat, both easing up 8 into place from
 * t = 58.4 and drifting in z by d = max(t - 53, 0).
 */
static void truck_train_boat(Rupture_Ctx *ctx, part_truck *p, double t, double d)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	double      x, r, one = 1.0;

	if (t > 53.0) {
		lit(ctx, p, p->pairTrain);
		glDisable(GL_CULL_FACE);
		glEnable(GL_CULL_FACE);
		Crt_Srand(0x409bfa);
		obj_begin();
		x = Clamp01(((t - 59.0) + 0.6) * 0.5);
		r = Math_Ease(&x, &one);
		gl1::Translatef(1386.2f, (float)(-69.0 - (8.0 - r * 8.0)), (float)((450.0 - d * 50.0) - 10.0));
		gl1::Rotatef(180.0f, 0.0f, 1.0f, 0.0f);
		gl1::Scalef(1.0f, 1.0f, 1.0f);
		gl1::Color3f(0.15f, 0.2f, 0.25f);
		obj_end(s, p->pairTrain);
		Obj_Draw(&p->trainlong);
		gl1::PopMatrix();
	}
	if (t > 60.5) {
		lit(ctx, p, p->pairTrain);
		glDisable(GL_CULL_FACE);
		glEnable(GL_CULL_FACE);
		Crt_Srand(0x409bfa);
		obj_begin();
		x = Clamp01(((t - 59.0) + 0.6) * 0.5);
		r = Math_Ease(&x, &one);
		gl1::Translatef(1386.2f, (float)((2.0 - ((8.0 - r * 8.0) + 69.0)) + 0.6),
		                (float)(((450.0 - d * 50.0) - 10.0) + 36.0));
		gl1::Rotatef(180.0f, 0.0f, 1.0f, 0.0f);
		gl1::Scalef(1.0f, 1.0f, 1.0f);
		gl1::Color3f(0.15f, 0.2f, 0.25f);
		obj_end(s, p->pairTrain);
		Obj_Draw(&g_boatObj);
		gl1::PopMatrix();
	}
}

/* ORIGIN 0x004a7324..0x004a78f4, section 3: trucklights.obj at the truck's place while the flag is on. */
static void truck_lights(Rupture_Ctx *ctx, part_truck *p, double t, double uu, double red)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const double d528 = p->seqHead[2];
	double       x, w, a2, ap2;
	Vec4         P;

	if (p->flicker == 0) {
		return;
	}
	lit(ctx, p, p->pairLight);
	glDisable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	obj_begin();
	x = Max0((t - 9.0) * 0.5);
	w = Math_SoftRamp(&x) * -0.8 * 10.0;
	if (t > 9.0) {
		w = w - 20.0;
	}
	a2 = Math_Random() * 0.0;
	Shader_SetParam1f(s, p->pairLight + 1, "alpha2", &a2);
	ap2 = (((((d528 * 0.0 + 2.0) - 30.0) + 10.0) + uu) - 4.0);
	ap2 = (ap2 - 4.0) + w;
	Shader_SetParam1f(s, p->pairLight + 1, "alphaplane2", &ap2);
	Shader_GetParam(s, p->pairLight, 4, "Reduction");
	Shader_SetParam3f(s, p->pairLight, 4, (float)(w + red), 0.0f, 0.0f);
	truck_on_road(p, t, &P);
	gl1::Translatef(P.x, P.y, P.z);
	gl1::Translatef(-180.0f, -20.0f, 0.0f);
	eases_unread(t);
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	gl1::Color3f(0.3f, 0.3f, 0.3f);
	obj_end(s, p->pairLight);
	Obj_Draw(&p->lights);
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x004a78fa..0x004a7fd6, section 4: the hex2 tiles, columns
 * 2m - 20 .. 2m + 10 reseeded each, rows -10B .. 10B, bobbing on cos
 * waves with a per-tile "doit" fade. j + 2 = k + cnt, so the test on
 * it zeroes the bob of rows -2..1; rows 0 and -1 are not drawn.
 */
static void hex2_grid(Rupture_Ctx *ctx, part_truck *p, double t, int n)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	int         m, off, i0, i1, j0, j, i, k, q, cnt, rem, rem2;
	unsigned    seed;
	double      B, jEnd, i02, i04, zoff, jd, sn, fade, h, tl, e, doit, cA, X1, cB, X2, rz, one;
	float       len, xf;
	Vec4        V;

	lit(ctx, p, p->pairHex2);
	glDisable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	m = n;
	if (t > 28.0) {
		m = (int)((double)n - 8.0);
	}
	B = t > 5.0 ? 0.3 : 1.0;
	if (t >= 56.0) {
		B = 0.8;
	}
	off = 0;
	if (t > 1.0 && t < 5.0) {
		off = -3;
		B = 0.8;
	}
	i0 = m * 2 - 20;
	i1 = off + m * 2 + 10;
	if (!(i0 < i1)) {
		return;
	}
	j0 = (int)(B * -10.0);
	jEnd = B * 10.0;
	k = -1998 - i0;
	seed = (unsigned)(i0 * 8000 + 0x409bfa);
	for (i = i0; i < i1; i++, seed += 8000, k--) {
		Crt_Srand(seed);
		for (q = 100; q; q--) {
			Math_Random();
		}
		j = j0;
		if (!((double)j < jEnd)) {
			continue;
		}
		V = Vec4_Make((float)i, 0.0f, 0.0f);
		i02 = (double)i * 0.2;
		i04 = (double)i * 0.4;
		rem = i % 2;
		zoff = (double)rem * 8.0;
		cnt = j + i + 2000;
		xf = (float)((double)i * 13.6);
		do {
			jd = (double)j;
			V.y = (float)j;
			len = Vec4_Length(&V);
			sn = sin(len + t * 0.3) * 4.0;
			fade = 1.0 - Clamp01((t - 56.0) * 0.3);
			h = sn * fade;
			tl = t - (Math_Random() * 7.0 - 9.5);
			e = (Math_Random() + 1.0) * 0.07 * tl;
			e = Clamp01(e);
			one = 1.0;
			doit = 2.0 - Math_Ease(&e, &one) * 2.0;
			Shader_SetParam1f(s, p->pairHex2 + 1, "doit", &doit);
			cA = cos(t * 0.51 + jd * 0.3);
			X1 = cos(t * 0.64 + i02) * cA * fade;
			cB = cos(t * 0.71 + jd * 0.6);
			X2 = cos(t * 0.8 + i04) * cB * X1;
			if ((unsigned)(k + cnt) <= 3) {
				X2 = 0.0;
			}
			obj_begin();
			gl1::Translatef(0.0f, (float)((Math_Random() * 0.0 - 70.0) + 7.0), 0.0f);
			gl1::Translatef(xf, (float)(X2 * 10.2 - 5.0), (float)((jd * 15.2 + 9.58) + zoff));
			if (t > 25.0) {
				gl1::Translatef(0.0f, 0.0f, -3.0f);
			}
			if (t < 56.0) {
				rz = Math_Random();
				gl1::Rotatef((float)(h * 10.0), 0.2f, 0.0f, (float)(rz - 0.3));
			} else {
				gl1::Translatef(0.0f, -5.8f, 0.0f);
			}
			gl1::Scalef(1.0f, 1.0f, 1.0f);
			obj_end(s, p->pairHex2);
			rem2 = cnt % 2;
			gl1::Color3f(1.0f, 1.0f, (float)(1.0 - rem2 * 0.1));
			if (j != 0 && j != -1) {
				Obj_Draw(&p->hex2);
			}
			gl1::PopMatrix();
			j++;
			cnt++;
		} while ((double)j < jEnd);
	}
}

/*
 * ORIGIN 0x004a7fd8..0x004a8308, section 5: for 17 < t < 56, two hex3
 * pieces per seed on a ring of radius up to 12 about x, spinning.
 */
static void hex3_ring(Rupture_Ctx *ctx, part_truck *p, double t)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const double pi = 3.141592653589793;
	double       k, N, cd, r1, ang, rad, doit;
	float        px, py, pz;
	unsigned     seed;
	int          c, q;

	lit(ctx, p, p->pairHex22);
	glDisable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	k = 1.0;
	if (t < 28.0) {
		k = 0.3;
	}
	if (t < 36.0) {
		k = 0.4;
	}
	if (!(t > 17.0 && t < 56.0)) {
		return;
	}
	N = k * 20.0;
	seed = 0x409bfa;
	for (c = 0; (double)c < N; c++, seed += 8000) {
		cd = (double)c;
		Crt_Srand(seed);
		for (q = 10; q; q--) {
			Math_Random();
		}
		for (q = 2; q; q--) {
			r1 = Math_Random();
			ang = (r1 + r1) * pi;
			rad = Math_Random() * 12.0;
			px = (float)((Math_Random() + cd) * 15.0 * 1.4);
			py = (float)(sin(ang) * rad);
			pz = (float)(cos(ang) * rad);
			doit = 0.0;
			Shader_SetParam1f(s, p->pairHex22 + 1, "doit", &doit);
			obj_begin();
			gl1::Translatef(758.0f, -60.0f, 0.0f);
			gl1::Translatef(px, py, pz);
			gl1::Rotatef((float)(t * 20.0), 1.0f, 0.0f, 0.0f);
			gl1::Rotatef(90.0f, 0.0f, 0.0f, 1.0f);
			obj_end(s, p->pairHex22);
			gl1::Color3f(1.0f, 1.0f, 0.9f);
			Obj_Draw(&p->hex3);
			gl1::PopMatrix();
		}
	}
}

/* OURS. The lit opening of sections 6-8: cull off, srand, two discarded draws, doit 0. */
static void doit_open(Rupture_Ctx *ctx, part_truck *p, int pair)
{
	double doit = 0.0;

	lit(ctx, p, pair);
	glDisable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	Math_Random();
	Math_Random();
	Shader_SetParam1f(&ctx->gfx.shader, pair + 1, "doit", &doit);
}

/* OURS. The empty placement sections 6-8 make before each drawn one: set up, nothing drawn. */
static void empty_place(Shader_Ctx *s, int pair, float x)
{
	obj_begin();
	gl1::Translatef(x, -70.2f, -3.6f);
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	obj_end(s, pair);
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	gl1::PopMatrix();
}

/* ORIGIN 0x004a830a..0x004a852c, section 6: sign.obj for 25 < t < 33. */
static void truck_sign(Rupture_Ctx *ctx, part_truck *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;

	if (!(t > 25.0 && t < 33.0)) {
		return;
	}
	doit_open(ctx, p, p->pairTruck);
	empty_place(s, p->pairTruck, 690.0f);
	obj_begin();
	gl1::Translatef((float)((p->seqHead[2] * 0.0 + 730.0) - 40.0), -70.2f, 4.0f);
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	obj_end(s, p->pairTruck);
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	Obj_Draw(&p->sign);
	gl1::PopMatrix();
}

/* ORIGIN 0x004a8532..0x004a8804, section 7: before t = 36 a row of tholos.obj, 3 left out, 0 and 1 too after t = 28. */
static void tholos_row(Rupture_Ctx *ctx, part_truck *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	int         i;

	doit_open(ctx, p, p->pairHex222);
	if (!(t < 36.0)) {
		return;
	}
	for (i = 0; i < 5; i++) {
		if (i == 3) {
			continue;
		}
		if (t > 28.0 && (i == 0 || i == 1)) {
			continue;
		}
		empty_place(s, p->pairHex222, 690.0f);
		obj_begin();
		gl1::Translatef((float)(((((((double)i * 1.7) * 4.6) * 10.0 - 100.0) + 730.0) - 40.0) - 210.0),
		                -48.2f, -1.6f);
		gl1::Scalef(1.0f, 1.0f, 1.0f);
		gl1::Rotatef(120.0f, 0.0f, 1.0f, 0.0f);
		if (i == 4) {
			gl1::Rotatef(110.0f, 0.0f, 1.0f, 0.0f);
		}
		obj_end(s, p->pairHex222);
		gl1::Color3f(1.0f, 1.0f, 0.9f);
		Obj_Draw(&p->tholos);
		gl1::PopMatrix();
	}
}

/* ORIGIN 0x004a8806..0x004a8c0c, section 8: for -1 < t < 17 two more tholos.obj, turning after t = 5. */
static void tholos_pair(Rupture_Ctx *ctx, part_truck *p, double t)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const double d528 = p->seqHead[2];
	double       q;

	if (!(t > -1.0 && t < 17.0)) {
		return;
	}
	doit_open(ctx, p, p->pairHex222);
	empty_place(s, p->pairHex222, (float)((d528 * 50.0 + 730.0) - 40.0));
	obj_begin();
	q = ((1.7 * 0.0) * 4.6) * 10.0;
	gl1::Translatef((float)((((q - 370.0) + 730.0) - 40.0) - 210.0), -48.2f, -1.6f);
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	gl1::Rotatef(270.0f, 0.0f, 1.0f, 0.0f);
	if (t > 5.0) {
		gl1::Rotatef(-120.0f, 0.0f, 1.0f, 0.0f);
	}
	obj_end(s, p->pairHex222);
	gl1::Color3f(1.0f, 1.0f, 0.9f);
	Obj_Draw(&p->tholos);
	gl1::PopMatrix();
	obj_begin();
	gl1::Translatef((float)((((q - 460.0) + 730.0) - 40.0) - 210.0), -48.2f, -1.6f);
	if (t < 1.0) {
		gl1::Translatef(-50.0f, 0.0f, 0.0f);
	}
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	gl1::Rotatef(270.0f, 0.0f, 1.0f, 0.0f);
	if (t > 5.0) {
		gl1::Rotatef((float)((((t - 5.0) * d528) * 0.0) - 40.0), 0.0f, 1.0f, 0.0f);
	}
	obj_end(s, p->pairHex222);
	gl1::Color3f(1.0f, 1.0f, 0.9f);
	Obj_Draw(&p->tholos);
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x004a8c12..0x004a97f6, the sparks: part_motor's (0x00492542)
 * instruction for instruction, on this part's road track, gated t < 17.
 */
static void truck_sparks(Rupture_Ctx *ctx, part_truck *p, double t, double u)
{
	const Vec4 down = Vec4_Make(0.0f, -1.0f, 0.0f), up = Vec4_Make(0.0f, 1.0f, 0.0f);
	double     w245, c, N, a, pw, r, sx, sy, ang;
	float      f, fr, wv, v, b, b2, q, s30, f124, fq, fr2, k3, k4, k02 = 0.2f, nm30;
	Vec4       P2 = Vec4_Make(0.0f, 0.0f, 0.0f), Q2 = P2, D, R, U2;
	Vec4       A, B, C, E, F, G, H, I, J, K, L, M, Nn, O;
	int        i;

	Crt_Srand(0x409bfa);
	Gfx_ResetState();
	Shader_DisableProfiles(&ctx->gfx.shader);
	if (!(t < 17.0)) {
		return;
	}
	Crt_Srand(0x409bfa);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	w245 = u + 245.0;
	Track_Eval(&p->road, (float)((w245 + 5.0) - 2.0), &P2);
	Track_Eval(&p->road, (float)(((w245 + 3.0) + 5.0) - 2.0), &Q2);
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
		pw = (double)(b2 * b2);
		q = v * v;
		r = ((1.0 - v) * 1.5) * ((1.0 - v) * 1.5);
		s30 = (float)(v * 30.0);
		gl1::Color3f((float)(pw * 0.2), (float)(pw * 0.05), (float)(pw * 0.01));
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
		ang = Math_Random() * 360.0 + pw * 60.0;
		Gfx_DrawQuad(&ang, &sx, &sy, 1);
		gl1::PopMatrix();
	}
	gl1::BatchEnd();
	gl1::PopMatrix();
}

/* OURS. One floor glow quad of the headlights, 28 x 7 on y = 0, scaled. */
static void floor_quad(float z)
{
	gl1::PushMatrix();
	gl1::Translatef(16.6f, 0.0f, z);
	gl1::Scalef(28.0f, 15.0f, 7.0f);
	gl1::Begin(gl1::QUADS);
	gl1::TexCoord2f(0.0f, 0.0f);
	gl1::Vertex3f(-1.0f, 0.0f, -1.0f);
	gl1::TexCoord2f(1.0f, 0.0f);
	gl1::Vertex3f(1.0f, 0.0f, -1.0f);
	gl1::TexCoord2f(1.0f, 1.0f);
	gl1::Vertex3f(1.0f, 0.0f, 1.0f);
	gl1::TexCoord2f(0.0f, 1.0f);
	gl1::Vertex3f(-1.0f, 0.0f, 1.0f);
	gl1::End();
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x004a97fc..0x004a9f12, section 10: while the flag is on, two
 * additive blob41b headlight billboards at the truck and two floor glow
 * quads ahead of it.
 */
static void truck_headlights(Rupture_Ctx *ctx, part_truck *p, double t)
{
	Gfx_Ctx *g = &ctx->gfx;
	double   a0 = 0.0, s2a = 2.0, s2b = 2.0;
	Vec4     P;

	if (p->flicker == 0) {
		return;
	}
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	truck_on_road(p, t, &P);
	gl1::Translatef(P.x, P.y, P.z);
	gl1::Translatef(-180.0f, -20.0f, 0.0f);
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	gl1::Enable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	Tex_Bind(g, p->texBase + 0x14);
	gl1::Color3f(0.05f, 0.07f, 0.1f);
	glDepthMask(GL_FALSE);
	gl1::PushMatrix();
	gl1::Translatef(0.0f, 1.4f, 1.33f);
	Gfx_DrawQuad(&a0, &s2a, &s2b, 1);
	gl1::PopMatrix();
	gl1::PushMatrix();
	gl1::Translatef(0.0f, 1.4f, -1.1f);
	Gfx_DrawQuad(&a0, &s2a, &s2b, 1);
	gl1::PopMatrix();
	glDisable(GL_CULL_FACE);
	gl1::Color3f(0.1f, 0.1f, 0.13f);
	floor_quad(1.33f);
	floor_quad(-1.1f);
	gl1::Color3f(0.6f, 0.4f, 0.2f);
	gl1::PushMatrix();
	gl1::Translatef(0.0f, 1.4f, 1.33f);
	gl1::PopMatrix();
	glDepthMask(GL_TRUE);
	gl1::PopMatrix();
	gl1::Disable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

/* ORIGIN 0x004a9f18..0x004aa18b, section 11: after t = 61.4 a trail of 30 additive blob41b quads. */
static void glow_trail(Rupture_Ctx *ctx, part_truck *p, double t, double d)
{
	Gfx_Ctx *g = &ctx->gfx;
	double   x, e, id, f, sc, sxy, sxy2, a0, one = 1.0;
	int      i;

	x = Clamp01(t - 61.4);
	e = Math_Ease(&x, &one);
	if (!(e > 0.0001)) {
		return;
	}
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	gl1::Translatef(1386.2f, -66.4f, (float)((((450.0 - d * 50.0) - 10.0) - 6.8) + 0.0));
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	gl1::Enable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	Tex_Bind(g, p->texBase + 0x14);
	gl1::Color3f(0.05f, 0.07f, 0.1f);
	glDepthMask(GL_FALSE);
	for (i = 0; i < 30; i++) {
		id = (double)i;
		f = 1.0 - id / 30.0;
		sc = (1.0 - f) * 1.4 + 1.0;
		if (i == 0) {
			f = 3.0;
		}
		gl1::Color3f((float)(f * 0.03), (float)(f * 0.04), (float)(f * 0.06));
		gl1::PushMatrix();
		gl1::Translatef(0.0f, 0.0f, (float)(id * -6.0 * 0.1));
		sxy = sc * e;
		sxy2 = sxy;
		a0 = 0.0;
		Gfx_DrawQuad(&a0, &sxy, &sxy2, 1);
		gl1::PopMatrix();
	}
	glDepthMask(GL_TRUE);
	gl1::PopMatrix();
	gl1::Disable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

/*
 * ORIGIN 0x004aa205..0x004aa5c7, section 13: once tLocal passes 63, four
 * passes (point size 2, 2, 3, 3) of the same 450 dust points from
 * srand(0xdc05549f), thrown from (1386.2, -67.4, -1.1).
 */
static void truck_dust(part_truck *p, double t)
{
	const float m04 = -0.4f, k20 = 20.0f;
	double      a, tt, g, q, gc, v, w;
	float       bx, by, bz, vy, vz;
	double      r6;
	Vec4        A, Bv, V1, W1, W2;
	int         k, i;

	if (!(p->tLocal > 63.0)) {
		return;
	}
	for (k = 0; k < 4; k++) {
		gl1::PointSize((float)(k / 2 + 2));
		Crt_Srand(0xdc05549f);
		gl1::Begin(gl1::POINTS);
		for (i = 0; i < 450; i++) {
			Math_Random();
			Math_Random();
			a = Math_Random() + 0.2;
			bx = (float)(Math_Random() * 7.0 - 3.5);
			by = (float)(Math_Random() * 2.8 - 1.0);
			r6 = Math_Random();
			bz = (float)(r6 + r6);
			A = Vec4_Make(1386.2f, -67.4f, -1.1f);
			Bv = Vec4_Make(bx, by, bz);
			Vec4_Add(&A, &Bv, &A);
			tt = (t + 0.06) - 61.6;
			g = (tt - Math_Random() * 0.2) - 0.08;
			q = Max0(g);
			gc = Clamp01(g);
			v = gc * 0.3 * a;
			if (i % 7 == 0) {
				v = 1.0;
			}
			w = v * 0.9;
			gl1::Color3f((float)(w * 0.15), (float)(w * 0.2), (float)(w * 0.23));
			vy = (float)(q * q * 0.5 + (Math_Random() * 1.3 - 0.5) * q);
			vz = (float)((Math_Random() * 3.8 + 1.0) * q);
			V1 = Vec4_Make(0.0f, vy, vz);
			Vec4_Scale(&V1, &m04, &W1);
			Vec4_Scale(&W1, &k20, &W2);
			Vec4_Add(&A, &W2, &A);
			gl1::Vertex3f(A.x, A.y, A.z);
		}
		gl1::End();
	}
}

/*
 * ORIGIN 0x004a2d00 part_truck::DrawScene (effects/truck.txt): the flag,
 * the sky, the pass set-up and the row count n, the road and the rails,
 * the motorcycle, the truck (whole before t = 59, emptied and shattering
 * after), the train and the boat, the lights, the hex and tholos
 * scenery, the sparks, the headlights, the glow trail, a fresh
 * projection and the dust. The modelview push after the sky is popped
 * by the function's last call. u = 27 t (0x004f4ef8). The n read by
 * the hex2 grid is the slot written at 0x004a3454: no write between
 * reaches [ESP+0x84] at the body's depth.
 */
void part_truck::DrawScene(Rupture_Ctx *ctx, int pass)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	double      uu, Ed, red, d;
	int         n;
	Vec4        a, b;
	static const GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1 };

	(void)pass;
	Crt_Srand(0xdc072526);
	truck_flag(this, t);
	truck_sky(ctx, this, t);
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
	b = Vec4_Make((float)(seqHead[2] + seqHead[2]), 0.0f, 0.0f);
	Vec4_Add(&a, &b, &vec2d4);
	Ed = (double)ctx->eye.eye.x;
	uu = t * 27.0;
	if (t >= 9.0) {
		Ed = ctx->eye.eye.x + 91.0;                    /* 0x004f5e10 */
	}
	Crt_Srand(0x40987e);
	glDisable(GL_CULL_FACE);
	n = (int)(Ed / 24.0 + 0.0);
	if (n < 5) {
		n = 5;
	}

	lit(ctx, this, pairS1);
	glEnable(GL_CULL_FACE);
	lit(ctx, this, pairS2);
	glDisable(GL_CULL_FACE);
	truck_road(ctx, this, t, uu);
	truck_cross(ctx, this, t, uu);
	truck_rails(ctx, this, t, uu);
	truck_motorcycle(ctx, this, t, &uu);

	red = (uu - 12.6) - 10.0;
	if (!(59.0 <= t)) {
		truck_whole(ctx, this, t, uu, red);
	} else {
		truck_empty(ctx, this, t, uu, red);
	}
	if (59.0 <= t) {
		truck_destruction(ctx, this, t);
	}

	d = Max0(t - 53.0);
	truck_train_boat(ctx, this, t, d);
	truck_lights(ctx, this, t, uu, red);
	hex2_grid(ctx, this, t, n);
	hex3_ring(ctx, this, t);
	truck_sign(ctx, this, t);
	tholos_row(ctx, this, t);
	tholos_pair(ctx, this, t);
	truck_sparks(ctx, this, t, uu);
	truck_headlights(ctx, this, t);
	glow_trail(ctx, this, t, d);

	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(ctx->fovOffset + 65.0, (double)ctx->width / ctx->height, ctx->nearZ, ctx->farZ);
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	glEnable(GL_DEPTH_TEST);
	truck_dust(this, t);
	gl1::PopMatrix();
}
