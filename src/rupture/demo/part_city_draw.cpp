#include "part_city_draw.h"

#include <math.h>

#include "../core/gl1.h"
#include "scene.h"

extern double g_unk_b1e698;   /* 0x00b1e698, part_video.cpp */

static const double k_pi = 3.141592653589793;   /* 0x004f36e8 */

/* OURS. x < 0 -> 0, the inline test the code makes. */
double city_max0(double x)
{
	return x < 0.0 ? 0.0 : x;
}

/* OURS. The object-matrix opening every mesh gets: push, TEXTURE, identity. */
void city_obj_begin(void)
{
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
}

/* OURS. The texture matrix handed to the pair as ModelViewObject, back to MODELVIEW. */
void city_obj_end(Shader_Ctx *s, int pair)
{
	Shader_SetTextureMatrix(s, pair, 0xe);
	gl1::MatrixMode(gl1::MODELVIEW);
}

/*
 * OURS. part_city's lit bracket, part_boat's shape: no push round
 * maybe_Scene_DrawLit, no Shader_DisableProfiles before it, bg08 (slot
 * texBase + 0x5a) as both of its textures, then the pair bound and
 * GL_CULL_FACE on. The demo passes *(double*)(ctx+0x19a0) as the time;
 * maybe_Scene_DrawLit ignores it.
 */
void city_lit(Rupture_Ctx *ctx, part_city *p, int pair)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   bg = p->texBase + 0x5a;

	ctx->v1920.kind = 1;
	ctx->v1920.x = 0.2f;                               /* 0x004f3754 */
	ctx->v1930.kind = 1;
	ctx->v1920.y = -0.3f;                              /* 0x004f4878 */
	ctx->v1930.x = 1.0f;
	ctx->v1920.z = 1.0f;
	ctx->litFlag = 0.0f;
	ctx->v1930.y = 1.0f;
	ctx->d1918 = 0.0;
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

/*
 * OURS. The city pairs' tick, -1 -> 1 over t = 0..2.5, then from t = 3
 * again over t = 3.5..13.5; and ttime 100.
 */
static void city_tick(Shader_Ctx *s, int pair, double t)
{
	double v;

	v = Clamp01((t - 0.0) * 0.4);
	v = (v + v) - 1.0;
	Shader_SetParam1f(s, pair + 1, "tick", &v);
	if (t >= 3.0) {
		v = Clamp01(((t - 3.5) + 4.0) * 0.1);
		v = (v + v) - 1.0;
		Shader_SetParam1f(s, pair + 1, "tick", &v);
	}
	v = 100.0;
	Shader_SetParam1f(s, pair + 1, "ttime", &v);
}

/*
 * ORIGIN 0x00471a30..0x00471f04, the opening: the heli pair's ttime 0,
 * the projection, 55 degrees narrowed by five terms in t, the aniso2h
 * pair's planecut, and the clear to a blue-grey fading in over
 * t = 6..8 and out over 52.5..54.5.
 */
