#include "part_end_draw.h"

#include <math.h>

#include "../core/gl1.h"
#include "scene.h"

extern double g_unk_b1e698;   /* 0x00b1e698, part_video.cpp */

/* OURS. x < 0 -> 0, the inline test the code makes. */
static double max0(double x)
{
	return x < 0.0 ? 0.0 : x;
}

/*
 * OURS. part_end's lit bracket, part_city's shape: no push round
 * maybe_Scene_DrawLit, no Shader_DisableProfiles before it, bg08 (slot
 * texBase + 0x5a) as both of its textures, then the pair bound and
 * GL_CULL_FACE on. The demo passes *(double*)(ctx+0x19a0) as the time;
 * maybe_Scene_DrawLit ignores it.
 */
static void end_lit(Rupture_Ctx *ctx, part_end *p, int pair)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   bg = p->texBase + 0x5a;

	ctx->litFlag = 0.0f;
	ctx->v1920.kind = 1;
	ctx->d1918 = 0.0;
	ctx->v1920.x = 0.2f;                               /* 0x004f3754 */
	ctx->v1920.y = -0.3f;                              /* 0x004f4878 */
	ctx->v1920.z = 1.0f;
	ctx->v1930.kind = 1;
	ctx->v1930.x = 1.0f;
	ctx->v1930.y = 1.0f;
	ctx->v1930.z = 1.0f;
	maybe_Scene_DrawLit(ctx, p->model, bg, bg, pair, &ctx->eye, &ctx->light, 0.0);
	ctx->litFlag = 1.0f;
	ctx->v1930.kind = 1;
	ctx->v1930.x = 1.0f;
	ctx->v1930.y = 1.0f;
	ctx->v1930.z = 1.0f;
	Shader_Bind(s, pair);
	Shader_Bind(s, pair + 1);
	Shader_EnableProfiles(s);
	glEnable(GL_CULL_FACE);
}

/* OURS. The five turns the heli groups and Hellidist's shards get, on the texture matrix. */
static void end_heli_turn(void)
{
	gl1::Translatef(0.0f, 0.0f, 0.0f);
	gl1::Rotatef(2.6f, 0.0f, 0.0f, 1.0f);              /* 0x004f5354 */
	gl1::Rotatef(48.0f, 0.0f, 1.0f, 0.0f);
	gl1::Rotatef(90.0f, 1.0f, 0.0f, 0.0f);
	gl1::Rotatef(12.0f, 1.0f, 0.0f, 0.0f);
	gl1::Rotatef(-4.0f, 0.0f, 0.0f, 1.0f);
}

/* OURS. P + (R * s1) * s2, the two Vec4_Scale calls the road makes for each edge. */
static Vec4 add_scaled2(const Vec4 *P, const Vec4 *R, float s1, float s2)
{
	Vec4 m1, m2, o;

	Vec4_Scale(R, &s1, &m1);
	Vec4_Scale(&m1, &s2, &m2);
	Vec4_Add(P, &m2, &o);
	return o;
}

/*
 * ORIGIN 0x00478f10 FUN_00478f10, maybe_Track_DrawRoadWide:
 * maybe_Track_DrawRoadDepth (0x004a1710) three times as wide, the top
 * from R * -12 to R * 12 and the sides dropped by (U * 1.5) * 0.3. The
 * depth argument is not read, nor is ctx. The far corners of a dash
 * swap y (E's x, z with C's y and back), as the code does.
 */
