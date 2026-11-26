#include "part_end_draw.h"

#include <math.h>

#include "../core/gl1.h"
#include "scene.h"

/*
 * OURS. part_end's lit bracket before the logo, city_lit's shape on
 * part_end's slots: bg08 (texBase + 0x5a) as both textures, the pair
 * bound, GL_CULL_FACE on.
 */
static void logo_lit(Rupture_Ctx *ctx, part_end *p, int pair)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	const int   bg = p->texBase + 0x5a;

	ctx->v1920.kind = 1;
	ctx->v1920.x = 0.2f;                               /* 0x004f3754 */
	ctx->v1930.kind = 1;
	ctx->v1920.y = -0.3f;                              /* 0x004f4878 */
	ctx->v1930.x = 1.0f;
	ctx->litFlag = 0.0f;
	ctx->v1930.y = 1.0f;
	ctx->v1920.z = 1.0f;
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
 * ORIGIN 0x0047c503..0x0047d8e6, outputobj.obj's shards, every frame,
 * on the heli2 pair with culling off and the object moved to
 * (0, 2, -19.5): each seeked by min(60 - 6 t + 3 (0.8 |rest - P0| +
 * rest.y) + 21 r, 60) - 0.5 (t + 44), running back up Init's
 * recorded fall into the logo; strong 0.4 fading to 0 over t = 7..11.
 * Each tetrahedron goes from orange (1, 0.3, 0) to grey
 * (0.7, 0.7, 0.6) as |pos - its corners' mean| closes from 16.7 to 0.
 */
void end_logo(Rupture_Ctx *ctx, part_end *p)
{
	static const int    tri[4][3] = { { 0, 1, 2 }, { 0, 1, 3 }, { 0, 2, 3 }, { 1, 2, 3 } };
	Shader_Ctx         *s = &ctx->gfx.shader;
	const int           P = p->pairHeli2;
	const double        t = p->t;
	const Vec4          P0 = Vec4_Make(-14.0f, 3.1f, -1.32f);   /* 0x004f4f74..0x004f4f6c */
	const Vec4          zero = Vec4_Make(0.0f, 0.0f, 0.0f);
	const float         one = 1.0f, quarter = 0.25f;           /* 0x004f4370 */
	const Vec4          orange = Vec4_Make(1.0f, 0.3f, 0.0f);    /* 0x004f47e4 */
	const Vec4          grey = Vec4_Make(0.7f, 0.7f, 0.6f);      /* 0x004f4060, 0x004f3750 */
	std::vector<Shard> &sh = p->outputobjShards;
	double              v, ry, r, c;
	float               xx, yy, zz, len, k, g;
	Vec4                d, a, b, V[4], N[4], col;
	size_t              i;
	int                 f, j;

	logo_lit(ctx, p, P);
	Crt_Srand(0x409bfa);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::Translatef(0.0f, 2.0f, -19.5f);               /* 0x004f36f0, 0x004f5320 */
	Shader_SetTextureMatrix(s, P, 0xe);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::Color3f(0.7f, 0.7f, 0.6f);
	glDisable(GL_CULL_FACE);
	v = 0.2;                                           /* 0x004f4570 */
	Shader_SetParam1f(s, P + 1, "strong", &v);
	v = 0.4 - Clamp01((t - 7.0) * 0.25) * 0.4;
	Shader_SetParam1f(s, P + 1, "strong", &v);
	for (i = 0; i < sh.size(); i++) {
		ry = (double)sh[i].rest.y;
		d = sh[i].rest;
		Vec4_Sub(&d, &P0, &d);
		r = Math_Random() * 3.0 * 7.0;
		Math_Random();
		xx = d.x * d.x;
		yy = d.y * d.y;
		zz = d.z * d.z;
		len = (float)sqrt((double)(float)(((double)xx + yy) + zz));
		v = (((p->seqHead[2] * 0.0 + 60.0) - t * 6.0) + ((double)len * 0.8 + ry) * 3.0) + r;
		if (60.0 < v) {
			v = 60.0;
		}
		v = v - (t - -44.0) * 0.5;                     /* 0x004f5318 */
		Shard_Seek(&sh[i], v);
	}
	gl1::BatchBegin();   /* OURS: one draw for the loop, docs/subsystems/render.txt */
	for (i = 0; i < sh.size(); i++) {
		const Shard *h = &sh[i];

		k = (float)(p->seqHead[2] * 0.0 + 1.0);
		for (j = 0; j < 4; j++) {
			Vec4_Scale(&h->turned[j], &k, &a);
			Vec4_Add(&a, &h->pos, &b);
			Vec4_Add(&b, &zero, &a);
			Vec4_Scale(&a, &one, &V[j]);
			N[j] = h->turnedN[j];
		}
		Vec4_Add(&h->turned[0], &h->turned[1], &a);
		Vec4_Add(&a, &h->turned[2], &b);
		Vec4_Add(&b, &h->turned[3], &a);
		Vec4_Scale(&a, &quarter, &b);
		Vec4_Sub(&h->pos, &b, &d);
		yy = d.y * d.y;
		xx = d.x * d.x;
		zz = d.z * d.z;
		len = (float)sqrt((double)(float)(((double)yy + xx) + zz));
		c = Clamp01((double)len * 0.6 * 0.1);          /* 0x004f3e30, 0x004f3e38 */
		g = (float)(1.0 - c);
		col = orange;
		Vec4_Sub(&grey, &col, &a);
		Vec4_Scale(&a, &g, &b);
		Vec4_Add(&col, &b, &col);
		gl1::Color3f(col.x, col.y, col.z);
		gl1::Begin(gl1::TRIANGLES);
		for (f = 0; f < 4; f++) {
			gl1::Normal3f(N[f].x, N[f].y, N[f].z);
			for (j = 0; j < 3; j++) {
				gl1::Vertex3f(V[tri[f][j]].x, V[tri[f][j]].y, V[tri[f][j]].z);
			}
		}
		gl1::End();
	}
	gl1::BatchEnd();
	gl1::PopMatrix();
}

/*
 * ORIGIN 0x0047d8e6 to DrawScene's end, 1100 grey points at radius 720
 * round the eye, the whole sphere, under a 65 / 1 / 1000 projection: as
 * city_stars but for two more randoms per point and no fabs on y. The
 * modelview push is popped in GL_PROJECTION mode, and the brightness
 * ease is computed and replaced by 1.
 */
void end_stars(Rupture_Ctx *ctx, part_end *p)
{
	Gfx_Ctx     *g = &ctx->gfx;
	const double t = p->t;
	const double aspect = (double)ctx->width / ctx->height;
	const float  k1800 = 1800.0f, k04 = 0.4f;          /* 0x004f4dcc, 0x004f47fc */
	double       k, r, e, one;
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
	e = Clamp01(t * 0.3);
	one = 1.0;
	Math_Ease(&e, &one);
	k = 1.0;
	gl1::PointSize(2.0f);
	gl1::Begin(gl1::POINTS);
	for (n = 1100; n != 0; n--) {
		Math_Random();
		Math_Random();
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
		V.y = b;
		V.z = z;
		Vec4_Normalize(&V);
		Vec4_Scale(&V, &k1800, &S);
		Vec4_Scale(&S, &k04, &V);
		Vec4_Add(&V, &ctx->eye.eye, &V);
		gl1::Vertex3f(V.x, V.y, V.z);
	}
	gl1::End();
}
