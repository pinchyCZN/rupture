#include "shader.h"

#include <stdio.h>
#include <string.h>

#include "file.h"
#include "gl1.h"

enum { FF_VERTEX = -1, FF_FRAGMENT = -2 };

static Shader_Ctx *g_ctx;
static GLuint      g_usedProgram;   /* the one glUseProgram last bound; only Shader_Apply binds */
static unsigned    g_version = 1;   /* bumped by every value set and texture bind */
static unsigned    g_appliedVersion;

/* The fixed-function stand-in's uniform locations in one program, and what they hold. */
struct Ff_Locs {
	GLint mvp, texture, textured, pointSize;
	bool  set;
	float mvpNow[16];
	int   texturedNow;
	float pointNow;
};

/*
 * OURS. Shader_Apply skips re-uploading a pair's values while its
 * program and this count stay the same - every draw of a shard loop
 * but the first. So every change an upload depends on must call it:
 * the value setters here, and every texture bind in core (the uploads
 * bind the samplers' textures to their units).
 */
void Shader_Touch(void)
{
	g_version++;
}

/* OURS. For gl1's batches: is the vertex stage the stand-in, which takes the modelview? */
static bool ff_vertex(void)
{
	return g_ctx != NULL && !g_ctx->vertexOn;
}

/* OURS. The stand-in's locations in prog, looked up once. */
static Ff_Locs &ff_locs(GLuint prog)
{
	static std::map<GLuint, Ff_Locs> cache;
	std::map<GLuint, Ff_Locs>::iterator it = cache.find(prog);

	if (it == cache.end()) {
		Ff_Locs l;

		memset(&l, 0, sizeof l);
		l.mvp = glGetUniformLocation(prog, "ffModelViewProj");
		l.texture = glGetUniformLocation(prog, "ffTexture");
		l.textured = glGetUniformLocation(prog, "ffTextured");
		l.pointSize = glGetUniformLocation(prog, "ffPointSize");
		it = cache.insert(std::make_pair(prog, l)).first;
	}
	return it->second;
}

/*
 * OURS. A demo path, "cgeffects/x.cpp", names the transcription
 * "shaders/x.glsl"; a path already under shaders/ is used as it is.
 */
static std::string glsl_path(const char *file)
{
	std::string name = file;
	size_t      slash = name.find_last_of('/');
	size_t      dot;

	if (name.compare(0, 8, "shaders/") == 0) {
		return name;
	}
	if (slash != std::string::npos) {
		name = name.substr(slash + 1);
	}
	dot = name.find_last_of('.');
	if (dot != std::string::npos) {
		name = name.substr(0, dot);
	}
	return "shaders/" + name + ".glsl";
}

/* OURS. Reads a stage's text into a slot; the version line is added at link. */
static bool stage_create(Shader_Ctx *s, const char *file, int slot, GLenum stage)
{
	Shader_Slot &sl = s->slots[slot];
	std::string  path = glsl_path(file);

	sl.stage = stage;
	sl.file = path;
	if (!File_ReadText(path.c_str(), sl.source)) {
		s->error += "cannot read " + path + "\n";
		return false;
	}
	return true;
}

/* OURS. One stage compiled, its log appended to the error on failure. */
static GLuint stage_compile(Shader_Ctx *s, const Shader_Slot &sl)
{
#ifdef __EMSCRIPTEN__
	static const char *head = "#version 300 es\nprecision highp float;\nprecision highp int;\n"
	                          "precision highp sampler2D;\n";
#else
	static const char *head = "#version 330 core\n";
#endif
	const GLchar *text[2];
	GLuint        sh = glCreateShader(sl.stage);
	GLint         ok = 0;
	char          log[4096];

	text[0] = head;
	text[1] = sl.source.c_str();
	glShaderSource(sh, 2, text, NULL);
	glCompileShader(sh);
	glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		glGetShaderInfoLog(sh, sizeof log, NULL, log);
		s->error += sl.file + ":\n" + log + "\n";
	}
	return sh;
}

