#include "part_video.h"

#include <math.h>
#include <string.h>

#include "../core/gl1.h"
#include "scene.h"

Obj_Mesh      g_room2Obj;
double        g_unk_b1e698;   /* 0x00b1e698, zeroed by DrawScene, read elsewhere */

/* OURS. The inline clamp to [0, 1] every Frame and DrawScene repeats. */
static double clamp01(double v)
{
	if (v > 1.0) {
		return 1.0;
	}
	if (v < 0.0) {
		return 0.0;
	}
	return v;
}

/* ORIGIN 0x00446730 part_video::part_video. */
part_video::part_video()
{
	Path_Construct(&path);
	t = 0.0;
	tCopy = 0.0;
	frames = 0;
	seqHead[0] = seqHead[1] = seqHead[2] = 0.0;
	video.plm = NULL;
}

/*
 * ORIGIN 0x004b10c0 part_video::Init. The video is one MPEG-1 file
 * decoded into one texture, not 838 PNGs in slots 10000 + i (CLAUDE.md,
 * the decisions). box.3ds is never drawn (DrawScene), so only its material,
 * meshes/marmaro.jpg, is loaded, into slot (7000 + 1000) * 100 where
 * maybe_Mesh_Draw looks [inferred]. pattern5, the vertex_boat +
 * fragment_train pair (0x1b7a) and the blur256FOV (0x1b75) and
 * Blur_circular (0x1b6d) passes are loaded and never bound
 * (effects/video.txt 2).
 */
void part_video::Init(Rupture_Ctx *ctx)
{
	Gfx_Ctx    *g = &ctx->gfx;
	Shader_Ctx *s = &g->shader;
	int         w = ctx->width, h = ctx->height;
	Vec4        off = { 1, 0.0f, 57.0f, -16.4f };     /* 0x004f6c40, 0x004f6c44 */
	Vec4        behind = { 1, 80.0f, 60.0f, -70.0f };
	Vec4        down = { 1, 0.0f, -150.0f, 0.0f };    /* 0x004f47d8 */
	const float k250 = 250.0f, k120 = 120.0f;         /* 0x004f6ce8, 0x004f66d8 */
	Vec4        d, r, dn, sum, v;
	double      r1, r2, r3;
	size_t      i;

	t = 0.0;
	tCopy = 0.0;
	texBase = 7000;
	frames = 0;
	texNormal = 0;
	texColour = 0xb;
	texQuarter = 2;
	texGlow = 3;
	pairRoom = 0x1b82;
	pairTv = 0x1b5a;
	passBlur = 0x1b6f;
	passBend = 0x1b77;
	passWire = 0x1b73;
	Tex_CreateTarget(g, texQuarter, w / 4, h / 4);
	Tex_CreateTarget(g, texGlow, w / 2, h / 2);
	Tex_CreateTarget(g, texNormal, w, h);
	Tex_CreateTarget(g, texColour, w, h);
	Fbo_AttachTexture(g, 0, 0, texNormal);
	Fbo_AttachTexture(g, 0, 1, texColour);
	Fbo_AttachTexture(g, 1, 0, texQuarter);
	Fbo_AttachTexture(g, 2, 0, texGlow);
	g->tex[texBase + 0x0a] = Tex_Load("textures/pattern5.jpg");
	g->tex[texBase + 0x5a] = Tex_Load("textures/bg08.jpg");
	g->tex[texBase + 0x14] = Tex_LoadWithAlpha("textures/blob41b.jpg", "textures/blob41b.jpg");
	g->tex[texBase + 0x1e] = Tex_Load("textures/cloudt2.jpg");
	g->tex[(7000 + 1000) * 100] = Tex_Load("meshes/marmaro.jpg");
	Obj_Load(&hex4, gl1::GenLists(1), "meshes/hex4.obj");
	Obj_Load(&walls2, gl1::GenLists(1), "meshes/walls2.obj");
	Obj_Load(&walls, gl1::GenLists(1), "meshes/walls.obj");
	Obj_Load(&g_room2Obj, gl1::GenLists(1), "meshes/room2.obj");
	videoFrames = 838;
	Video_Open(&video, "video/test_video2.mpg");

	Obj_MakeShards(&walls, &shards, 0.05);
	Crt_Srand(0xfcdd0218u);
	for (i = 0; i < shards.size(); i++) {
		Shard *sh = &shards[i];

		Math_Random();
		Vec4_Sub(&sh->rest, &off, &d);
		Vec4_Sub(&d, &behind, &d);
		r1 = Math_Random();
		r2 = Math_Random();
		r3 = Math_Random();
		r.kind = 1;
		r.x = (float)(r3 + r3 - 1.0);
		r.y = (float)(r2 + r2 - 1.0);
		r.z = (float)(r1 + r1 - 1.0);
		Vec4_Scale(&r, &k120, &r);
		Vec4_Normalize(&d);
		Vec4_Scale(&d, &k250, &dn);
		Vec4_Add(&dn, &r, &sum);
		Vec4_Add(&sum, &down, &v);
		if (sh->rest.x < -120.0) {                     /* 0x004f6ce0 */
			v.kind = 1;
			v.x = (float)(Math_Random() * 20.0 + 200.0);
			v.y = 0.0f;
			v.z = 70.0f;                               /* 0x004f6cdc */
		}
		sh->a.kind = 1;
		sh->a.x = 0.0f;
		sh->a.y = 0.0f;
		sh->a.z = -90.0f;                              /* 0x004f4708 */
		sh->v = v;
	}
	Obj_BakeShards(&shards, 12, 20);

	Shader_LoadPair(s, "cgeffects/vertex_projection_anisotropic.cpp",
	                "cgeffects/fragment_projection_anisotropic2hh.cpp", pairTv);
	Shader_LoadPair(s, "cgeffects/vertex_projection_anisotropic4.cpp",
	                "cgeffects/fragment_projection_anisotropic2box.cpp", pairRoom);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_train.cpp", 0x1b7a);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256.cpp", passBlur);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256FOV.cpp", 0x1b75);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_Blur_circular.cpp", 0x1b6d);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_bend3.cpp", passBend);
	Shader_LoadFragmentOnly(s, "cgeffects/fragment_wireframe.cpp", passWire);
	Path_Load(&path, "meshes/try1.sav");
}

