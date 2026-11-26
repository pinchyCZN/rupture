#include "obj.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "gl1.h"

/* ORIGIN 0x004b8d60 Vec4_Add. */
void Vec4_Add(const Vec4 *a, const Vec4 *b, Vec4 *out)
{
	if (a->kind != 0 && b->kind != 0) {
		out->x = a->x + b->x;
		out->y = a->y + b->y;
		out->kind = 1;
		out->z = a->z + b->z;
		return;
	}
	out->kind = 1;
	out->x = 0.0f;
	out->y = 0.0f;
	out->z = 0.0f;
}

/* ORIGIN 0x004b8d00 Vec4_Neg. The kind is copied, not reset. */
void Vec4_Neg(const Vec4 *v, Vec4 *out)
{
	if (v->kind != 0) {
		out->x = -v->x;
		out->y = -v->y;
		out->z = -v->z;
		out->kind = v->kind;
		return;
	}
	out->kind = 1;
	out->x = 0.0f;
	out->y = 0.0f;
	out->z = 0.0f;
}

/* ORIGIN 0x004b8dd0 Vec4_Sub. */
void Vec4_Sub(const Vec4 *a, const Vec4 *b, Vec4 *out)
{
	if (a->kind != 0 && b->kind != 0) {
		out->x = a->x - b->x;
		out->y = a->y - b->y;
		out->kind = 1;
		out->z = a->z - b->z;
		return;
	}
	out->kind = 1;
	out->x = 0.0f;
	out->y = 0.0f;
	out->z = 0.0f;
}

/* ORIGIN 0x004b8e40 Vec4_Cross. out = a x b. */
void Vec4_Cross(const Vec4 *a, const Vec4 *b, Vec4 *out)
{
	if (a->kind != 0 && b->kind != 0) {
		out->x = a->y * b->z - a->z * b->y;
		out->y = a->z * b->x - b->z * a->x;
		out->kind = 1;
		out->z = b->y * a->x - a->y * b->x;
		return;
	}
	out->kind = 1;
	out->x = 0.0f;
	out->y = 0.0f;
	out->z = 0.0f;
}

/* ORIGIN 0x00406900 Vec4_Length. */
float Vec4_Length(const Vec4 *v)
{
	return (float)sqrt(v->y * v->y + v->x * v->x + v->z * v->z);
}

/* ORIGIN 0x004b9060 Vec4_Normalize. 1e-8 is the double at 0x004f3748. */
Vec4 *Vec4_Normalize(Vec4 *v)
{
	float len, inv;

	if (v->kind == 1) {
		len = Vec4_Length(v);
		if (len < (float)1e-8) {
			v->kind = 2;
			v->x = 0.0f;
			v->y = 0.0f;
			v->z = 0.0f;
			return v;
		}
		v->kind = 2;
		inv = 1.0f / len;
		v->x = inv * v->x;
		v->y = v->y * inv;
		v->z = inv * v->z;
	}
	return v;
}

/* ORIGIN 0x004b8ed0 Vec4_Scale. */
void Vec4_Scale(const Vec4 *v, const float *s, Vec4 *out)
{
	if (v->kind != 0) {
		out->x = v->x * *s;
		out->y = v->y * *s;
		out->z = v->z * *s;
		out->kind = v->kind;
		return;
	}
	out->kind = 1;
	out->x = 0.0f;
	out->y = 0.0f;
	out->z = 0.0f;
}

/*
 * ORIGIN 0x004b7c80 Mat_FromEuler: Rx(a) Ry(b) Rz(c), each sine and
 * cosine rounded to float as the x87 code stores them.
 */
float *Mat_FromEuler(float *m, float a, float b, float c)
{
	float ca = (float)cos(a), sa = (float)sin(a);
	float cb = (float)cos(b), sb = (float)sin(b);
	float cc = (float)cos(c), sc = (float)sin(c);

	m[0] = cb * cc;
	m[1] = -cb * sc;
	m[2] = sb;
	m[4] = sa * sb * cc + ca * sc;
	m[5] = cc * ca - sa * sb * sc;
	m[6] = -sa * cb;
	m[8] = sc * sa - cc * (sb * ca);
	m[9] = sa * cc + sb * ca * sc;
	m[10] = cb * ca;
	return m;
}

/* ORIGIN 0x004b7fb0 Mat_MulVec4: out = m v over the 3x3, kind 1. */
void Mat_MulVec4(const float *m, const Vec4 *v, Vec4 *out)
{
	out->kind = 1;
	out->x = m[1] * v->y + v->x * m[0] + m[2] * v->z;
	out->y = m[5] * v->y + v->x * m[4] + m[6] * v->z;
	out->z = m[9] * v->y + v->x * m[8] + m[10] * v->z;
}

