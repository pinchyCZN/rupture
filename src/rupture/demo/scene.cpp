#include "scene.h"

#include <math.h>

#include "../core/camera.h"
#include "../core/gl1.h"

/*
 * OURS. What maybe_Mesh_Draw 0x00453c50 leaves behind when the model's
 * triangle count is scaled by ctx+0x1bd8 = 0, as every call from
 * part_video is: the decal and light samplers on the fragment
 * program, the model's material as texture2 (slot (model + 1000) * 100
 * + material, material 0 assumed [inferred]), GL_TEXTURE_2D on, and no
 * triangles. The model's ModelViewObject it builds is overwritten
 * before any real draw, so it is not built here. "texture" (slot + 50)
 * is not set: no transcribed pair reads it.
 */
static void mesh_draw_samplers(Rupture_Ctx *ctx, int model, GLuint texA, GLuint texB, int prog)
{
	Shader_Ctx *s = &ctx->gfx.shader;

	gl1::Enable(GL_TEXTURE_2D);
	Shader_SetTexture(s, prog + 1, "decal", texA);
	Shader_SetTexture(s, prog + 1, "light", texB);
	Shader_SetTexture(s, prog + 1, "texture2", Tex_Get(&ctx->gfx, (model + 1000) * 100));
}

/*
 * ORIGIN 0x004bbc90 maybe_Scene_DrawLit. CameraPos is always the
 * context's eye camera, whatever eye is passed. The light projection
 * is the texture matrix captured as ModelViewLightProj; the camera's
 * is ModelViewProj.
 */
void maybe_Scene_DrawLit(Rupture_Ctx *ctx, int model, int slotA, int slotB, int prog,
                         const Camera *eye, const Camera *light, double time)
{
	Shader_Ctx *s = &ctx->gfx.shader;
	GLuint      texA, texB;

	(void)time;
	gl1::PushMatrix();
	gl1::LoadIdentity();
	Scene_LookOffset(ctx);
	gl1::LookAt(eye->eye.x, eye->eye.y, eye->eye.z, eye->target.x, eye->target.y,
	            eye->target.z, eye->up.x, eye->up.y, eye->up.z);
	gl1::Disable(GL_TEXTURE_2D);
	gl1::PushMatrix();
	Shader_DisableFragment(s);
	Shader_DisableVertexProfile(s);
	gl1::Enable(GL_TEXTURE_2D);
	gl1::PopMatrix();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	Shader_Bind(s, prog);
	Shader_Bind(s, prog + 1);
	Shader_EnableFragmentProfile(s);
	Shader_EnableVertexProfile(s);
	Shader_GetParam(s, prog, 1, "CameraPos");
	Shader_SetParam3f(s, prog, 1, ctx->eye.eye.x, ctx->eye.eye.y, ctx->eye.eye.z);
	Shader_GetParam(s, prog, 2, "LightPos");
	Shader_SetParam3f(s, prog, 2, light->eye.x, light->eye.y, light->eye.z);
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::Translatef(0.5f, 0.5f, 0.5f);
	gl1::Scalef(0.5f, 0.5f, 0.5f);
	gl1::Perspective(ctx->fovOffset + 75.0, ctx->lightAspect, ctx->nearZ, ctx->farZ);
	gl1::LookAt(light->eye.x, light->eye.y, light->eye.z, light->target.x, light->target.y,
	            light->target.z, light->up.x, light->up.y, light->up.z);
	Shader_SetTextureMatrix(s, prog, 6);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::PopMatrix();
	gl1::PushMatrix();
	Shader_SetMvpMatrix(s, prog, 5);
	texA = Tex_Get(&ctx->gfx, slotA);
	texB = Tex_Get(&ctx->gfx, slotB);
	mesh_draw_samplers(ctx, model, texA, texB, prog);
	gl1::PopMatrix();
	Shader_DisableFragment(s);
	Shader_DisableVertexProfile(s);
	gl1::PopMatrix();
	gl1::PushMatrix();
	gl1::MatrixMode(gl1::TEXTURE);
	gl1::LoadIdentity();
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::PopMatrix();
}