/*
 * ORIGIN 0x004b19c0 part_video::Frame (effects/video.txt 5): the gate,
 * the clock, the near plane, the cameras and the shake from try1.sav,
 * then DrawScene and the post chain.
 */
void part_video::Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq)
{
	Gfx_Ctx *g = &ctx->gfx;
	Camera  *eye = &ctx->eye;
	Camera   cam2;
	double   a1, a2, a3, b1, b2, b3, c1, c2, c3, v, x, E, q, T2, e2, L, one = 1.0, two = 2.0;
	double   w, A, B, rightV, upV, alpha, zero = 0.0;
	float    Bf, px, py;
	Vec4     F, Fc, R, s1, s2, lx;
	unsigned idx;
	int      W = ctx->width, H = ctx->height;
	const float lightStep = -14.0f;                    /* 0x004f4f74 */

	if (seq->relayMode == 1 && (seq->started[0] == 0 || seq->done[0] == 1)) {
		return;
	}
	if (29.0 <= t) {                                   /* 0x004f6120 */
		*dt = *dt * 1.1;                               /* 0x004f4648 */
	}
	Attrib_Push();
	ctx->seq = seq;
	memcpy(seqHead, seq->head, sizeof seqHead);
	dtCopy = *dt;
	t = *dt + t;
	if (seq->override28 != 0) {
		t = seqHead[0] * 3.0;
	}
	frames++;
	tCopy = t;
	seq->partClock = t;
	x = clamp01((t - 20.0) * 0.3);
	ctx->nearZ = 36.0 - x * 0.6 * 50.0;
	ctx->farZ = 750.0;
	if (39.0 <= t) {                                   /* 0x004f4d78 */
		Attrib_Pop();
		g_partTimeCarry = t - 39.0;
		seq->done[0] = 1;
		seq->started[1] = 1;
		return;
	}
	gl1::PushMatrix();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Fbo_Bind(g, 0, 2);

	a1 = sin(seqHead[0]) * (((seqHead[2] - 1.2) + 2.0) * 15.0);
	a2 = seqHead[1] * 8.0;
	a3 = cos(seqHead[0]) * ((seqHead[2] + 0.8) * 15.0);
	b1 = 0.0;
	b2 = 1.0;
	Camera_Set(eye, &a1, &a2, &a3, &zero, &zero, &zero, &zero, &b2, &b1);
	a1 = 20.0 * seqHead[0];
	a2 = 100.0;
	a3 = 20.0 * seqHead[1];
	Camera_Set(eye, &a1, &a2, &a3, &zero, &zero, &zero, &zero, &b2, &b1);
	v = t * 10.0;
	a1 = 320.0 - v;                                    /* 0x004f6da8 */
	a2 = 1.5 * v + 120.0;                              /* 0x004f3758, 0x004f4710 */
	b3 = -50.0;
	c2 = -0.2;                                         /* 0x004f4900 */
	Camera_Set(eye, &a1, &a2, &zero, &zero, &zero, &b3, &zero, &c2, &one);

	E = 0.0;
	if (29.0 <= t) {
		x = clamp01((t - 29.0) * 0.3);
		E = Math_Ease(&x, &two) * 20.0;
	}
	q = 2.0 - ((t - 7.0) + E) * 0.15;                  /* 0x004f4ce0, 0x004f4828 */
	x = t - 27.0;
	x = (x < 0.0 ? 0.0 : x) * 0.5;
	T2 = t - Math_SoftRamp(&x);
	x = clamp01((t - 22.0) * 0.15);
	e2 = Math_Ease(&x, &one);
	c1 = 0.1;
	c3 = 0.0 * sin(T2);
	a1 = q * 50.0;
	a2 = 390.0 - (T2 - 4.0) * 9.0;                     /* 0x004f6d98 */
	a3 = e2 * -19.5;                                   /* 0x004f6da0 */
	Camera_Init(&cam2, &a1, &a2, &a3, &zero, &zero, &zero, &c3, &c1, &one);
	x = clamp01((t - 5.0) * 0.1);
	L = Math_Ease(&x, &one);
	Camera_Lerp(eye, &L, eye, &cam2);

	Vec4_Sub(&eye->target, &eye->eye, &F);
	Fc = F;
	Vec4_Normalize(&Fc);
	Vec4_Normalize(&F);
	Vec4_Cross(&F, &eye->up, &R);
	idx = (unsigned)(long)(t * 35.0) % (unsigned)(path.points.size() - 1);   /* 0x004f47c0 */
	x = clamp01((t - 29.5) * 0.3);
	w = sin(x * 3.141592653589793) * 3.0;
	A = 0.011 + w * 0.6 * 0.011;                       /* 0x004f6080 */
	px = path.points[idx].x;
	py = path.points[idx].y;
	rightV = px * 1.1 * A * 0.01 * 0.8;
	upV = py * 1.1 * A * 0.01 * 0.8;
	B = (w + 1.0) * A;
	Bf = (float)B;
	Vec4_Scale(&R, &px, &s1);
	Vec4_Scale(&s1, &Bf, &s2);
	Vec4_Add(&eye->target, &s2, &eye->target);
	Vec4_Scale(&eye->up, &py, &s1);
	Vec4_Scale(&s1, &Bf, &s2);
	Vec4_Add(&eye->target, &s2, &eye->target);
	eye->up.x = (float)(sin(path.a[idx] * 1.7188733853924696 * B) * 0.5 * 0.0 + eye->up.x);
	tTen = t * 10.0;
	ctx->light = *eye;
	lx.kind = 1;
	lx.x = 30.0f;                                      /* 0x004f47dc */
	lx.y = 0.0f;
	lx.z = 0.0f;
	Vec4_Scale(&lx, &lightStep, &s1);
	Vec4_Add(&ctx->light.eye, &s1, &ctx->light.eye);

	DrawScene(ctx, 1);
	Fbo_Resolve(g, 0, W, H, 2);
	Fbo_Unbind();
	glDisable(GL_DEPTH_TEST);
	Post_RunPass(g, passBlur, 2, 1, W / 2, H / 2, texNormal, texGlow, -1, -1);
	Post_RunPass(g, passWire, 0, 1, W, H, texNormal, texNormal, texGlow, texColour);
	Post_RunPass(g, passBlur, 2, 6, W / 2, H / 2, texNormal, texGlow, -1, -1);
	x = clamp01((t - 2.0) * 0.4);
	alpha = (1.0 - Math_Ease(&x, &one)) * 0.5;
	x = (t - 35.0) * 0.5;
	x = x < 0.0 ? 0.0 : x;
	alpha = alpha + 0.15 + Math_SoftRamp(&x) * 0.6;
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
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	Gfx_DrawQuad(&zero, &one, &one, 1);
	gl1::PopMatrix();
	gl1::PopMatrix();
	Attrib_Pop();
	seq->partEnd = t;
	glDisable(GL_BLEND);
}

