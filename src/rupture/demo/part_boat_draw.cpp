#include "part_boat.h"

#include <math.h>

#include "../core/gl1.h"
#include "part_truck.h"
#include "scene.h"

extern double g_unk_b1e698;   /* 0x00b1e698, part_video.cpp */

static const double k_pi = 3.141592653589793;   /* 0x004f36e8 */

/* OURS. x < 0 -> 0, the inline test the code repeats. */
static double Max0(double x)
{
	return x < 0.0 ? 0.0 : x;
}

/* ORIGIN 0x004023b0 Vec4_RotateEuler: *v = Mat_FromEuler(a, b, c) applied to *v, each angle via float. */
static void vec4_rotate_euler(Vec4 *v, const double *a, const double *b, const double *c)
{
	float m[16];

	Mat_FromEuler(m, (float)*a, (float)*b, (float)*c);
	Mat_Apply(m, v, v);
}

/*
 * ORIGIN 0x00469930 FUN_00469930, maybe_Track_DrawRails2: an
 * instruction-for-instruction copy of maybe_Track_DrawRails 0x0049f9b0,
 * only its branch targets differ.
 */
static void maybe_Track_DrawRails2(Rupture_Ctx *ctx, Track *T, double depth)
{
	maybe_Track_DrawRails(ctx, T, depth);
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

/* OURS. Identity on TEXTURE and MODELVIEW, then the eye camera, as each billboard set opens. */
static void eye_open(Rupture_Ctx *ctx)
{
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	Gfx_ResetState();
}

/*
 * OURS. part_boat's lit bracket, not Scene_Lit: no push round
 * maybe_Scene_DrawLit, the ctx+0x1918.. stores, and in the second half
 * "planecut" 20 before the binds. The callers make the
 * Shader_DisableProfiles where the code has one. The demo passes
 * *(double*)(ctx+0x19a0) as the time; maybe_Scene_DrawLit ignores it.
 */
static void boat_lit(Rupture_Ctx *ctx, part_boat *p, int pair, bool planecut)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	double      pc = 20.0;                             /* 0x004f3798 */

	ctx->litFlag = 0.0f;
	ctx->v1920.kind = 1;
	ctx->d1918 = 0.0;
	ctx->v1920.x = 0.2f;                               /* 0x004f3754 */
	ctx->v1920.y = -0.3f;                              /* 0x004f4878 */
	ctx->v1930.kind = 1;
	ctx->v1920.z = 1.0f;
	ctx->v1930.x = 1.0f;
	ctx->v1930.y = 1.0f;
	ctx->v1930.z = 1.0f;
	maybe_Scene_DrawLit(ctx, p->model, p->texBase + 0xc, p->texNormal, pair, &ctx->eye, &ctx->light, 0.0);
	ctx->v1930.kind = 1;
	ctx->litFlag = 1.0f;
	ctx->v1930.x = 1.0f;
	ctx->v1930.y = 1.0f;
	ctx->v1930.z = 1.0f;
	if (planecut) {
		Shader_SetParam1f(s, pair + 1, "planecut", &pc);
	}
	Shader_Bind(s, pair);
	Shader_Bind(s, pair + 1);
	Shader_EnableProfiles(s);
}

/*
 * ORIGIN 0x0046b690..0x0046bb58, the opening: the clear, the projection
 * with the FOV pull after t = 51, and the hex pair's planecut,
 * Reduction, sides, ttime and blue.
 */
