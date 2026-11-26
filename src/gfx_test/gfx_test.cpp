/*
 * gfx_test - OURS. The look of RUPTURE on one mesh at a time, for
 * someone who wants to see how it is made: the demo's own shaders
 * (transcribed to GLSL, src/rupture/shaders) and the post chain every
 * part runs, through src/rupture/core.
 *
 *   [ ]    the mesh: a cube, the motorcycle, the room (walls.obj), or
 *          the shatter - walls.obj's shards flying as part_video
 *          replays them, its clock looping 0 to 39 s (R restarts it)
 *   P      the shader pair: the road look or the room look
 *   Z      the swiss cheese - the room pair's world-space hole lattice
 *   X      the dissolve - the room pair's world-space noise threshold
 *   M      slide the mesh through the world, to show that the holes,
 *          the dissolve and the speckle stay where they are
 *   C      the post chain (blur, edges, bend) on or off
 *   N      the final image, the colour target, the normal target
 *   V      the TV's video on its own, frame by frame as part_video
 *          picks it, from src/video/test_video2.mpg
 *   drag / wheel / Home   orbit, zoom, back to the starting view
 *
 * Every switch is ours; with all of them at their defaults each pair
 * runs exactly as the demo binds it. effects/look.txt is the
 * reference, effects/motor.txt 2 the road pair, effects/video.txt
 * 3-4 the room pair and effects/video.txt 3.4 the shards.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "../rupture/core/gl1.h"
#include "../rupture/core/obj.h"
#include "../rupture/core/post.h"
#include "../rupture/core/shard.h"
#include "../rupture/core/video.h"
#include "../rupture/host/host.h"

/* part_motor::Init's texture numbers: this+0x1138, 0x113c, 0x1144, 0x1e4. */
enum { T_NORMAL = 0, T_COLOUR = 11, T_QUARTER = 2, T_GLOW = 3 };

/* OURS - the demo's slots are this+0x1180, this+0x1164 and the post range. */
enum { S_ROAD = 0, S_ROOM = 2, S_BLUR = 4, S_WIRE = 5, S_BEND = 6 };

enum { MESH_CUBE, MESH_BIKE, MESH_ROOM, MESH_SHATTER, MESH_COUNT };
enum { PAIR_ROAD, PAIR_ROOM };
enum { VIEW_FINAL, VIEW_COLOUR, VIEW_NORMAL };

static const char *k_meshes[MESH_COUNT] = { "cube", "motorcycle", "room (walls.obj)", "shatter" };

static Gfx_Ctx            g_gfx;
static Obj_Mesh           g_mesh[MESH_COUNT];
static std::vector<Shard> g_shards;
static double             g_shatterStart;
static GLuint   g_bg08;
static int      g_width, g_height;
static int      g_meshIdx = MESH_ROOM;
static int      g_pair = PAIR_ROOM;
static int      g_view = VIEW_FINAL;
static bool     g_cheese = true, g_dissolve = true, g_slide, g_post = true, g_showVideo;
static Video    g_video;
static bool     g_haveVideo;
static double   g_yaw, g_pitch, g_dist;
static double   g_now;
static char     g_title[256];
static char     g_shatterClock[32];

/*
 * The room's view: part_video's eye and light cameras as read from the
 * running demo at clock 10.29 [observed, effects/video.txt 6]. Its
 * world has +z up.
 */
static const double k_roomEye[3] = { 139.87285, 306.51602, 0.0 };
static const double k_roomAt[3] = { 0.5519681, -0.26115873, -22.489803 };
static const double k_roomLight[3] = { -280.12714, 306.51602, 0.0 };

/* OURS. The room and the shatter share part_video's camera and the room pair. */
static bool room_view(void)
{
	return g_meshIdx == MESH_ROOM || g_meshIdx == MESH_SHATTER;
}