/* OURS. DrawScene's multip term: 2 sin(pi clamp((t - a) b)), clamped. */
static double multip_term(double t, double a, double b)
{
	double c = clamp01((t - a) * b);
	double v = sin(3.141592653589793 * c);

	return clamp01(v + v);
}

/*
 * ORIGIN 0x004aebb2..0x004aef09, DrawScene's TV: the hex4 grid under
 * the pair this+0x115c, forty columns of twenty-one cells, each lifted
 * by a travelling sine, tipped by a seeded random turn and scaled in
 * by Math_Ease (effects/video.txt 3.1). i % 2 is C's remainder, -1 for
 * odd negative i.
 */
static void hex_grid(Shader_Ctx *s, int pair, Obj_Mesh *hex, double t, double s358)
{
	double one = 1.0, p, jd, ang, e;
	float  xi, fi2, fj, r, rz, sc;
	int    i, j;

	Crt_Srand(0x409bfa);
	for (i = -20; i < 20; i++) {
		p = (double)(i % 2);
		xi = (float)(i * 2.6775);
		fi2 = (float)i * (float)i;
		jd = -9.0;
		for (j = -9; j < 12; j++) {
			gl1::PushMatrix();
			gl1::MatrixMode(gl1::TEXTURE);
			gl1::LoadIdentity();
			fj = (float)j;
			gl1::Translatef(xi, (float)(sin(t * 2.2 * 0.9 + jd * 0.02) * 0.1 + s358 * 0.2),
			                (float)(2.9925 * jd + 0.0 + p));
			r = (float)sqrt((float)(fj * fj) + fi2 + 0.0f);
			ang = r * 1.2;
			ang = sin(ang - (Math_Random() + 3.0) * (t * 0.9)) * 30.0;
			rz = (float)(Math_Random() * 2.0 - 1.0);
			gl1::Rotatef((float)ang, 1.0f, -0.2f, rz);
			e = clamp01(((t - 2.7) - ang * 0.05) * 0.25);
			e = Math_Ease(&e, &one);
			if (t > 10.0) {
				e -= clamp01((t - 32.0) * 0.2);
			}
			glEnable(GL_CULL_FACE);
			sc = (float)(e * 1.1 * 1.4);
			gl1::Scalef(sc, sc, sc);
			Shader_SetTextureMatrix(s, pair, 0xe);
			gl1::MatrixMode(gl1::MODELVIEW);
			gl1::Color3f(0.7f, 0.7f, 0.6f);
			Obj_Draw(hex);
			gl1::PopMatrix();
			jd = (double)(j + 1);
		}
	}
}

