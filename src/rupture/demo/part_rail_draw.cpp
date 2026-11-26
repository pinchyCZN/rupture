#include "part_rail.h"

#include <math.h>

#include "../core/gl1.h"
#include "part_truck.h"
#include "scene.h"

int g_railHeadFlag;

/*
 * OURS. DrawScene's frame slots that one section leaves for later ones,
 * named by their offset at the body's stack depth. The train's car 0
 * writes rotAcc and carY, every car writes zAcc; the glow trail stores
 * carY * -0.4 back into carY.
 */
struct Rail_Shared {
	double zAcc;      /* [ESP+0xa8] */
	double rotAcc;    /* [ESP+0xd0], degrees */
	double carY;      /* [ESP+0xf0] */
	double d0;        /* [ESP+0x98], max(t - 0, 0) */
	bool   trainOn;   /* byte [ESP+0xa7] */
};

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

/* OURS. One rail top of maybe_Track_DrawRailsAlt: R from w0 to w1, normal U. */
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

/* OURS. One rail wall: R at w, dropped (U * 1.5) * depth, normal R, or -R when negR. */
static void rail_wall(Track *T, float w, double depth, bool negR)
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
		if (negR) {
			Vec4_Scale(&R, &neg, &R);
		}
		gl1::Normal3f(R.x, R.y, R.z);
		gl1::Vertex3f(a.x, a.y, a.z);
		gl1::Vertex3f(b.x, b.y, b.z);
	}
	gl1::End();
}

/*
 * ORIGIN 0x004953c0 FUN_004953c0, maybe_Track_DrawRailsAlt: a copy of
 * maybe_Track_DrawRails (0x0049f9b0) whose walls stand at R * -1.2
 * (normal +R) and R * 1.6 (normal -R), with the empty strips after the
 * tops. The sleeper pass is the same. ctx is not read.
 */
static void maybe_Track_DrawRailsAlt(Rupture_Ctx *ctx, Track *T, double depth)
{
	Vec4  P, D, R, U, A, B, P2, D2, R2, U2, C, E;
	float Ay4, By4, Cy4, Ey4, Ay1, By1, Cy1, Ey1;
	int   c;

	(void)ctx;
	glDisable(GL_CULL_FACE);
	T->loop = 0;
	rail_top(T, -1.4f, -1.2f);
	gl1::Begin(gl1::QUAD_STRIP);
	gl1::End();
	rail_wall(T, -1.2f, depth, false);
	rail_top(T, 1.4f, 1.6f);
	gl1::Begin(gl1::QUAD_STRIP);
	gl1::End();
	rail_wall(T, 1.6f, depth, true);
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
 * OURS. Scene_Lit with part_rail's slots. The demo passes
 * *(double*)(ctx+0x19a0) as the time; maybe_Scene_DrawLit ignores it.
 */
static void lit(Rupture_Ctx *ctx, part_rail *p, int pair)
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

/* OURS. The sun and glow anchor, eye-relative, moved at t = 51. */
static void sky_anchor(Rupture_Ctx *ctx, double t, float *x, float *y, float *z)
{
	if (t < 51.0) {
		*x = (float)(ctx->eye.eye.x - 200.0);
		*y = 0.0f;
		*z = (float)(ctx->eye.eye.z - 60.0);
	} else {
		*x = (float)(ctx->eye.eye.x - 50.0);
		*y = -10.0f;
		*z = (float)(ctx->eye.eye.z + 220.0);
	}
}

/*
 * ORIGIN 0x00497145..0x004976df, the sky: part_truck's, its stars
 * offset by the eye's x and z, the sun 20 for 30 < t < 35, 30 before
 * t = 51 and 26 after, the glows' reset flag 0 from t = 51.
 */
static void rail_sky(Rupture_Ctx *ctx, part_rail *p, double t)
{
	Gfx_Ctx    *g = &ctx->gfx;
	const float k400 = 400.0f;
	double      zero, sA, sB, size, s90a, s90b, ang, dk;
	float       gr, px, py, pz;
	Vec4        v, o;
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
		v = Vec4_RandomDir(false);
		Vec4_Normalize(&v);
		o = Vec4_Make(0.0f, 0.0f, 0.0f);
		Vec4_Scale(&v, &k400, &o);
		gl1::Vertex3f(o.x + ctx->eye.eye.x, (float)(o.y - 160.0), o.z + ctx->eye.eye.z);
	}
	gl1::End();

	sky_anchor(ctx, t, &px, &py, &pz);
	gl1::PushMatrix();
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	gl1::Translatef(px, py, pz);
	gl1::Color3f(0.4f, 0.3f, 0.3f);
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(g, p->texBase + 0x14);
	if (t > 30.0 && t < 35.0) {
		size = 20.0;
	} else {
		size = t < 51.0 ? 30.0 : 26.0;
	}
	zero = 0.0;
	sA = size;
	sB = size;
	Gfx_DrawQuad(&zero, &sB, &sA, 1);
	glDisable(GL_BLEND);
	gl1::PopMatrix();
	for (k = 0; k < 3; k++) {
		gl1::PushMatrix();
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		sky_anchor(ctx, t, &px, &py, &pz);
		gl1::Translatef(px, py, pz);
		dk = (double)k;
		gl1::Translatef(0.0f, 0.0f, (float)(sin(dk * 4.0 + t * 0.8) * 11.0 + (double)(30 * k)));
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
		s90a = 90.0;
		s90b = 90.0;
		Gfx_DrawQuad(&ang, &s90b, &s90a, t < 51.0 ? 1 : 0);
		glDisable(GL_BLEND);
		gl1::PopMatrix();
	}
}

/* OURS. "alphaengine" = a^4, a easing 5 -> 0 over t = 2 .. 22, 5 after t = 24. */
static void alpha_engine(Shader_Ctx *s, int P, double t)
{
	double x, e, a, a2, ae, one = 1.0;

	x = Clamp01((((t - 18.0) + 7.0) + 9.0) * 0.05);
	e = Math_Ease(&x, &one);
	a = t > 24.0 ? 5.0 : 5.0 - e * 5.0;
	a2 = a * a;
	ae = a2 * a2;
	Shader_SetParam1f(s, P + 1, "alphaengine", &ae);
}

/*
 * OURS. The train's z before the t >= 44 step. K110 at 0x004f63d8 is
 * 110.00000000000001, one ulp above 110.
 */