/*
 * OURS. glPushAttrib(GL_ALL_ATTRIB_BITS) and glPopAttrib, which GL 3.3
 * lacks, for the state the parts change: the enables, the depth
 * mask and function, gl1's GL_TEXTURE_2D, the point size and the
 * matrix mode. The restore matters: a Frame leaves depth testing off,
 * and every frame must start with Demo_InitGL's on, in the matrix mode
 * it began in.
 */
struct Attrib {
	GLboolean depth, blend, cull, depthMask;
	GLint     depthFunc;
	bool      texture2D;
	int       matrixMode;
	float     pointSize;
};

static Attrib g_attrib;

/* OURS */
void Attrib_Push(void)
{
	g_attrib.depth = glIsEnabled(GL_DEPTH_TEST);
	g_attrib.blend = glIsEnabled(GL_BLEND);
	g_attrib.cull = glIsEnabled(GL_CULL_FACE);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &g_attrib.depthMask);
	glGetIntegerv(GL_DEPTH_FUNC, &g_attrib.depthFunc);
	g_attrib.texture2D = gl1::Texture2DEnabled();
	g_attrib.matrixMode = gl1::MatrixModeNow();
	g_attrib.pointSize = gl1::PointSizeNow();
}

/* OURS */
static void gl_set(GLenum cap, GLboolean on)
{
	if (on) {
		glEnable(cap);
	} else {
		glDisable(cap);
	}
}

/* OURS */
void Attrib_Pop(void)
{
	gl_set(GL_DEPTH_TEST, g_attrib.depth);
	gl_set(GL_BLEND, g_attrib.blend);
	gl_set(GL_CULL_FACE, g_attrib.cull);
	glDepthMask(g_attrib.depthMask);
	glDepthFunc((GLenum)g_attrib.depthFunc);
	if (g_attrib.texture2D) {
		gl1::Enable(GL_TEXTURE_2D);
	} else {
		gl1::Disable(GL_TEXTURE_2D);
	}
	gl1::MatrixMode(g_attrib.matrixMode);
	gl1::PointSize(g_attrib.pointSize);
}

/* OURS. The inline clamp to [0, 1] the parts repeat. */
double Clamp01(double v)
{
	if (v > 1.0) {
		return 1.0;
	}
	if (v < 0.0) {
		return 0.0;
	}
	return v;
}

/* OURS. Camera_Set from nine values, as each camera window passes them. */
void Cam9(Camera *c, double ex, double ey, double ez, double tx, double ty, double tz,
          double ux, double uy, double uz)
{
	Camera_Set(c, &ex, &ey, &ez, &tx, &ty, &tz, &ux, &uy, &uz);
}

/*
 * OURS. The mouse look (app.cpp): the eye camera's view turned by the
 * drag, yaw about its up axis and pitch about its right one. Called
 * before every gluLookAt of the eye camera, after its LoadIdentity, so
 * the turn is in eye space; the eye itself does not move. Nothing at
 * 0 offset, so an untouched run is the original's.
 */
void Scene_LookOffset(const Rupture_Ctx *ctx)
{
	if (ctx->lookPitch != 0.0) {
		gl1::Rotatef((float)ctx->lookPitch, 1.0f, 0.0f, 0.0f);
	}
	if (ctx->lookYaw != 0.0) {
		gl1::Rotatef((float)ctx->lookYaw, 0.0f, 1.0f, 0.0f);
	}
}

/* ORIGIN 0x004698c0 maybe_Camera_LookAtEye: gluLookAt from the eye camera, after the mouse look. */
void maybe_Camera_LookAtEye(Rupture_Ctx *ctx)
{
	const Camera *e = &ctx->eye;

	Scene_LookOffset(ctx);

	gl1::LookAt(e->eye.x, e->eye.y, e->eye.z, e->target.x, e->target.y, e->target.z,
	            e->up.x, e->up.y, e->up.z);
}