/* OURS. A cube from -1 to 1 as six quads wound counter-clockwise outward. */
static void cube_build(Obj_Mesh *m)
{
	static const float q[6][4][3] = {
		{ { -1, -1,  1 }, {  1, -1,  1 }, {  1,  1,  1 }, { -1,  1,  1 } },
		{ {  1, -1, -1 }, { -1, -1, -1 }, { -1,  1, -1 }, {  1,  1, -1 } },
		{ {  1, -1,  1 }, {  1, -1, -1 }, {  1,  1, -1 }, {  1,  1,  1 } },
		{ { -1, -1, -1 }, { -1, -1,  1 }, { -1,  1,  1 }, { -1,  1, -1 } },
		{ { -1,  1,  1 }, {  1,  1,  1 }, {  1,  1, -1 }, { -1,  1, -1 } },
		{ { -1, -1, -1 }, {  1, -1, -1 }, {  1, -1,  1 }, { -1, -1,  1 } }
	};
	Obj_Face f;
	int      i, k;

	m->list = gl1::GenLists(1);
	m->compile = 1;
	for (i = 0; i < 6; i++) {
		for (k = 0; k < 4; k++) {
			f.c[k].kind = 1;
			f.c[k].x = q[i][k][0];
			f.c[k].y = q[i][k][1];
			f.c[k].z = q[i][k][2];
		}
		Obj_AddFace(m, &f);
	}
}

/* OURS. The orbit each mesh starts from. */
static void camera_home(void)
{
	double dx, dy, dz;

	if (room_view()) {
		dx = k_roomEye[0] - k_roomAt[0];
		dy = k_roomEye[1] - k_roomAt[1];
		dz = k_roomEye[2] - k_roomAt[2];
		g_dist = sqrt(dx * dx + dy * dy + dz * dz);
		g_yaw = atan2(dy, dx);
		g_pitch = asin(dz / g_dist);
	} else {
		g_dist = g_meshIdx == MESH_CUBE ? 6.0 : 5.0;
		g_yaw = 1.57;
		g_pitch = 0.3;
	}
}

/* OURS. The orbit's eye about the target; the room orbits about +z, the others +y. */
static void camera_eye(const double at[3], double eye[3])
{
	if (room_view()) {
		eye[0] = at[0] + g_dist * cos(g_pitch) * cos(g_yaw);
		eye[1] = at[1] + g_dist * cos(g_pitch) * sin(g_yaw);
		eye[2] = at[2] + g_dist * sin(g_pitch);
	} else {
		eye[0] = at[0] + g_dist * cos(g_pitch) * cos(g_yaw);
		eye[1] = at[1] + g_dist * sin(g_pitch);
		eye[2] = at[2] + g_dist * cos(g_pitch) * sin(g_yaw);
	}
}

/* OURS. Loads a pair or a post pass, failing with what went wrong. */
static bool load_shaders(std::string &error)
{
	Shader_Ctx *s = &g_gfx.shader;
	bool        ok = Cg_Init(s);

	ok = Shader_LoadPair(s, "cgeffects/vertex_boat_test2cut.cpp",
	                     "cgeffects/fragment_boat_test2.cpp", S_ROAD) && ok;
	ok = Shader_LoadPair(s, "cgeffects/vertex_projection_anisotropic4.cpp",
	                     "cgeffects/fragment_projection_anisotropic2box.cpp", S_ROOM) && ok;
	ok = Shader_LoadFragmentOnly(s, "cgeffects/fragment_blur256.cpp", S_BLUR) && ok;
	ok = Shader_LoadFragmentOnly(s, "cgeffects/fragment_wireframe.cpp", S_WIRE) && ok;
	ok = Shader_LoadFragmentOnly(s, "cgeffects/fragment_bend3.cpp", S_BEND) && ok;
	Shader_GetParam(s, S_ROAD, 1, "CameraPos");
	Shader_GetParam(s, S_ROAD, 2, "LightPos");
	Shader_GetParam(s, S_ROAD, 4, "Reduction");
	Shader_GetParam(s, S_ROOM, 1, "CameraPos");
	Shader_GetParam(s, S_ROOM, 2, "LightPos");
	error = s->error;
	return ok;
}