static double train_z(double t, double d0)
{
	const double K110 = 110.00000000000001;

	if (t >= 42.0) {
		return ((((((450.0 - d0 * 95.0) - 10.0) + K110) + 1000.0) + 1180.0) + 4.0) + 1.6;
	}
	return ((450.0 - d0 * 45.0) - 10.0) + K110;
}

/* OURS. The t >= 58 tumble: max(0, SoftRamp(max(0, q))) with the macro's two calls. */
static double train_tumble(double t, double h2, int j)
{
	double y, q, z1, z2, r;

	y = ((t - 58.0) + h2 * 0.0) + (double)(2 - j) * 0.15;
	y = Max0(y);
	q = 5.0 - (y + y);
	z1 = Max0(q);
	r = Math_SoftRamp(&z1);
	if (r < 0.0) {
		return 0.0;
	}
	z2 = Max0(q);
	return Math_SoftRamp(&z2);
}

/* OURS. The flames' crash term and the "upper" offset of the t >= 58 block. */
static void train_crash(double t, double res, double *lo, double *fl)
{
	double c, x, one = 1.0;

	*lo = 1.9;
	if (res < 0.1) {
		c = Clamp01((t - 60.0) * 3.0);
		x = Clamp01((t - 60.4) * 0.3);
		*fl = (c - Math_Ease(&x, &one)) * 4.4;
		*lo = -0.30000000000000004;
	} else {
		*fl = 0.0;
	}
}

/* OURS. A jolt envelope: Clamp01((t - t0) * 7) less the ease of (t - 60.2) * 0.8. */
static double train_jolt(double t, double t0)
{
	double b, x, one = 1.0;

	b = Clamp01((t - t0) * 7.0);
	x = Clamp01((t - 60.2) * 0.8);
	return b - Math_Ease(&x, &one);
}

/* OURS. The car's lift g, gone from t = 51, and the unread ease before it. */
static double train_lift(double t, double jd)
{
	double x, q, pw, a, M, one = 1.0;

	x = Clamp01((t - 33.0) * 0.25);
	Math_Ease(&x, &one);
	q = Clamp01(((t - 33.0) - jd * 0.2) * 0.28);
	pw = pow(q, 4.2) * 1.6;
	a = sin(t * 38.0);
	M = Max0(sin(t * 5.5) * a);
	if (t >= 51.0) {
		return 0.0;
	}
	return M * (pw * 0.02) + pw;
}

/* OURS. "flames": the ease in after t = 36.7 + 0.7 j, out after t = 51, plus the crash term. */
static void train_flames(Shader_Ctx *s, int P, double t, double jd, double fl)
{
	double x, e1, flames, one = 1.0;

	x = Clamp01(((t - 36.7) - jd * 0.7) * 0.33);
	e1 = Math_Ease(&x, &one);
	x = Clamp01((t - 51.0) * 0.33);
	flames = (e1 - Math_Ease(&x, &one)) + fl;
	Shader_SetParam1f(s, P + 1, "flames", &flames);
}

/*
 * ORIGIN 0x00497809..0x00498652, section 1: trainlong.obj, whole before
 * t = 24 and its three cars (lists 2000..2002) after; each car sways,
 * sinks and tilts from t = 46.3, spins from t = 51, tumbles and jolts
 * from t = 58. Car 0 (j = 2) feeds the shared slots.
 */
static void rail_train(Rupture_Ctx *ctx, part_rail *p, double t, Rail_Shared *F)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const double h2 = p->seqHead[2], pi2 = 6.283185307179586;
	const int    P = p->pairTrain;
	double       v, w, jd, x, sr, x2, s2, lo, fl, res, r, b50, b68, y, sn, f, S, g, upper, one;
	int          i, j;

	lit(ctx, p, P);
	glDisable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	for (i = 0; i < 3; i++) {
		obj_begin();
		alpha_engine(s, P, t);
		v = train_z(t, F->d0);
		gl1::Translatef(1386.2f, -69.0f, (float)v);
		F->zAcc = v;
		if (t >= 44.0) {
			w = (F->d0 * 20.0 + 70.0) - 891.0;
			F->zAcc = v + w;
			gl1::Translatef(0.0f, 0.0f, (float)w);
		}
		j = i == 0 ? 2 : (i == 1 ? 0 : 1);
		jd = (double)j;
		x = Max0((t - 49.0) - jd) * 0.6;
		sr = Math_SoftRamp(&x) * 0.5;
		if (t >= 51.0) {
			sr = 0.0;
		}
		lo = 0.0;
		if (t < 58.0) {
			if (t > 46.3) {
				if (i == 0) {
					F->carY = -20.0 * sr;
				}
				gl1::Translatef(0.0f, (float)(sr * -20.0), 88.0f);
				F->zAcc = F->zAcc + 88.0;
				x2 = Max0((t - 48.0) - jd) * 0.6;
				s2 = Math_SoftRamp(&x2) * 0.5;
				if (t < 51.0) {
					gl1::Rotatef((float)(s2 * 100.0 * 3.6), 0.0f, 0.0f, 1.0f);
					if (i == 0) {
						F->rotAcc = s2 * 100.0 * 3.6 + F->rotAcc;
					}
				}
			}
			if (t >= 51.0) {
				r = (double)(j * 30);
				gl1::Rotatef((float)(t * 80.0 - r), 0.0f, 0.0f, 1.0f);
				if (i == 0) {
					F->rotAcc = (t * 80.0 - r) + F->rotAcc;
				}
				one = 1.0;
				x = Clamp01((t - 51.0) * 0.33);
				lo = Math_Ease(&x, &one) * 2.0;
			}
		}
		fl = 0.0;
		if (t >= 58.0) {
			res = train_tumble(t, h2, j);
			r = res * 100.0;
			gl1::Rotatef((float)r, 0.0f, 0.0f, 1.0f);
			if (i == 0) {
				F->rotAcc = r + F->rotAcc;
			}
			train_crash(t, res, &lo, &fl);
			b50 = train_jolt(t, 59.85);
			b68 = train_jolt(t, 59.75);
			y = Max0(sin(t * 31.0)) * ((b50 * 10.0) * 0.02);
			gl1::Translatef((float)(Max0(sin(t * 25.0)) * ((b50 * 4.0) * 0.02)), (float)y, 0.0f);
			sn = sin(t * 10.0 - 0.2);
			gl1::Rotatef((float)((((b68 * (b68 * -10.0)) * 0.02) * sn) * 6.0 * 10.0), 0.0f, 0.0f, 1.0f);
			if (i == 0) {
				F->carY = y;
			}
		}
		f = (t * 6.0) * 5.0;
		S = (int)f % 6 == 0 ? sin(f * pi2 + jd) : 0.0;
		if (t < 25.0) {
			gl1::Rotatef((float)(S * 0.3), 0.0f, 0.0f, 1.0f);
		}
		gl1::Rotatef(180.0f, 0.0f, 1.0f, 0.0f);
		g = train_lift(t, jd);
		upper = lo - (g * 0.2 - sr * 20.0);
		Shader_SetParam1f(s, P + 1, "upper", &upper);
		train_flames(s, P, t, jd, fl);
		y = 0.4 * g;
		gl1::Translatef((float)(((g * -0.5) * 1.2) * -1.0), (float)y, 0.0f);
		if (i == 0) {
			F->carY = y;
		}
		r = g * 50.0;
		gl1::Rotatef((float)r, 0.0f, 0.0f, 1.0f);
		if (i == 0) {
			F->rotAcc = r + F->rotAcc;
		}
		gl1::Scalef(1.0f, 1.0f, 1.0f);
		gl1::Color3f(0.15f, 0.2f, 0.25f);
		obj_end(s, P);
		if (t < 24.0) {
			/* Landmine: the demo breaks here without the PopMatrix, one modelview push per frame left on. */
			Obj_Draw(&p->trainlong);
			break;
		}
		gl1::CallList(2000 + i);
		gl1::PopMatrix();
	}
}