/* OURS. The length the shard loop inlines: float squares, one sqrt. */
static float len3(const Vec4 *v)
{
	float xx = v->x * v->x, yy = v->y * v->y, zz = v->z * v->z;

	return (float)sqrt(xx + yy + zz);
}

/*
 * ORIGIN 0x004af1c0..0x004afa35, DrawScene's shard loop: each shard's
 * record index s from t and its rest centre (effects/video.txt 3.4).
 * The factor 20 is Math_SoftRamp(...) * 0 + 1 tested against k_dbl_20,
 * which always picks 20.
 */
static void shards_seek(std::vector<Shard> &shards, double t)
{
	static const Vec4 off = { 1, 0.0f, 57.0f, -16.4f };      /* 0x004f6c40, 0x004f6c44 */
	static const Vec4 off2 = { 1, 0.0f, 57.0f, 16.4f };      /* 0x004f6c2c */
	static const Vec4 p1 = { 1, -127.0f, -66.5f, 27.0f };    /* 0x004f6c3c.. */
	static const Vec4 p2 = { 1, 90.0f, -60.0f, 60.0f };
	static const Vec4 p3 = { 1, 117.0f, -100.0f, 3.0f };
	static const Vec4 p4 = { 1, -3.0f, -70.0f, 0.0f };
	static const Vec4 p5 = { 1, -28.0f, -49.0f, 27.0f };     /* 0x004f6c24.. */
	double            two = 2.0, q, m1, m2, m3, m, b, r, c, C, A, B, T, mn, x;
	Vec4              e1, e2, e3, e4, d5;
	size_t            i;

	for (i = 0; i < shards.size(); i++) {
		Shard *sh = &shards[i];

		Vec4_Sub(&sh->rest, &off, &e1);
		Vec4_Sub(&e1, &p1, &e1);
		Vec4_Sub(&sh->rest, &off, &e2);
		Vec4_Sub(&e2, &p2, &e2);
		Vec4_Sub(&sh->rest, &off, &e3);
		Vec4_Sub(&e3, &p3, &e3);
		Vec4_Sub(&sh->rest, &off2, &e4);
		Vec4_Sub(&e4, &p4, &e4);
		Vec4_Sub(&p5, &e1, &d5);
		b = len3(&d5) * 0.4 * 0.1;
		r = b * (b * b);
		c = clamp01((t - 24.0) * 0.35);
		C = (t - 24.0) * 15.0 * (c / (r + 0.1));
		m1 = len3(&e2) * 0.4 * 0.1;
		m2 = len3(&e3) * 0.4 * 0.1 * 1.4;
		m3 = len3(&e4) * 0.4 * 0.1;
		q = len3(&e1) * 0.4 * 0.2 * 0.5 * 4.4;
		m = m1 < m2 ? m1 : m2;
		if (m3 * 1.48 < m) {
			m = 100000000.0;
		}
		T = (t - -6.0) - -1.0;
		mn = m * 2.2 * 0.2 < q ? m * 2.2 * 0.2 : q;
		A = (T - mn) - -4.0;
		if (A < 0.0) {
			A = 0.0;
		}
		A = A * 20.0 * 0.02;
		B = ((t - 8.0) - m * 4.0) - -4.0;
		if (B < 0.0) {
			B = 0.0;
		}
		B = B * 20.0 * 0.18 * 1.0;
		x = (A > 10.05 ? 10.05 : A) / 10.05;
		Shard_Seek(sh, Math_Ease(&x, &two) * 10.05 + B + C);
	}
}