static void city_open(Rupture_Ctx *ctx, part_city *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	double      v, c, one, two, fov, a, b, k;

	v = 0.0;
	Shader_SetParam1f(s, p->pairHeli + 1, "ttime", &v);
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
	c = Clamp01(((t - 10.0) + 0.6) * 0.3);
	fov = fov - sin(c * k_pi) * 11.0;
	if (t < 32.0) {
		fov = fov - Clamp01((t - 30.5) * 0.3) * 50.0;
	}
	if (t >= 32.0) {
		fov = fov - (1.0 - Clamp01((t - 31.5) * 0.3)) * 30.0;
	}
	gl1::Perspective((fov + 55.0) + ctx->fovOffset, (double)ctx->width / ctx->height, ctx->nearZ,
	                 ctx->farZ);
	v = 11.2;                                          /* 0x004f4930 */
	Shader_SetParam1f(s, p->pairAniso2h + 1, "planecut", &v);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	Shader_DisableProfiles(s);
	glDisable(GL_CULL_FACE);
	a = Clamp01((t - 6.0) * 0.5);
	b = Clamp01((t - 52.5) * 0.5);
	k = a * 0.15 - 0.15 * b;
	glClearColor((float)(k * 0.1), (float)(0.13 * k), (float)(0.16 * k), 1.0f);
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

/* ORIGIN 0x00471f04..0x0047208e, the sun: one additive blob41b quad of size 21 at the anchor. */
static void city_sun(Rupture_Ctx *ctx, part_city *p)
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
	h = 21.0;
	w = 21.0;
	zero = 0.0;
	Gfx_DrawQuad(&zero, &w, &h, 1);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x0047208e..0x004722de, two cloudt2 glows of size 60 at the
 * anchor, 30 apart and wobbling in x. The eye camera is the demo's
 * inline gluLookAt here, not a call.
 */
static void city_glows(Rupture_Ctx *ctx, part_city *p, double t)
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
		gl1::Translatef((float)(sin(dk * 4.0 + t * 0.8) * 11.0 + (double)kk), 0.0f, 0.0f);
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
 * ORIGIN 0x004722de..0x0047260e, the city pair: city1.obj before t = 32
 * and city_less.obj after, black, then citywater.obj in (0, 0.3, 0.7).
 */
static void city_ground(Rupture_Ctx *ctx, part_city *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;

	city_lit(ctx, p, p->pairCity);
	city_tick(s, p->pairCity, t);
	Crt_Srand(0x409bfa);
	city_obj_begin();
	city_obj_end(s, p->pairCity);
	gl1::Color3f(0.0f, 0.0f, 0.0f);
	Obj_Draw(t >= 32.0 ? &p->cityLess : &p->city1);
	gl1::PopMatrix();
	city_obj_begin();
	city_obj_end(s, p->pairCity);
	gl1::Color3f(0.0f, 0.3f, 0.7f);
	Obj_Draw(&p->citywater);
	gl1::PopMatrix();
}

/* ORIGIN 0x0047260e..0x00472a44, from t = 3 five tablet.obj copies placed by hand, on the city2 pair. */
static void city_tablets(Rupture_Ctx *ctx, part_city *p, double t)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	int         i;

	if (!(t >= 3.0)) {
		return;
	}
	for (i = 0; i < 5; i++) {
		city_lit(ctx, p, p->pairCity2);
		city_tick(s, p->pairCity2, t);
		Crt_Srand(0x409bfa);
		city_obj_begin();
		if (i == 0) {
			gl1::Translatef(104.0f, 295.4f, -106.2f);
			gl1::Rotatef(-128.0f, 0.0f, 1.0f, 0.0f);
		} else if (i == 1) {
			gl1::Translatef(-51.0f, 108.8f, -561.0f);
			gl1::Rotatef(-90.0f, 0.0f, 1.0f, 0.0f);
		} else if (i == 2) {
			gl1::Translatef(52.8f, 104.4f, -484.8f);
		} else if (i == 3) {
			gl1::Translatef(-125.0f, 266.0f, 157.0f);
			gl1::Rotatef(50.0f, 0.0f, 1.0f, 0.0f);
		} else if (i == 4) {
			gl1::Translatef(140.2f, 162.4f, 12.6f);
		}
		city_obj_end(s, p->pairCity2);
		gl1::Color3f(0.0f, 0.0f, 0.0f);
		Obj_Draw(&p->tablet);
		gl1::PopMatrix();
	}
}

/*
 * OURS. A shard centre's distance as the code inlines it: the squares
 * rounded to float, summed, rounded, rooted, rounded. flat drops y for
 * a float 0 * 0.
 */
static float shard_len(const Vec4 *d, bool flat)
{
	float xx = d->x * d->x, yy = flat ? 0.0f * 0.0f : d->y * d->y, zz = d->z * d->z;

	return (float)sqrt((double)(float)(((double)xx + yy) + zz));
}

/*
 * ORIGIN 0x00472a44..0x004739c7, from t = 32 citycrash.obj's shards on
 * the city pair: each seeked by how far its rest centre lies from
 * (-14, 3.1, -1.32) + (-107, 106, -403), the nearer the sooner, and
 * before t = 36 not at all beyond 108 across; then drawn as four
 * triangles each, with 100 skipped at the front and 200 at the back
 * for 45 <= t < 53.99. From t = 45 a shard that has moved further than
 * sqrt(231) from rest is drawn culled.
 */