/* OURS. The program for a vertex and fragment slot, linked on first use. */
static GLuint program_for(Shader_Ctx *s, int vs, int fs)
{
	std::pair<int, int> key(vs, fs);
	GLuint              prog, v, f;
	GLint               ok = 0;
	char                log[4096];

	if (s->programs.count(key)) {
		return s->programs[key];
	}
	prog = glCreateProgram();
	v = stage_compile(s, s->slots[vs]);
	f = stage_compile(s, s->slots[fs]);
	glAttachShader(prog, v);
	glAttachShader(prog, f);
	glBindAttribLocation(prog, gl1::ATTRIB_POSITION, "position");
	glBindAttribLocation(prog, gl1::ATTRIB_NORMAL, "normal");
	glBindAttribLocation(prog, gl1::ATTRIB_DIFFUSE, "diffuse");
	glBindAttribLocation(prog, gl1::ATTRIB_UV, "uvs");
	glBindAttribLocation(prog, gl1::ATTRIB_TANGENT, "tangent");
	glLinkProgram(prog);
	glDeleteShader(v);
	glDeleteShader(f);
	glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if (!ok) {
		glGetProgramInfoLog(prog, sizeof log, NULL, log);
		s->error += "link " + s->slots[vs].file + " + " + s->slots[fs].file + ":\n" + log + "\n";
	}
	s->programs[key] = prog;
	return prog;
}

/*
 * OURS. One slot's values into the current program. Each value keeps
 * its location for the last program it went into, so the name is
 * looked up once per program, not once per draw.
 */
static void upload(GLuint prog, Shader_Slot &sl, int &unit)
{
	std::map<std::string, Shader_Value>::iterator it;

	for (it = sl.values.begin(); it != sl.values.end(); ++it) {
		GLint loc;

		if (it->second.locProgram != prog) {
			it->second.loc = glGetUniformLocation(prog, it->first.c_str());
			it->second.locProgram = prog;
		}
		loc = it->second.loc;
		if (loc < 0) {
			continue;
		}
		switch (it->second.kind) {
		case 0:
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_2D, it->second.texture);
			glUniform1i(loc, unit);
			unit++;
			break;
		case 1:
			glUniform1f(loc, it->second.v[0]);
			break;
		case 3:
			glUniform3f(loc, it->second.v[0], it->second.v[1], it->second.v[2]);
			break;
		default:
			glUniformMatrix4fv(loc, 1, GL_FALSE, it->second.v);
			break;
		}
	}
	glActiveTexture(GL_TEXTURE0);
}

/* OURS */
void Shader_Apply(void)
{
	Shader_Ctx *s = g_ctx;
	int         vs, fs, unit = 0;
	GLuint      prog;
	float       mvp[16];
	bool        same;

	if (s == NULL) {
		return;
	}
	vs = s->vertexOn ? s->vertexSlot : FF_VERTEX;
	fs = s->fragmentOn ? s->fragmentSlot : FF_FRAGMENT;
	if (!s->slots.count(vs) || !s->slots.count(fs)) {
		return;
	}
	prog = program_for(s, vs, fs);
	same = prog == g_usedProgram && g_version == g_appliedVersion;
	if (prog != g_usedProgram) {
		glUseProgram(prog);
		g_usedProgram = prog;
	}
	if (vs == FF_VERTEX || fs == FF_FRAGMENT) {
		Ff_Locs &l = ff_locs(prog);
		int      textured = gl1::Texture2DEnabled() ? 1 : 0;
		float    point = gl1::PointSizeNow();

		gl1::ModelViewProjection(mvp);
		if (!l.set || memcmp(mvp, l.mvpNow, sizeof mvp) != 0) {
			glUniformMatrix4fv(l.mvp, 1, GL_FALSE, mvp);
			memcpy(l.mvpNow, mvp, sizeof mvp);
		}
		if (!l.set) {
			glUniform1i(l.texture, 0);
		}
		if (!l.set || textured != l.texturedNow) {
			glUniform1i(l.textured, textured);
			l.texturedNow = textured;
		}
		if (!l.set || point != l.pointNow) {
			glUniform1f(l.pointSize, point);
			l.pointNow = point;
		}
		l.set = true;
	}
	if (same) {
		return;
	}
	if (vs != FF_VERTEX) {
		upload(prog, s->slots[vs], unit);
	}
	if (fs != FF_FRAGMENT) {
		upload(prog, s->slots[fs], unit);
	}
	g_appliedVersion = g_version;
}