/*
 * ORIGIN 0x004afa3b..0x004b01ad, DrawScene's shards in one
 * glBegin(GL_TRIANGLES) under GL_TEXTURE = translate(0, 57, -16.4), in
 * glColor3f(0.4, 0.7, 0.9); a shard past s = 110 is not drawn. Four
 * triangles a shard: V0 V1 V2, V0 V1 V3, V0 V2 V3, V1 V2 V3.
 */
static void shards_draw(Shader_Ctx *s, int pair, const std::vector<Shard> &shards)
{
	static const Vec4 zero = { 1, 0.0f, 0.0f, 0.0f };
	static const int  tri[4][3] = { { 0, 1, 2 }, { 0, 1, 3 }, { 0, 2, 3 }, { 1, 2, 3 } };
	Vec4              w, v[4];
	size_t            i;
	int               f, k;

	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::Translatef(0.0f, 57.0f, -16.4f);
	Shader_SetTextureMatrix(s, pair, 0xe);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::Color3f(0.4f, 0.7f, 0.9f);
	Crt_Srand(0x675ff);
	gl1::Begin(gl1::TRIANGLES);
	for (i = 0; i < shards.size(); i++) {
		const Shard *sh = &shards[i];

		if (!((double)sh->s <= 110.00000000000001)) {
			continue;
		}
		for (k = 0; k < 4; k++) {
			Vec4_Add(&sh->turned[k], &sh->pos, &w);
			Vec4_Add(&w, &zero, &v[k]);
		}
		for (f = 0; f < 4; f++) {
			const Vec4 *n = &sh->turnedN[f];

			gl1::Normal3f(n->x, n->y, n->z);
			for (k = 0; k < 3; k++) {
				gl1::Vertex3f(v[tri[f][k]].x, v[tri[f][k]].y, v[tri[f][k]].z);
			}
		}
	}
	gl1::End();
	gl1::PopMatrix();
}