static void boat_open(Rupture_Ctx *ctx, part_boat *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   P = p->pairAnisoHex;
	double      c, one, two, fov, v, r, sd, c2, bl;

	g_unk_b1e698 = 0.0;
	glViewport(0, 0, ctx->width, ctx->height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl1::MatrixMode(gl1::PROJECTION);
	c = Clamp01((t - 51.0) * 0.5);
	one = 1.0;
	fov = Math_Ease(&c, &one) * -24.0;
	/* this+0xca8: last frame's seq->partClock, from maybe_Seq_Copy. */
	if (293.66 <= p->seqPartClock && p->seqPartClock < 293.96) {
		fov = fov - 15.0;
	}
	if (293.96 <= p->seqPartClock && p->seqPartClock < 294.45) {
		fov = fov - 30.0;
	}
	gl1::LoadIdentity();
	gl1::Perspective(fov + 65.0 + ctx->fovOffset, (double)ctx->width / ctx->height, ctx->nearZ,
	                 ctx->farZ);
	v = 11.2;                                          /* 0x004f4930 */
	Shader_SetParam1f(s, P + 1, "planecut", &v);

	c = Clamp01((t - 40.3) * 0.3);
	one = 1.0;
	r = Math_Ease(&c, &one);
	if (t >= 46.0) {
		r = 0.0;
	}
	if (t >= 49.0) {
		r = -0.2;
	}
	Shader_GetParam(s, P, 4, "Reduction");
	Shader_SetParam3f(s, P, 4, (float)r, 0.0f, 0.0f);

	c = Clamp01(((t - 24.0) - -12.5) * 0.05);
	two = 2.0;
	sd = 2000.0 - Math_Ease(&c, &two) * 1540.0;
	if (t >= 33.0) {
		sd = 374.0;
	}
	if (t >= 44.0) {
		sd = sd + 17.0;
	}
	if (t >= 46.0) {
		sd = sd + 130.0;
	}
	c2 = Clamp01(t - 51.4);
	if (t >= 49.0) {
		sd = sd - ((130.0 - p->seqHead[2] * 50.0) - (50.0 * c2) * 10.0);
	}
	Shader_SetParam1f(s, P + 1, "sides", &sd);
	Shader_SetParam1f(s, P + 1, "ttime", &p->t);
	bl = Clamp01(((t - 24.0) + 1.2) * 0.2) * 10.0 + 1.0;
	Shader_SetParam1f(s, P + 1, "blue", &bl);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
}

/*
 * ORIGIN 0x0046bcd6..0x0046c517, the hex grid: hex.obj tiles in rows i
 * and columns j, the window sliding with u = 10 t, each bobbing on
 * sines and lifted by a wave travelling along z.
 */
static void boat_hex_grid(Rupture_Ctx *ctx, part_boat *p, double t)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const double u = p->t10;
	double       fi, fi01, fpar, one, fj, c, h, zj, wb, w7, dA, dB, m, c1, c2, ai, inv, bump, x;
	double       g, cg, ce, v, ci, cv, s3, Y;
	float        fif, fi2, z0, xi, fjf, fj2, rr, r, ys, zs, yy, xx, zz, dist;
	int          A, off, iS, iE, jS0, jE0, jS, jE, i, j, k8, par;
	Vec4         P, D;

	A = (int)(u * 0.5);
	iS = -22;
	iE = 20;
	jS0 = -20;
	jE0 = 15;
	if (t >= 28.0) {
		iS = -11;
		iE = 13;
		jE0 = 45;
		jS0 = -25;
	}
	off = (int)(180.0 >= (double)A ? (double)A : 180.0);
	if (t >= 33.0) {
		jS0 += 10;
		jE0 += 10;
	}
	if (t >= 49.0) {
		iS = -20;
		iE = 20;
		jS0 = -30;
		jE0 = 30;
		off = (int)((double)off + 47.0);
	}
	for (i = iS; i < iE; i++) {
		jS = jS0 + off;
		jE = jE0 + off;
		fi = (double)i;
		fi01 = 0.1 * fi;
		fif = (float)i;
		fi2 = fif * fif;
		z0 = 0.0f * 0.0f;
		xi = (float)(fi * 1.7);
		/* MSVC's remainder: -1 for a negative odd i. */
		par = i < 0 ? -(-i % 2) : i % 2;
		fpar = (double)par;
		k8 = (i + jS) * 8;
		for (j = jS; j < jE; j++, k8 += 8) {
			gl1::PushMatrix();
			gl1::MatrixMode(gl1::TEXTURE);
			gl1::LoadIdentity();
			one = 1.0;
			fj = (double)j;
			c = Clamp01(1.0 - ((sin(fj * 0.4 + t) * 0.3 + fi01) + t * 0.4));
			h = Math_Ease(&c, &one) * 3.5;
			fjf = (float)j;
			fj2 = fjf * fjf;
			rr = (float)(((double)fj2 + fi2) + z0);
			r = (float)sqrt(rr);
			ys = (float)(sin(((double)r + r) + t) * 0.3);
			zj = fj * 1.9;
			zs = (float)((zj + 0.0) + fpar);
			gl1::Translatef(xi, ys, zs);
			P = Vec4_Make(xi, ys, zs);
			D = Vec4_Make(0.0f, 0.0f, 0.0f);
			Vec4_Sub(&P, &ctx->eye.eye, &D);
			yy = D.y * D.y;
			xx = D.x * D.x;
			zz = D.z * D.z;
			dist = (float)sqrt((float)(((double)yy + xx) + zz));
			/*
			 * LANDMINE: a tile nearer the eye than 1.68 leaves its push
			 * unpopped and the mode on TEXTURE, as the demo does.
			 */
			if (dist < 1.68) {
				continue;
			}
			wb = 1.0 - fabs(sin(t * 1.5 + 3.0));
			w7 = (wb * (wb * wb)) * ((wb * wb) * (wb * wb));
			gl1::Rotatef((float)(sin(sin((double)k8 * 0.2 + t)) * 20.0), 1.0f, -0.2f, 0.0f);
			dA = u - (6.6000000000000005 + zj);        /* 0x004f4840, one ulp above 6.6 */
			dB = u - (zj + 4.8);
			m = 1.0 - pow(dA, 0.5) * 14.0 * 0.01;
			if (dA < 0.0) {
				m = 1.0;
			}
			c1 = Clamp01(dA * 0.25);
			c2 = Clamp01(dB * 0.15);
			ai = fabs(fi - 0.0);
			inv = 1.0 / (m * ai + 0.1);
			if ((unsigned)(i + 1) <= 2) {
				x = sin(c2 * k_pi) * w7;
				bump = (x + x) * 6.0 * 0.2;
			} else {
				bump = 0.0;
			}
			g = inv * inv + dA * 0.5 * 0.01;
			cg = Clamp01(g);
			ce = Clamp01(cg * (bump + c1));
			v = (((((p->seqHead[2] + 6.0) - 3.4) - 2.0) - 1.2) - zj) + u;
			ci = Clamp01(1.0 / (ai * 0.6 + 0.1));
			cv = Clamp01(v);
			s3 = sin(fi * 0.3 + t * 1.5) * 0.6;
			Y = ((sin(v * k_pi * 0.05) * ci) * s3) * (1.0 - cv) + ce * 2.0;
			if (!(2.0 >= Y)) {
				Y = 2.0;
			}
			gl1::Translatef(0.0f, (float)(Y + h), 0.0f);
			obj_end(s, p->pairAnisoHex);
			gl1::Color3f(0.7f, 0.7f, 0.6f);
			Obj_Draw(&p->hex);
			gl1::PopMatrix();
		}
	}
}