/*
 * ORIGIN 0x0044cea0 Cg_Init. There is no profile to pick in GLSL; this
 * loads the fixed-function stand-in (OURS) and makes s the context
 * gl1's draw hook applies.
 */
bool Cg_Init(Shader_Ctx *s)
{
	s->vertexSlot = s->fragmentSlot = 0;
	s->vertexOn = s->fragmentOn = false;
	g_ctx = s;
	gl1::SetDrawHook(Shader_Apply);
	gl1::SetFfQuery(ff_vertex);
#ifndef __EMSCRIPTEN__
	glEnable(GL_PROGRAM_POINT_SIZE);
#endif
	return stage_create(s, "shaders/ff.vert", FF_VERTEX, GL_VERTEX_SHADER)
	    && stage_create(s, "shaders/ff.frag", FF_FRAGMENT, GL_FRAGMENT_SHADER);
}

/* ORIGIN 0x004607d0 Shader_CreateVertex. */
bool Shader_CreateVertex(Shader_Ctx *s, const char *file, int slot)
{
	return stage_create(s, file, slot, GL_VERTEX_SHADER);
}

/* ORIGIN 0x004608c0 Shader_CreateFragment. */
bool Shader_CreateFragment(Shader_Ctx *s, const char *file, int slot)
{
	return stage_create(s, file, slot, GL_FRAGMENT_SHADER);
}

/* ORIGIN 0x0044d030 Shader_LoadVertex. Compiling waits for the link. */
void Shader_LoadVertex(Shader_Ctx *s, int slot)
{
	(void)s;
	(void)slot;
}

/* ORIGIN 0x0044cfe0 Shader_LoadFragment. Compiling waits for the link. */
void Shader_LoadFragment(Shader_Ctx *s, int slot)
{
	(void)s;
	(void)slot;
}

/*
 * ORIGIN 0x004bb640 Shader_LoadPair: vertex in slot, fragment in
 * slot + 1, and the three matrices at indices 5, 6 and 0xe.
 */
bool Shader_LoadPair(Shader_Ctx *s, const char *vertex, const char *fragment, int slot)
{
	bool ok = Shader_CreateVertex(s, vertex, slot) && Shader_CreateFragment(s, fragment, slot + 1);

	Shader_LoadVertex(s, slot);
	Shader_LoadFragment(s, slot + 1);
	Shader_GetParam(s, slot, 5, "ModelViewProj");
	Shader_GetParam(s, slot, 6, "ModelViewLightProj");
	Shader_GetParam(s, slot, 0xe, "ModelViewObject");
	return ok;
}

/* ORIGIN 0x004bb600 Shader_LoadFragmentOnly, the loader for every post pass. */
bool Shader_LoadFragmentOnly(Shader_Ctx *s, const char *file, int slot)
{
	return Shader_CreateFragment(s, file, slot);
}

/* ORIGIN 0x0044cf10 Shader_GetParam. */
void Shader_GetParam(Shader_Ctx *s, int slot, int index, const char *name)
{
	s->slots[slot].names[index & 15] = name;
}