static void city_crash(Rupture_Ctx *ctx, part_city *p, double t)
{
	Shader_Ctx  *s = &ctx->gfx.shader;
	const Vec4   P0 = Vec4_Make(-14.0f, 3.1f, -1.32f), Q = Vec4_Make(-107.0f, 106.0f, -403.0f);
	const Vec4   zero = Vec4_Make(0.0f, 0.0f, 0.0f);
	const float  one = 1.0f;
	std::vector<Shard> &sh = p->citycrashShards;
	double       v, w;
	float        len2;
	Vec4         d, R, t0, t1, V[4], D;
	unsigned     i, start, skip, end;
	int          k;

	if (!(t >= 32.0)) {
		return;
	}
	city_lit(ctx, p, p->pairCity);
	Crt_Srand(0x409bfa);
	city_obj_begin();
	city_obj_end(s, p->pairCity);
	gl1::Color3f(0.0f, 0.0f, 0.0f);
	glDisable(GL_CULL_FACE);
	for (i = 0; i < (unsigned)sh.size(); i++) {
		d = sh[i].rest;
		Vec4_Sub(&d, &P0, &d);
		Vec4_Sub(&d, &Q, &R);
		v = (double)shard_len(&R, false) * 0.010000000000000002;   /* 0x004f4f58 */
		Vec4_Sub(&d, &Q, &R);
		if (t < 36.0 && shard_len(&R, true) > 108.0) {
			v = 100000.0;
		}
		if (t >= 42.0) {
			v = v + 2.9;
		}
		if (t >= 45.0) {
			v = v + 7.0;
		}
		w = city_max0(((((((((t + 0.2) - 3.0) - 55.0) + 47.0) - 0.0) - 20.0) - 4.3) + 2.6) - v * 1.1);
		Shard_Seek(&sh[i], w * 4.0 * 10.0 * 0.27);
	}
	glDisable(GL_CULL_FACE);
	skip = 0;
	start = 0;
	if (t < 53.99 && t >= 45.0) {
		skip = 300;
		start = 100;
	}
	/* Divergence: stops at 0 where the demo's unsigned bound would wrap (city.txt 4.1). */
	end = (unsigned)sh.size() > skip ? (unsigned)sh.size() - skip : 0;
	gl1::BatchBegin();   /* OURS: one draw for the loop, docs/subsystems/render.txt */
	for (i = start; i < end; i++) {
		Shard *h = &sh[i];

		for (k = 0; k < 4; k++) {
			Vec4_Add(&h->turned[k], &h->pos, &t0);
			Vec4_Add(&t0, &zero, &t1);
			Vec4_Scale(&t1, &one, &V[k]);
		}
		Vec4_Sub(&h->rest, &h->pos, &D);
		if (t >= 45.0) {
			len2 = D.kind == 0 ? 0.0f
			                   : (float)(((double)D.x * D.x + (double)D.y * D.y) + (double)D.z * D.z);
			if (len2 > 231.0) {
				gl1::Enable(GL_CULL_FACE);
			} else {
				gl1::Disable(GL_CULL_FACE);
			}
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
 * ORIGIN 0x00471a30 part_city::DrawScene: the opening and clear, the
 * sun and its glows, the city with its water, the tablets, citycrash's
 * shards; then part_city_draw2.cpp's, in address order: the boat, the
 * helicopter, Hellidist's shards, the stars and the sparks. pass is
 * not read.
 */
void part_city::DrawScene(Rupture_Ctx *ctx, int pass)
{
	City_Draw D;

	(void)pass;
	D.s80 = Vec4_Make(0.0f, 0.0f, 0.0f);
	D.t22 = t >= 22.0;
	city_open(ctx, this, t);
	city_sun(ctx, this);
	city_glows(ctx, this, t);
	city_ground(ctx, this, t);
	city_tablets(ctx, this, t);
	city_crash(ctx, this, t);
	city_boat(ctx, this, &D);
	city_heli(ctx, this, &D);
	city_heli_shards(ctx, this, &D);
	city_stars(ctx, this);
	city_sparks(ctx, this, &D);
}