/*
 * ORIGIN 0x00498652..0x004993d9, section 2: boat.obj on the train's
 * machinery for car j = 2 alone, with its own offsets; it writes carY
 * but neither zAcc nor rotAcc, and has no jitter and no 59.75 roll.
 */
static void rail_boat(Rupture_Ctx *ctx, part_rail *p, double t, Rail_Shared *F)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const double h2 = p->seqHead[2];
	const int    P = p->pairTrain;
	double       w, v, jd, x, sr, x2, s2, lo, fl, res, b50, y, g, up, upper, one;
	int          i, j;

	lit(ctx, p, P);
	glDisable(GL_CULL_FACE);
	glEnable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	for (i = 0; i < 1; i++) {
		obj_begin();
		alpha_engine(s, P, t);
		w = t >= 58.0 ? 0.0 : Max0(t - 56.0) * -0.25;
		gl1::Translatef(-0.6f, 2.3f, (float)(w * 10.0 + 35.0));
		if (t < 62.0 && t > 35.0) {
			gl1::Translatef(2.6f, -2.2f, 0.0f);
		}
		if (t > 62.0) {
			gl1::Translatef(0.6f, 0.0f, 0.0f);
		}
		v = train_z(t, F->d0);
		gl1::Translatef(1386.2f, -69.0f, (float)v);
		if (t >= 44.0) {
			gl1::Translatef(0.0f, 0.0f, (float)((F->d0 * 20.0 + 70.0) - 891.0));
		}
		j = i == 0 ? 2 : (i == 1 ? 0 : 1);
		jd = (double)j;
		x = Max0((t - 49.0) - jd) * 0.6;
		sr = Math_SoftRamp(&x) * 0.5;
		if (t >= 51.0) {
			sr = 0.0;
		}
		lo = 0.0;
		if (t < 58.0) {
			if (t > 46.3) {
				if (i == 0) {
					F->carY = -20.0 * sr;
				}
				gl1::Translatef(0.0f, (float)(sr * -20.0), 88.0f);
				x2 = Max0((t - 48.0) - jd) * 0.6;
				s2 = Math_SoftRamp(&x2) * 0.5;
				if (t < 51.0) {
					gl1::Rotatef((float)(s2 * 100.0 * 3.6), 0.0f, 0.0f, 1.0f);
				}
			}
			if (t >= 51.0) {
				gl1::Rotatef((float)(t * 80.0 - (double)(j * 30)), 0.0f, 0.0f, 1.0f);
				one = 1.0;
				x = Clamp01((t - 51.0) * 0.33);
				lo = Math_Ease(&x, &one) * 2.0;
			}
		}
		fl = 0.0;
		if (t >= 58.0) {
			res = train_tumble(t, h2, j);
			gl1::Rotatef((float)(res * 100.0), 0.0f, 0.0f, 1.0f);
			train_crash(t, res, &lo, &fl);
			b50 = train_jolt(t, 59.85);
			y = Max0(sin(t * 31.0)) * ((b50 * 10.0) * 0.02);
			gl1::Translatef((float)(Max0(sin(t * 25.0)) * ((b50 * 4.0) * 0.02)), (float)y, 0.0f);
			if (i == 0) {
				F->carY = y;
			}
		}
		gl1::Rotatef(180.0f, 0.0f, 1.0f, 0.0f);
		g = train_lift(t, jd);
		up = lo;
		if (t > 54.0) {
			up = lo + 2.2;
		}
		upper = up - (g * 0.2 - sr * 20.0);
		Shader_SetParam1f(s, P + 1, "upper", &upper);
		train_flames(s, P, t, jd, fl);
		gl1::Translatef((float)(((g * -0.5) * 1.2) * -1.0), (float)(0.4 * g), 0.0f);
		if (t < 62.0 && t > 54.0) {
			gl1::Translatef(0.0f, 2.8f, 2.8f);
		}
		gl1::Rotatef((float)(g * 50.0), 0.0f, 0.0f, 1.0f);
		if (t < 62.0 && t > 54.0) {
			gl1::Translatef(0.0f, 0.0f, (float)(h2 * 0.0 - 10.0));
			gl1::Rotatef((float)(116.0 - ((t * 40.0 + t * 40.0) + 16.0)), 0.0f, 0.3f, 1.0f);
		}
		gl1::Scalef(1.0f, 1.0f, 1.0f);
		gl1::Color3f(0.15f, 0.2f, 0.25f);
		obj_end(s, P);
		Obj_Draw(&g_boatObj);
		gl1::PopMatrix();
	}
}