/* OURS. The three targets part_motor::Init sets up. */
static bool targets_setup(int w, int h)
{
	bool ok = Fbo_Create(&g_gfx, 0, w, h, 4)
	       && Fbo_Create(&g_gfx, 1, w / 4, h / 4, 0)
	       && Fbo_Create(&g_gfx, 2, w / 2, h / 2, 0);

	g_gfx.texBase = 0;
	g_gfx.oned = -1;
	Tex_CreateTarget(&g_gfx, T_QUARTER, w / 4, h / 4);
	Tex_CreateTarget(&g_gfx, T_GLOW, w / 2, h / 2);
	Tex_CreateTarget(&g_gfx, T_NORMAL, w, h);
	Tex_CreateTarget(&g_gfx, T_COLOUR, w, h);
	Fbo_AttachTexture(&g_gfx, 0, 0, T_NORMAL);
	Fbo_AttachTexture(&g_gfx, 0, 1, T_COLOUR);
	Fbo_AttachTexture(&g_gfx, 1, 0, T_QUARTER);
	Fbo_AttachTexture(&g_gfx, 2, 0, T_GLOW);
	Fbo_Unbind();
	return ok;
}

/*
 * ORIGIN 0x004b143d..0x004b18e5, a slice of part_video::Init: walls.obj
 * into shards, the launch and the bake (effects/video.txt 3.4); the
 * whole function is part_video::Init in src/rupture/demo.
 * Obj_MakeShards' spin axes come from the rand stream as everything
 * before part_video::Init leaves it; here the stream is fresh (seed 1),
 * so the axes and spins are not the demo's. The launch after the
 * _srand is.
 */
static void shatter_setup(void)
{
	static const float k250 = 250.0f;        /* 0x004f6ce8 */
	static const float k120 = 120.0f;        /* 0x004f66d8 */
	Vec4               off = { 1, 0.0f, 57.0f, -16.4f };   /* 0x004f6c40, 0x004f6c44 */
	Vec4               behind = { 1, 80.0f, 60.0f, -70.0f };
	Vec4               down = { 1, 0.0f, -150.0f, 0.0f };   /* 0x004f47d8 */
	Vec4               d, r, dn, sum, v;
	double             r1, r2, r3;
	size_t             i;

	Obj_MakeShards(&g_mesh[MESH_ROOM], &g_shards, 0.05);
	Crt_Srand(0xfcdd0218u);
	for (i = 0; i < g_shards.size(); i++) {
		Shard *s = &g_shards[i];

		Math_Random();
		Vec4_Sub(&s->rest, &off, &d);
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
		if (s->rest.x < -120.0) {                /* 0x004f6ce0 */
			v.kind = 1;
			v.x = (float)(Math_Random() * 20.0 + 200.0);
			v.y = 0.0f;
			v.z = 70.0f;                     /* 0x004f6cdc */
		}
		s->a.kind = 1;
		s->a.x = 0.0f;
		s->a.y = 0.0f;
		s->a.z = -90.0f;                     /* 0x004f4708 */
		s->v = v;
	}
	Obj_BakeShards(&g_shards, 12, 20);
}

/* OURS */
bool App_Init(int width, int height, std::string &error)
{
	g_width = width;
	g_height = height;
	if (!load_shaders(error)) {
		return false;
	}
	if (!targets_setup(width, height)) {
		error = "a render target is incomplete";
		return false;
	}
	g_bg08 = Tex_Load("textures/bg08.jpg");
	if (g_bg08 == 0) {
		error = "textures/bg08.jpg is missing";
		return false;
	}
	cube_build(&g_mesh[MESH_CUBE]);
	if (!Obj_Load(&g_mesh[MESH_BIKE], gl1::GenLists(1), "meshes/motorcycle.obj")
	 || !Obj_Load(&g_mesh[MESH_ROOM], gl1::GenLists(1), "meshes/walls.obj")) {
		error = "meshes/motorcycle.obj or meshes/walls.obj is missing";
		return false;
	}
	shatter_setup();
	g_haveVideo = Video_Open(&g_video, "video/test_video2.mpg");
	camera_home();
	return true;
}

/*
 * OURS. The road pair as part_motor::DrawScene binds it (effects/motor.txt
 * 2): the colour 0.2, 0.22, 0.2, decal bg08, LightPos (0, 0, 30).
 * Ours: Reduction, alpha2 and alphaplane2 are set where the widening,
 * the noise cut and the burning front stay off.
 */