static void maybe_Track_DrawRoadWide(Rupture_Ctx *ctx, Track *T, double depth)
{
	const float k15 = 1.5f, k03 = 0.3f, neg = -1.0f;
	Vec4        P, D, R, U, a, b, c, m1, m2, P2, D2, R2, U2, A, B, C, E;
	int         k;

	(void)ctx;
	(void)depth;
	glDisable(GL_CULL_FACE);
	T->loop = 0;
	gl1::Begin(gl1::QUAD_STRIP);
	for (k = 1; k < T->count; k++) {
		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		a = add_scaled2(&P, &R, -4.0f, 3.0f);
		b = add_scaled2(&P, &R, 4.0f, 3.0f);
		gl1::Normal3f(U.x, U.y, U.z);
		gl1::Vertex3f(a.x, a.y, a.z);
		gl1::Vertex3f(b.x, b.y, b.z);
	}
	gl1::End();
	gl1::Begin(gl1::QUAD_STRIP);
	for (k = 1; k < T->count; k++) {
		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		a = add_scaled2(&P, &R, -4.0f, 3.0f);
		Vec4_Scale(&U, &k15, &m1);
		Vec4_Scale(&m1, &k03, &m2);
		c = add_scaled2(&P, &R, -4.0f, 3.0f);
		Vec4_Sub(&c, &m2, &b);
		gl1::Normal3f(R.x, R.y, R.z);
		gl1::Vertex3f(a.x, a.y, a.z);
		gl1::Vertex3f(b.x, b.y, b.z);
	}
	gl1::End();
	gl1::Begin(gl1::QUAD_STRIP);
	for (k = 1; k < T->count; k++) {
		Scene_RoadFrame(T, k, &P, &D, &R, &U);
		a = add_scaled2(&P, &R, 4.0f, 3.0f);
		Vec4_Scale(&U, &k15, &m1);
		Vec4_Scale(&m1, &k03, &m2);
		c = add_scaled2(&P, &R, 4.0f, 3.0f);
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
		A = Vec4_AddScaled(&P, &R, -0.1f);
		B = Vec4_AddScaled(&P, &R, 0.1f);
		gl1::Normal3f(0.0f, -1.0f, 0.0f);
		gl1::Vertex3f(A.x, (float)(A.y + 0.1), A.z);
		gl1::Vertex3f(B.x, (float)(B.y + 0.1), B.z);
		Scene_RoadFrame(T, k + 1, &P2, &D2, &R2, &U2);
		C = Vec4_AddScaled(&P2, &R2, -0.1f);
		E = Vec4_AddScaled(&P2, &R2, 0.1f);
		gl1::Normal3f(0.0f, -1.0f, 0.0f);
		gl1::Vertex3f(E.x, (float)(C.y + 0.1), E.z);
		gl1::Vertex3f(C.x, (float)(E.y + 0.1), C.z);
	}
	gl1::End();
}

/*
 * ORIGIN 0x0047a770..0x0047a97e, the opening: the heli2 pair's ttime
 * 0, the projection, 55 degrees narrowed by 30 over t = 48.5..50.2,
 * the aniso2h pair's planecut, and the clear to black.
 */
static void end_open(Rupture_Ctx *ctx, part_end *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	double      v, c, one, two, fov;

	v = 0.0;
	Shader_SetParam1f(s, p->pairHeli2 + 1, "ttime", &v);
	g_unk_b1e698 = 0.0;
	glViewport(0, 0, ctx->width, ctx->height);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	c = Clamp01(((t - 19.0) * 1.5) * 1.5);
	two = 2.0;
	fov = Math_Ease(&c, &two) * -0.0;                  /* 0x004f5010 */
	one = 1.0;
	c = Clamp01((t - 48.5) * 0.6);
	fov = fov - Math_Ease(&c, &one) * 30.0;
	gl1::Perspective((fov + 55.0) + ctx->fovOffset, (double)ctx->width / ctx->height, ctx->nearZ,
	                 ctx->farZ);
	v = 11.2;                                          /* 0x004f4930 */
	Shader_SetParam1f(s, p->pairAniso2h + 1, "planecut", &v);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	Shader_DisableProfiles(s);
	glDisable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);
	gl1::Color3f(1.0f, 0.6f, 0.2f);
}

/* OURS. eye + (20, 30, -150), where the sun and the glows sit, depth writes off. */
static Vec4 sky_anchor(Rupture_Ctx *ctx)
{
	Vec4 B = Vec4_Make(20.0f, 30.0f, -150.0f), Q = Vec4_Make(0.0f, 0.0f, 0.0f);

	Vec4_Add(&ctx->eye.eye, &B, &Q);
	glDepthMask(GL_FALSE);
	return Q;
}

