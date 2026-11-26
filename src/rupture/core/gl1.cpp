#include "gl1.h"

#include <math.h>
#include <string.h>
#include <map>
#include <vector>

namespace gl1 {

enum { STACK_DEPTH = 32, VERTEX_FLOATS = 12, STREAM_BYTES = 16 << 20 };

struct Stack {
	float m[STACK_DEPTH][16];
	int   top;
};

static Stack              g_stacks[3];
static int                g_mode = MODELVIEW;
static float              g_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static float              g_normal[3] = { 0.0f, 0.0f, 1.0f };
static float              g_uv[2];
static int                g_primitive;
static std::vector<float> g_verts;
static GLuint             g_vao, g_vbo;
static size_t             g_vboSize, g_vboUsed;   /* the stream buffer, filled front to back */
static bool               g_texture2D;
static float              g_pointSize = 1.0f;
static double             g_zoom = 1.0;       /* the mouse wheel's lens, SetZoom */

struct List {
	GLuint             vao;
	GLuint             vbo;
	GLsizei            count;
	std::vector<float> verts;
};

static std::map<int, List> g_lists;
static int                 g_listNext = 1;
static int                 g_listOpen;
static int                 g_listMode;
static void             (*g_drawHook)(void);
static bool             (*g_ffQuery)(void);   /* true while the vertex stage is the stand-in */
static bool               g_batching;
static bool               g_batchOn = true;
static std::vector<float> g_batch;
static int                g_batchKind;        /* 0 triangles, 1 points, 2 lines */
static bool               g_batchEye;         /* the batch's positions are eye space */
static bool               g_projOnly;         /* ModelViewProjection gives the projection alone */

struct CullRun {
	size_t first;   /* the batch vertex it starts at */
	bool   cull;
};

static std::vector<CullRun> g_cullRuns;      /* empty: the whole batch under the current culling */

/* OURS. A change the open batch cannot carry: draw what it holds first. */
static void batch_break(void)
{
	if (g_batching && !g_batch.empty()) {
		BatchFlush();
	}
}

/* OURS. Face culling switched inside a batch: the vertices from here on draw as a run of their own. */
static void cull_run(bool on)
{
	CullRun r;

	if (!g_batching || g_batch.empty()) {
		return;
	}
	if (g_cullRuns.empty()) {
		r.first = 0;
		r.cull = !on;
		g_cullRuns.push_back(r);
	}
	r.first = g_batch.size() / VERTEX_FLOATS;
	r.cull = on;
	if (g_cullRuns.back().first == r.first) {
		g_cullRuns.back().cull = on;
	} else {
		g_cullRuns.push_back(r);
	}
}

/* OURS. A change to the current matrix: only the projection's matters to a batch. */
static void matrix_break(void)
{
	if (g_mode == PROJECTION) {
		batch_break();
	}
}

/* OURS */
static Stack &current(void)
{
	return g_stacks[g_mode - MODELVIEW];
}

/* OURS. out = a * b, column major, out may alias neither. */
static void multiply(const float *a, const float *b, float *out)
{
	int c, r;

	for (c = 0; c < 4; c++) {
		for (r = 0; r < 4; r++) {
			out[c * 4 + r] = a[r] * b[c * 4] + a[4 + r] * b[c * 4 + 1] + a[8 + r] * b[c * 4 + 2]
			               + a[12 + r] * b[c * 4 + 3];
		}
	}
}

/* OURS. Every stack starts with one identity entry. */
static void init_stacks(void)
{
	static bool done;
	int         i;

	if (done) {
		return;
	}
	done = true;
	for (i = 0; i < 3; i++) {
		memset(g_stacks[i].m[0], 0, sizeof g_stacks[i].m[0]);
		g_stacks[i].m[0][0] = g_stacks[i].m[0][5] = g_stacks[i].m[0][10] = g_stacks[i].m[0][15] = 1.0f;
	}
}

/* OURS, as glMatrixMode. */
void MatrixMode(int mode)
{
	init_stacks();
	g_mode = mode;
}

/* OURS, as glLoadIdentity. */
void LoadIdentity(void)
{
	float *m;

	matrix_break();
	init_stacks();
	m = current().m[current().top];
	memset(m, 0, 16 * sizeof(float));
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}

/* OURS, as glGetIntegerv(GL_MATRIX_MODE). */
int MatrixModeNow(void)
{
	return g_mode;
}

/* OURS, as glLoadMatrixf. */
void LoadMatrixf(const float *m)
{
	matrix_break();
	init_stacks();
	memcpy(current().m[current().top], m, 16 * sizeof(float));
}

/* OURS, as glPushMatrix. */
void PushMatrix(void)
{
	Stack &s = current();

	init_stacks();
	if (s.top + 1 < STACK_DEPTH) {
		memcpy(s.m[s.top + 1], s.m[s.top], 16 * sizeof(float));
		s.top++;
	}
}

/* OURS, as glPopMatrix. */
void PopMatrix(void)
{
	Stack &s = current();

	matrix_break();
	if (s.top > 0) {
		s.top--;
	}
}

/* OURS, as glMultMatrixf. */
void MultMatrixf(const float *m)
{
	float  out[16];
	float *top;

	matrix_break();
	init_stacks();
	top = current().m[current().top];
	multiply(top, m, out);
	memcpy(top, out, sizeof out);
}

/* OURS, as glTranslatef. */
void Translatef(float x, float y, float z)
{
	float m[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

	m[12] = x;
	m[13] = y;
	m[14] = z;
	MultMatrixf(m);
}

/* OURS, as glRotatef: angle in degrees about (x, y, z), normalised. */
void Rotatef(float angle, float x, float y, float z)
{
	float m[16] = { 0 };
	float len = (float)sqrt(x * x + y * y + z * z);
	float c, s, k;

	if (len == 0.0f) {
		return;
	}
	x /= len;
	y /= len;
	z /= len;
	c = (float)cos(angle * 3.14159265358979323846 / 180.0);
	s = (float)sin(angle * 3.14159265358979323846 / 180.0);
	k = 1.0f - c;
	m[0] = x * x * k + c;
	m[1] = y * x * k + z * s;
	m[2] = x * z * k - y * s;
	m[4] = x * y * k - z * s;
	m[5] = y * y * k + c;
	m[6] = y * z * k + x * s;
	m[8] = x * z * k + y * s;
	m[9] = y * z * k - x * s;
	m[10] = z * z * k + c;
	m[15] = 1.0f;
	MultMatrixf(m);
}

/* OURS, as glScalef. */
void Scalef(float x, float y, float z)
{
	float m[16] = { 0 };

	m[0] = x;
	m[5] = y;
	m[10] = z;
	m[15] = 1.0f;
	MultMatrixf(m);
}

/* OURS, as gluPerspective. */
void Perspective(double fovy, double aspect, double zNear, double zFar)
{
	float  m[16] = { 0 };
	double f = 1.0 / tan(fovy * 3.14159265358979323846 / 360.0);

	if (g_mode == PROJECTION) {
		f *= g_zoom;
	}

	m[0] = (float)(f / aspect);
	m[5] = (float)f;
	m[10] = (float)((zFar + zNear) / (zNear - zFar));
	m[11] = -1.0f;
	m[14] = (float)(2.0 * zFar * zNear / (zNear - zFar));
	MultMatrixf(m);
}

/*
 * OURS. The viewer's zoom (app.cpp): every perspective built into the
 * projection matrix is narrowed by it, 1 / tan(fovy / 2) times zoom.
 * The light's projection is built in the texture matrix and is left
 * alone. At 1, the default, nothing changes.
 */
void SetZoom(double zoom)
{
	g_zoom = zoom;
}

/* OURS, as gluLookAt. */
void LookAt(double eyeX, double eyeY, double eyeZ, double atX, double atY, double atZ,
            double upX, double upY, double upZ)
{
	double f[3], s[3], u[3], len;
	float  m[16] = { 0 };

	f[0] = atX - eyeX;
	f[1] = atY - eyeY;
	f[2] = atZ - eyeZ;
	len = sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
	f[0] /= len;
	f[1] /= len;
	f[2] /= len;
	s[0] = f[1] * upZ - f[2] * upY;
	s[1] = f[2] * upX - f[0] * upZ;
	s[2] = f[0] * upY - f[1] * upX;
	len = sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
	s[0] /= len;
	s[1] /= len;
	s[2] /= len;
	u[0] = s[1] * f[2] - s[2] * f[1];
	u[1] = s[2] * f[0] - s[0] * f[2];
	u[2] = s[0] * f[1] - s[1] * f[0];
	m[0] = (float)s[0];
	m[4] = (float)s[1];
	m[8] = (float)s[2];
	m[1] = (float)u[0];
	m[5] = (float)u[1];
	m[9] = (float)u[2];
	m[2] = (float)-f[0];
	m[6] = (float)-f[1];
	m[10] = (float)-f[2];
	m[15] = 1.0f;
	MultMatrixf(m);
	Translatef((float)-eyeX, (float)-eyeY, (float)-eyeZ);
}

/* OURS */
const float *Matrix(int mode)
{
	Stack &s = g_stacks[mode - MODELVIEW];

	init_stacks();
	return s.m[s.top];
}

/* OURS */
void ModelViewProjection(float out[16])
{
	if (g_projOnly) {
		memcpy(out, Matrix(PROJECTION), 16 * sizeof(float));
		return;
	}
	multiply(Matrix(PROJECTION), Matrix(MODELVIEW), out);
}

/* OURS, as glColor3f. */
void Color3f(float r, float g, float b)
{
	Color4f(r, g, b, 1.0f);
}

/* OURS, as glColor4f. */
void Color4f(float r, float g, float b, float a)
{
	g_color[0] = r;
	g_color[1] = g;
	g_color[2] = b;
	g_color[3] = a;
}

/* OURS */
void ApplyColor(void)
{
	glVertexAttrib4f(ATTRIB_DIFFUSE, g_color[0], g_color[1], g_color[2], g_color[3]);
}

/* OURS, as glEnable. */
void Enable(GLenum cap)
{
	if (cap == GL_CULL_FACE && !glIsEnabled(cap)) {
		cull_run(true);
	} else if (cap == GL_TEXTURE_2D ? !g_texture2D : !glIsEnabled(cap)) {
		batch_break();
	}
	if (cap == GL_TEXTURE_2D) {
		g_texture2D = true;
	} else {
		glEnable(cap);
	}
}

/* OURS, as glDisable. */
void Disable(GLenum cap)
{
	if (cap == GL_CULL_FACE && glIsEnabled(cap)) {
		cull_run(false);
	} else if (cap == GL_TEXTURE_2D ? g_texture2D : glIsEnabled(cap)) {
		batch_break();
	}
	if (cap == GL_TEXTURE_2D) {
		g_texture2D = false;
	} else {
		glDisable(cap);
	}
}

/* OURS */
bool Texture2DEnabled(void)
{
	return g_texture2D;
}

/* OURS */
void SetDrawHook(void (*hook)(void))
{
	g_drawHook = hook;
}

/* OURS */
void BeforeDraw(void)
{
	if (g_drawHook != NULL) {
		g_drawHook();
	}
}

/* OURS, as glBegin. */
void Begin(int primitive)
{
	g_primitive = primitive;
	g_verts.clear();
}

/* OURS, as glNormal3f. */
void Normal3f(float x, float y, float z)
{
	g_normal[0] = x;
	g_normal[1] = y;
	g_normal[2] = z;
}

/* OURS, as glTexCoord2f. */
void TexCoord2f(float s, float t)
{
	g_uv[0] = s;
	g_uv[1] = t;
}

/* OURS, as glVertex3f: position, normal, colour, uv. */
void Vertex3f(float x, float y, float z)
{
	float v[VERTEX_FLOATS];

	v[0] = x;
	v[1] = y;
	v[2] = z;
	memcpy(v + 3, g_normal, sizeof g_normal);
	memcpy(v + 6, g_color, sizeof g_color);
	memcpy(v + 10, g_uv, sizeof g_uv);
	g_verts.insert(g_verts.end(), v, v + VERTEX_FLOATS);
}

/* OURS. A quad's four corners as two triangles. */
static void quads_to_triangles(std::vector<float> &out)
{
	static const int order[6] = { 0, 1, 2, 0, 2, 3 };
	size_t           quads = g_verts.size() / (4 * VERTEX_FLOATS);
	size_t           q;
	int              k;

	out.clear();
	for (q = 0; q < quads; q++) {
		for (k = 0; k < 6; k++) {
			const float *v = &g_verts[(q * 4 + order[k]) * VERTEX_FLOATS];

			out.insert(out.end(), v, v + VERTEX_FLOATS);
		}
	}
}

/* OURS. A quad strip as triangles: quad i is v2i v2i+1 v2i+3 v2i+2, as GL takes it. */
static void quad_strip_to_triangles(std::vector<float> &out)
{
	static const int order[6] = { 0, 1, 3, 0, 3, 2 };
	size_t           pairs = g_verts.size() / (2 * VERTEX_FLOATS);
	size_t           q;
	int              k;

	out.clear();
	for (q = 0; q + 1 < pairs; q++) {
		for (k = 0; k < 6; k++) {
			const float *v = &g_verts[(q * 2 + order[k]) * VERTEX_FLOATS];

			out.insert(out.end(), v, v + VERTEX_FLOATS);
		}
	}
}

/* OURS, as glPointSize. */
void PointSize(float size)
{
	batch_break();
	g_pointSize = size;
}

/* OURS */
float PointSizeNow(void)
{
	return g_pointSize;
}

/*
 * OURS. Vertices into the stream buffer, which fills front to back and
 * is renewed when full; the index of the first, with the buffer's
 * vertex array bound. The range written lies past everything drawn
 * since the renewal, so nothing the GPU may still read is touched and
 * the unsynchronized map is safe. The browser has no map.
 */
static GLint stream_upload(const std::vector<float> &data)
{
	GLsizei stride = VERTEX_FLOATS * sizeof(float);
	size_t  bytes = data.size() * sizeof(float);
	GLint   first;

	if (g_vao == 0) {
		glGenVertexArrays(1, &g_vao);
		glGenBuffers(1, &g_vbo);
		glBindVertexArray(g_vao);
		glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
		glEnableVertexAttribArray(ATTRIB_POSITION);
		glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, stride, (const void *)0);
		glEnableVertexAttribArray(ATTRIB_NORMAL);
		glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, stride,
		                      (const void *)(3 * sizeof(float)));
		glEnableVertexAttribArray(ATTRIB_DIFFUSE);
		glVertexAttribPointer(ATTRIB_DIFFUSE, 4, GL_FLOAT, GL_FALSE, stride,
		                      (const void *)(6 * sizeof(float)));
		glEnableVertexAttribArray(ATTRIB_UV);
		glVertexAttribPointer(ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, stride,
		                      (const void *)(10 * sizeof(float)));
	}
	glBindVertexArray(g_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
	if (g_vboUsed + bytes > g_vboSize) {
		if (bytes > g_vboSize) {
			g_vboSize = bytes > STREAM_BYTES ? bytes : (size_t)STREAM_BYTES;
		}
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)g_vboSize, NULL, GL_STREAM_DRAW);
		g_vboUsed = 0;
	}