/* ORIGIN 0x004994ab..0x004996d0, the first fireplace track, its walls easing to depth 20 over t = 0 .. 2.5. */
static void rail_fireplace1(Rupture_Ctx *ctx, part_rail *p, double t, double fade, bool flag77)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   P = p->pairBridge2;
	double      one, x, depth, zero;

	if (fade > 0.3 && flag77) {
		lit(ctx, p, P);
		glDisable(GL_CULL_FACE);
		glEnable(GL_CULL_FACE);
		obj_begin();
		one = 1.0;
		Shader_SetParam1f(s, P + 1, "style", &one);
		gl1::Translatef(1346.2f, -19.74f, 110.0f);
		gl1::Rotatef(-5.44f, 1.0f, 0.0f, 0.0f);
		gl1::Color4f(0.6f, 0.8f, 1.0f, 1.0f);
		obj_end(s, P);
		x = Clamp01(t * 0.4);
		one = 1.0;
		depth = Math_Ease(&x, &one) * 20.0;
		zero = 0.0;
		Shader_SetParam1f(s, P + 1, "fireplacealpha", &zero);
		Scene_DrawRailsCached(ctx, &p->track, depth, maybe_Track_DrawRailsAlt);
		gl1::PopMatrix();
	}
	Crt_Srand(0x108a);
}

/*
 * ORIGIN 0x004996d5..0x00499a77, the second fireplace track: pushed
 * down z after t = 38 and 58, "fireplace" the train's zAcc + 26, its
 * alpha in after t = 37.2, out after 62, a tenth for 47 < t < 50.
 */
static void rail_fireplace2(Rupture_Ctx *ctx, part_rail *p, double t, double fade, bool flag77,
                            const Rail_Shared *F)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   P = p->pairBridge2;
	double      v, fa, x, e, one, zero;

	if ((t < 53.0 || t > 59.1) && fade > 0.1 && flag77) {
		lit(ctx, p, P);
		glDisable(GL_CULL_FACE);
		obj_begin();
		v = 1.0;
		Shader_SetParam1f(s, P + 1, "style", &v);
		v = F->zAcc + 26.0;
		Shader_SetParam1f(s, P + 1, "fireplace", &v);
		fa = Clamp01((t - 37.2) * 0.6) - Clamp01((t - 62.0) * 0.7);
		if (t > 47.0 && t < 50.0) {
			fa = fa * 0.1;
		}
		Shader_SetParam1f(s, P + 1, "fireplacealpha", &fa);
		gl1::Translatef(1386.2f, -19.74f, 30.0f);
		if (t >= 38.0) {
			gl1::Translatef(0.0f, 0.0f, -452.0f);
		}
		if (t > 58.0) {
			gl1::Translatef(0.0f, 0.0f, -3010.0f);
		}
		gl1::Color4f(0.6f, 0.8f, 1.0f, 1.0f);
		obj_end(s, P);
		x = Clamp01((t - 37.0) * 0.25);
		one = 1.0;
		e = Math_Ease(&x, &one);
		if (t >= 40.0) {
			e = 1.0;
		}
		Scene_DrawRailsCached(ctx, &p->track, 200.0 - e * 198.0, maybe_Track_DrawRailsAlt);
		zero = 0.0;
		Shader_SetParam1f(s, P + 1, "fireplacealpha", &zero);
		gl1::PopMatrix();
	}
	Crt_Srand(0x108a);
}

/* OURS. The focus-pass tail: the unread ease of (t - 2) / 2. */
static void ease_unread(double t)
{
	double x, one = 1.0;

	x = Clamp01(t - 2.0) * 0.5;
	Math_Ease(&x, &one);
}

/*
 * ORIGIN 0x00499a7c..0x00499fa2, the rail stack: before t = 52.9 up to
 * four rails[] drawn twice, the second turned 188 degrees about y, with
 * the focus blur; from t = 24 each shakes by two draws.
 */
static void rail_stack(Rupture_Ctx *ctx, part_rail *p, double t, double fade, bool flag77)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   P = p->pairBridge2;
	double      n, wd, depth, focus, zero, r2;
	float       r1;
	int         i, k;
	Track      *tr;

	if (!(t < 52.9 && flag77)) {
		return;
	}
	n = fade * 4.0;
	i = 0;
	k = 0;
	tr = &p->rails[0];
	wd = 0.0;
	if (!(n > 0.0)) {
		return;
	}
	do {
		lit(ctx, p, P);
		glDisable(GL_CULL_FACE);
		obj_begin();
		i++;
		gl1::Translatef(1396.2f, (float)(((2.0 - ((double)i * 10.0 + 1.6)) - 20.0) - 0.06),
		                (float)((double)k + 112.00000000000001));
		if (t >= 40.0) {
			gl1::Translatef(0.0f, 0.0f, -340.0f);
		}
		if (t >= 16.0) {
			gl1::Translatef(0.0f, 4.0f, -630.0f);
		}
		if (t >= 24.0) {
			gl1::Translatef(29.2f, (float)(-50.0 - wd * 20.0), -619.5f);
			if (t >= 24.0) {
				r1 = (float)Math_Random();
				r2 = Math_Random();
				gl1::Rotatef((float)((t - 24.0) * r2), 1.0f, 0.2f, r1);
			}
		}
		gl1::Color4f(0.7f, 0.8f, 0.8f, 1.0f);
		obj_end(s, P);
		depth = 10.0;
		if (t >= 24.0) {
			depth = 3.0;
		}
		ease_unread(t);
		focus = 1.0 - Clamp01(((t - 1.0) + 56.0) * 0.015);
		if (t >= 6.0) {
			focus = 1.0 - Clamp01(((t - 1.0) + 58.0) * 0.015);
		}
		zero = 0.0;
		Shader_SetParam1f(s, P + 1, "style", &zero);
		Shader_SetParam1f(s, P + 1, "focus", &focus);
		Scene_DrawRailsCached(ctx, tr, depth, maybe_Track_DrawRailsAlt);
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::Rotatef(188.0f, 0.0f, 1.0f, 0.0f);
		gl1::Color4f(0.7f, 0.8f, 0.8f, 1.0f);
		obj_end(s, P);
		Scene_DrawRailsCached(ctx, tr, depth, maybe_Track_DrawRailsAlt);
		gl1::PopMatrix();
		k += 80;
		tr++;
		wd = (double)i;
	} while (wd < n);
}

/*
 * ORIGIN 0x00499fa8..0x0049a31f, rails[1] and rails[3] after t = 28.5
 * (the index and the pointer step twice a pass): before t = 58 a depth
 * 10/3 draw and a depth 1 draw turned 188 degrees, after it the turned
 * draw alone with style 1.
 */
