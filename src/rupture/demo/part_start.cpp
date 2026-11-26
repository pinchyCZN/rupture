#include "part_start.h"

#include "../core/gl1.h"
#include "scene.h"

/*
 * ORIGIN 0x00446440 part_start::part_start. It builds the members
 * every part class shares - a Path, two meshes, a vector, some Vec4s -
 * which part_start never uses; only the four Init sets are kept.
 */
part_start::part_start()
{
	d1d0 = 0.0;
	d1d8 = 0.0;
	i1e8 = 0;
	texBase = 0;
}

/* ORIGIN 0x0049f800 part_start::Init: textures/theloader.jpg, a 1024x1024 image, into slot 10. */
void part_start::Init(Rupture_Ctx *ctx)
{
	d1d0 = 0.0;
	d1d8 = 0.0;
	i1e8 = 0;
	texBase = 0;
	ctx->gfx.tex[10] = Tex_Load("textures/theloader.jpg");
}

/*
 * ORIGIN 0x0049f830 part_start::Frame: the image on Gfx_DrawQuad's
 * quad scaled by 3.3 (0x004f4e88), 5 (0x004f40c8) from a 65 degree
 * view (0x004f4938, plus the context's offset). At that distance the
 * view is 6.37 high, so the square image fills the screen's height and
 * a little more, centred, black beside it at any width.
 */
void part_start::Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq)
{
	double angle = 0.0, sx = 3.3, sy = 3.3;

	(void)dt;
	(void)seq;
	Attrib_Push();
	gl1::PushMatrix();
	Fbo_Unbind();
	glViewport(0, 0, ctx->width, ctx->height);
	gl1::MatrixMode(gl1::PROJECTION);
	gl1::LoadIdentity();
	gl1::Perspective(ctx->fovOffset + 65.0, (double)ctx->width / (double)ctx->height, 1.0, 100.0);
	gl1::MatrixMode(gl1::MODELVIEW);
	gl1::LoadIdentity();
	gl1::LookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl1::Enable(GL_TEXTURE_2D);
	Tex_Bind(&ctx->gfx, texBase + 10);
	glDisable(GL_BLEND);
	gl1::Color3f(1.0f, 1.0f, 1.0f);
	Gfx_DrawQuad(&angle, &sx, &sy, 1);
	gl1::PopMatrix();
	Attrib_Pop();
	glFlush();
}