static void road_uniforms(const double eye[3])
{
	Shader_Ctx *s = &g_gfx.shader;
	double      alpha2 = -1.0, alphaplane2 = -1000.0;

	Shader_SetParam3f(s, S_ROAD, 1, (float)eye[0], (float)eye[1], (float)eye[2]);
	Shader_SetParam3f(s, S_ROAD, 2, 0.0f, 0.0f, 30.0f);
	Shader_SetParam3f(s, S_ROAD, 4, -1000.0f, 0.0f, 0.0f);
	Shader_SetParam1f(s, S_ROAD + 1, "alpha2", &alpha2);
	Shader_SetParam1f(s, S_ROAD + 1, "alphaplane2", &alphaplane2);
	Shader_SetTexture(s, S_ROAD + 1, "decal", g_bg08);
	gl1::Color4f(0.2f, 0.22f, 0.2f, 1.0f);
}

/*
 * OURS. The room pair as part_video::DrawScene binds it to walls.obj's
 * shards (effects/video.txt 3.2): the colour 0.4, 0.7, 0.9, decal bg08,
 * LightPos the light camera. ttime is the dissolve threshold: the X
 * switch sweeps it the way room2's rises from t = 5, 0 to 4 and back.
 * Z switches the lattice by uniform, a switch the demo's shader has
 * not got.
 */
static void room_uniforms(const double eye[3])
{
	Shader_Ctx *s = &g_gfx.shader;
	double      ttime = 400.0, cheeseOff = g_cheese ? 0.0 : 1.0;

	if (g_dissolve) {
		ttime = 2.0 + 2.0 * sin(g_now * 0.5);
	}
	Shader_SetParam3f(s, S_ROOM, 1, (float)eye[0], (float)eye[1], (float)eye[2]);
	Shader_SetParam3f(s, S_ROOM, 2, (float)k_roomLight[0], (float)k_roomLight[1],
	                  (float)k_roomLight[2]);
	Shader_SetParam1f(s, S_ROOM + 1, "ttime", &ttime);
	Shader_SetParam1f(s, S_ROOM + 1, "oursCheeseOff", &cheeseOff);
	Shader_SetTexture(s, S_ROOM + 1, "decal", g_bg08);
	gl1::Color3f(0.4f, 0.7f, 0.9f);
}

/* OURS. The length the shard loop inlines: float squares, one sqrt. */
static float len3(const Vec4 *v)
{
	float xx = v->x * v->x, yy = v->y * v->y, zz = v->z * v->z;

	return (float)sqrt(xx + yy + zz);
}

/*
 * ORIGIN 0x004af210..0x004afa03, a slice of part_video::DrawScene: each
 * shard's record index s from the clock t and its rest centre
 * (effects/video.txt 3.4), then Shard_Seek. The factor 20 (k_dbl_20)
 * is FUN_00401590(...) * 0 + 1 tested against 20, which always picks
 * 20 and leaves 1 beside it. [open] t here is ours, looping 0 to 39 s
 * (R restarts it); in the demo it is part_video's clock this+0x1b8.
 */
static void shatter_seek(double clock)
{
	static const Vec4 off = { 1, 0.0f, 57.0f, -16.4f };      /* 0x004f6c40, 0x004f6c44 */
	static const Vec4 off2 = { 1, 0.0f, 57.0f, 16.4f };      /* 0x004f6c2c */
	static const Vec4 p1 = { 1, -127.0f, -66.5f, 27.0f };    /* 0x004f6c3c.. */
	static const Vec4 p2 = { 1, 90.0f, -60.0f, 60.0f };
	static const Vec4 p3 = { 1, 117.0f, -100.0f, 3.0f };
	static const Vec4 p4 = { 1, -3.0f, -70.0f, 0.0f };
	static const Vec4 p5 = { 1, -28.0f, -49.0f, 27.0f };     /* 0x004f6c24.. */
	double            t = fmod(clock, 39.0), two = 2.0;
	double            q, m1, m2, m3, m, b, r, c, C, A, B, T, mn, x;
	Vec4              e1, e2, e3, e4, d5;
	size_t            i;

	for (i = 0; i < g_shards.size(); i++) {
		Shard *s = &g_shards[i];

		Vec4_Sub(&s->rest, &off, &e1);
		Vec4_Sub(&e1, &p1, &e1);
		Vec4_Sub(&s->rest, &off, &e2);
		Vec4_Sub(&e2, &p2, &e2);
		Vec4_Sub(&s->rest, &off, &e3);
		Vec4_Sub(&e3, &p3, &e3);
		Vec4_Sub(&s->rest, &off2, &e4);
		Vec4_Sub(&e4, &p4, &e4);
		Vec4_Sub(&p5, &e1, &d5);
		b = len3(&d5) * 0.4 * 0.1;
		r = b * (b * b);
		c = (t - 24.0) * 0.35;
		c = c > 1.0 ? 1.0 : c < 0.0 ? 0.0 : c;
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
		Shard_Seek(s, Math_Ease(&x, &two) * 10.05 + B + C);
	}
}