#ifdef __EMSCRIPTEN__
	glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)g_vboUsed, (GLsizeiptr)bytes, &data[0]);
#else
	void *dst = glMapBufferRange(GL_ARRAY_BUFFER, (GLintptr)g_vboUsed, (GLsizeiptr)bytes,
	                             GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

	if (dst != NULL) {
		memcpy(dst, &data[0], bytes);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	} else {
		glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)g_vboUsed, (GLsizeiptr)bytes, &data[0]);
	}
#endif
	first = (GLint)(g_vboUsed / stride);
	g_vboUsed += bytes;
	return first;
}

/* OURS. Vertices in the stream buffer, drawn as mode through the draw hook. */
static void stream_draw(const std::vector<float> &data, GLenum mode)
{
	GLint first;

	BeforeDraw();
	first = stream_upload(data);
	glDrawArrays(mode, first, (GLsizei)(data.size() / VERTEX_FLOATS));
	glBindVertexArray(0);
}

/* OURS. The draw mode a primitive's vertices end up as. */
static GLenum draw_mode(int primitive)
{
	return primitive == POINTS ? GL_POINTS : (primitive == LINES ? GL_LINES : GL_TRIANGLES);
}

/* OURS, as glEnd: the vertices since Begin, drawn from a stream buffer. */
void End(void)
{
	static std::vector<float> tris;
	const std::vector<float> *data = &g_verts;
	GLenum                    mode = draw_mode(g_primitive);
	bool                      eye;
	size_t                    n, v;
	const float              *mv;
	float                    *p, x, y, z;

	if (g_primitive == QUADS) {
		quads_to_triangles(tris);
		data = &tris;
	} else if (g_primitive == QUAD_STRIP) {
		quad_strip_to_triangles(tris);
		data = &tris;
	}
	if (data->empty()) {
		return;
	}
	if (g_listOpen != 0) {
		std::vector<float> &l = g_lists[g_listOpen].verts;

		l.insert(l.end(), data->begin(), data->end());
		return;
	}
	if (!g_batching) {
		stream_draw(*data, mode);
		return;
	}
	eye = g_ffQuery != NULL && g_ffQuery();
	if (!g_batch.empty() && ((int)mode != g_batchKind || eye != g_batchEye)) {
		BatchFlush();
	}
	g_batchKind = (int)mode;
	g_batchEye = eye;
	n = g_batch.size();
	g_batch.insert(g_batch.end(), data->begin(), data->end());
	if (eye) {
		mv = Matrix(MODELVIEW);
		for (v = n; v < g_batch.size(); v += VERTEX_FLOATS) {
			p = &g_batch[v];
			x = p[0];
			y = p[1];
			z = p[2];
			p[0] = mv[0] * x + mv[4] * y + mv[8] * z + mv[12];
			p[1] = mv[1] * x + mv[5] * y + mv[9] * z + mv[13];
			p[2] = mv[2] * x + mv[6] * y + mv[10] * z + mv[14];
		}
	}
}