/* OURS. A kind-1 Vec4. */
Vec4 Vec4_Make(float x, float y, float z)
{
	Vec4 v;

	v.kind = 1;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

/* OURS. out = a + b * s, through the demo's Vec4_Scale and Vec4_Add. */
Vec4 Vec4_AddScaled(const Vec4 *a, const Vec4 *b, float s)
{
	Vec4 m, o;

	Vec4_Scale(b, &s, &m);
	Vec4_Add(a, &m, &o);
	return o;
}

/* OURS. A Vec4 of three draws, 2r - 1 each, x y z in order, y made positive or not. */
Vec4 Vec4_RandomDir(bool absY)
{
	Vec4 v;

	v.kind = 1;
	v.x = (float)(Math_Random() * 2.0 - 1.0);
	v.y = (float)(Math_Random() * 2.0 - 1.0);
	v.z = (float)(Math_Random() * 2.0 - 1.0);
	if (absY) {
		v.y = (float)fabs(v.y);
	}
	return v;
}

/*
 * OURS. The frame a road track's point k gives: D its direction from
 * point k - 1, R = D x (0, 1, 0) (not normalised), U = normalize(R x D).
 */
void Scene_RoadFrame(const Track *T, int k, Vec4 *P, Vec4 *D, Vec4 *R, Vec4 *U)
{
	const Vec4 up = Vec4_Make(0.0f, 1.0f, 0.0f);

	*P = T->points[k];
	Vec4_Sub(P, &T->points[k - 1], D);
	Vec4_Normalize(D);
	Vec4_Cross(D, &up, R);
	Vec4_Cross(R, D, U);
	Vec4_Normalize(U);
}

/*
 * OURS, the rails cache (subsystems/render.txt 5). A rails draw's
 * vertices depend on its track's points and its depth alone, and the
 * parts fill their tracks in Init. So the first draw of a (draw,
 * track, depth) is compiled into a gl1 display list and the next ones
 * call it, in the same colour, which the draws never set. The state
 * a draw sets on its way - cull face off, T->loop = 0 - is set on
 * every call. The points are hashed each time, so a track that did
 * change would be compiled again rather than drawn stale. Off with
 * gl1::SetBatching(false), which draws as the transcription does.
 */
void Scene_DrawRailsCached(Rupture_Ctx *ctx, Track *T, double depth,
                           void (*draw)(Rupture_Ctx *, Track *, double))
{
	struct Entry {
		void (*draw)(Rupture_Ctx *, Track *, double);
		const Track *track;
		double       depth;
		unsigned     hash;
		int          list;
		unsigned     used;
	};
	static Entry          cache[16];
	static unsigned       clock;
	const unsigned char  *b;
	unsigned              h = 2166136261u;
	size_t                i, n;
	Entry                *e = NULL;

	if (!gl1::Batching() || T->points.empty()) {
		draw(ctx, T, depth);
		return;
	}
	b = (const unsigned char *)&T->points[0];
	n = T->points.size() * sizeof(Vec4);
	for (i = 0; i < n; i++) {
		h = (h ^ b[i]) * 16777619u;
	}
	h = (h ^ (unsigned)T->count) * 16777619u;
	clock++;
	for (i = 0; i < 16; i++) {
		Entry *c = &cache[i];

		if (c->list != 0 && c->draw == draw && c->track == T && c->depth == depth && c->hash == h) {
			c->used = clock;
			glDisable(GL_CULL_FACE);
			T->loop = 0;
			gl1::CallList(c->list);
			return;
		}
		if (e == NULL || c->used < e->used) {
			e = c;
		}
	}
	if (e->list == 0) {
		e->list = gl1::GenLists(1);
	}
	e->draw = draw;
	e->track = T;
	e->depth = depth;
	e->hash = h;
	e->used = clock;
	gl1::NewList(e->list, gl1::COMPILE_AND_EXECUTE);
	draw(ctx, T, depth);
	gl1::EndList();
}

/*
 * OURS. The bracket every lit section of a DrawScene puts round
 * maybe_Scene_DrawLit: profiles off, ctx+0x1bd8 = 0, the set-up with
 * decal = texBase + 12 and light = texNormal, ctx+0x1bd8 = 1, then
 * the pair bound and the profiles on.
 */
void Scene_Lit(Rupture_Ctx *ctx, int model, int texBase, int texNormal, int pair, double time)
{
	Shader_Ctx *s = &ctx->gfx.shader;

	Shader_DisableProfiles(s);
	ctx->litFlag = 0.0f;
	gl1::PushMatrix();
	maybe_Scene_DrawLit(ctx, model, texBase + 0xc, texNormal, pair, &ctx->eye, &ctx->light, time);
	gl1::PopMatrix();
	ctx->litFlag = 1.0f;
	Shader_Bind(s, pair);
	Shader_Bind(s, pair + 1);
	Shader_EnableProfiles(s);
}