/* ORIGIN 0x0047a97e..0x0047ab0f, the sun: one additive blob41b quad of size 18 at the anchor. */
static void end_sun(Rupture_Ctx *ctx, part_end *p)
{
	Gfx_Ctx *g = &ctx->gfx;
	double   zero, w, h;
	Vec4     Q;

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
	Tex_Bind(g, p->texBase + 0x14);
	Q = sky_anchor(ctx);
	gl1::Translatef(Q.x, Q.y, Q.z);
	h = 18.0;
	w = 18.0;
	zero = 0.0;
	Gfx_DrawQuad(&zero, &w, &h, 1);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x0047ab0f..0x0047ad53, two cloudt2 glows of size 60 at the
 * anchor, 30 apart and wobbling in x. The eye camera is the demo's
 * inline gluLookAt here, not a call.
 */
static void end_glows(Rupture_Ctx *ctx, part_end *p, double t)
{
	Gfx_Ctx      *g = &ctx->gfx;
	const Camera *e = &ctx->eye;
	double        dk, ang, w, h;
	Vec4          G;
	int           k, kk;

	for (k = 0, kk = 0; kk < 60; k++, kk += 30) {
		Shader_DisableProfiles(&g->shader);
		gl1::PushMatrix();
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::LoadIdentity();
		gl1::MatrixMode(gl1::MODELVIEW);
		gl1::LoadIdentity();
		Scene_LookOffset(ctx);
		gl1::LookAt(e->eye.x, e->eye.y, e->eye.z, e->target.x, e->target.y, e->target.z, e->up.x,
		            e->up.y, e->up.z);
		Gfx_ResetState();
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		gl1::Color3f(0.03f, 0.05f, 0.08f);
		gl1::Enable(GL_TEXTURE_2D);
		Tex_Bind(g, p->texBase + 0x1e);
		G = sky_anchor(ctx);
		gl1::Translatef(G.x, G.y, G.z);
		dk = (double)k;
		gl1::Translatef((float)(sin(dk * 4.0 + t * 0.32000000000000006) * 11.0 + (double)kk), 0.0f,
		                0.0f);                             /* 0x004f5378 */
		ang = dk * 40.0 - 48.0;
		h = 60.0;
		w = 60.0;
		Gfx_DrawQuad(&ang, &w, &h, 1);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		gl1::PopMatrix();
	}
}

/*
 * ORIGIN 0x0047ad53..0x0047af5f, the road on the heli2 pair, ttime
 * 2.04 and strong 0, in (0.7, 0.7, 0.6): track's points as a strip 24
 * wide, the texture matrix moved by (0, -2, -892) without a reset.
 */
static void end_road(Rupture_Ctx *ctx, part_end *p)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   P = p->pairHeli2;
	double      v;

	end_lit(ctx, p, P);
	v = 2.04;                                          /* 0x004f5370 */
	Shader_SetParam1f(s, P + 1, "ttime", &v);
	v = 0.0;
	Shader_SetParam1f(s, P + 1, "strong", &v);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::Translatef(0.0f, -2.0f, -892.0f);
	Shader_SetTextureMatrix(s, P, 0xe);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::Color3f(0.7f, 0.7f, 0.6f);
	maybe_Track_DrawRoadWide(ctx, &p->track, 1.0);
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x0047af5f..0x0047b666, the wreck: three groups, one lit
 * bracket each on the heli2r pair, with ttime 2.04 less a flicker of
 * two sines and strong 1 set on both heli pairs. Group 0 is
 * hellicrash2.obj's 0; group 1 helix.obj's 0, turned about y by
 * 188.6 plus a twitch of two sines and 30 more over t = 12.5..15.8;
 * group 2 hellicrash2.obj's 2, spun 270 degrees a second about z
 * round (9.07, 2.82, -0.44). The SoftRamp and Ease results the code
 * computes and drops are computed and dropped.
 */
static void end_heli(Rupture_Ctx *ctx, part_end *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   P = p->pairHeli2r;
	const float rot0 = (float)(0.0 * 90.0);           /* 0x004f4788 */
	double      w1, w2, v, x, one, s1, s2, A, c;
	int         i;

	for (i = 0; i < 3; i++) {
		end_lit(ctx, p, P);
		w1 = sin(t * 3.0);
		if (w1 < -1.0) {
			w1 = -1.0;
		}
		w2 = sin(t * 12.7);
		if (w2 < -1.0) {
			w2 = -1.0;
		}
		v = 2.04 - (((w1 * 0.25) * w2) * 0.1) * 0.5;
		Shader_SetParam1f(s, P + 1, "ttime", &v);
		x = 1.0;
		Shader_SetParam1f(s, P + 1, "strong", &x);
		Shader_SetParam1f(s, p->pairHeli2 + 1, "ttime", &v);
		x = 1.0;
		Shader_SetParam1f(s, p->pairHeli2 + 1, "strong", &x);
		Crt_Srand(0x409bfa);
		gl1::PushMatrix();
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::LoadIdentity();
		x = max0((t - 5.5) * 0.8);
		(void)Math_SoftRamp(&x);
		one = 1.0;
		x = Clamp01(t - 24.0);
		(void)Math_Ease(&x, &one);
		x = max0(t - 18.0) * 0.5;
		(void)Math_SoftRamp(&x);
		end_heli_turn();
		x = Clamp01((t - 9.5) * 0.3);
		one = 1.0;
		(void)Math_Ease(&x, &one);
		gl1::Rotatef(rot0, 0.3f, 1.0f, 0.0f);
		s1 = max0(sin(t * 7.0));
		s2 = max0(sin(t * 23.0));
		A = -2.4 - (s1 * 1.6) * s2;
		c = Clamp01((t - 12.5) * 0.3);
		if (i == 1) {
			gl1::Rotatef((float)((A + c * 30.0) + (0.0 * 300.0 + 191.0)), 0.0f, 1.0f, 0.0f);
		} else if (i == 2) {
			gl1::Translatef(9.07f, 2.82f, -0.44f);
			gl1::Rotatef((float)(t * 270.0), 0.0f, 0.0f, 1.0f);
			gl1::Translatef(-9.07f, -2.82f, 0.44f);
		}
		Shader_SetTextureMatrix(s, P, 0xe);
		gl1::MatrixMode(gl1::MODELVIEW);
		gl1::Color3f(0.7f, 0.7f, 0.6f);
		Obj_DrawGroup(i == 1 ? &p->helix.groups[0] : &p->hellicrash2.groups[i]);
		gl1::PopMatrix();
	}
}

/*
 * OURS. A shard's spread as the code inlines it: the squares rounded
 * to float, summed, rounded, rooted, rounded.
 */
static float shard_len(const Vec4 *d)
{
	float xx = d->x * d->x, yy = d->y * d->y, zz = d->z * d->z;

	return (float)sqrt((double)(float)(((double)xx + yy) + zz));
}

/*
 * ORIGIN 0x0047b666..0x0047c503, before t = 32 Hellidist.obj's shards
 * on the heli2 pair, under the heli's five turns: each seeked to
 * 0.2 * max0(t + 6 - 3 rest.x), capped at 9.7, plus
 * 10.8 * max0(t + 368 - rest.x - r1) when rest.x is below a second
 * random draw less 3.4 (r1 a first draw, times 3). A shard is drawn
 * as four triangles while pos is within 10 of its first turned
 * corner. this+0x1408 is set to {1, 0, 0, 0} first, whatever t is.
 */
static void end_heli_shards(Rupture_Ctx *ctx, part_end *p, double t)
{
	Shader_Ctx         *s = &ctx->gfx.shader;
	const int           P = p->pairHeli2;
	const Vec4          Q = Vec4_Make(-14.0f, 3.1f, -1.32f);
	const Vec4          zero = Vec4_Make(0.0f, 0.0f, 0.0f);
	const float         one = 1.0f;
	std::vector<Shard> &sh = p->hellidistShards;
	double              r, u, m, w, sk, r1;
	Vec4                d, t0, t1, V[4];
	unsigned            i;
	int                 k;

	p->vec1408 = zero;
	if (!(t < 32.0)) {
		return;
	}
	end_lit(ctx, p, P);
	Crt_Srand(0x409bfa);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	end_heli_turn();
	Shader_SetTextureMatrix(s, P, 0xe);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::Color3f(0.7f, 0.7f, 0.6f);
	glDisable(GL_CULL_FACE);
	for (i = 0; i < (unsigned)sh.size(); i++) {
		r = (double)sh[i].rest.x;
		d = sh[i].rest;
		Vec4_Sub(&d, &Q, &d);
		u = ((((t - 3.0) - 3.0 * r) + 30.0) - 20.0) - 1.0;
		m = ((max0(u) * 4.0) * 10.0) * 0.005;
		sk = m > 9.7 ? 9.7 : m;                        /* 0x004f4dd0 */
		r1 = Math_Random() * 3.0;
		if (r < Math_Random() - 3.4) {
			w = (((((((t - 3.0) - r) - 55.0) + 47.0) - r1) - 20.0) - 1.0) + 400.0;
			sk = ((max0(w) * 4.0) * 10.0) * 0.27 + sk;
		}
		Shard_Seek(&sh[i], sk);
	}
	gl1::BatchBegin();   /* OURS: one draw for the loop, docs/subsystems/render.txt */
	for (i = 0; i < (unsigned)sh.size(); i++) {
		Shard *h = &sh[i];

		Vec4_Sub(&h->pos, &h->turned[0], &d);
		if (!(shard_len(&d) <= 10.0f)) {
			continue;
		}
		for (k = 0; k < 4; k++) {
			Vec4_Add(&h->turned[k], &h->pos, &t0);
			Vec4_Add(&t0, &zero, &t1);
			Vec4_Scale(&t1, &one, &V[k]);
		}
		gl1::Begin(gl1::TRIANGLES);
		gl1::Normal3f(h->turnedN[0].x, h->turnedN[0].y, h->turnedN[0].z);
		gl1::Vertex3f(V[0].x, V[0].y, V[0].z);
		gl1::Vertex3f(V[1].x, V[1].y, V[1].z);
		gl1::Vertex3f(V[2].x, V[2].y, V[2].z);
		gl1::Normal3f(h->turnedN[1].x, h->turnedN[1].y, h->turnedN[1].z);
		gl1::Vertex3f(V[0].x, V[0].y, V[0].z);
		gl1::Vertex3f(V[1].x, V[1].y, V[1].z);
		gl1::Vertex3f(V[3].x, V[3].y, V[3].z);
		gl1::Normal3f(h->turnedN[2].x, h->turnedN[2].y, h->turnedN[2].z);
		gl1::Vertex3f(V[0].x, V[0].y, V[0].z);
		gl1::Vertex3f(V[2].x, V[2].y, V[2].z);
		gl1::Vertex3f(V[3].x, V[3].y, V[3].z);
		gl1::Normal3f(h->turnedN[3].x, h->turnedN[3].y, h->turnedN[3].z);
		gl1::Vertex3f(V[1].x, V[1].y, V[1].z);
		gl1::Vertex3f(V[2].x, V[2].y, V[2].z);
		gl1::Vertex3f(V[3].x, V[3].y, V[3].z);
		gl1::End();
	}
	gl1::BatchEnd();
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x0047a770 part_end::DrawScene: the opening and clear, the
 * sun and its glows, the road, the wreck's three groups, Hellidist's
 * shards; then part_end_draw2.cpp's, in address order: the logo and
 * the stars. pass is not read.
 */
void part_end::DrawScene(Rupture_Ctx *ctx, int pass)
{
	(void)pass;
	end_open(ctx, this, t);
	end_sun(ctx, this);
	end_glows(ctx, this, t);
	end_road(ctx, this);
	end_heli(ctx, this, t);
	end_heli_shards(ctx, this, t);
	end_logo(ctx, this);
	end_stars(ctx, this);
}