/*
 * ORIGIN 0x004afa3b..0x004b01ad, a slice of part_video::DrawScene: the
 * shards in one glBegin(GL_TRIANGLES) under GL_TEXTURE =
 * translate(0, 57, -16.4), in glColor3f(0.4, 0.7, 0.9) (0x004f47fc,
 * 0x004f4060, 0x004f5938); a shard past s = 110 (0x004f63d8) is not
 * drawn. Ours: the M slide added to the translate.
 */
static void shatter_draw(int slot, double slide)
{
	static const Vec4 zero = { 1, 0.0f, 0.0f, 0.0f };
	static const int  tri[4][3] = { { 0, 1, 2 }, { 0, 1, 3 }, { 0, 2, 3 }, { 1, 2, 3 } };
	Vec4              w, v[4];
	size_t            i;
	int               f, k;

	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::Translatef((float)(slide * 40.0), 57.0f, -16.4f);
	Shader_SetTextureMatrix(&g_gfx.shader, slot, 0xe);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::Color3f(0.4f, 0.7f, 0.9f);
	Crt_Srand(0x675ff);
	gl1::Begin(gl1::TRIANGLES);
	for (i = 0; i < g_shards.size(); i++) {
		const Shard *s = &g_shards[i];

		if (!((double)s->s <= 110.00000000000001)) {
			continue;
		}
		for (k = 0; k < 4; k++) {
			Vec4_Add(&s->turned[k], &s->pos, &w);
			Vec4_Add(&w, &zero, &v[k]);
		}
		for (f = 0; f < 4; f++) {
			const Vec4 *n = &s->turnedN[f];

			gl1::Normal3f(n->x, n->y, n->z);
			for (k = 0; k < 3; k++) {
				gl1::Vertex3f(v[tri[f][k]].x, v[tri[f][k]].y, v[tri[f][k]].z);
			}
		}
	}
	gl1::End();
	gl1::PopMatrix();
}

/* OURS. The mesh into target 0: colour and normal, with the pair chosen. */
static void scene_draw(void)
{
	static const double origin[3] = { 0.0, 0.0, 0.0 };
	Shader_Ctx         *s = &g_gfx.shader;
	const double       *at = room_view() ? k_roomAt : origin;
	double              eye[3], slide = g_slide ? sin(g_now * 0.7) : 0.0;
	int                 slot = g_pair == PAIR_ROAD ? S_ROAD : S_ROOM;

	camera_eye(at, eye);
	glViewport(0, 0, g_width, g_height);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_BLEND);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	if (room_view()) {
		gl1::Perspective(55.0, (double)g_width / g_height, 36.0, 750.0);
	} else {
		gl1::Perspective(45.0, (double)g_width / g_height, 0.1, 100.0);
	}
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	if (room_view()) {
		gl1::LookAt(eye[0], eye[1], eye[2], at[0], at[1], at[2], 0.0, -0.036520034, 1.0);
	} else {
		gl1::LookAt(eye[0], eye[1], eye[2], at[0], at[1], at[2], 0.0, 1.0, 0.0);
	}

	Shader_EnableProfiles(s);
	Shader_Bind(s, slot);
	Shader_Bind(s, slot + 1);
	Shader_SetMvpMatrix(s, slot, 5);
	if (g_pair == PAIR_ROAD) {
		road_uniforms(eye);
	} else {
		room_uniforms(eye);
	}
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	Shader_SetTextureMatrix(s, slot, 6);
	if (g_meshIdx == MESH_SHATTER) {
		shatter_seek(g_now - g_shatterStart);
		shatter_draw(slot, slide);
		Shader_DisableProfiles(s);
		return;
	}
	if (room_view()) {
		gl1::Translatef((float)(slide * 40.0), 0.0f, 0.0f);
	} else {
		gl1::Translatef((float)(slide * 3.0), 0.0f, 0.0f);
		if (g_meshIdx == MESH_BIKE) {
			gl1::Translatef(0.0f, -1.25f, 0.24f);
		}
	}
	Shader_SetTextureMatrix(s, slot, 0xe);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	Obj_Draw(&g_mesh[g_meshIdx]);
	Shader_DisableProfiles(s);
}