static void rail_pair(Rupture_Ctx *ctx, part_rail *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   P = p->pairBridge2;
	double      c, depth, zero, one;
	int         j;
	Track      *tr;

	j = 0;
	tr = &p->rails[0];
	for (;;) {
		c = Clamp01(t - 28.5);
		if (c * 4.0 <= (double)j) {
			break;
		}
		j++;
		tr++;
		lit(ctx, p, P);
		glDisable(GL_CULL_FACE);
		obj_begin();
		gl1::Translatef(1396.2f, -21.66f, -961.3f);
		if (t >= 58.0) {
			gl1::Translatef(0.0f, 0.0f, -1712.0f);
		}
		gl1::Color4f(0.7f, 0.8f, 0.8f, 1.0f);
		obj_end(s, P);
		depth = 10.0;
		if (t >= 24.0) {
			depth = 3.0;
		}
		ease_unread(t);
		zero = 0.0;
		Shader_SetParam1f(s, P + 1, "style", &zero);
		zero = 0.0;
		Shader_SetParam1f(s, P + 1, "focus", &zero);
		if (t < 58.0) {
			Scene_DrawRailsCached(ctx, tr, depth, maybe_Track_DrawRailsAlt);
		}
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::Rotatef(188.0f, 0.0f, 1.0f, 0.0f);
		gl1::Color4f(0.7f, 0.8f, 0.8f, 1.0f);
		obj_end(s, P);
		if (t >= 58.0) {
			one = 1.0;
			Shader_SetParam1f(s, P + 1, "style", &one);
		}
		Scene_DrawRailsCached(ctx, tr, 1.0, maybe_Track_DrawRailsAlt);
		gl1::PopMatrix();
		j++;
		tr++;
	}
}

/*
 * ORIGIN 0x0049a324..0x0049a6f8, after t = 60 the bridge rails again,
 * turned 202 degrees. The demo steps two pointers from this+0x14c0 and
 * this+0x1460, one Track below rails[], so the second is rails[i - 1].
 */
static void rail_bridge(Rupture_Ctx *ctx, part_rail *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   P = p->pairBridge2;
	double      c, w, z, one;
	int         i;
	Track      *pA, *pB;

	if (!(60.0 < t)) {
		return;
	}
	i = 0;
	for (;;) {
		c = Clamp01(t - 28.5);
		if (!((double)i < c * 4.0)) {
			break;
		}
		i++;
		pA = &p->rails[i];
		pB = &p->rails[i - 1];
		lit(ctx, p, P);
		glDisable(GL_CULL_FACE);
		obj_begin();
		gl1::Translatef(1396.2f, -21.66f, -961.3f);
		if (t >= 58.0) {
			gl1::Translatef(0.0f, 16.0f, -1792.0f);
		}
		gl1::Scalef(1.0f, 1.6f, 1.0f);
		gl1::Color4f(0.7f, 0.8f, 0.8f, 1.0f);
		obj_end(s, P);
		w = 10.0;
		if (t >= 24.0) {
			w = 3.0;
		}
		ease_unread(t);
		z = 0.0;
		Shader_SetParam1f(s, P + 1, "style", &z);
		z = 0.0;
		Shader_SetParam1f(s, P + 1, "focus", &z);
		if (t < 58.0) {
			Scene_DrawRailsCached(ctx, pA, w, maybe_Track_DrawRailsAlt);
		}
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::Rotatef(202.0f, 0.0f, 1.0f, 0.0f);
		gl1::Color4f(0.7f, 0.8f, 0.8f, 1.0f);
		obj_end(s, P);
		if (t >= 58.0) {
			one = 1.0;
			Shader_SetParam1f(s, P + 1, "style", &one);
			Scene_DrawRailsCached(ctx, pB, 1.0, maybe_Track_DrawRailsAlt);
		} else {
			Scene_DrawRailsCached(ctx, pA, 1.0, maybe_Track_DrawRailsAlt);
		}
		gl1::PopMatrix();
		i++;
	}
}

/*
 * ORIGIN 0x0049a6fa..0x0049aaac, for 24 < t < 28.2 hand.obj twice,
 * falling 7.4 a second; the second placement's ease is computed and
 * multiplied by a literal 0.
 */
static void rail_hands(Rupture_Ctx *ctx, part_rail *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   P = p->pairHand;
	double      u, p4, x;

	if (!(t < 28.2 && 24.0 < t)) {
		return;
	}
	lit(ctx, p, P);
	glDisable(GL_CULL_FACE);
	glEnable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	u = (t - 24.0) * -7.4;
	obj_begin();
	gl1::Translatef(1390.8f, (float)(((((u - 134.0) - 20.6) - 69.0) - 20.0) - 2.8), -965.4f);
	gl1::Rotatef(0.0f, 0.0f, 1.0f, 0.0f);
	gl1::Rotatef(11.2f, 1.0f, 0.0f, 0.0f);
	gl1::Rotatef(10.0f, 0.0f, 0.0f, 1.0f);
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	gl1::Color3f(0.15f, 0.2f, 0.25f);
	obj_end(s, P);
	if (t < 28.0) {
		Obj_Draw(&p->hand);
	}
	gl1::PopMatrix();
	obj_begin();
	gl1::Translatef(1390.8f, (float)((((((u + 27.500000000000004) - 134.0) - 20.6) - 69.0) - 20.0) + 32.0),
	                -909.6f);
	p4 = 4.0;
	x = Clamp01(((t - 23.0) - 1.0) * 0.3);
	Math_Ease(&x, &p4);
	gl1::Translatef((float)(0.0 * -0.2), (float)(0.0 - 58.0), 0.0f);
	gl1::Rotatef(10.0f, 0.0f, 0.0f, 1.0f);
	gl1::Scalef(1.0f, 1.0f, -1.0f);
	gl1::Color3f(0.15f, 0.2f, 0.25f);
	obj_end(s, P);
	if (t < 27.5) {
		Obj_Draw(&p->hand);
	}
	gl1::PopMatrix();
}