/* OURS. A 4x4 into a slot's value under the name at index. */
static void set_matrix(Shader_Ctx *s, int slot, int index, const float *m)
{
	gl1::BatchFlush();
	Shader_Slot  &sl = s->slots[slot];
	Shader_Value &v = sl.values[sl.names[index & 15]];

	v.kind = 16;
	memcpy(v.v, m, sizeof v.v);
	Shader_Touch();
}

/* ORIGIN 0x0044cf80 Shader_SetMvpMatrix: the matrix as it is now. */
void Shader_SetMvpMatrix(Shader_Ctx *s, int slot, int index)
{
	float m[16];

	gl1::ModelViewProjection(m);
	set_matrix(s, slot, index, m);
}

/* ORIGIN 0x0044cfb0 Shader_SetTextureMatrix: the GL_TEXTURE matrix as it is now. */
void Shader_SetTextureMatrix(Shader_Ctx *s, int slot, int index)
{
	set_matrix(s, slot, index, gl1::Matrix(gl1::TEXTURE));
}

/* ORIGIN 0x0044d080 Shader_Bind. */
void Shader_Bind(Shader_Ctx *s, int slot)
{
	gl1::BatchFlush();
	if (s->slots[slot].stage == GL_VERTEX_SHADER) {
		s->vertexSlot = slot;
	} else {
		s->fragmentSlot = slot;
	}
}

/* ORIGIN 0x0044d0a0 Shader_SetParam3f. */
void Shader_SetParam3f(Shader_Ctx *s, int slot, int index, float x, float y, float z)
{
	gl1::BatchFlush();
	Shader_Slot  &sl = s->slots[slot];
	Shader_Value &v = sl.values[sl.names[index & 15]];

	v.kind = 3;
	v.v[0] = x;
	v.v[1] = y;
	v.v[2] = z;
	Shader_Touch();
}

/* ORIGIN 0x004bb070 Shader_SetParam1f: the double narrowed to float, as cgGLSetParameter1f takes it. */
void Shader_SetParam1f(Shader_Ctx *s, int slot, const char *name, const double *value)
{
	gl1::BatchFlush();
	Shader_Value &v = s->slots[slot].values[name];

	v.kind = 1;
	v.v[0] = (float)*value;
	Shader_Touch();
}

/* OURS, as cgGLSetTextureParameter and cgGLEnableTextureParameter together. */
void Shader_SetTexture(Shader_Ctx *s, int slot, const char *name, GLuint texture)
{
	gl1::BatchFlush();
	Shader_Value &v = s->slots[slot].values[name];

	v.kind = 0;
	v.texture = texture;
	Shader_Touch();
}

/* ORIGIN 0x0044d0e0 Shader_EnableVertexProfile. */
void Shader_EnableVertexProfile(Shader_Ctx *s)
{
	gl1::BatchFlush();
	s->vertexOn = true;
}

/* ORIGIN 0x0044d0f0 Shader_EnableFragmentProfile. */
void Shader_EnableFragmentProfile(Shader_Ctx *s)
{
	gl1::BatchFlush();
	s->fragmentOn = true;
}

/* ORIGIN 0x0044d100 Shader_DisableVertexProfile. */
void Shader_DisableVertexProfile(Shader_Ctx *s)
{
	gl1::BatchFlush();
	s->vertexOn = false;
}

/* ORIGIN 0x0044d110 Shader_DisableFragment. */
void Shader_DisableFragment(Shader_Ctx *s)
{
	gl1::BatchFlush();
	s->fragmentOn = false;
}

/* ORIGIN 0x004bc7e0 Shader_EnableProfiles. */
void Shader_EnableProfiles(Shader_Ctx *s)
{
	Shader_EnableVertexProfile(s);
	Shader_EnableFragmentProfile(s);
}

/* ORIGIN 0x004bc7c0 Shader_DisableProfiles. */
void Shader_DisableProfiles(Shader_Ctx *s)
{
	Shader_DisableVertexProfile(s);
	Shader_DisableFragment(s);
}