/* OURS. A target straight to the window, for the colour and normal views. */
static void show_texture(int tex)
{
	double one = 1.0, zero = 0.0;

	glViewport(0, 0, g_width, g_height);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(&g_gfx, tex);
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	Gfx_DrawQuad(&zero, &one, &one, 1);
	gl1::Disable(GL_TEXTURE_2D);
}

/*
 * OURS. The video flat on the window. The frame is DrawScene's,
 * __ftol2(max(t - 3, 0) * 30) % 838 (effects/video.txt 1), with t
 * this program's clock.
 */
static void show_video(void)
{
	double one = 1.0, zero = 0.0, t = g_now - 3.0 > 0.0 ? g_now - 3.0 : 0.0;

	Video_Show(&g_video, (int)(t * 30.0) % 838);
	glViewport(0, 0, g_width, g_height);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	gl1::Enable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_video.texture);
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	Gfx_DrawQuad(&zero, &one, &one, 1);
	gl1::Disable(GL_TEXTURE_2D);
}

/*
 * OURS. One frame in the order the parts' Frames run it: the scene into
 * target 0, blur, edges, blur six times, bend3 back into target 0,
 * then that target to the window with GL_ONE, GL_ONE. Both Frames bind
 * the glow and the quarter targets with their colours first, but only
 * the last bind reaches the one quad drawn. The road look takes
 * part_motor's bend3 alpha once its ramp ends (0.2), the room look
 * part_video's (0.15). RightV and UpV are 0: no camera motion here.
 * With the chain off, the colour target goes out instead (ours).
 * It never ends.
 */
bool App_Frame(double seconds)
{
	double one = 1.0, zero = 0.0, rightV = 0.0, upV = 0.0;
	double alpha = g_pair == PAIR_ROAD ? 0.2 : 0.15;
	float  quarter = g_pair == PAIR_ROAD ? 0.3f : 0.15f;

	g_now = seconds;
	Fbo_Unbind();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Fbo_Bind(&g_gfx, 0, 2);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	scene_draw();
	Fbo_Resolve(&g_gfx, 0, g_width, g_height, 2);
	Fbo_Unbind();
	glDisable(GL_DEPTH_TEST);
	if (g_showVideo && g_haveVideo) {
		show_video();
	} else if (g_view != VIEW_FINAL) {
		show_texture(g_view == VIEW_COLOUR ? T_COLOUR : T_NORMAL);
	} else {
		if (g_post) {
			Post_RunPass(&g_gfx, S_BLUR, 2, 1, g_width / 2, g_height / 2, T_NORMAL, T_GLOW, -1, -1);
			Post_RunPass(&g_gfx, S_WIRE, 0, 1, g_width, g_height, T_NORMAL, T_NORMAL, T_GLOW, T_COLOUR);
			Post_RunPass(&g_gfx, S_BLUR, 2, 6, g_width / 2, g_height / 2, T_NORMAL, T_GLOW, -1, -1);
			Shader_SetParam1f(&g_gfx.shader, S_BEND, "alpha", &alpha);
			Shader_SetParam1f(&g_gfx.shader, S_BEND, "RightV", &rightV);
			Shader_SetParam1f(&g_gfx.shader, S_BEND, "UpV", &upV);
			Post_RunPass(&g_gfx, S_BEND, 0, 1, g_width, g_height, T_NORMAL, T_NORMAL, T_GLOW, -1);
		}
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glViewport(0, 0, g_width, g_height);
		gl1::MatrixMode(gl1::PROJECTION);
		gl1::LoadIdentity();
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::LoadIdentity();
		gl1::MatrixMode(gl1::MODELVIEW);
		gl1::LoadIdentity();
		gl1::Enable(GL_TEXTURE_2D);
		if (g_post) {
			Tex_Bind(&g_gfx, T_GLOW);
			gl1::Color3f(1.0f, 1.0f, 1.0f);
			Tex_Bind(&g_gfx, T_QUARTER);
			gl1::Color3f(quarter, quarter, quarter);
		}
		Tex_Bind(&g_gfx, g_post ? T_NORMAL : T_COLOUR);
		gl1::Color3f(1.0f, 1.0f, 1.0f);
		Gfx_DrawQuad(&zero, &one, &one, 1);
		glDisable(GL_BLEND);
		gl1::Disable(GL_TEXTURE_2D);
	}
	g_shatterClock[0] = '\0';
	if (g_meshIdx == MESH_SHATTER) {
		sprintf(g_shatterClock, " t %.1f", fmod(g_now - g_shatterStart, 39.0));
	}
	sprintf(g_title, "gfx_test - [ ] %s%s   P %s   Z cheese %s   X dissolve %s   M slide %s   "
	        "C post %s   N %s   V video %s", k_meshes[g_meshIdx], g_shatterClock,
	        g_pair == PAIR_ROAD ? "road look" : "room look",
	        g_cheese ? "on" : "off", g_dissolve ? "on" : "off", g_slide ? "on" : "off",
	        g_post ? "on" : "off",
	        g_view == VIEW_FINAL ? "final" : g_view == VIEW_COLOUR ? "colour" : "normal",
	        !g_haveVideo ? "missing" : g_showVideo ? "on" : "off");
	return true;
}

