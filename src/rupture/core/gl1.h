/*
 * The fixed-function GL the original calls, on top of GL 3.3 core and
 * WebGL2, which have none of it: the three matrix stacks, gluPerspective
 * and gluLookAt, the current colour, and glBegin/glEnd. The calls keep
 * the original's names and argument order, so a transcribed routine
 * reads like the listing with gl1:: in front.
 *
 * Shaders read the matrices as uniforms (core/shader.h) and the vertex
 * data from fixed attribute slots: position, normal, the colour the
 * Cg programs call DIFFUSE, the texture coordinate and the tangent.
 */
#ifndef RUPTURE_GL1_H
#define RUPTURE_GL1_H

#include "glapi.h"

namespace gl1 {

enum Mode {
	MODELVIEW  = 0x1700,
	PROJECTION = 0x1701,
	TEXTURE    = 0x1702
};

enum Attrib {
	ATTRIB_POSITION = 0,
	ATTRIB_NORMAL   = 1,
	ATTRIB_DIFFUSE  = 2,
	ATTRIB_UV       = 3,
	ATTRIB_TANGENT  = 4
};

enum Primitive {
	POINTS     = 0x0000,
	LINES      = 0x0001,
	TRIANGLES  = 0x0004,
	QUADS      = 0x0007,
	QUAD_STRIP = 0x0008
};

enum ListMode {
	COMPILE             = 0x1300,
	COMPILE_AND_EXECUTE = 0x1301
};

void MatrixMode(int mode);
int  MatrixModeNow(void);
void LoadIdentity(void);
void PushMatrix(void);
void PopMatrix(void);
void LoadMatrixf(const float *m);
void MultMatrixf(const float *m);
void Translatef(float x, float y, float z);
void Rotatef(float angle, float x, float y, float z);
void Scalef(float x, float y, float z);
void Perspective(double fovy, double aspect, double zNear, double zFar);
void SetZoom(double zoom);   /* > 1 narrows the projection's perspectives */
void LookAt(double eyeX, double eyeY, double eyeZ, double atX, double atY, double atZ,
            double upX, double upY, double upZ);

/* The top of a stack, column major, as glGetFloatv would give it. */
const float *Matrix(int mode);

/* Projection times modelview: what Cg's MODELVIEW_PROJECTION reads. */
void ModelViewProjection(float out[16]);

void Color3f(float r, float g, float b);
void Color4f(float r, float g, float b, float a);

/* Sets the DIFFUSE attribute to the current colour for an array draw. */
void ApplyColor(void);

/*
 * glEnable and glDisable, except GL_TEXTURE_2D, which core GL refuses:
 * that one is recorded for the fixed-function stand-in shader.
 */
void Enable(GLenum cap);
void Disable(GLenum cap);
bool Texture2DEnabled(void);

/* Called before every draw; the shader layer hangs its apply here. */
void SetDrawHook(void (*hook)(void));
void BeforeDraw(void);

/*
 * OURS, the hot-spot batching (docs/subsystems/render.txt). Between
 * BatchBegin and BatchEnd an End does not draw: its vertices join one
 * buffer, drawn by BatchFlush in a single call. Anything that would
 * change how they draw flushes first - a shader value, texture or
 * program, the projection, the texture switch, the point size, a GL
 * cap through Enable or Disable, a display list, another primitive
 * kind - so the draws keep the order and look the original's have.
 * For the stand-in's draws the modelview is applied as each End joins
 * (it is affine, so the positions stay three floats) and the flush
 * draws with the projection alone; a pair's matrices are its own
 * uniforms. A raw gl* state call inside a batch is not seen: flush
 * before one.
 */
void BatchBegin(void);
void BatchEnd(void);
void BatchFlush(void);
void SetFfQuery(bool (*query)(void));
void SetBatching(bool on);   /* off: BatchBegin does nothing, every End draws */
bool Batching(void);         /* on unless --nobatch: the hot-spot work may run */

/* glPointSize, read by the stand-in shader as gl_PointSize. */
void  PointSize(float size);
float PointSizeNow(void);

void Begin(int primitive);
void Normal3f(float x, float y, float z);
void TexCoord2f(float s, float t);
void Vertex3f(float x, float y, float z);
void End(void);

/*
 * Display lists: what Begin/End draw between NewList and EndList is
 * kept in a vertex buffer and drawn again by CallList. A list records
 * no colour - the original's lists never call glColor - so CallList
 * draws in the current one.
 */
int  GenLists(int range);
void NewList(int list, int mode);
void EndList(void);
void CallList(int list);

}

#endif
