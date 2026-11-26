#include "post.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "file.h"
#include "gl1.h"

/*
 * OURS. The texture lookup every function below inlines: a number
 * under 85000 is per-part and offset by texBase, anything else is
 * absolute.
 */
static GLuint *tex_slot(Gfx_Ctx *c, int tex)
{
	if (tex < 85000) {
		return &c->tex[c->texBase + tex];
	}
	return &c->tex[tex];
}

/*
 * ORIGIN 0x0044c140 Fbo_Create: render FBO fbo (clamped to 0..15) with
 * two GL_RGBA8 colour renderbuffers and a GL_DEPTH_COMPONENT32 one, all
 * with `samples` samples, and an empty resolve FBO for Fbo_AttachTexture.
 * Demo_Init makes target 0 at the view size with 4 samples and targets
 * 1 and 2 at a quarter and a half with 0 (effects/look.txt 1). The
 * original checks no status; the return is ours. Divergence: WebGL2
 * has no 32-bit fixed-point depth, so the browser gets
 * GL_DEPTH_COMPONENT32F, which coplanar faces can resolve differently.
 */
bool Fbo_Create(Gfx_Ctx *c, int fbo, int w, int h, int samples)
{
#ifdef __EMSCRIPTEN__
	static const GLenum formats[3] = { GL_RGBA8, GL_RGBA8, GL_DEPTH_COMPONENT32F };
#else
	static const GLenum formats[3] = { GL_RGBA8, GL_RGBA8, GL_DEPTH_COMPONENT32 };
#endif
	static const GLenum points[3] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1, GL_DEPTH_ATTACHMENT
	};
	GLuint rb[3];
	int    i;
	bool   ok;

	if (fbo > 15) {
		fbo = 15;
	} else if (fbo < 0) {
		fbo = 0;
	}
	for (i = 0; i < 3; i++) {
		glGenRenderbuffers(1, &rb[i]);
		glBindRenderbuffer(GL_RENDERBUFFER, rb[i]);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, formats[i], w, h);
	}
	glGenFramebuffers(1, &c->renderFbo[fbo]);
	glBindFramebuffer(GL_FRAMEBUFFER, c->renderFbo[fbo]);
	for (i = 0; i < 3; i++) {
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, points[i], GL_RENDERBUFFER, rb[i]);
	}
	ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	glGenFramebuffers(1, &c->resolveFbo[fbo]);
	glBindFramebuffer(GL_FRAMEBUFFER, c->resolveFbo[fbo]);
	return ok;
}

/* OURS. The texture a slot number names, as the demo's callers look it up. */
GLuint Tex_Get(Gfx_Ctx *c, int tex)
{
	return *tex_slot(c, tex);
}

/*
 * ORIGIN 0x004bc800 Gfx_ResetState. GL_LIGHTING, GL_COLOR_MATERIAL,
 * GL_LIGHT0 and GL_FOG, which it also disables, do not exist in core
 * GL and nothing here enables them.
 */
void Gfx_ResetState(void)
{
	glDisable(GL_BLEND);
	gl1::Disable(GL_TEXTURE_2D);
	gl1::Color4f(1.0f, 1.0f, 1.0f, 1.0f);
}

/*
 * ORIGIN 0x004647e0 maybe_Tex_LoadWithAlpha: colourPath's RGB with
 * alphaPath's green as alpha, internal format 4, GL_LINEAR both ways,
 * rows bottom up. Row 0 is left zero and the alpha image's bottom
 * scanline lands in row 1, as the demo's row loop clamps its index to
 * 1. The compressed-texture branch is left out, as Tex_Load leaves it.
 */
GLuint Tex_LoadWithAlpha(const char *colourPath, const char *alphaPath)
{
	Image  col, alp;
	GLuint tex = 0;
	int    w, h, r, x;

	if (!Image_Load(colourPath, col) || !Image_Load(alphaPath, alp)) {
		return 0;
	}
	w = col.width;
	h = col.height;
	std::vector<unsigned char> px((size_t)w * h * 4, 0);
	for (r = 1; r < h; r++) {
		int ar = r == 1 ? 0 : r;   /* the alpha row that lands in row r */

		for (x = 0; x < w; x++) {
			unsigned char       *d = &px[((size_t)r * w + x) * 4];
			const unsigned char *c = &col.rgba[((size_t)r * w + x) * 4];

			d[0] = c[0];
			d[1] = c[1];
			d[2] = c[2];
			d[3] = alp.rgba[((size_t)ar * alp.width + x) * 4 + 1];
		}
	}
	glGenTextures(1, &tex);
	gl1::BatchFlush();
	glBindTexture(GL_TEXTURE_2D, tex);
	Shader_Touch();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &px[0]);
	return tex;
}