/* ORIGIN 0x004b01ad..0x004b04fb, one room2 copy: the transform both loops share. */
static void room2_copy(Shader_Ctx *s, int pair, float x, float y, int k, float turn)
{
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::Translatef(x, y, -80.0f);
	if (k == 1) {
		gl1::Translatef(0.0f, 0.0f, 60.0f);
	}
	gl1::Scalef(1.0f, 1.0f, 1.0f);
	gl1::Rotatef(turn, 0.0f, 0.0f, 1.0f);
	gl1::Rotatef(90.0f, 0.0f, 1.0f, 0.0f);
	gl1::Rotatef(90.0f, 0.0f, 0.0f, 1.0f);
	Shader_SetTextureMatrix(s, pair, 0xe);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::Color3f(0.4f, 0.7f, 0.9f);
	Obj_Draw(&g_room2Obj);
	gl1::PopMatrix();
}

/* OURS. The preamble DrawScene's two billboard blocks repeat. */
static void billboard_begin(Rupture_Ctx *ctx)
{
	Camera *e = &ctx->eye;

	Shader_DisableProfiles(&ctx->gfx.shader);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	Scene_LookOffset(ctx);
	gl1::LookAt(e->eye.x, e->eye.y, e->eye.z, e->target.x, e->target.y, e->target.z,
	            e->up.x, e->up.y, e->up.z);
	Gfx_ResetState();
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	gl1::Color3f(0.4f, 0.3f, 0.3f);
}

/*
 * ORIGIN 0x004b04fb.., DrawScene's tail to its end: 2500 stars, then
 * before t = 30 a sun billboard, before t = 12 three glows beside it,
 * and a sun disc fading in from t = 12 to out at t = 26. The modelview
 * push at its head is never popped and the pop that follows runs in
 * GL_PROJECTION, an underflow GL ignores; gl1 ignores it too.
 */
