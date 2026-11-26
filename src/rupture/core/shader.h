/*
 * The demo's Cg layer (ctx+0x1d0c and the Shader_* functions) carried
 * on GLSL. The demo binds a vertex program in slot n and a fragment
 * program in slot n + 1 separately and sets parameters on each; GLSL
 * links the two into one program. So a slot keeps its parameter
 * values the way a Cg program object does, and Shader_Apply, run
 * before every draw, links the bound pair (once) and uploads them.
 * State matrices are copied when they are set, as Cg copies them.
 *
 * Every varying is a vec4 named for its Cg semantic - TEXCOORD0..7,
 * COLOR0, COLOR1 - because Cg matches stages by semantic and lets the
 * two sides disagree on width, where GLSL matches by name and type.
 * With the profiles disabled, a draw goes through the fixed-function
 * stand-in, shaders/ff.vert and ff.frag (OURS).
 */
#ifndef RUPTURE_SHADER_H
#define RUPTURE_SHADER_H

#include <map>
#include <string>
#include <vector>

#include "glapi.h"

struct Shader_Value {
	int    kind;        /* 1, 3 or 16 floats; 0 a sampler */
	float  v[16];
	GLuint texture;
	GLuint locProgram;  /* OURS: the program loc was looked up in, 0 for none */
	GLint  loc;
};

struct Shader_Slot {
	GLenum                              stage;
	std::string                         file;
	std::string                         source;
	std::string                         names[16];
	std::map<std::string, Shader_Value> values;
};

struct Shader_Ctx {
	std::map<int, Shader_Slot>             slots;
	std::map<std::pair<int, int>, GLuint>  programs;
	int                                    vertexSlot;
	int                                    fragmentSlot;
	bool                                   vertexOn;
	bool                                   fragmentOn;
	std::string                            error;
};

bool Cg_Init(Shader_Ctx *s);
bool Shader_CreateVertex(Shader_Ctx *s, const char *file, int slot);
bool Shader_CreateFragment(Shader_Ctx *s, const char *file, int slot);
void Shader_LoadVertex(Shader_Ctx *s, int slot);
void Shader_LoadFragment(Shader_Ctx *s, int slot);
bool Shader_LoadPair(Shader_Ctx *s, const char *vertex, const char *fragment, int slot);
bool Shader_LoadFragmentOnly(Shader_Ctx *s, const char *file, int slot);
void Shader_GetParam(Shader_Ctx *s, int slot, int index, const char *name);
void Shader_SetMvpMatrix(Shader_Ctx *s, int slot, int index);
void Shader_SetTextureMatrix(Shader_Ctx *s, int slot, int index);
void Shader_Bind(Shader_Ctx *s, int slot);
void Shader_SetParam3f(Shader_Ctx *s, int slot, int index, float x, float y, float z);
void Shader_Touch(void);
void Shader_SetParam1f(Shader_Ctx *s, int slot, const char *name, const double *value);
void Shader_SetTexture(Shader_Ctx *s, int slot, const char *name, GLuint texture);
void Shader_EnableVertexProfile(Shader_Ctx *s);
void Shader_EnableFragmentProfile(Shader_Ctx *s);
void Shader_DisableVertexProfile(Shader_Ctx *s);
void Shader_DisableFragment(Shader_Ctx *s);
void Shader_EnableProfiles(Shader_Ctx *s);
void Shader_DisableProfiles(Shader_Ctx *s);

/* Links what is bound and uploads its values; gl1's draw hook. */
void Shader_Apply(void);

#endif