/* ORIGIN 0x0049aab2..0x0049aca2, for 40 <= t < 58 human.obj, pushed back at t = 51 and 54. */
static void rail_head(Rupture_Ctx *ctx, part_rail *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   P = p->pairHead;

	if (!(t >= 40.0 && t < 58.0)) {
		return;
	}
	lit(ctx, p, P);
	glDisable(GL_CULL_FACE);
	Crt_Srand(0x409bfa);
	obj_begin();
	gl1::Translatef(1390.8f, -84.412f, -1478.73f);
	if (t >= 51.0) {
		gl1::Translatef(0.0f, 0.0f, -80.0f);
	}
	if (t >= 54.0) {
		gl1::Translatef(0.0f, 0.0f, -100.0f);
	}
	gl1::Rotatef(180.0f, 0.0f, 1.0f, 0.0f);
	gl1::Scalef(1.0f, -1.0f, 1.0f);
	gl1::Color3f(0.15f, 0.2f, 0.25f);
	obj_end(s, P);
	Obj_Draw(&p->human);
	gl1::PopMatrix();
}

/* OURS. An engine piece's opening: object matrix, the base translate, the k pass offset. */
static void engine_place(float x, float y, float z, float px, float py, float pz)
{
	obj_begin();
	gl1::Translatef(x, y, z);
	gl1::Translatef(px, py, pz);
}

/* OURS. An engine piece's close: unit scale, the matrix to f_engine, the list, the pop. */
static void engine_list(Shader_Ctx *s, int P, int list)
{
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	obj_end(s, P);
	gl1::CallList(list);
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x0049aca8..0x0049bbdc, for 11 < t < 24 the cylinders.obj
 * engine twice (k = 0, 1): gears spinning at 380 degrees a second times
 * s1 or s2, and rings of pieces on sin/cos circles, at r_d: the
 * train's z before t = 42, plus 2.5.
 */
static void rail_engine(Rupture_Ctx *ctx, part_rail *p, double t, const Rail_Shared *F)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const int    P = p->pairEngine;
	const double pi = 3.141592653589793;
	double       r_d, s1, s2, dj, jpi, ang, c, sn, B, arg, sA, cA, b, e;
	float        r, px, py, pz;
	int          k, j, pass, m1, m2;

	if (!(t < 24.0 && 11.0 < t)) {
		return;
	}
	k = 0;
	r_d = (((2.5 - F->d0 * 45.0) + 450.0) - 10.0) + 110.00000000000001;
	r = (float)r_d;
	do {
		lit(ctx, p, P);
		glDisable(GL_CULL_FACE);
		Crt_Srand(0x409bfa);
		px = (float)(0.4 - (double)k * 0.8);
		py = (float)((double)k * 0.4 - 0.8);
		pz = (float)((double)k * 2.2 * 0.1 + 0.8);
		gl1::Color3f(2.575f, 0.1f, 0.125f);
		s1 = 1.0;
		s2 = 4.0;
		if (k == 1) {
			s1 = 1.9;
			s2 = 3.8;
		}

		engine_place(1386.6f, -67.2775f, r, px, py, pz);
		gl1::Rotatef((float)(s1 * t * 380.0), 0.0f, 0.0f, 1.0f);
		engine_list(s, P, 1004);

		engine_place(1386.6f, -67.2775f, r, (float)(px - 0.2), (float)((py * 0.0 - 0.6) - 0.8), pz);
		gl1::Rotatef((float)(s1 * t * 0.5 * 380.0), 0.0f, 0.0f, 1.0f);
		engine_list(s, P, 1004);

		engine_place(1386.22f, -64.9775f, r, px, py, pz);
		gl1::Rotatef((float)(s2 * t * 380.0), 0.0f, 0.0f, 1.0f);
		gl1::Scalef(1.0f, 1.0f, 1.0f);
		obj_end(s, P);
		if (p->cylinders.groups.size() > 5) {
			Obj_DrawGroup(&p->cylinders.groups[5]);
		}
		gl1::PopMatrix();

		j = 0;
		do {
			j++;
			engine_place(1387.62f,
			             (float)(((((((((0.0 - (double)j * 0.4) - 0.1) - 0.6) + 2.0) + 2.9225) - 4.8) - 69.0)
			                      - 1.0) + 5.6),
			             r, px, py, pz);
			gl1::Rotatef((float)(s2 * t * -380.0 * 0.4), 0.0f, 0.0f, 1.0f);
			engine_list(s, P, 1005);
			j++;
		} while (j < 4);

		engine_place(1386.22f, -64.9775f, r, (float)(px - 1.0), (float)((py - 1.0) - 0.8), (float)(pz - 0.1));
		gl1::Rotatef((float)((s2 * t + s2 * t) * 380.0), 0.0f, 0.0f, 1.0f);
		gl1::Scalef(0.9f, 0.9f, 0.9f);
		obj_end(s, P);
		if (k == 0) {
			gl1::CallList(1005);
		}
		gl1::PopMatrix();

		for (j = -2; j < 5; j++) {
			obj_begin();
			dj = (double)j;
			ang = ((s1 * t + s1 * t) * pi * 380.0) / 360.0 + (dj * pi + 1.4);
			c = cos(ang);
			sn = sin(ang);
			gl1::Translatef((float)((((((((c * 0.4 - 0.77) - 1.0) - 3.4) + 800.0) + 580.0) + 1.4) + 4.8) + 5.57),
			                (float)(((((sn * 0.4 + 2.9225) - 4.8) - 69.0) - 1.0) + 4.6),
			                (float)(dj * 0.6 + r_d));
			gl1::Translatef(px, py, pz);
			gl1::Rotatef((float)((c + c) * 5.0), 0.0f, 0.0f, 1.0f);
			engine_list(s, P, 1003);
		}

		for (pass = 0; pass < 2; pass++) {
			for (j = -4; j < 10; j++) {
				obj_begin();
				dj = (double)j;
				jpi = dj * pi;
				B = ((s2 * t + s2 * t) * pi * 380.0) / 360.0;
				m1 = ((j + 100) / 2) % 2;
				m2 = ((j + 101) / 2) % 2;
				arg = B + ((jpi + ((((double)m1 * 1.5707963267948966 - 1.2) + pi) + 1.4))
				           + ((double)m2 * pi) * 0.0);
				sA = sin(arg);
				cA = cos((jpi + 1.4) + B);
				if (sA >= 0.0) {
					b = 1.0 - sA;
					e = 1.0 - b * (b * b);
				} else {
					b = 1.0 - fabs(sA);
					e = -(1.0 - b * (b * b));
				}
				if ((j + 10) % 2 != 0) {
					gl1::Translatef(0.0f, 0.0f, -0.08f);
				}
				gl1::Translatef(
				        (float)(((((((((cA * 0.0 - 0.77) - 1.0) - 3.4) + 800.0) + 580.0) + 1.4) + 4.8) + 5.57) - 0.4),
				        (float)((((((e * 0.05 + 2.9225) - 4.8) - 69.0) - 1.0) + 4.6) - 0.6),
				        (float)(((dj * 0.6) * 0.5 + r_d) - 0.1));
				gl1::Translatef(px, py, pz);
				engine_list(s, P, pass == 0 ? 1001 : 1000);
			}
		}

		for (j = -2; j < 5; j++) {
			obj_begin();
			ang = ((s1 * t + s1 * t) * pi * 380.0) / 360.0 + (pi * (double)j + 1.4);
			gl1::Translatef((float)((((((((cos(ang) * 0.4 - 0.77) - 1.0) - 3.4) + 800.0) + 580.0) + 1.4) + 4.8) + 5.57),
			                (float)((((((sin(ang) * 0.4 + 0.5) + 2.9225) - 4.8) - 69.0) - 1.0) + 4.6),
			                (float)((double)j * 0.6 + r_d));
			gl1::Translatef(px, py, pz);
			engine_list(s, P, 1002);
		}
		k++;
	} while (k < 2);
}