static void sky(Rupture_Ctx *ctx, int texBase, double t, double s358)
{
	Gfx_Ctx    *g = &ctx->gfx;
	Shader_Ctx *s = &g->shader;
	Camera     *e = &ctx->eye;
	double      aspect = (double)ctx->width / ctx->height, one = 1.0, zero = 0.0;
	double      E, gr, a, ang, f, x, fifty = 50.0, size150 = 150.0, size117 = 117.0;
	const float k400 = 400.0f;                         /* 0x004f4638 */
	Vec4        v, P;
	int         i, k;

	Crt_Srand(0xa567);
	Shader_DisableProfiles(s);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	Scene_LookOffset(ctx);
	gl1::LookAt(e->eye.x, e->eye.y, e->eye.z, e->target.x, e->target.y, e->target.z,
	            e->up.x, e->up.y, e->up.z);
	Gfx_ResetState();
	gl1::PushMatrix();
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(g, texBase + 0x14);
	gl1::PopMatrix();
	glDisable(GL_BLEND);
	gl1::Disable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(65.0, aspect, 1.0, 1000.0);
	gl1::PopMatrix();
	Gfx_ResetState();
	Shader_DisableProfiles(s);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(65.0, aspect, 1.0, 1000.0);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	Scene_LookOffset(ctx);
	gl1::LookAt(e->eye.x, e->eye.y, e->eye.z, e->target.x, e->target.y, e->target.z,
	            e->up.x, e->up.y, e->up.z);
	glEnable(GL_DEPTH_TEST);

	x = clamp01(t * 0.3);
	E = Math_Ease(&x, &one);
	gl1::PointSize(2.0f);
	gl1::Begin(gl1::POINTS);
	for (i = 0; i < 2500; i++) {
		Math_Random();
		Math_Random();
		gr = (1.0 - clamp01((t - 29.0) * 0.2)) * ((Math_Random() + 0.2) * E) * 0.2;
		gl1::Color3f((float)gr, (float)gr, (float)gr);
		v.kind = 1;
		v.x = (float)(Math_Random() * 2.0 - 1.0);
		v.y = (float)(Math_Random() * 2.0 - 1.0);
		v.z = (float)(Math_Random() * 2.0 - 1.0);
		Vec4_Normalize(&v);
		Vec4_Scale(&v, &k400, &v);
		gl1::Vertex3f(v.x, v.y, v.z);
	}
	gl1::End();

	if (t < 30.0) {
		billboard_begin(ctx);
		gl1::Enable(GL_TEXTURE_2D);
		Tex_Bind(g, texBase + 0x14);
		Crt_Srand(0x286182f);
		P.kind = 1;
		P.x = (float)(Math_Random() * 2.0 - 1.0);
		P.y = (float)(Math_Random() * 2.0 - 1.0);
		P.z = (float)(Math_Random() * 2.0 - 1.0);
		Vec4_Normalize(&P);
		Vec4_Scale(&P, &k400, &P);
		P.z = (float)(P.z + 190.0);                    /* 0x004f5678 */
		glDepthMask(GL_FALSE);
		gl1::Translatef(P.x, P.y, P.z);
		Gfx_DrawQuad(&zero, &fifty, &fifty, 1);
		glDepthMask(GL_TRUE);
		gl1::Disable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		gl1::PopMatrix();
		if (t < 12.0) {
			for (k = 0; k < 3; k++) {
				a = clamp01(t * 0.5);
				gl1::PushMatrix();
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				gl1::Translatef(P.x, P.y, P.z);
				gl1::Translatef(0.0f, 0.0f,
				                (float)(sin(4.0 * k + t * 0.8) * 11.0 + (double)(30 * k)));
				ang = k == 2 ? -40.0 : k * 40.0 - 48.0;
				ang += 170.0;
				gl1::Color3f((float)(0.03 * a), (float)(0.05 * a), (float)(0.08 * a));
				if (k == 2) {
					gl1::Color3f((float)(0.05 * a), (float)(0.04 * a), (float)(0.04 * a));
				}
				gl1::Enable(GL_TEXTURE_2D);
				Tex_Bind(g, texBase + 0x1e);
				Gfx_DrawQuad(&ang, &size150, &size150, 1);
				glDisable(GL_BLEND);
				gl1::PopMatrix();
			}
		}
	}
	if (t < 30.0) {
		billboard_begin(ctx);
		gl1::Enable(GL_TEXTURE_2D);
		Tex_Bind(g, texBase + 0x14);
		Crt_Srand(0x286182f);
		Math_Random();
		Math_Random();
		Math_Random();
		f = clamp01(t - 12.0) - clamp01(t - 26.0);
		gl1::Color3f((float)f, 0.8f, 0.2f);
		glDepthMask(GL_FALSE);
		gl1::Translatef(0.0f + 196.5f, -220.0f + 180.0f, (float)(s358 * 0.0 - 80.0) + 50.0f);
		if (f > 0.0001) {
			Gfx_DrawQuad(&zero, &size117, &size117, 1);
		}
		glDepthMask(GL_TRUE);
		gl1::Disable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		gl1::PopMatrix();
	}
}

/*
 * ORIGIN 0x004ae0a0 part_video::DrawScene (effects/video.txt 3): the
 * projection, multip, the clear, the TV pair's set-up and the hex
 * grid; the room pair's, walls2, the shards and room2; then the sky.
 * maybe_Scene_DrawLit draws no triangles here: the count is scaled by
 * ctx+0x1bd8, set to 0 around each call. The one MPEG texture stands
 * in for frame slot 10000 + frame during the call.
 */
