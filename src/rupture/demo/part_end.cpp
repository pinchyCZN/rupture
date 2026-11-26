#include "part_end.h"

#include <math.h>
#include <string.h>

#include "../core/gl1.h"
#include "part_truck.h"
#include "scene.h"

/*
 * ORIGIN 0x00447110 part_end::part_end. [open] The four Obj meshes at
 * +0x3e8..+0x49c and the one at +0x550 that Init never loads, and the
 * container at +0x658 (maybe_StdVector_Construct), are left out.
 */
part_end::part_end()
{
	Path_Construct(&path);
	Track_Construct(&track);
	d3a0 = 0.0;
	cam3a8.eye = Vec4_Make(0.0f, 0.0f, 0.0f);
	cam3a8.target = Vec4_Make(0.0f, 0.0f, 0.0f);
	cam3a8.up = Vec4_Make(0.0f, 0.0f, 0.0f);
	vec1378 = Vec4_Make(0.0f, 0.0f, 0.0f);
	vec13cc = Vec4_Make(0.0f, 0.0f, 0.0f);
	vec1408 = Vec4_Make(0.0f, 0.0f, 0.0f);
}

/*
 * ORIGIN 0x0047f4e0 part_end::Frame: the gate, the clock held at 0 at
 * least, one base camera from the seq head and one window on t, the
 * shake from try1.sav, the light camera, DrawScene, the post chain
 * and the two credit quads (R0.jpg, R1.jpg) for 3 < t < 13. At t = 54
 * the part sets done[6] and started[6] and draws nothing more; no
 * overshoot is carried (effects/end.txt 3.3).
 */