/*
 * ORIGIN 0x0049bbf7..0x0049c16d, the glow trail: part_truck's 30
 * blob41b quads, while the train flag is on, at the train's car 0,
 * turned by its z angle and faded out at t = 49..50 or after t = 59.
 * It stores carY * -0.4 back into carY for the streaks.
 */
static void rail_glow_trail(Rupture_Ctx *ctx, part_rail *p, double t, Rail_Shared *F)
{
	Gfx_Ctx *g = &ctx->gfx;
	double   fd, off, ang, a0, b0, sel, di, f, sc, sxy, sxy2, a;
	Vec4     V, W;
	int      i;

	fd = 1.0 - Clamp01(t - 49.0);
	if (t >= 51.0) {
		fd = 1.0 - Clamp01((t - 59.0) * 0.8);
	}
	if (!(F->trainOn && fd > 0.0001)) {
		return;
	}
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	off = 0.0;
	if (t >= 51.0) {
		off = 0.2;
	}
	if (t >= 58.0) {
		off = Clamp01((t - 59.7) + (t - 59.7)) * 0.4 + 0.0;
	}
	V = Vec4_Make(0.0f, (float)(((F->carY + 1.8) + 0.6) + off), 0.0f);
	if (t < 51.0) {
		ang = (F->rotAcc * -6.283185307179586) / 360.0;
	} else if (t < 58.0) {
		ang = (F->rotAcc * 6.283185307179586) / 360.0 + 1.2;
	} else {
		ang = (F->rotAcc * 6.283185307179586) / 360.0 - 1.2;
	}
	a0 = 0.0;
	b0 = 0.0;
	vec4_rotate_euler(&V, &a0, &b0, &ang);
	F->carY = F->carY * -0.4;
	W = Vec4_Make(0.0f, (float)(((1.8 - F->carY) + 0.6) - off), 0.0f);
	Vec4_Sub(&V, &W, &V);
	sel = t >= 58.0 ? -0.2 : 0.0;
	W = Vec4_Make((float)((((sel + 800.0) + 580.0) + 1.4) + 4.8), (float)(F->carY - 66.4),
	              (float)(F->zAcc - 6.8));
	Vec4_Add(&V, &W, &V);
	gl1::Translatef(V.x, V.y, V.z);
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	gl1::Enable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	Tex_Bind(g, p->texBase + 0x14);
	gl1::Color3f(0.05f, 0.07f, 0.12f);
	glDepthMask(GL_FALSE);
	for (i = 0; i < 30; i++) {
		di = (double)i;
		f = 1.0 - di / 30.0;
		sc = (1.0 - f) * 1.4 + 1.0;
		if (i == 0) {
			f = 3.0;
		}
		gl1::Color3f((float)(f * 0.03), (float)(0.04 * f), (float)(0.06 * f));
		gl1::PushMatrix();
		gl1::Translatef(0.0f, 0.0f, (float)(di * -6.0 * 0.1));
		sxy = fd * sc;
		sxy2 = sxy;
		a = 0.0;
		Gfx_DrawQuad(&a, &sxy, &sxy2, 1);
		gl1::PopMatrix();
	}
	glDepthMask(GL_TRUE);
	gl1::PopMatrix();
	gl1::Disable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

/* OURS. The streak sets' opening: projection, camera, srand(0x9600b3bf), additive and untextured. */
static void streak_open(Rupture_Ctx *ctx)
{
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(ctx->fovOffset + 65.0, (double)ctx->width / ctx->height, ctx->nearZ, ctx->farZ);
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	glEnable(GL_DEPTH_TEST);
	Crt_Srand(0x9600b3bf);
	gl1::Color3f(1.0f, 0.0f, 0.0f);
	Gfx_ResetState();
	gl1::Disable(GL_TEXTURE_2D);
	gl1::PointSize(1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
}

/*
 * ORIGIN 0x0049c16f..0x0049c5d4, streaks 1: 1000 additive lines at the
 * train's car 0, fading in after t = 37.2, out over 47.5 .. 49.5, in
 * again after 60 and out after 62.2.
 */
static void rail_streaks1(Rupture_Ctx *ctx, double t, const Rail_Shared *F)
{
	double L, ph, r2, r3, fr, len;
	float  Py, Ay, Az, cb, cg, cr, u2, u3;
	Vec4   Pv, A;
	int    n;

	streak_open(ctx);
	L = ((Clamp01(t - 37.2) - Clamp01((t - 47.5) * 0.5)) + Clamp01(t - 60.0)) - Clamp01(((t - 60.0) - 2.2) * 0.6);
	gl1::Begin(gl1::LINES);
	if (L > 0.0001) {
		Py = (float)(0.0 - 3.38);
		Ay = (float)(F->carY - 66.4);
		Az = (float)(F->zAcc - 6.8);
		cb = (float)(0.35 * L);
		cg = (float)(0.55 * L);
		cr = (float)(L * 0.8);
		for (n = 1000; n != 0; n--) {
			ph = (Math_Random() * 3.0 + 1.0) * (t * 3.0);
			r2 = Math_Random();
			u2 = (float)((r2 + r2) - 1.0);
			r3 = Math_Random();
			u3 = (float)((r3 + r3) - 1.0);
			Pv.kind = 1;
			Pv.x = (float)(u2 * 1.36);
			Pv.z = (float)(u3 * 23.0);
			Pv.z = (float)(Pv.z + 26.0);
			Pv.y = Py;
			Pv.x = (float)(Pv.x + 0.0);
			fr = ph - (double)(int)ph;
			Pv.z = (float)(fr * 4.8 * 1.2 * 0.6 + Pv.z);
			len = Math_Random() * 2.8 + 1.0;
			A = Vec4_Make(1386.2f, Ay, Az);
			Vec4_Add(&Pv, &A, &Pv);
			gl1::Color3f(cr, cg, cb);
			gl1::Vertex3f(Pv.x, Pv.y, Pv.z);
			gl1::Color3f(0.0f, 0.0f, 0.0f);
			gl1::Vertex3f(Pv.x, Pv.y, (float)(len * 1.1 + Pv.z));
		}
	}
	gl1::End();
	glDisable(GL_BLEND);
}

/*
 * ORIGIN 0x0049c5da..0x0049cb3f, streaks 2: for 51 <= t < 58 the same
 * seed's 1000 lines, lifted 1.8, turned about z by the train's angle and
 * put back, brighter toward the middle of their run. The demo builds
 * the vectors in the slots of zAcc, carY and rotAcc after reading them.
 */
static void rail_streaks2(Rupture_Ctx *ctx, double t, const Rail_Shared *F)
{
	const double pi = 3.141592653589793;
	double       G, yaw, Gc, ph, r2, r3, r4, fr, len, c, a0, b0, kk;
	float        Ay, Az, u2, w3, u4, cc;
	Vec4         Pv, W, W2, A;
	int          n;

	if (!(t < 58.0 && t >= 51.0)) {
		return;
	}
	streak_open(ctx);
	G = 1.0;
	if (t >= 54.0) {
		G = 0.5;
	}
	gl1::Begin(gl1::LINES);
	if (G > 0.0001) {
		yaw = (F->rotAcc * 6.283185307179586) / 360.0;
		Ay = (float)(F->carY - 66.4);
		Az = (float)(F->zAcc - 18.0);
		Gc = G * 0.3;
		for (n = 1000; n != 0; n--) {
			ph = (Math_Random() * 3.0 + 1.0) * (t * 1.3);
			r2 = Math_Random();
			u2 = (float)((r2 + r2) - 1.0);
			r3 = Math_Random();
			w3 = (float)(r3 * 2.8 + r3 * 2.8);
			r4 = Math_Random();
			u4 = (float)(r4 * 2.0 - 1.0);
			Pv.kind = 1;
			Pv.x = (float)(u2 * 1.36);
			Pv.z = (float)(u4 * 23.0);
			Pv.z = (float)(Pv.z + 26.0);
			Pv.y = (float)(w3 - 3.38);
			Pv.x = (float)(Pv.x + 0.0);
			fr = ph - (double)(int)ph;
			Pv.z = (float)(2.0 * (fr * 4.8 * 1.2 * 0.6) + Pv.z);
			len = Math_Random() * 0.8 + 1.0;
			W = Vec4_Make(0.0f, 1.8f, 0.0f);
			Vec4_Add(&Pv, &W, &Pv);
			c = t >= 58.0 ? yaw - 1.2 : yaw + 1.2;
			a0 = 0.0;
			b0 = 0.0;
			vec4_rotate_euler(&Pv, &a0, &b0, &c);
			W2 = Vec4_Make(0.0f, -1.8f, 0.0f);
			Vec4_Add(&Pv, &W2, &Pv);
			kk = Clamp01(sin(((Pv.z * 0.02) - 0.2) * pi));
			A = Vec4_Make(1386.2f, Ay, Az);
			Vec4_Add(&Pv, &A, &Pv);
			gl1::Color3f(0.0f, 0.0f, 0.0f);
			gl1::Vertex3f(Pv.x, Pv.y, Pv.z);
			cc = (float)(Gc * kk);
			gl1::Color3f(cc, cc, cc);
			gl1::Vertex3f(Pv.x, Pv.y, (float)(len * 0.2 + Pv.z));
		}
	}
	gl1::End();
	glDisable(GL_BLEND);
}

/*
 * ORIGIN 0x00497100 part_rail::DrawScene: the sky, the pass set-up, the
 * train and the boat (not for 24 < t < 31), the fireplace tracks and
 * the rails, the bridge rails, the hands, the head, the engine, the
 * glow trail and the two streak sets. The modelview push after the sky
 * is popped by the last call. pass is not read.
 */
void part_rail::DrawScene(Rupture_Ctx *ctx, int pass)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	Rail_Shared F;
	bool        flag77;
	double      fade;
	static const GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1 };

	(void)pass;
	Crt_Srand(0xdc072526);
	g_railHeadFlag = seqHead[2] > 0.1 ? 1 : 0;
	F.zAcc = 0.0;
	F.rotAcc = 0.0;
	F.carY = 0.0;
	rail_sky(ctx, this, t);
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
	glDisable(GL_CULL_FACE);
	F.d0 = Max0(t - 0.0);
	F.trainOn = t <= 24.0 || t >= 31.0;
	if (F.trainOn) {
		rail_train(ctx, this, t, &F);
		rail_boat(ctx, this, t, &F);
	}

	flag77 = !(t >= 20.0 && t < 24.0);
	fade = 1.0 - (Clamp01(t - 14.0) - Clamp01(t - 16.0));
	rail_fireplace1(ctx, this, t, fade, flag77);
	rail_fireplace2(ctx, this, t, fade, flag77, &F);
	rail_stack(ctx, this, t, fade, flag77);
	rail_pair(ctx, this, t);

	rail_bridge(ctx, this, t);
	rail_hands(ctx, this, t);
	rail_head(ctx, this, t);
	rail_engine(ctx, this, t, &F);
	Crt_Srand(0x409bfa);
	Gfx_ResetState();
	Shader_DisableProfiles(s);
	rail_glow_trail(ctx, this, t, &F);
	rail_streaks1(ctx, t, &F);
	rail_streaks2(ctx, t, &F);
	gl1::PopMatrix();
}