/* OURS */
void BatchBegin(void)
{
	batch_break();
	g_batching = g_batchOn;
}

/* OURS */
void BatchEnd(void)
{
	batch_break();
	g_batching = false;
}

/*
 * OURS. One upload for everything the batch holds, and one draw, or
 * one per culling run. The last run's culling is the current state.
 */
void BatchFlush(void)
{
	GLint  first;
	size_t i, from, to;

	if (g_batch.empty()) {
		return;
	}
	g_projOnly = g_batchEye;
	if (g_cullRuns.empty()) {
		stream_draw(g_batch, (GLenum)g_batchKind);
	} else {
		BeforeDraw();
		first = stream_upload(g_batch);
		for (i = 0; i < g_cullRuns.size(); i++) {
			from = g_cullRuns[i].first;
			to = i + 1 < g_cullRuns.size() ? g_cullRuns[i + 1].first : g_batch.size() / VERTEX_FLOATS;
			if (g_cullRuns[i].cull) {
				glEnable(GL_CULL_FACE);
			} else {
				glDisable(GL_CULL_FACE);
			}
			if (to > from) {
				glDrawArrays((GLenum)g_batchKind, first + (GLint)from, (GLsizei)(to - from));
			}
		}
		glBindVertexArray(0);
		g_cullRuns.clear();
	}
	g_projOnly = false;
	g_batch.clear();
}