void part_video::DrawScene(Rupture_Ctx *ctx, int pass)
{
	Gfx_Ctx    *g = &ctx->gfx;
	Shader_Ctx *s = &g->shader;
	double      s358 = seqHead[2], x, e, two = 2.0, multip, planecut = 11.2, f, v, w;
	int         frame, slotA, k;

	(void)pass;
	g_unk_b1e698 = 0.0;
	glViewport(0, 0, ctx->width, ctx->height);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	x = clamp01((t - 19.0) * 1.5 * 1.5);
	e = Math_Ease(&x, &two);
	gl1::Perspective(e * -0.0 + 55.0 + ctx->fovOffset, (double)ctx->width / ctx->height,
	                 ctx->nearZ, ctx->farZ);
	multip = ((multip_term(t, 28.0, 0.4) * 0.3
	           + (((multip_term(t, 7.0, 0.6) + 0.0) + multip_term(t, 10.7, 0.45))
	              + multip_term(t, 16.2, 0.55) + multip_term(t, 21.0, 0.5)))
	          + 0.1) + clamp01(t - 29.0) * 3.0;
	Shader_SetParam1f(s, pairTv + 1, "planecut", &planecut);
	Shader_SetParam1f(s, pairTv + 1, "ttime", &t);
	Shader_SetParam1f(s, pairTv + 1, "multip", &multip);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	Shader_DisableProfiles(s);
	glDisable(GL_CULL_FACE);
	f = clamp01((t - 6.0) * 0.5) * 0.15 - 0.15 * clamp01((t - 36.1) * 0.5);
	glClearColor((float)(f * 0.1), (float)(0.13 * f), (float)(0.16 * f), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);
	gl1::Color3f(1.0f, 0.6f, 0.2f);
	ctx->light.eye.x = (float)(s358 * 10.0 + ctx->light.eye.x);

	x = t - 3.0 > 0.0 ? t - 3.0 : 0.0;
	frame = (int)(x * 30.0) % videoFrames;
	slotA = 10000 + frame;
	Video_Show(&video, frame);
	g->tex[slotA] = video.texture;
	ctx->litFlag = 0.0f;
	maybe_Scene_DrawLit(ctx, 7000, slotA, texBase + 0x5a, pairTv, &ctx->eye, &ctx->light, 0.0);
	ctx->litFlag = 1.0f;
	g->tex.erase(slotA);
	Shader_Bind(s, pairTv);
	Shader_Bind(s, pairTv + 1);
	Shader_EnableProfiles(s);
	hex_grid(s, pairTv, &hex4, t, s358);

	ctx->litFlag = 0.0f;
	maybe_Scene_DrawLit(ctx, 7000, texBase + 0x5a, texBase + 0x5a, pairRoom, &ctx->eye,
	                    &ctx->light, 0.0);
	ctx->litFlag = 1.0f;
	Shader_Bind(s, pairRoom);
	Shader_Bind(s, pairRoom + 1);
	Shader_EnableProfiles(s);
	glEnable(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);

	v = (t - 0.0) * 0.2;
	v = v < 0.0 ? 0.0 : v;
	v = v * v;
	if (!(s358 + 22.0 >= t)) {
		w = (t - 22.0 + s358) * 1.6;
		v = 10.0 - pow(w < 0.0 ? 0.0 : w, 0.7);
	}
	Shader_SetParam1f(s, pairRoom + 1, "ttime", &v);
	Crt_Srand(0x409bfa);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::Translatef(0.0f, 57.0f, -16.4f);
	Shader_SetTextureMatrix(s, pairRoom, 0xe);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::Color3f(0.7f, 0.7f, 0.6f);
	Obj_Draw(&walls2);
	gl1::PopMatrix();

	shards_seek(shards, t);
	shards_draw(s, pairRoom, shards);

	v = (t - 5.0) * 0.2;
	v = v < 0.0 ? 0.0 : v;
	if (!(22.0 >= t)) {
		w = (t - 22.0) * 1.6;
		v = 10.0 - pow(w < 0.0 ? 0.0 : w, 0.7);
	}
	Shader_SetParam1f(s, pairRoom + 1, "ttime", &v);
	for (k = 0; k < 3; k++) {
		room2_copy(s, pairRoom, (float)(k * 260.0), -220.0f, k, -10.0f);
	}
	if (t < 11.0) {
		room2_copy(s, pairRoom, -400.0f, (float)(0 * 260.0 + 200.0), 0, -108.0f);
	}

	sky(ctx, texBase, t, s358);
}
