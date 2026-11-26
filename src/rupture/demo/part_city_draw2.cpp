#include "part_city_draw.h"

#include <math.h>

#include "../core/gl1.h"
#include "scene.h"

static const double k_pi = 3.141592653589793;   /* 0x004f36e8 */

/* OURS. The ease the heli sections take, Ease(Clamp01(v), 1). */
static double ease1(double v)
{
	double x = Clamp01(v), one = 1.0;

	return Math_Ease(&x, &one);
}

/* OURS. The five turns the heli gets after t = 22 and its shards get. */
static void heli_sway(double t)
{
	gl1::Rotatef(12.0f, 1.0f, 0.0f, 0.0f);
	gl1::Rotatef(-4.0f, 0.0f, 0.0f, 1.0f);
	gl1::Rotatef((float)(sin(t * 1.3) * 0.36), 0.0f, 0.0f, 1.0f);
	gl1::Rotatef(90.0f, 0.0f, 1.0f, 0.0f);
	gl1::Rotatef((float)(sin(t + t) * 0.24), 0.2f, 0.0f, 0.8f);
}

/* OURS. The two eased tilts that follow, about (1, 0, -0.3) and x. */
static void heli_tilt(double t)
{
	gl1::Rotatef((float)(ease1((t - 12.0) * 0.29) * 38.0), 1.0f, 0.0f, -0.3f);
	gl1::Rotatef((float)(ease1((t - 9.5) * 0.3) * -42.0), 1.0f, 0.0f, 0.0f);
}

/*
 * ORIGIN 0x004739c7..0x00473d7e, boat.obj on the boat pair before
 * t = 15: dropping 110 a second, from t = 9 moved to x = 104 and
 * flown down and forward, turning -60 degrees a second (doubled from
 * t = 9). Its translation is kept for city_sparks.
 */
void city_boat(Rupture_Ctx *ctx, part_city *p, City_Draw *D)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const double t = p->t;
	double       tt = 100.0, u;
	Vec4         B;

	city_lit(ctx, p, p->pairBoat);
	Shader_SetParam1f(s, p->pairBoat + 1, "ttime", &tt);
	Crt_Srand(0x409bfa);
	city_obj_begin();
	B = Vec4_Make(0.0f, (float)(1340.0 - t * 110.0), 0.0f);
	if (9.0 <= t) {
		u = (t - 9.0) - 3.6;
		B.x = 104.0f;
		B.y = (float)(((268.6 - (t - 10.0) * 0.0) + 90.0) - u * 20.0);
		B.z = (float)(u * 23.0 + 27.0);
	}
	gl1::Translatef(B.x, B.y, B.z);
	D->s270 = B;
	gl1::Rotatef(-90.0f, 1.0f, 0.23f, 0.0f);
	gl1::Rotatef((float)(t * -60.0), 0.1f, 0.0f, 1.0f);
	if (9.0 <= t) {
		gl1::Rotatef((float)(t * -60.0), 0.1f, 0.0f, 1.0f);
	}
	city_obj_end(s, p->pairBoat);
	gl1::Color3f(0.7f, 0.7f, 0.6f);
	if (t < 15.0) {
		Obj_Draw(&p->boat);
	}
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x00473d7e..0x00474e93, before t = 34 the helicopter on the
 * heli pair as three groups, one lit bracket each: before t = 15
 * drawn toward the eye from (129, 268.6, -73), then following the
 * camera windows of 15..22, 22..32 and 32..52. Group 1 is the main
 * rotor, spun about y; the tail rotor spins about z round
 * (9.07, 2.82, -0.44). From t = 22 hellicrash2.obj's groups 0 and 2
 * stand in for helicopter.obj's. The window's position goes to
 * this+0x13e0 and its z, times 0.12, to posx.
 */
