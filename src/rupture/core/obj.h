/*
 * The demo's .obj meshes: Obj_Load reads a Wings 3D export into groups
 * of faces that each hold their own copies of their corners, and
 * Obj_DrawGroup draws them as quads with one normal per face
 * (effects/look.txt 4). The groups and faces are the std::vectors the
 * original keeps.
 *
 * Vec4 is the demo's 16-byte vector: a kind word, then x, y, z. Kind 0
 * is "unset" (Vec4_Sub and Vec4_Cross give zero), 1 a plain vector, 2
 * one Vec4_Normalize has normalised.
 */
#ifndef RUPTURE_OBJ_H
#define RUPTURE_OBJ_H

#include <vector>

struct Vec4 {
	int   kind;
	float x, y, z;
};

struct Obj_Face {                 /* 0x40 bytes */
	Vec4 c[4];                    /* a triangle repeats c[2] as c[3] */
};

struct Obj_Group {                /* 0x18 bytes in the demo */
	std::vector<Obj_Face> faces;  /* +0xc / +0x10 */
};

struct Obj_Mesh {
	int                    list;     /* +0, GL display list, -1 = none */
	char                   compile;  /* +4, compile on the next draw */
	std::vector<Obj_Group> groups;   /* +0x14 / +0x18 */
};

void  Vec4_Add(const Vec4 *a, const Vec4 *b, Vec4 *out);
void  Vec4_Sub(const Vec4 *a, const Vec4 *b, Vec4 *out);
void  Vec4_Neg(const Vec4 *v, Vec4 *out);
void  Vec4_Cross(const Vec4 *a, const Vec4 *b, Vec4 *out);
float Vec4_Length(const Vec4 *v);
Vec4 *Vec4_Normalize(Vec4 *v);
void  Vec4_Scale(const Vec4 *v, const float *s, Vec4 *out);

/* A 3x3 rotation in a 4x4 float block, rows four floats apart. */
float *Mat_FromEuler(float *m, float a, float b, float c);
void   Mat_MulVec4(const float *m, const Vec4 *v, Vec4 *out);
Vec4  *Mat_Apply(const float *m, Vec4 *out, const Vec4 *v);

bool Obj_Load(Obj_Mesh *m, int list, const char *path);
void Obj_AddFace(Obj_Mesh *m, const Obj_Face *f);
void Obj_DrawGroup(const Obj_Group *g);
void Obj_Draw(Obj_Mesh *m);

#endif