void part_end::Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq)
{
	Gfx_Ctx *g = &ctx->gfx;
	Camera  *eye = &ctx->eye;
	double   h0, h1, h2, r, a, x, F, G, c3, c13, c15, Bd, rightV, upV, alpha, s, hw;
	double   one = 1.0, zero = 0.0, half = 0.5;
	float    x1, z1, ty, tz, ex, fB, px, py, Lf, sf, k14 = -14.0f;
	Vec4     Fv, Fc, R, m1, m2;
	unsigned idx, n;
	int      W = ctx->width, H = ctx->height, i;

	if (seq->relayMode == 1 && (seq->started[6] == 0 || seq->done[6] == 1)) {
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
		t = h0 * 5.0;                                  /* 0x004f40c8 */
	}
	if (0.0 > t) {
		t = 0.0;
	}
	frames++;
	ctx->nearZ = 1.4;                                  /* 0x004f4408 */
	ctx->farZ = 600.0;                                 /* 0x004f3e00 */
	seq->partClock = ((((((t + 39.0) + 64.0) + 70.0) + 65.0) + 61.0) + 7.0) + 59.0;
	/* The 0 is the one the floor test leaves on the x87 stack. */
	ctx->nearZ = ctx->nearZ + 0.0;
	if (t >= 54.0) {                                   /* 0x004f4d50 */
		Attrib_Pop();
		seq->done[6] = 1;
		seq->started[6] = 1;
		return;
	}
	gl1::PushMatrix();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Fbo_Bind(g, 0, 2);

	r = h2 * 80.0 + 10.0;
	Cam9(eye, sin(h0) * r, h1 * 128.0, cos(h0) * r, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	x = Clamp01(t * 0.35);
	F = 1.0 - Math_Ease(&x, &one);
	if (t >= 0.0 && t < 130.0) {                       /* 0x004f4088 */
		a = 0.6 - t * 0.03;
		x1 = (float)(sin(a) * 35.0);
		z1 = (float)(cos(a) * 35.0 - t * 2.1);
		ty = (float)(t * 0.05 * 7.2);
		tz = (float)(0.0 - t * 1.5);
		x = Clamp01((t - 18.0) * 0.2);
		ex = (float)(x1 - Math_Ease(&x, &one) * 1.4);
		Cam9(eye, ex, 2.0, z1, 0.0, ty, tz, F * -0.8, 1.0, 0.02);
	}

	Vec4_Sub(&eye->target, &eye->eye, &Fv);
	Fc = Fv;
	Vec4_Normalize(&Fc);
	Vec4_Normalize(&Fv);
	Vec4_Cross(&Fv, &eye->up, &R);
	n = (unsigned)path.points.size() - 1;
	idx = (unsigned)(long)(t * 35.0) % n;              /* 0x004f47c0 */
	c3 = Clamp01((t - 3.0) * 0.3);
	c13 = Clamp01((t - 13.0) * 0.5);
	Bd = c3 * 0.0064 * (1.0 - c13);                    /* 0x004f5450 */
	px = path.points[idx].x;
	py = path.points[idx].y;
	rightV = (((((px * 1.1) * Bd) * 0.01) * 4.8) * 0.4) * 1.6;
	upV = (((((py * 1.1) * Bd) * 0.01) * 4.8) * 0.4) * 1.6;
	fB = (float)Bd;
	Vec4_Scale(&R, &px, &m1);
	Vec4_Scale(&m1, &fB, &m2);
	Vec4_Add(&eye->target, &m2, &eye->target);
	Vec4_Scale(&eye->up, &py, &m1);
	Vec4_Scale(&m1, &fB, &m2);
	Vec4_Add(&eye->target, &m2, &eye->target);
	eye->up.x = (float)(sin((path.a[idx] * 1.7188733853924696) * Bd) * 0.05 + eye->up.x);
	t10 = t * 10.0;
	ctx->light = *eye;
	m1 = Vec4_Make(30.0f, 0.0f, 0.0f);                 /* 0x004f47dc, 0x004f4f74 */
	Vec4_Scale(&m1, &k14, &m2);
	Vec4_Add(&ctx->light.eye, &m2, &ctx->light.eye);

	DrawScene(ctx, 1);
	Fbo_Resolve(g, 0, W, H, 2);
	Fbo_Unbind();
	glDisable(GL_DEPTH_TEST);
	G = 1.0 - Clamp01((t - 24.0) * 0.2);              /* 0x004f48e8 */
	Post_RunPass(g, passBlur, 2, 1, W / 2, H / 2, texNormal, texGlow, -1, -1);
	Post_RunPass(g, passWire, 0, 1, W, H, texNormal, texNormal, texGlow, texColour);
	Post_RunPass(g, passBlur, 2, 6, W / 2, H / 2, texNormal, texGlow, -1, -1);
	c15 = Clamp01((t - 15.0) * 0.3);
	alpha = (1.0 - c15 * 0.7) * 0.195 + F * 0.7;      /* 0x004f44a0, 0x004f5448 */
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
	Lf = (float)((1.0 - F) * G);
	gl1::Color3f(Lf, Lf, Lf);
	Gfx_DrawQuad(&zero, &one, &one, 1);
	gl1::PopMatrix();
	if (t < 13.0 && 3.0 < t) {                         /* 0x004f4ec0, 0x004f4068 */
		for (i = 0; i < 2; i++) {
			gl1::PushMatrix();
			glViewport(0, 0, W, H);
			gl1::MatrixMode(gl1::PROJECTION);
			gl1::LoadIdentity();
			gl1::MatrixMode(gl1::MODELVIEW);
			gl1::LoadIdentity();
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			gl1::LoadIdentity();
			gl1::Enable(GL_TEXTURE_2D);
			Tex_Bind(g, texBase + 0x1f + i);
			x = Clamp01(((t - 3.0) - (double)i * 0.9 * 4.0 * 1.6 * 0.7) * 0.21428571428571427);
			s = Clamp01(sin(x * 3.141592653589793) * 1.4);   /* 0x004f5440, 0x004f36e8 */
			sf = (float)s;
			gl1::Color3f(sf, sf, sf);
			if ((double)W / (double)H < 1.5555555555555556) {   /* 0x004f5438 */
				gl1::Translatef(-0.2f, 0.0f, 0.0f);
			}
			gl1::Translatef(0.402f, 0.0f, 0.0f);       /* 0x004f5430 */
			if (0.0001 < s) {
				hw = (double)H / (double)W;
				Gfx_DrawQuad(&zero, &hw, &half, 1);
			}
			gl1::PopMatrix();
			glDisable(GL_BLEND);
		}
	}
	gl1::PopMatrix();
	Attrib_Pop();
	seq->partEnd = t;
	glDisable(GL_BLEND);
	glFlush();
}

/*
 * ORIGIN 0x0047dd60 part_end::Init: part_city::Init's shape in slot
 * band 5000, with outputobj.obj and the R0 / R1 credit images in place
 * of citycrash.obj and tablet.obj. The targets only with
 * seq->relayMode 0, blob41b through Tex_LoadWithAlpha, box.3ds stood
 * in for by its material as part_motor::Init explains. There is no
 * srand: the shards draw on from where part_city::Init left the
 * generator. outputobj.obj's shards are flown here by a step inlined
 * with dt = 0.025 and a floor at y = -4, Hellidist.obj's by
 * Obj_BakeShards; the first v each throw builds is overwritten before
 * use, as in the demo (effects/end.txt 2).
 */
void part_end::Init(Rupture_Ctx *ctx)
{
	Gfx_Ctx    *g = &ctx->gfx;
	Shader_Ctx *s = &g->shader;
	int         w = ctx->width, h = ctx->height, k, j2, sub;
	const float k05 = 0.5f, k1 = 1.0f, k02 = 0.2f;     /* 0x004f3720, 0x004f3754 */
	const float fdt = 0.025f, damp = 0.98875f;         /* 0x004f53e4, 0x004f53e0 */
	const float slow = 0.96875f;                       /* 0x004f53d0 */
	Vec4        off = Vec4_Make(-3.71f, 1.81f, 0.0f); /* 0x004f50e8, 0x004f50e4 */
	Vec4        up3 = Vec4_Make(0.0f, 3.0f, 0.0f);    /* 0x004f3854 */
	Vec4        gv = Vec4_Make(0.0f, -2.0f, 0.0f);    /* 0x004f4400 */
	Vec4        d, v, m, pt, step;
	double      r, D, P;
	float       fP, a1, a2, a3, b1, b2, xx, yy, zz, len;
	size_t      j;

	t = 0.0;
	val1390 = 0x70;
	tLocal = 0.0;
	val1394 = 0x70;
	frames = 0;
	texNormal = 0;
	texColour = 0xb;
	slot13a0 = 0x6f;
	texQuarter = 2;
	texGlow = 3;
	texSlot = 1;
	texBase = 5000;
	model = 5000;
	pairBoat = 0x13b8;
	pairAniso2h = 0x138a;
	passBlurCirc = 0x139d;
	passBlur = 0x139f;
	passBend = 0x13a7;
	passBlurFOV = 0x13a5;
	passWire = 0x13a3;
	slot13e0 = 0x13a8;
	pairTrain = 0x13aa;
	pairHeli2 = 0x13ae;
	pairHeli2r = 0x13e0;
	pairCity = 0x13b2;
	slot33c = 0x13a1;
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
	g->tex[texBase + 0x1f] = Tex_Load("textures/R0.jpg");
	g->tex[texBase + 0x20] = Tex_Load("textures/R1.jpg");
	g->tex[texBase + 0x14] = Tex_LoadWithAlpha("textures/blob41b.jpg", "textures/blob41b.jpg");
	Obj_Load(&g_boatObj, gl1::GenLists(1), "meshes/boat.obj");
	Obj_Load(&helicopter, gl1::GenLists(1), "meshes/helicopter.obj");
	Obj_Load(&hellicrash2, gl1::GenLists(1), "meshes/hellicrash2.obj");
	Obj_Load(&helix, gl1::GenLists(1), "meshes/helix.obj");
	Obj_Load(&hellidist, gl1::GenLists(1), "meshes/Hellidist.obj");
	Obj_Load(&city1, gl1::GenLists(1), "meshes/city1.obj");
	Obj_Load(&cityLess, gl1::GenLists(1), "meshes/city_less.obj");
	Obj_Load(&citywater, gl1::GenLists(1), "meshes/citywater.obj");
	Obj_Load(&outputobj, gl1::GenLists(1), "meshes/outputobj.obj");

	Obj_MakeShards(&outputobj, &outputobjShards, 0.52);   /* 0x004f53f0 */
	for (j = 0; j < outputobjShards.size(); j++) {
		Shard *sh = &outputobjShards[j];

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
		b1 = (float)(Math_Random() * 9.0 - 2.0);
		b2 = (float)(Math_Random() * 8.0 - 4.0);
		d = Vec4_Make(b2, b1, (float)(P * 4.0));
		Vec4_Scale(&d, &k05, &m);
		Vec4_Scale(&m, &k1, &v);
		sh->a = Vec4_Make(0.0f, -66.0f, 0.0f);         /* 0x004f50e0 */
		sh->v = v;
	}
	for (j = 0; j < outputobjShards.size(); j++) {
		Shard *sh = &outputobjShards[j];

		for (k = 0; k < 0x78; k++) {
			Shard_Record(sh);
			if (k == 0) {
				for (j2 = 0; j2 < 5; j2++) {
					Shard_Record(sh);
					Shard_Record(sh);
				}
			}
			for (sub = 0; sub < 2; sub++) {
				Vec4_Scale(&gv, &fdt, &m);
				Vec4_Add(&sh->a, &m, &sh->a);
				Vec4_Scale(&sh->a, &fdt, &m);
				Vec4_Add(&sh->v, &m, &sh->v);
				Vec4_Scale(&sh->v, &damp, &sh->v);
				Vec4_Scale(&sh->a, &damp, &sh->a);
				Vec4_Scale(&sh->v, &fdt, &m);
				Vec4_Add(&sh->pos, &m, &sh->pos);
				if (sh->pos.y >= -3.95) {              /* 0x004f53d8 */
					xx = sh->v.x * sh->v.x;
					yy = sh->v.y * sh->v.y;
					zz = sh->v.z * sh->v.z;
					len = (float)sqrt((float)(((double)xx + yy) + zz));
					sh->angle = (float)((((len + 1.0) * sh->spin) * 4.3) * 0.025 + sh->angle);
				} else {
					Vec4_Scale(&sh->v, &slow, &sh->v);
					Vec4_Scale(&sh->a, &slow, &sh->a);
				}
				if (sh->pos.y < -4.0f) {               /* 0x004f46c8 */
					sh->pos.y = -4.0f;
					sh->v.y = (float)(sh->v.y * -0.29);
					sh->v.x = (float)(sh->v.x * 0.92);
					sh->v.z = (float)(sh->v.z * 0.92);
					Vec4_Scale(&sh->v, &fdt, &m);
					Vec4_Add(&sh->pos, &m, &sh->pos);
				}
			}
		}
	}
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
		sh->a = Vec4_Make(10.0f, -6.0f, 0.0f);         /* 0x004f3bbc, 0x004f50dc */
		sh->v = v;
	}
	Obj_BakeShards(&hellidistShards, 6, 20);

	val1418 = 200;
	Track_Clear(&track);
	step = Vec4_Make(0.0f, 0.0f, 1.0f);
	pt = Vec4_Make(0.0f, 0.0f, 0.0f);
	for (k = 0; k < 1000; k++) {
		Vec4_Add(&pt, &step, &pt);
		track.points.push_back(pt);
		track.count++;
	}
	g->tex[(model + 1000) * 100] = Tex_Load("meshes/marmaro.jpg");
	Shader_LoadPair(s, "cgeffects/vertex_projection_anisotropic.cpp",
	                "cgeffects/fragment_projection_anisotropic2h.cpp", pairAniso2h);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_boat.cpp", pairBoat);
	Shader_LoadPair(s, "cgeffects/vertex_boat2.cpp", "cgeffects/fragment_city.cpp", pairCity);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_heli2.cpp", pairHeli2);
	Shader_LoadPair(s, "cgeffects/vertex_boat.cpp", "cgeffects/fragment_heli2r.cpp", pairHeli2r);
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