/* OURS */
void SetBatching(bool on)
{
	g_batchOn = on;
}

/* OURS */
bool Batching(void)
{
	return g_batchOn;
}

/* OURS */
void SetFfQuery(bool (*query)(void))
{
	g_ffQuery = query;
}

/* OURS, as glGenLists. */
int GenLists(int range)
{
	int first = g_listNext;

	g_listNext += range;
	return first;
}

/* OURS, as glNewList. */
void NewList(int list, int mode)
{
	g_lists[list].verts.clear();
	g_listOpen = list;
	g_listMode = mode;
}

/* OURS, as glEndList: the recorded triangles into the list's buffer. */
void EndList(void)
{
	List   &l = g_lists[g_listOpen];
	GLsizei stride = VERTEX_FLOATS * sizeof(float);
	int     list = g_listOpen;

	g_listOpen = 0;
	if (l.vao == 0) {
		glGenVertexArrays(1, &l.vao);
		glGenBuffers(1, &l.vbo);
	}
	glBindVertexArray(l.vao);
	glBindBuffer(GL_ARRAY_BUFFER, l.vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(l.verts.size() * sizeof(float)),
	             l.verts.empty() ? NULL : &l.verts[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTRIB_POSITION);
	glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, stride, (const void *)0);
	glEnableVertexAttribArray(ATTRIB_NORMAL);
	glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, stride,
	                      (const void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(ATTRIB_UV);
	glVertexAttribPointer(ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, stride,
	                      (const void *)(10 * sizeof(float)));
	glBindVertexArray(0);
	l.count = (GLsizei)(l.verts.size() / VERTEX_FLOATS);
	l.verts.clear();
	if (g_listMode == COMPILE_AND_EXECUTE) {
		CallList(list);
	}
}

/* OURS, as glCallList. */
void CallList(int list)
{
	std::map<int, List>::iterator it = g_lists.find(list);

	if (it == g_lists.end() || it->second.count == 0) {
		return;
	}
	batch_break();
	BeforeDraw();
	ApplyColor();
	glBindVertexArray(it->second.vao);
	glDrawArrays(GL_TRIANGLES, 0, it->second.count);
	glBindVertexArray(0);
}

}