/* OURS */
void App_Key(int key)
{
	switch (key) {
	case '[':
		g_meshIdx = (g_meshIdx + MESH_COUNT - 1) % MESH_COUNT;
		g_pair = room_view() ? PAIR_ROOM : PAIR_ROAD;
		g_shatterStart = g_now;
		camera_home();
		break;
	case ']':
		g_meshIdx = (g_meshIdx + 1) % MESH_COUNT;
		g_pair = room_view() ? PAIR_ROOM : PAIR_ROAD;
		g_shatterStart = g_now;
		camera_home();
		break;
	case 'R':
		g_shatterStart = g_now;
		break;
	case 'P':
		g_pair = g_pair == PAIR_ROAD ? PAIR_ROOM : PAIR_ROAD;
		break;
	case 'Z':
		g_cheese = !g_cheese;
		break;
	case 'X':
		g_dissolve = !g_dissolve;
		break;
	case 'M':
		g_slide = !g_slide;
		break;
	case 'C':
		g_post = !g_post;
		break;
	case 'N':
		g_view = (g_view + 1) % 3;
		break;
	case 'V':
		g_showVideo = !g_showVideo;
		break;
	case KEY_HOME:
		camera_home();
		break;
	}
}

/* OURS */
void App_Drag(int dx, int dy)
{
	g_yaw -= dx * 0.005;
	g_pitch += dy * 0.005;
	if (g_pitch > 1.5) {
		g_pitch = 1.5;
	} else if (g_pitch < -1.5) {
		g_pitch = -1.5;
	}
}

/* OURS. The orbit stays where it was left. */
void App_DragEnd(void)
{
}

/* OURS. The wheel is the orbit's distance here; nothing to reset. */
void App_ViewReset(void)
{
}

/* OURS. The orbit's distance, a notch at a time; Ctrl does nothing here. */
void App_Wheel(int notches, bool fine)
{
	(void)fine;
	g_dist *= notches > 0 ? 0.9 : 1.0 / 0.9;
}

/* OURS. gfx_test's clock starts at 0. */
double App_TimeBase(void)
{
	return 0.0;
}

/* OURS. gfx_test has no loading screen: App_Init does it all. */
bool App_Load(std::string &error)
{
	(void)error;
	return true;
}

/* OURS. gfx_test has no parts to jump to. */
bool App_SetJump(const char *spec)
{
	(void)spec;
	return false;
}

/* OURS. gfx_test has no music; the dialog that asks never opens for it. */
bool App_Muted(void)
{
	return true;
}

/* OURS */
void App_SetMuted(bool muted)
{
	(void)muted;
}


/* OURS. Nor any to name, for --perflog. */
int App_Part(const char **name, double *t)
{
	*name = "gfx_test";
	*t = 0.0;
	return 0;
}

/* OURS */
const char *App_Title(void)
{
	return g_title;
}