/* ORIGIN 0x004b82b0 Mat_Apply: Mat_MulVec4 through a temporary. */
Vec4 *Mat_Apply(const float *m, Vec4 *out, const Vec4 *v)
{
	Vec4 t;

	t.kind = 1;
	t.x = 0.0f;
	t.y = 0.0f;
	t.z = 0.0f;
	Mat_MulVec4(m, v, &t);
	*out = t;
	return out;
}

/* OURS. A face pushed into the last group, as Obj_Load's inlined push_back. */
void Obj_AddFace(Obj_Mesh *m, const Obj_Face *f)
{
	if (m->groups.empty()) {
		m->groups.push_back(Obj_Group());
	}
	m->groups.back().faces.push_back(*f);
}

/*
 * ORIGIN 0x00466660 Obj_Load, its fstream replaced by stdio. It reads
 * the subset Wings 3D writes: "v x y z", "f a// b// c// [d//]" with
 * 1-based indices, and "o" to start a group; every other line is
 * skipped. Returns false if the file cannot be opened.
 */
bool Obj_Load(Obj_Mesh *m, int list, const char *path)
{
	FILE             *f;
	char              line[512];
	std::vector<Vec4> v;
	Vec4              p;
	int               idx[4], n, i;
	Obj_Face          face;

	m->groups.clear();
	m->list = list;
	m->compile = 1;
	f = fopen(path, "r");
	if (f == NULL) {
		return false;
	}
	while (fgets(line, sizeof line, f) != NULL) {
		if (line[0] == 'v' && line[1] == ' ') {
			p.kind = 1;
			sscanf(line + 2, "%f %f %f", &p.x, &p.y, &p.z);
			v.push_back(p);
		} else if (line[0] == 'f' && line[1] == ' ') {
			n = sscanf(line + 2, "%d// %d// %d// %d//", &idx[0], &idx[1], &idx[2], &idx[3]);
			if (n < 3) {
				continue;
			}
			if (n == 3) {
				idx[3] = idx[2];
			}
			for (i = 0; i < 4; i++) {
				face.c[i] = v[idx[i] - 1];
			}
			Obj_AddFace(m, &face);
		} else if (line[0] == 'o' && line[1] == ' ') {
			m->groups.push_back(Obj_Group());
		}
	}
	fclose(f);
	return true;
}

/*
 * ORIGIN 0x00451a10 Obj_DrawGroup. One normal per face, from the two
 * edges at corner 1. The dominant-axis tests run x, y, z in that order
 * and each sees the components as the previous test left them.
 */
void Obj_DrawGroup(const Obj_Group *g)
{
	static const float k14 = 1.4f;          /* k_dbl_1_4 */
	static const float neg = -1.0f;         /* k_flt_m1 */
	Vec4               e0, e1, n;
	float              ax, ay;
	size_t             i;

	gl1::Begin(gl1::QUADS);
	for (i = 0; i < g->faces.size(); i++) {
		const Obj_Face *f = &g->faces[i];

		Vec4_Sub(&f->c[2], &f->c[1], &e1);
		Vec4_Sub(&f->c[0], &f->c[1], &e0);
		Vec4_Normalize(&e1);
		Vec4_Normalize(&e0);
		Vec4_Cross(&e0, &e1, &n);
		if (fabs(n.y) < fabs(n.x) && fabs(n.z) < fabs(n.x)) {
			n.x = n.x * k14;
		}
		ax = (float)fabs(n.x);
		if (ax < fabs(n.y) && fabs(n.z) < fabs(n.y)) {
			n.y = n.y * k14;
		}
		ay = (float)fabs(n.y);
		if (ay < fabs(n.z) && ax < fabs(n.z)) {
			n.z = n.z * k14;
		}
		Vec4_Normalize(&n);
		Vec4_Scale(&n, &neg, &n);
		gl1::Normal3f(n.x, n.y, n.z);
		gl1::Vertex3f(f->c[0].x, f->c[0].y, f->c[0].z);
		gl1::Vertex3f(f->c[1].x, f->c[1].y, f->c[1].z);
		gl1::Vertex3f(f->c[2].x, f->c[2].y, f->c[2].z);
		gl1::Vertex3f(f->c[3].x, f->c[3].y, f->c[3].z);
	}
	gl1::End();
}

/*
 * ORIGIN 0x00455b80 Obj_Draw. The first draw compiles the groups into
 * the display list with GL_COMPILE_AND_EXECUTE; later draws call it.
 */
void Obj_Draw(Obj_Mesh *m)
{
	size_t i;

	if (m->list >= 0) {
		if (!m->compile) {
			gl1::CallList(m->list);
			return;
		}
		gl1::NewList(m->list, gl1::COMPILE_AND_EXECUTE);
	}
	for (i = 0; i < m->groups.size(); i++) {
		Obj_DrawGroup(&m->groups[i]);
	}
	if (m->list >= 0 && m->compile) {
		gl1::EndList();
		m->compile = 0;
	}
}