void city_heli(Rupture_Ctx *ctx, part_city *p, City_Draw *D)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const int    P = p->pairHeli;
	const double t = p->t;
	const float  k89 = 0.89f;                          /* 0x004f4ec8 */
	double       tt, x, R, q, e, sr, a, u, posx;
	float        ef;
	Vec4         V, d, m1, m2, W;
	int          i;

	if (t < 34.0) {
		for (i = 0; i < 3; i++) {
			city_lit(ctx, p, P);
			tt = 3.0 - Clamp01(((t - 21.0) + 2.0) * 0.06) * 1.5;
			Shader_SetParam1f(s, P + 1, "ttime", &tt);
			Crt_Srand(0x409bfa);
			city_obj_begin();
			x = city_max0((t - 5.5) * 0.8);
			R = Math_SoftRamp(&x) * -2.0;
			q = 0.0;
			if (t < 15.0) {
				V = Vec4_Make(129.0f, 268.6f, -73.0f);
				e = ease1((t - 6.0) * 0.2);
				ef = (float)e;
				Vec4_Sub(&ctx->eye.eye, &V, &d);
				Vec4_Scale(&d, &ef, &m1);
				Vec4_Scale(&m1, &k89, &m2);
				Vec4_Add(&V, &m2, &V);
				x = city_max0(t - 10.0) * 0.9;
				V.z = (float)(Math_SoftRamp(&x) * 0.2 * 10.0 + V.z);
				x = city_max0(t - 13.0) * 0.9;
				V.z = (float)(Math_SoftRamp(&x) * 0.2 * 10.0 * 2.2 + V.z);
				x = city_max0(t - 13.0) * 0.9;
				V.x = (float)(Math_SoftRamp(&x) * 0.2 * 10.0 * 2.2 + V.x);
				q = e * -3.2;
				gl1::Translatef(V.x, V.y, V.z);
				if (3.0 <= t && t < 5.0) {
					gl1::Rotatef(-114.0f, 0.0f, 1.0f, 0.0f);
				}
			}
			if (15.0 <= t && t < 22.0) {
				W = Vec4_Make(-10.0f, 96.8f, (float)((((t - 15.0) * 0.5 - 8.0) * 100.0 + 657.0) - 6.0));
				x = 0.5 * city_max0(t - 18.0);
				sr = Math_SoftRamp(&x);
				gl1::Translatef(-10.0f, (float)(sr * 4.0 + 96.80000305175781), W.z);   /* 0x004f4e98 */
				D->s80 = W;
				gl1::Rotatef((float)(sr * 23.0), 1.0f, -3.8f, 0.0f);
				gl1::Rotatef(9.0f, 1.0f, 0.0f, 0.0f);
				gl1::Rotatef((float)(sin(t * 3.3) * 1.2000000000000002), 0.0f, 0.0f, 1.0f);
				gl1::Rotatef(90.0f, 0.0f, 1.0f, 0.0f);
				gl1::Rotatef((float)(sin(t * 4.0) * 0.8), 0.2f, 0.0f, 0.8f);
			}
			if (22.0 <= t && t < 32.0) {
				a = (t - 15.0) * 0.5 - 8.0;
				e = ease1(t - 24.0);
				W = Vec4_Make(-10.0f, (float)((1.0 - e) * 0.8 + 96.8), (float)((a * 100.0 - 157.0) - 6.0));
				x = city_max0(t - 18.0) * 0.5;
				Math_SoftRamp(&x);
				gl1::Translatef(-10.0f, W.y, W.z);
				D->s80 = W;
				heli_sway(t);
			}
			if (32.0 <= t && t < 52.0) {
				u = t - 32.0;
				W = Vec4_Make(-96.0f, (float)(150.0 - 20.0 * u), (float)(((u * 90.0) * 1.6 - 558.0) - 100.0));
				x = city_max0(t - 18.0) * 0.5;
				Math_SoftRamp(&x);
				gl1::Translatef(-96.0f, W.y, W.z);
				D->s80 = W;
				heli_sway(t);
			}
			heli_tilt(t);
			gl1::Rotatef((float)(q * 90.0), 0.3f, 1.0f, 0.0f);
			if (i == 1) {
				gl1::Rotatef((float)(R * 1.4 * 300.0), 0.0f, 1.0f, 0.0f);
			} else if (i == (D->t22 ? 2 : 0)) {
				gl1::Translatef(9.07f, 2.82f, -0.44f);
				gl1::Rotatef((float)(R * 1.4 * 700.0), 0.0f, 0.0f, 1.0f);
				gl1::Translatef(-9.07f, -2.82f, 0.44f);
			}
			posx = (double)D->s80.z * 0.12;            /* 0x004f4600 */
			Shader_SetParam1f(s, P + 1, "posx", &posx);
			city_obj_end(s, P);
			gl1::Color3f(0.7f, 0.7f, 0.6f);
			if (!D->t22 || i == 1) {
				Obj_DrawGroup(&p->helicopter.groups[i]);
			} else {
				Obj_DrawGroup(&p->hellicrash2.groups[i]);
			}
			gl1::PopMatrix();
		}
	}
	p->vec13e0 = D->s80;
}

