/*
 * The render targets and the post chain every part runs
 * (effects/look.txt 3), and the small maths functions that go with
 * them. Gfx_Ctx is the part of the render context (g_renderCtx) they
 * use, at the offsets noted.
 */
#ifndef RUPTURE_POST_H
#define RUPTURE_POST_H

#include <map>

#include "glapi.h"
#include "shader.h"

enum {
	GFX_TARGETS = 16     /* Fbo_Bind clamps the target to 0..15 */
};

struct Gfx_Ctx {
	Shader_Ctx            shader;                   /* ctx+0x1d0c */
	std::map<int, GLuint> tex;                      /* ctx+0xb1998, an array there */
	int                   texBase;                  /* ctx+0x23851c */
	GLuint                renderFbo[GFX_TARGETS];   /* ctx+0x238498 */
	GLuint                resolveFbo[GFX_TARGETS];  /* ctx+0x2384d8 */
	int                   oned;                     /* ctx+0x19c0, -1 = none */
};

bool   Fbo_Create(Gfx_Ctx *c, int fbo, int w, int h, int samples);
GLuint Tex_Get(Gfx_Ctx *c, int tex);
void   Tex_Bind(Gfx_Ctx *c, int tex);
void   Gfx_ResetState(void);
void   Tex_CreateTarget(Gfx_Ctx *c, int tex, int w, int h);
void   Fbo_AttachTexture(Gfx_Ctx *c, int fbo, int attachment, int tex);
void   Fbo_Bind(Gfx_Ctx *c, int fbo, int buffers);
void   Fbo_Unbind(void);
void   Fbo_SetScreen(GLuint fbo);
void   Fbo_Resolve(Gfx_Ctx *c, int fbo, int w, int h, int buffers);
void   Gfx_DrawQuad(const double *angle, const double *sx, const double *sy, char reset);
void   Post_BindDecal(Gfx_Ctx *c, int slot, int tex);
void   Post_BindDecal2(Gfx_Ctx *c, int slot, int tex, int tex2);
void   Post_BindDecal3(Gfx_Ctx *c, int slot, int tex, int tex2, int tex3);
void   Post_RunPass(Gfx_Ctx *c, int slot, int target, int passes, int w, int h,
                    int texFirst, int texRest, int tex2, int tex3);

double Math_Ease(const double *x, const double *p);
double Math_SoftRamp(const double *u);
void   Crt_Srand(unsigned seed);
int    Crt_Rand(void);
double Math_Random(void);

/* A texture from a release image, uploaded as maybe_Tex_Load does. */
GLuint Tex_Load(const char *path);
GLuint Tex_LoadWithAlpha(const char *colourPath, const char *alphaPath);

#endif