/* ORIGIN 0x0046c672..0x0046c986, the haze: a blue additive blob41b quad fading in from t = 42. */
static void boat_haze(Rupture_Ctx *ctx, part_boat *p, double t)
{
	Gfx_Ctx *g = &ctx->gfx;
	double   a, ang, sx, sy;
	float    af;

	a = Clamp01((t - 42.0) * 0.3);
	if (!(a > 0.0001)) {
		return;
	}
	Crt_Srand(0xa567);
	Shader_DisableProfiles(&g->shader);
	gl1::PushMatrix();
	eye_open(ctx);
	gl1::PushMatrix();
	glDisable(GL_CULL_FACE);
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(g, p->texBase + 0x14);
	glDepthMask(GL_FALSE);
	af = (float)a;
	gl1::Color4f(0.02f, 0.08f, 0.2f, af);
	gl1::Color4f(0.07f, 0.1f, 0.19f, af);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	if (t >= 49.0) {
		gl1::Translatef(0.0f, 100.0f, 0.0f);
	}
	if (t >= 49.0) {
		gl1::Color4f(0.1f, 0.2f, 0.4f, (float)((1.0 - Clamp01(t - 52.2)) * 0.6));
	}
	gl1::Translatef(0.0f, (float)(Max0(t - 49.0) * -1.2 * 30.0), 430.0f);
	ang = 0.0;
	sx = 83.5;
	sy = 83.5;
	Gfx_DrawQuad(&ang, &sx, &sy, 1);
	gl1::PopMatrix();
	gl1::PopMatrix();
	glDepthMask(GL_TRUE);
	Shader_EnableProfiles(&g->shader);
	glDisable(GL_BLEND);
}

/* OURS. The sun and glow anchor: eye + (20, 30, -150), moved after t = 33. */
static Vec4 sky_anchor(Rupture_Ctx *ctx, double t)
{
	Vec4 B = Vec4_Make(20.0f, 30.0f, -150.0f), Q = Vec4_Make(0.0f, 0.0f, 0.0f);

	Vec4_Add(&ctx->eye.eye, &B, &Q);
	glDepthMask(GL_FALSE);
	if (t >= 33.0) {
		Q.z = (float)(Q.z + 230.0);
		Q.x = (float)(Q.x - 70.0);
		Q.y = (float)(Q.y - 2.4);
	}
	return Q;
}