/* ORIGIN 0x004baf70 maybe_Tex_Bind. */
void Tex_Bind(Gfx_Ctx *c, int tex)
{
	gl1::BatchFlush();
	glBindTexture(GL_TEXTURE_2D, *tex_slot(c, tex));
	Shader_Touch();
}

/*
 * ORIGIN 0x0044e6c0 Tex_CreateTarget, filled with _rand() noise as the
 * original fills it. No wrap mode is set: targets wrap with GL_REPEAT,
 * so a blur near one edge of the screen samples the other. Do not
 * "fix" it to GL_CLAMP_TO_EDGE. The mipmapped variant behind
 * ctx+0x19b8 is left out: the flag is always 0 (effects/look.txt 1).
 */
void Tex_CreateTarget(Gfx_Ctx *c, int tex, int w, int h)
{
	std::vector<unsigned char> pixels((size_t)w * h * 4);
	size_t                     i;

	glGenTextures(1, tex_slot(c, tex));
	gl1::BatchFlush();
	glBindTexture(GL_TEXTURE_2D, *tex_slot(c, tex));
	Shader_Touch();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	for (i = 0; i < pixels.size(); i++) {
		pixels[i] = (unsigned char)(Crt_Rand() % 0xff);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
}

/* ORIGIN 0x0044c370 Fbo_AttachTexture. Attaches to the resolve FBO. */
void Fbo_AttachTexture(Gfx_Ctx *c, int fbo, int attachment, int tex)
{
	if (attachment > 2) {
		attachment = 2;
	} else if (attachment < 0) {
		attachment = 0;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, c->resolveFbo[fbo]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D,
	                       *tex_slot(c, tex), 0);
	glCheckFramebufferStatus(GL_FRAMEBUFFER);
}

/* ORIGIN 0x0044c4d0 Fbo_Bind. Binds the render FBO, 1 to 3 buffers. */
void Fbo_Bind(Gfx_Ctx *c, int fbo, int buffers)
{
	static const GLenum bufs[3] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1, GL_COLOR_ATTACHMENT0 + 2
	};

	if (buffers > 3) {
		buffers = 3;
	} else if (buffers < 1) {
		buffers = 1;
	}
	if (fbo > 15) {
		fbo = 15;
	} else if (fbo < 0) {
		fbo = 0;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, c->renderFbo[fbo]);
	glDrawBuffers(buffers, bufs);
}

static GLuint g_screenFbo;   /* what Fbo_Unbind binds: 0, or the host's */

/*
 * ORIGIN 0x0044c4b0 Fbo_Unbind. Divergence: the screen is the
 * framebuffer the host set, 0 unless it places the picture itself
 * (Fbo_SetScreen).
 */
void Fbo_Unbind(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, g_screenFbo);
}

/* OURS. The host's borderless fullscreen draws the frame here and copies it to the monitor. */
void Fbo_SetScreen(GLuint fbo)
{
	g_screenFbo = fbo;
}

/*
 * ORIGIN 0x0044be80 Fbo_Resolve. Render FBO -> resolve FBO, one blit
 * per colour attachment. The depth blit after these is behind a flag
 * the code holds at 0 and is left out. glDrawBuffer(attachment i) is
 * glDrawBuffers with i in slot i, the form GLES3 accepts.
 */
void Fbo_Resolve(Gfx_Ctx *c, int fbo, int w, int h, int buffers)
{
	GLenum bufs[3];
	int    i, k;

	for (i = 0; i < 3 && i < buffers; i++) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, c->renderFbo[fbo]);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, c->resolveFbo[fbo]);
		for (k = 0; k <= i; k++) {
			bufs[k] = k == i ? GL_COLOR_ATTACHMENT0 + i : GL_NONE;
		}
		glDrawBuffers(i + 1, bufs);
		glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
		glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
}