/*
 * ORIGIN 0x00474e93..0x00475e28, for 22 <= t < 32 Hellidist.obj's
 * shards at the helicopter, on the heli pair with culling off: each
 * seeked by s = Clamp(0.2 (t - 3 rest.x + 6), 0, 9.7), plus a late
 * push for the ones a random test picks; then four triangles each.
 */
void city_heli_shards(Rupture_Ctx *ctx, part_city *p, City_Draw *D)
{
	static const int   tri[4][3] = { { 0, 1, 2 }, { 0, 1, 3 }, { 0, 2, 3 }, { 1, 2, 3 } };
	Shader_Ctx         *s = &ctx->gfx.shader;
	const int           P = p->pairHeli;
	const double        t = p->t;
	const Vec4          P0 = Vec4_Make(-14.0f, 3.1f, -1.32f);   /* 0x004f4f74..0x004f4f6c */
	const Vec4          zero = Vec4_Make(0.0f, 0.0f, 0.0f);
	const float         one = 1.0f;
	std::vector<Shard> &sh = p->hellidistShards;
	double              rx, v, r1;
	Vec4                d, t0, t1, V[4];
	size_t              i;
	int                 f, k;

	if (!(t < 32.0) || !D->t22) {
		return;
	}
	city_lit(ctx, p, P);
	Crt_Srand(0x409bfa);
	city_obj_begin();
	gl1::Translatef(D->s80.x, D->s80.y, D->s80.z);
	heli_sway(t);
	heli_tilt(t);
	city_obj_end(s, P);
	gl1::Color3f(0.7f, 0.7f, 0.6f);
	glDisable(GL_CULL_FACE);
	for (i = 0; i < sh.size(); i++) {
		rx = (double)sh[i].rest.x;
		d = sh[i].rest;
		Vec4_Sub(&d, &P0, &d);                         /* never read */
		v = city_max0(((((t - 3.0) - 3.0 * rx) + 30.0) - 20.0) - 1.0) * 4.0 * 10.0 * 0.005;
		if (!(v <= 9.7)) {
			v = 9.7;
		}
		r1 = Math_Random() * 3.0;
		if (rx < Math_Random() - 3.4) {
			v = city_max0(((((((t - 3.0) - rx) - 55.0) + 47.0) - r1) - 20.0) - 1.0) * 4.0 * 10.0 * 0.27 + v;
		}
		if (t >= 32.0) {
			v = v - 45.0;
		}
		Shard_Seek(&sh[i], v);
	}
	gl1::BatchBegin();   /* OURS: one draw for the loop, docs/subsystems/render.txt */
	for (i = 0; i < sh.size(); i++) {
		const Shard *h = &sh[i];

		for (k = 0; k < 4; k++) {
			Vec4_Add(&h->turned[k], &h->pos, &t0);
			Vec4_Add(&t0, &zero, &t1);
			Vec4_Scale(&t1, &one, &V[k]);
		}
		gl1::Begin(gl1::TRIANGLES);
		for (f = 0; f < 4; f++) {
			gl1::Normal3f(h->turnedN[f].x, h->turnedN[f].y, h->turnedN[f].z);
			for (k = 0; k < 3; k++) {
				gl1::Vertex3f(V[tri[f][k]].x, V[tri[f][k]].y, V[tri[f][k]].z);
			}
		}
		gl1::End();
	}
	gl1::BatchEnd();
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x00475e28..0x00476258, 1100 grey points at radius 720 over
 * the eye, the upper half sphere, under a 65 / 1 / 1000 projection.
 * The modelview push here is popped in GL_PROJECTION mode, so the
 * modelview stack is left one deeper. The brightness ease is computed
 * and replaced by 1.
 */
void city_stars(Rupture_Ctx *ctx, part_city *p)
{
	Gfx_Ctx     *g = &ctx->gfx;
	const double t = p->t;
	const double aspect = (double)ctx->width / ctx->height;
	const float  k1800 = 1800.0f, k04 = 0.4f;          /* 0x004f4dcc, 0x004f47fc */
	double       k, r;
	float        c, a, b, z;
	Vec4         V, S;
	int          n;

	Crt_Srand(0xa567);
	Shader_DisableProfiles(&g->shader);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	Gfx_ResetState();
	gl1::PushMatrix();
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(g, p->texBase + 0x14);
	gl1::PopMatrix();
	glDisable(GL_BLEND);
	gl1::Disable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(65.0, aspect, 1.0, 1000.0);
	gl1::PopMatrix();
	Gfx_ResetState();
	Shader_DisableProfiles(&g->shader);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(65.0, aspect, 1.0, 1000.0);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	glEnable(GL_DEPTH_TEST);
	ease1(t * 0.3);
	k = 1.0;
	gl1::PointSize(2.0f);
	gl1::Begin(gl1::POINTS);
	for (n = 1100; n != 0; n--) {
		c = (float)((Math_Random() + 0.2) * k * 0.2);
		gl1::Color3f(c, c, c);
		r = Math_Random();
		a = (float)((r + r) - 1.0);
		r = Math_Random();
		b = (float)((r + r) - 1.0);
		r = Math_Random();
		z = (float)((r + r) - 1.0);
		V.kind = 1;
		V.x = a;
		V.y = (float)fabs(b);
		V.z = z;
		Vec4_Normalize(&V);
		Vec4_Scale(&V, &k1800, &S);
		Vec4_Scale(&S, &k04, &V);
		Vec4_Add(&V, &ctx->eye.eye, &V);
		gl1::Vertex3f(V.x, V.y, V.z);
	}
	gl1::End();
}

/*
 * ORIGIN 0x00476258..0x00476608, before t = 3 350 additive points
 * round the boat, rising 18 on a sawtooth and blue-white on its sine.
 */
void city_sparks(Rupture_Ctx *ctx, part_city *p, City_Draw *D)
{
	const double t = p->t;
	const float  k07 = 0.7f, k12 = 1.2f, k17 = 1.7f;   /* 0x004f4060, 0x004f4770, 0x004f4dc8 */
	double       f, fr, c, r;
	float        a, b, z;
	Vec4         V;
	int          n;

	if (!(t < 3.0)) {
		return;
	}
	Shader_DisableProfiles(&ctx->gfx.shader);
	Gfx_ResetState();
	glDisable(GL_BLEND);
	gl1::Disable(GL_TEXTURE_2D);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	maybe_Camera_LookAtEye(ctx);
	Gfx_ResetState();
	gl1::PointSize(1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	gl1::Begin(gl1::POINTS);
	for (n = 350; n != 0; n--) {
		Math_Random();
		r = Math_Random();
		a = (float)((r + r) - 1.0);
		r = Math_Random();
		b = (float)((r + r) - 1.0);
		r = Math_Random();
		z = (float)((r + r) - 1.0);
		V = Vec4_Make(a, b, z);
		Vec4_Scale(&V, &k07, &V);
		f = (Math_Random() * 0.8 + 1.0) * ((t + 10000.0) * 0.5) + 100.0;
		fr = Clamp01(f - (double)(long)f);
		c = Clamp01(sin(fr * k_pi) * 0.9);
		gl1::Color3f((float)(c * 0.1), (float)(0.14 * c), (float)(0.16 * c));
		Vec4_Scale(&V, &k12, &V);
		V.y = (float)((fr * 10.0 * 1.8 + 2.0) + V.y);
		Vec4_Scale(&V, &k17, &V);
		V.y = (float)(V.y - 10.0);
		Vec4_Add(&V, &D->s270, &V);
		gl1::Vertex3f(V.x, V.y, V.z);
	}
	gl1::End();
	glDisable(GL_BLEND);
	gl1::PopMatrix();
}