/* ORIGIN 0x0046c98c..0x0046cbf5, the sun: blob41b, hidden for 24 <= t < 33, 14 after and 21 before. */
static void boat_sun(Rupture_Ctx *ctx, part_boat *p, double t)
{
	Gfx_Ctx *g = &ctx->gfx;
	double   size, zero;
	bool     show;
	Vec4     Q;

	Shader_DisableProfiles(&g->shader);
	gl1::PushMatrix();
	eye_open(ctx);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	gl1::Color3f(0.4f, 0.3f, 0.3f);
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(g, p->texBase + 0x14);
	show = t < 24.0 || t >= 33.0;
	Q = sky_anchor(ctx, t);
	gl1::Translatef(Q.x, Q.y, Q.z);
	if (show) {
		size = t >= 33.0 ? 14.0 : 21.0;
		zero = 0.0;
		Gfx_DrawQuad(&zero, &size, &size, 1);
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	gl1::PopMatrix();
}

/* ORIGIN 0x0046cbf6..0x0046cfa7, two cloudt2 glows at the sun's anchor, wobbling in x. */
static void boat_glows(Rupture_Ctx *ctx, part_boat *p, double t)
{
	Gfx_Ctx *g = &ctx->gfx;
	double   gl, dk, ang, size;
	bool     show;
	Vec4     G;
	int      k, kk;

	for (k = 0, kk = 0; kk < 60; k++, kk += 30) {
		Shader_DisableProfiles(&g->shader);
		gl1::PushMatrix();
		eye_open(ctx);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		gl = t < 33.0 ? 1.0 : Clamp01((t - 35.0) * 0.4);
		gl1::Color3f((float)(gl * 0.03), (float)(0.05 * gl), (float)(0.08 * gl));
		gl1::Enable(GL_TEXTURE_2D);
		Tex_Bind(g, p->texBase + 0x1e);
		show = t < 24.0 || t >= 33.0;
		G = sky_anchor(ctx, t);
		gl1::Translatef(G.x, G.y, G.z);
		dk = (double)k;
		gl1::Translatef((float)(sin(dk * 4.0 + t * 0.8) * 11.0 + (double)kk), 0.0f, 0.0f);
		if (t > 33.0) {
			gl1::Translatef(-64.0f, 0.0f, 0.0f);
		}
		ang = dk * 40.0 - 48.0;
		if (show) {
			size = t >= 33.0 ? 60.0 : 90.0;
			Gfx_DrawQuad(&ang, &size, &size, 1);
		}
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		gl1::PopMatrix();
	}
}

/* ORIGIN 0x0046cfa8..0x0046d3b3, from t = 44 500 additive 3-pixel points rising past the camera. */
static void boat_rising(Rupture_Ctx *ctx, double t)
{
	const float k12 = 1.2f, k3 = 3.0f;
	double      a0, a1, a2, f, fr, c;
	Vec4        V, O, W;
	int         n;

	if (t >= 44.0) {
		Shader_DisableProfiles(&ctx->gfx.shader);
		Gfx_ResetState();
		glDisable(GL_BLEND);
		gl1::Disable(GL_TEXTURE_2D);
		gl1::PushMatrix();
		eye_open(ctx);
		gl1::PointSize(3.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		gl1::Begin(gl1::POINTS);
		for (n = 500; n != 0; n--) {
			Math_Random();
			V = Vec4_RandomDir(false);
			a0 = 0.0;
			a1 = Math_Random() * t * 0.2;
			a2 = 0.0;
			vec4_rotate_euler(&V, &a0, &a1, &a2);
			f = (Math_Random() * 0.8 + 1.0) * (t * 0.3) + 100.0;
			fr = f - (double)(int)f;
			c = Clamp01(sin(fr * k_pi) * 0.9);
			gl1::Color3f((float)(c * 0.1), (float)(0.14 * c), (float)(0.2 * c));
			Vec4_Scale(&V, &k12, &V);
			V.y = (float)((fr * 10.0 + 2.0) + V.y);
			O = Vec4_Make(0.0f, 0.0f, 0.0f);
			Vec4_Scale(&V, &k3, &O);
			V = O;
			V.z = (float)(V.z + 430.0);
			V.y = (float)(V.y - (t - 43.5) * 8.0);
			if (t >= 49.0) {
				W = Vec4_Make(0.0f, (float)(84.0 - (t - 49.0) * 17.0), 0.0f);
				Vec4_Add(&V, &W, &V);
			}
			gl1::Vertex3f(V.x, V.y, V.z);
		}
		gl1::End();
		glDisable(GL_BLEND);
		gl1::PopMatrix();
	}
	gl1::PointSize(1.0f);
}

/*
 * ORIGIN 0x0046d3b9..0x0046da1b, boat.obj: bobbing and pitching up
 * after t = 42.8, from t = 44 sinking and turning over, with a roll
 * decaying from t = 0.7.
 */
static void boat_ship(Rupture_Ctx *ctx, part_boat *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	double      x, one, e1, a, y, sr, m, k, q, w;

	Shader_DisableProfiles(s);
	boat_lit(ctx, p, p->pairBoat, true);
	x = Clamp01((t - 0.5) * 0.5);
	one = 1.0;
	e1 = Math_Ease(&x, &one);
	obj_begin();
	if (t >= 49.0) {
		gl1::Translatef(0.0f, (float)(104.0 - (t - 49.0) * 17.0), 0.0f);
	}
	if (t < 44.0) {
		a = fabs(sin(t * 1.5)) * 1.3;
		gl1::Translatef(0.0f, (float)(a * 0.9 * e1 + 1.0), (float)(p->t10 - 10.0));
		gl1::Rotatef((float)(a * -7.0 * e1), 1.0f, 0.0f, 0.0f);
		y = Max0(t - 42.8);
		sr = Math_SoftRamp(&y);
		gl1::Rotatef((float)(sr * -56.0), 1.0f, 0.0f, 0.0f);
		gl1::Translatef(0.0f, (float)(sr * -5.0), 0.0f);
		gl1::Rotatef((float)(sr * -5.0 * 5.0 * -5.4), 0.1f, 0.0f, -0.3f);
	}
	if (t >= 44.0) {
		gl1::Translatef(0.0f, (float)((t - 43.5) * -8.0), 430.0f);
		gl1::Rotatef((float)(50.0 - ((t - 44.0) * 20.0 + 90.0)), 1.0f, 0.0f, 0.0f);
		if (t < 46.0) {
			gl1::Rotatef((float)((0.0 - (t - 44.0) * 20.0 * 3.0) - 120.0), 0.1f, 0.0f, -0.3f);
		}
		if (t >= 46.0) {
			gl1::Rotatef((float)((0.0 - (t - 44.0) * 20.0 * 1.7) - 120.0), 0.1f, 0.0f, -0.3f);
			gl1::Rotatef(-90.0f, -0.1f, 0.0f, -0.3f);
			gl1::Rotatef((float)(t * 70.0), 0.0f, 0.0f, 1.0f);
		}
	}
	m = Max0(t - 0.7);
	k = 0.0;
	if (m > 1e-5) {
		k = Clamp01(1.0 - m * 0.25);
	}
	q = m * 0.7000000000000001;                        /* 0x004f4700, not the 0.7 at 0x004f44a0 */
	w = sin((q + q) * k_pi) * 15.0 * k;
	gl1::Rotatef((float)w, 0.0f, 0.0f, 1.0f);
	gl1::Rotatef((float)(w * -0.6), 1.0f, 0.0f, 0.0f);
	gl1::Color3f(1.0f, 1.0f, 0.8f);
	obj_end(s, p->pairBoat);
	Obj_Draw(&p->boat);
	gl1::PopMatrix();
}

/* OURS. The ease section 2 computes before each piece and never reads. */
static void ease_unread(double t)
{
	double x = Clamp01((t - 0.5) * 0.5), one = 1.0;

	Math_Ease(&x, &one);
}

/*
 * OURS. One scattered cylinders.obj piece of section 2: seven draws;
 * the second batch is moved by (8 + 4, 6.5, 18) and spins about z
 * instead of being scaled.
 */
static void wreck_piece(Shader_Ctx *s, part_boat *p, double t, int kk, bool second)
{
	double r0, m, z0, r1, r2, r3, r4, r5, r6;
	float  px, py, pz, az;

	ease_unread(t);
	r0 = Math_Random() * 4.0;
	obj_begin();
	m = Max0(t - 25.0);
	z0 = 0.0 * p->seqHead[1];
	gl1::Translatef(-4.0f, (float)((((z0 + (((10.0 - m * 2.0) - 36.0) + 28.0)) + r0) + m * 0.2) - 2.0),
	                (float)(z0 + 290.0));
	r1 = Math_Random() * 9.0 - 4.0 - 0.0;
	r2 = Math_Random();
	if (second) {
		pz = (float)((((r2 * 24.0 - 6.0) + 10.0) - 12.0) + 18.0);
	} else {
		pz = (float)(((r2 * 24.0 - 6.0) + 10.0) - 12.0);
	}
	r3 = Math_Random();
	py = second ? (float)((r1 - r3 * 4.0) + 6.5) : (float)(r1 - r3 * 4.0);
	r4 = Math_Random();
	px = second ? (float)(((r4 * 8.0 - 4.0) + 8.0) + 4.0) : (float)(r4 * 8.0 - 4.0);
	gl1::Translatef(px, py, pz);
	gl1::Translatef(0.0f, 0.0f, 0.0f);
	gl1::Color3f(1.0f, 1.0f, 0.8f);
	gl1::Color3f(1.0f, 0.5f, 0.3f);
	gl1::Translatef(0.0f, 0.0f, -1.6f);
	r5 = Math_Random();
	az = (float)(r5 * 2.0 - 1.0);
	r6 = Math_Random();
	gl1::Rotatef((float)((((r6 * 3.0 + 0.5) * (m * 10.0)) * 0.2) * 5.0 + 40.0), 0.3f, 1.0f, az);
	gl1::Translatef(0.0f, 0.0f, 1.6f);
	if (second) {
		gl1::Rotatef((float)(t * 15.8 * 10.0), 0.0f, 0.0f, 1.0f);
	} else {
		gl1::Scalef(1.09f, 1.09f, 1.09f);
	}
	obj_end(s, p->pairBoat2);
	gl1::CallList(kk + (334 - kk / 3) * 3);
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x0046da21..0x0046e535, section 2, for 28 <= t < 33: three
 * groups of trainlong.obj stacked, then 56 and 20 pieces from lists
 * 1002..1004 (cylinders.obj groups 2..4).
 */
static void boat_wreck(Rupture_Ctx *ctx, part_boat *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	double      m, z0, r;
	int         i, kk;

	if (!(t >= 28.0 && t < 33.0)) {
		return;
	}
	Shader_DisableProfiles(s);
	boat_lit(ctx, p, p->pairBoat2, true);
	glEnable(GL_CULL_FACE);
	Crt_Srand(0xe0ae70f8);
	for (i = 0; i < 3; i++) {
		ease_unread(t);
		kk = 0;
		if (i == 0) {
			kk = 2;
		} else if (i == 1) {
			kk = 0;
		} else if (i == 2) {
			kk = 1;
		}
		obj_begin();
		m = Max0(t - 25.0);
		z0 = 0.0 * p->seqHead[1];
		gl1::Translatef((float)(5 * kk - 4),
		                (float)(((z0 + (((10.0 - (m + m)) - 36.0) + 28.0)) + (double)(5 * kk)) + m * 0.2),
		                (float)((double)(35 * kk + 290) + z0));
		gl1::Translatef(0.0f, 0.0f, 0.0f);
		gl1::Color3f(1.0f, 1.0f, 0.8f);
		gl1::Color3f(1.0f, 0.5f, 0.3f);
		gl1::Translatef(0.0f, 0.0f, (float)(-(double)(20 * kk)));
		r = Math_Random();
		gl1::Rotatef((float)(10.0 * (((m * 10.0) * (r + 0.5)) * 0.2) + 40.0), 0.3f, 1.0f, 0.0f);
		if (kk == 0) {
			gl1::Rotatef(-20.0f, 0.0f, 0.0f, 1.0f);
		}
		gl1::Translatef(0.0f, 0.0f, (float)(double)(20 * kk));
		obj_end(s, p->pairBoat2);
		Obj_DrawGroup(&p->trainlong.groups[i]);
		gl1::PopMatrix();
	}
	for (kk = 0; kk < 56; kk++) {
		wreck_piece(s, p, t, kk, false);
	}
	Crt_Srand(0x287547f);
	for (kk = 0; kk < 20; kk++) {
		wreck_piece(s, p, t, kk, true);
	}
}

/* ORIGIN 0x0046e535..0x0046e875, section 3: for t < 3.5 trainlong.obj whole, dropping and jittering. */
static void boat_train(Rupture_Ctx *ctx, part_boat *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	double      x, v, a, b;
	float       y;

	Shader_DisableProfiles(s);
	boat_lit(ctx, p, p->pairTrain2, true);
	if (t < 3.5) {
		obj_begin();
		if (t > 0.0) {
			x = (t - 0.0) * 0.3;
			v = -2.0 - (x * x) * 5.0;
			if (v > 0.0) {
				v = 0.0;
			}
			y = (float)v;
		} else {
			y = -2.0f;
		}
		gl1::Translatef(0.0f, y, (float)((p->t10 + 2.0) - -23.5));
		a = Max0(sin(t * 31.0)) * 0.034;
		b = Max0(sin(t * 25.0));
		gl1::Translatef((float)(b * 0.013600000000000001), (float)a, 0.0f);
		gl1::Color3f(1.0f, 1.0f, 0.4f);
		gl1::Color3f(0.15f, 0.2f, 0.25f);
		obj_end(s, p->pairTrain2);
		Obj_Draw(&p->trainlong);
		gl1::PopMatrix();
	}
}

/*
 * ORIGIN 0x0046e87b..0x0046eb58, section 4: for t < 4.2 the rails on
 * the 600-point track under the bridge pair, pushed back 14 t. Its
 * bracket has no Shader_DisableProfiles.
 */
static void boat_track(Rupture_Ctx *ctx, part_boat *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	double      x, v;

	boat_lit(ctx, p, p->pairBridge23, true);
	if (t < 4.2) {
		obj_begin();
		gl1::Translatef(0.0f, 0.0f, (float)(t * -1.4 * 10.0));
		if (t >= 0.0) {
			x = (t - 0.0) * 0.3;
			v = -2.0 - (x * x) * 5.0;
			if (v > 0.0) {
				v = 0.0;
			}
			gl1::Translatef(0.0f, (float)(v + 2.0), 0.0f);
		} else {
			gl1::Translatef(0.0f, 0.0f, 0.0f);
		}
		gl1::Color4f(0.6f, 0.8f, 1.0f, 1.0f);
		gl1::Translatef(0.0f, -2.8f, -450.0f);
		obj_end(s, p->pairBridge23);
		Scene_DrawRailsCached(ctx, &p->track, 0.7, maybe_Track_DrawRails2);
		gl1::PopMatrix();
	}
}

/* OURS. The two flashing blob41b quads over the boat, blue then red, in antiphase. */
static void flare_pair(Gfx_Ctx *g, part_boat *p, double t, double c, float b1, float b2, float r1)
{
	double a0 = 0.0, w = 1.5, h = 1.5;

	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(g, p->texBase + 0x14);
	gl1::Color4f(b1, b1, b2, (float)(sin(t * 8.0) * c));
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gl1::Translatef(0.0f, (float)((fabs(sin(t * 1.5)) * 1.3 * 0.9 + 1.0) + 1.6), (float)(p->t10 - 10.2));
	Gfx_DrawQuad(&a0, &w, &h, 1);
	gl1::Color4f(r1, b1, 0.0f, (float)(sin(t * 8.0 + k_pi) * c));
	Gfx_DrawQuad(&a0, &w, &h, 1);
}

/*
 * ORIGIN 0x0046eb5e..0x0046ee0e, flares 1, fading in from t = 3.
 * Returns c, which flares 2 reads.
 */
static double boat_flares1(Rupture_Ctx *ctx, part_boat *p, double t)
{
	Gfx_Ctx *g = &ctx->gfx;
	double   c;
	float    c01;

	Crt_Srand(0xa567);
	Shader_DisableProfiles(&g->shader);
	/* The demo never pops this modelview push. */
	gl1::PushMatrix();
	eye_open(ctx);
	gl1::PushMatrix();
	c = Clamp01(t - 3.0);
	c01 = (float)(c * 0.1);
	flare_pair(g, p, t, c, c01, (float)(c * 0.8), (float)(c * 1.1));
	gl1::PopMatrix();
	glDisable(GL_BLEND);
	gl1::Disable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	return c;
}

/*
 * ORIGIN 0x0046ee14..0x0046f036, before t = 49 2500 stars at radius
 * 400 under a fixed 65 / 1 / 1000 projection, above the horizon from
 * t = 22 and climbing from t = 44. Depth testing stays on.
 */
static void boat_stars(Rupture_Ctx *ctx, double t)
{
	const float k400 = 400.0f;
	float       gr;
	Vec4        v, o;
	int         n;

	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(65.0, (double)ctx->width / ctx->height, 1.0, 1000.0);
	gl1::Begin(gl1::POINTS);
	if (t < 49.0) {
		for (n = 2500; n != 0; n--) {
			Math_Random();
			Math_Random();
			gr = (float)((Math_Random() + 0.2) * 0.4);
			gl1::Color3f(gr, gr, gr);
			v = Vec4_RandomDir(false);
			Vec4_Normalize(&v);
			o = Vec4_Make(0.0f, 0.0f, 0.0f);
			Vec4_Scale(&v, &k400, &o);
			v = o;
			if (t >= 22.0) {
				v.z = (float)(v.z + 200.0);
			}
			if (t >= 22.0) {
				v.y = (float)fabs(v.y);
			}
			if (t >= 44.0) {
				v.y = (float)(((((t - 44.0) * 0.6) * 50.0 + 176.0) + 80.0) + v.y);
			}
			gl1::Vertex3f(v.x, v.y, v.z);
		}
	}
	gl1::End();
}

/*
 * ORIGIN 0x0046f03c..0x0046f336, flares 2: flares 1 with constant
 * colours, then a push and pop that draws nothing. The first push is
 * made in GL_PROJECTION mode; boat_dust's last pop takes it.
 */
static void boat_flares2(Rupture_Ctx *ctx, part_boat *p, double t, double c)
{
	Gfx_Ctx *g = &ctx->gfx;

	Crt_Srand(0xa567);
	Shader_DisableProfiles(&g->shader);
	gl1::PushMatrix();
	eye_open(ctx);
	gl1::PushMatrix();
	flare_pair(g, p, t, c, 0.1f, 0.8f, 1.1f);
	gl1::PopMatrix();
	gl1::PushMatrix();
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(g, p->texBase + 0x14);
	gl1::Color4f(0.1f, 0.2f, 0.3f, 1.0f);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gl1::Translatef(0.0f, (float)(p->seqHead[2] * 50.0), 430.0f);
	gl1::PopMatrix();
	glDisable(GL_BLEND);
	gl1::Disable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

/*
 * ORIGIN 0x0046f338..0x0046f601, the dust: for 24 < t < 33 up to 2500
 * grey points round the eye's z, drifting and fading on a sawtooth.
 * The closing pop runs in GL_PROJECTION mode.
 */
static void boat_dust(Rupture_Ctx *ctx, double t)
{
	double c2, N, a, r7, f, fr, sn, b;
	float  px, py, pz, gr;
	int    i;

	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(ctx->fovOffset + 65.0, (double)ctx->width / ctx->height, ctx->nearZ, ctx->farZ);
	gl1::PointSize(2.0f);
	c2 = Clamp01((t - 24.0) + (t - 24.0));
	if (t >= 33.0) {
		c2 = 0.0;
	}
	gl1::Begin(gl1::POINTS);
	N = c2 * 2500.0;
	if (N > 0.0) {
		i = 0;
		do {
			Math_Random();
			Math_Random();
			a = Math_Random() + 0.2;
			px = (float)(Math_Random() * 8.0 - 4.0);
			py = (float)(Math_Random() * 4.0 - 2.0);
			pz = (float)(Math_Random() * 4.0 - 2.0);
			px = (float)(px * 5.0);
			py = (float)(py * 5.0);
			pz = (float)(pz * 5.0);
			py = (float)(py - 10.0);
			pz = (float)(pz + ctx->eye.eye.z);
			r7 = Math_Random() * 1000.0;
			f = r7 - (Math_Random() * 1.5 + 0.5) * (t * 0.1);
			fr = f - (double)(int)f;
			pz = (float)(fr * 70.0 + pz);
			sn = sin(fr * k_pi);
			b = Clamp01(sn + sn);
			gr = (float)(b * a * 0.12);
			gl1::Color3f(gr, gr, gr);
			pz = (float)(pz - 30.0);
			gl1::Vertex3f(px, py, pz);
			i++;
		} while ((double)i < N);
	}
	gl1::End();
	gl1::PointSize(1.0f);
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x0046b690 part_boat::DrawScene: the opening, the hex pair's
 * box and grid, the hex2 pair's box, the haze, the sun and glows, the
 * rising points; then boat.obj, the wreck, the train, the rails, the
 * flares, the stars and the dust. It leaves the modelview stack one
 * deeper and the mode on GL_PROJECTION. pass is not read.
 */
void part_boat::DrawScene(Rupture_Ctx *ctx, int pass)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	double      sd2 = 2000.0, c;

	(void)pass;
	boat_open(ctx, this, t);
	Shader_DisableProfiles(s);
	glDisable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_BLEND);
	gl1::Color3f(1.0f, 0.6f, 0.2f);
	boat_lit(ctx, this, pairAnisoHex, false);
	boat_hex_grid(ctx, this, t);
	boat_lit(ctx, this, pairAnisoHex2, false);
	Shader_SetParam1f(s, pairAnisoHex2 + 1, "sides", &sd2);
	glEnable(GL_CULL_FACE);
	boat_haze(ctx, this, t);
	boat_sun(ctx, this, t);
	boat_glows(ctx, this, t);
	boat_rising(ctx, t);

	boat_ship(ctx, this, t);
	boat_wreck(ctx, this, t);
	boat_train(ctx, this, t);
	boat_track(ctx, this, t);
	c = boat_flares1(ctx, this, t);
	boat_stars(ctx, t);
	boat_flares2(ctx, this, t, c);
	boat_dust(ctx, t);
}