/*
 * ORIGIN 0x004049f0 Gfx_DrawQuad: a billboard. With reset, the
 * modelview's 3x3 becomes identity and its translation stays, loaded
 * into the current matrix. sx <= 0 mirrors u.
 */
void Gfx_DrawQuad(const double *angle, const double *sx, const double *sy, char reset)
{
	float m[16];

	memcpy(m, gl1::Matrix(gl1::MODELVIEW), sizeof m);
	m[0] = 1.0f;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[4] = 0.0f;
	m[5] = 1.0f;
	m[6] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[10] = 1.0f;
	if (reset) {
		gl1::LoadMatrixf(m);
	}
	gl1::Rotatef((float)*angle, 0.0f, 0.0f, 1.0f);
	gl1::Scalef((float)fabs((float)*sx), (float)*sy, 0.0f);
	gl1::Begin(gl1::QUADS);
	if (*sx <= 0.0) {
		gl1::TexCoord2f(1.0f, 0.0f);
		gl1::Vertex3f(-1.0f, -1.0f, 0.0f);
		gl1::TexCoord2f(0.0f, 0.0f);
		gl1::Vertex3f(1.0f, -1.0f, 0.0f);
		gl1::TexCoord2f(0.0f, 1.0f);
		gl1::Vertex3f(1.0f, 1.0f, 0.0f);
		gl1::TexCoord2f(1.0f, 1.0f);
	} else {
		gl1::TexCoord2f(0.0f, 0.0f);
		gl1::Vertex3f(-1.0f, -1.0f, 0.0f);
		gl1::TexCoord2f(1.0f, 0.0f);
		gl1::Vertex3f(1.0f, -1.0f, 0.0f);
		gl1::TexCoord2f(1.0f, 1.0f);
		gl1::Vertex3f(1.0f, 1.0f, 0.0f);
		gl1::TexCoord2f(0.0f, 1.0f);
	}
	gl1::Vertex3f(-1.0f, 1.0f, 0.0f);
	gl1::End();
}

/* ORIGIN 0x004bb7c0 Post_BindDecal. */
void Post_BindDecal(Gfx_Ctx *c, int slot, int tex)
{
	Shader_DisableVertexProfile(&c->shader);
	Shader_DisableFragment(&c->shader);
	Shader_EnableFragmentProfile(&c->shader);
	Shader_Bind(&c->shader, slot);
	Shader_SetTexture(&c->shader, slot, "decal", *tex_slot(c, tex));
	if (c->oned >= 0) {
		Shader_SetTexture(&c->shader, slot, "oned", *tex_slot(c, c->oned));
	}
}

/* ORIGIN 0x004bba20 Post_BindDecal2. */
void Post_BindDecal2(Gfx_Ctx *c, int slot, int tex, int tex2)
{
	Shader_DisableVertexProfile(&c->shader);
	Shader_EnableFragmentProfile(&c->shader);
	Shader_Bind(&c->shader, slot);
	Shader_SetTexture(&c->shader, slot, "decal", *tex_slot(c, tex));
	Shader_SetTexture(&c->shader, slot, "decal2", *tex_slot(c, tex2));
	if (c->oned >= 0) {
		Shader_SetTexture(&c->shader, slot, "oned", *tex_slot(c, c->oned));
	}
}

/* ORIGIN 0x004bb8a0 Post_BindDecal3. */
void Post_BindDecal3(Gfx_Ctx *c, int slot, int tex, int tex2, int tex3)
{
	Shader_DisableVertexProfile(&c->shader);
	Shader_EnableFragmentProfile(&c->shader);
	Shader_Bind(&c->shader, slot);
	Shader_SetTexture(&c->shader, slot, "decal", *tex_slot(c, tex));
	Shader_SetTexture(&c->shader, slot, "decal2", *tex_slot(c, tex2));
	Shader_SetTexture(&c->shader, slot, "decal3", *tex_slot(c, tex3));
	if (c->oned >= 0) {
		Shader_SetTexture(&c->shader, slot, "oned", *tex_slot(c, c->oned));
	}
}

/*
 * ORIGIN 0x004bc980 Post_RunPass. Fragment program `slot` as a
 * full-screen quad, `passes` times, into `target` at w x h. decal is
 * texFirst on the first pass and texRest after, which is how a
 * multi-pass blur reads its own previous result.
 */
void Post_RunPass(Gfx_Ctx *c, int slot, int target, int passes, int w, int h,
                  int texFirst, int texRest, int tex2, int tex3)
{
	double one = 1.0, zero = 0.0;
	int    pass;

	for (pass = 0; pass < passes; pass++) {
		Shader_EnableProfiles(&c->shader);
		gl1::PushMatrix();
		glViewport(0, 0, w, h);
		Fbo_Bind(c, target, 1);
		gl1::MatrixMode(gl1::PROJECTION);
		gl1::LoadIdentity();
		gl1::MatrixMode(gl1::TEXTURE);
		gl1::LoadIdentity();
		gl1::MatrixMode(gl1::MODELVIEW);
		gl1::LoadIdentity();
		gl1::Color3f(1.0f, 1.0f, 1.0f);
		gl1::LoadIdentity();
		gl1::Enable(GL_TEXTURE_2D);
		if (tex2 >= 0) {
			if (tex3 < 0) {
				Post_BindDecal2(c, slot, pass == 0 ? texFirst : texRest, tex2);
			} else {
				Post_BindDecal3(c, slot, pass == 0 ? texFirst : texRest, tex2, tex3);
			}
		} else if (tex3 < 0) {
			Post_BindDecal(c, slot, pass == 0 ? texFirst : texRest);
		}
		Gfx_DrawQuad(&zero, &one, &one, 1);
		gl1::PopMatrix();
		Fbo_Resolve(c, target, w, h, 1);
		Fbo_Unbind();
	}
	Shader_DisableProfiles(&c->shader);
}

/*
 * ORIGIN 0x00402830 Math_Ease. x87 code; with p = 1 it reduces to
 * (1 - cos(pi x)) / 2.
 */
double Math_Ease(const double *x, const double *p)
{
	double h, r;

	h = 0.5 * (1.0 + cos(*x * 3.141592653589793));
	r = pow(h, 1.0 + 0.5 * (*p - 1.0));
	return pow(1.0 - r, *p);
}

/*
 * ORIGIN 0x00401590 Math_SoftRamp: pow(u / 2, 2.2) blending into u as
 * 0.3 u runs from 0 to 1.
 */
double Math_SoftRamp(const double *u)
{
	double p = pow(*u * 0.5, 2.2), c = *u * 0.3;

	c = c > 1.0 ? 1.0 : c < 0.0 ? 0.0 : c;
	return (1.0 - c) * (p - *u) + *u;
}

static unsigned long g_holdrand = 1;

/* ORIGIN 0x004c36ce _srand, the CRT's; one seed here, per thread there. */
void Crt_Srand(unsigned seed)
{
	g_holdrand = seed;
}

/* ORIGIN 0x004c36e0 _rand, the CRT's linear congruential generator. */
int Crt_Rand(void)
{
	g_holdrand = (g_holdrand * 0x343fdUL + 0x269ec3UL) & 0xffffffffUL;
	return (int)((g_holdrand >> 16) & 0x7fff);
}

/* ORIGIN 0x0044ccb0 Math_Random. */
double Math_Random(void)
{
	return Crt_Rand() / 32767.0;
}

/*
 * OURS, standing in for maybe_Tex_Load 0x00462450: the demo decodes
 * with its own JPEG decoder and uploads RGB, internal format 3, with
 * GL_LINEAR both ways, rows bottom up. Image_Load decodes JPEGs with
 * jpgd, as the demo does (core/file.cpp).
 */
GLuint Tex_Load(const char *path)
{
	Image  img;
	GLuint tex = 0;

	if (!Image_Load(path, img)) {
		return 0;
	}
	glGenTextures(1, &tex);
	gl1::BatchFlush();
	glBindTexture(GL_TEXTURE_2D, tex);
	Shader_Touch();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
	             &img.rgba[0]);
	return tex;
}
