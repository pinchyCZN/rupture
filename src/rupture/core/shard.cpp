#include "shard.h"

#include <math.h>

#include "post.h"

/* OURS. A kind-1 zero, the value every Vec4 temporary starts as. */
static Vec4 vec4_zero(void)
{
	Vec4 z;

	z.kind = 1;
	z.x = 0.0f;
	z.y = 0.0f;
	z.z = 0.0f;
	return z;
}

/* OURS. pow(x, n) for a whole n, as MSVC inlines it: square and multiply. */
double pow_int(double x, unsigned n)
{
	double r = 1.0;

	for (;;) {
		if (n & 1) {
			r = r * x;
		}
		n >>= 1;
		if (n == 0) {
			break;
		}
		x = x * x;
	}
	return r;
}

/* ORIGIN 0x0045bcf0 Shard_Construct. Every Vec4 a kind-1 zero, then the corners. */
Shard *Shard_Construct(Shard *sh, Vec4 c0, Vec4 c1, Vec4 c2, Vec4 c3)
{
	int i;

	for (i = 0; i < 4; i++) {
		sh->c[i] = vec4_zero();
		sh->turned[i] = vec4_zero();
		sh->n[i] = vec4_zero();
		sh->turnedN[i] = vec4_zero();
	}
	sh->pos = vec4_zero();
	sh->rest = vec4_zero();
	sh->axis = vec4_zero();
	Track_Construct(&sh->posTrack);
	Track_Construct(&sh->angleTrack);
	sh->s = 0.0f;
	sh->c[0] = c0;
	sh->c[1] = c1;
	sh->c[2] = c2;
	sh->c[3] = c3;
	sh->v = vec4_zero();
	sh->a = vec4_zero();
	sh->angle = 0.0f;
	sh->spin = 0.0f;
	return sh;
}

/*
 * ORIGIN 0x0044b370 Shard_Centre. The mean of the four corners (0.25,
 * the float at 0x004f4370) becomes pos and rest, and the corners
 * become relative to it.
 */
void Shard_Centre(Shard *sh)
{
	static const float quarter = 0.25f;
	Vec4               t0 = vec4_zero(), t1 = vec4_zero(), t2 = vec4_zero();
	int                i;

	Vec4_Add(&sh->c[0], &sh->c[1], &t0);
	Vec4_Add(&t0, &sh->c[2], &t1);
	Vec4_Add(&t1, &sh->c[3], &t2);
	sh->pos = t2;
	Vec4_Scale(&sh->pos, &quarter, &sh->pos);
	for (i = 0; i < 4; i++) {
		Vec4_Sub(&sh->c[i], &sh->pos, &sh->c[i]);
	}
	sh->rest = sh->pos;
}

/* OURS. Shard_Normals' step: normalise both edges, cross second x first. */
static Vec4 edge_cross(const Vec4 *a1, const Vec4 *b1, const Vec4 *a2, const Vec4 *b2)
{
	Vec4 first = vec4_zero(), second = vec4_zero(), n = vec4_zero();

	Vec4_Sub(a1, b1, &first);
	Vec4_Sub(a2, b2, &second);
	Vec4_Normalize(&first);
	Vec4_Normalize(&second);
	Vec4_Cross(&second, &first, &n);
	Vec4_Normalize(&n);
	return n;
}

/*
 * ORIGIN 0x0044b4a0 Shard_Normals. One normal per face of the
 * tetrahedron; the first and last are negated (Vec4_Neg). Then the
 * turned corners are the corners plus pos and the turned normals the
 * normals.
 */
void Shard_Normals(Shard *sh)
{
	Vec4 n;
	int  i;

	n = edge_cross(&sh->c[2], &sh->c[1], &sh->c[0], &sh->c[1]);
	Vec4_Neg(&n, &sh->n[0]);
	sh->n[1] = edge_cross(&sh->c[3], &sh->c[1], &sh->c[0], &sh->c[1]);
	sh->n[2] = edge_cross(&sh->c[3], &sh->c[0], &sh->c[2], &sh->c[0]);
	n = edge_cross(&sh->c[3], &sh->c[1], &sh->c[2], &sh->c[1]);
	Vec4_Neg(&n, &sh->n[3]);
	for (i = 0; i < 4; i++) {
		Vec4_Add(&sh->c[i], &sh->pos, &sh->turned[i]);
	}
	for (i = 0; i < 4; i++) {
		sh->turnedN[i] = sh->n[i];
	}
}

/*
 * ORIGIN 0x00451070 Shard_RandomSpin. A random unit axis, x y z in draw
 * order, and a spin of (r + 0.3)^2 * k (k_dbl_0_3).
 */
void Shard_RandomSpin(Shard *sh, double k)
{
	double rx = Math_Random(), ry = Math_Random(), rz = Math_Random();

	sh->axis.kind = 1;
	sh->axis.x = (float)(rx + rx - 1.0);
	sh->axis.y = (float)(ry + ry - 1.0);
	sh->axis.z = (float)(rz + rz - 1.0);
	Vec4_Normalize(&sh->axis);
	sh->spin = (float)(pow_int(Math_Random() + 0.3, 2) * k);
}

/*
 * ORIGIN 0x00451210 Shard_Step. Gravity (0, -2, 0) (0x004f4400) into a,
 * a into v, both damped by 1 - 0.45 dt (0x004f42e8), v into pos; the
 * angle grows with speed while pos.y >= -11999.95 (0x004f43f8), else
 * the damping is 1 - 1.25 dt (0x004f43f0). Below -12000 (0x004f43e8)
 * the shard bounces: v.y * -0.59, v.x and v.z * 0.89 (0x004f43e0,
 * 0x004f43d8).
 */
void Shard_Step(Shard *sh, double dt)
{
	float       fdt = (float)dt, k;
	double      damp;
	Vec4        g = vec4_zero(), t = vec4_zero();
	const float bounce = (float)0.89;

	g.y = -2.0f;
	Vec4_Scale(&g, &fdt, &t);
	Vec4_Add(&sh->a, &t, &sh->a);
	Vec4_Scale(&sh->a, &fdt, &t);
	Vec4_Add(&sh->v, &t, &sh->v);
	damp = 1.0 - dt * 0.45;
	k = (float)(damp < 0.0 ? 0.0 : damp);
	Vec4_Scale(&sh->v, &k, &sh->v);
	k = (float)damp;
	if (k < 0.0f) {
		k = 0.0f;
	}
	Vec4_Scale(&sh->a, &k, &sh->a);
	Vec4_Scale(&sh->v, &fdt, &t);
	Vec4_Add(&sh->pos, &t, &sh->pos);
	if ((float)-11999.95 <= sh->pos.y) {
		sh->angle = (float)(Vec4_Length(&sh->v) * sh->spin * 0.3 * dt + sh->angle);
	} else {
		damp = 1.0 - dt * 1.25;
		k = (float)(damp < 0.0 ? 0.0 : damp);
		Vec4_Scale(&sh->v, &k, &sh->v);
		k = (float)damp;
		if (k < 0.0f) {
			k = 0.0f;
		}
		Vec4_Scale(&sh->a, &k, &sh->a);
	}
	if (-12000.0f <= sh->pos.y) {
		return;
	}
	sh->pos.y = -12000.0f;
	sh->v.y = sh->v.y * (float)-0.59;
	sh->v.x = sh->v.x * bounce;
	sh->v.z = bounce * sh->v.z;
	Vec4_Scale(&sh->v, &fdt, &t);
	Vec4_Add(&sh->pos, &t, &sh->pos);
}

/* ORIGIN 0x0045ca80 Shard_Record. pos onto one track, (angle, 0, 0) onto the other. */
void Shard_Record(Shard *sh)
{
	Vec4 a = vec4_zero();

	sh->posTrack.loop = 0;
	sh->angleTrack.loop = 0;
	sh->posTrack.points.push_back(sh->pos);
	sh->posTrack.count++;
	a.x = sh->angle;
	sh->angleTrack.points.push_back(a);
	sh->angleTrack.count++;
}

/*
 * ORIGIN 0x00451110 Shard_Seek. s counts records; below 0.001
 * (0x004f3768) the shard is at its first record, unturned.
 */
void Shard_Seek(Shard *sh, double s)
{
	float fs = (float)s;
	Vec4  p = vec4_zero(), a = vec4_zero();

	sh->s = fs;
	if (0.001 <= s) {
		Track_Eval(&sh->posTrack, fs, &p);
		Track_Eval(&sh->angleTrack, fs, &a);
		sh->angle = a.x;
		sh->pos = p;
	} else {
		sh->angle = 0.0f;
		sh->pos = sh->posTrack.points[0];
	}
	Shard_Rotate(sh);
}

/*
 * ORIGIN 0x0044bb10 Shard_Rotate. Corners and normals turned by
 * Mat_FromEuler(angle * axis); at |angle| <= 0.001 (0x004f3768) copied.
 * The corners stay relative to pos: the draw adds it.
 */
void Shard_Rotate(Shard *sh)
{
	float m[16];
	float ang = sh->angle;
	int   i;

	if (0.001 < fabs(ang)) {
		Mat_FromEuler(m, ang * sh->axis.x, sh->axis.y * ang, sh->axis.z * ang);
		for (i = 0; i < 4; i++) {
			Mat_Apply(m, &sh->turned[i], &sh->c[i]);
		}
		for (i = 0; i < 4; i++) {
			Mat_Apply(m, &sh->turnedN[i], &sh->n[i]);
		}
		return;
	}
	for (i = 0; i < 4; i++) {
		sh->turned[i] = sh->c[i];
	}
	for (i = 0; i < 4; i++) {
		sh->turnedN[i] = sh->n[i];
	}
}

/*
 * ORIGIN 0x004670e0 Obj_MakeShards. One shard per group but the last:
 * faces[0]'s corners 0, 1, 2 and faces[1]'s corner 1. Obj_PushShard
 * (0x00467030) is std::vector push_back. After centring, spinning and
 * the normals, each shard draws 11 Math_Randoms whose products end as
 * zero a and v - every term is scaled by 0 (k_dbl_0, 0.0f) before it
 * lands - and turned copies of the corners and normals.
 */
void Obj_MakeShards(const Obj_Mesh *m, std::vector<Shard> *shards, double spin)
{
	static const float k16 = 1.6f;           /* 0x004f4598 */
	static const float zero = 0.0f;
	size_t             g, i;
	int                k;
	Shard              sh;
	Vec4               t = vec4_zero(), t1 = vec4_zero(), t2 = vec4_zero();
	float              f5;
	double             r;

	for (g = 0; g + 1 < m->groups.size(); g++) {
		const std::vector<Obj_Face> &faces = m->groups[g].faces;

		Shard_Construct(&sh, faces[0].c[0], faces[0].c[1], faces[0].c[2], faces[1].c[1]);
		shards->push_back(sh);
	}
	Obj_CentreShards(shards);
	Obj_SpinShards(shards, spin);
	Obj_ShardNormals(shards);
	for (i = 0; i < shards->size(); i++) {
		Shard *s = &(*shards)[i];

		f5 = (float)(pow_int(Math_Random(), 3) * 5.0);
		t.kind = 1;
		t.x = (float)(Math_Random() * 9.0);
		r = Math_Random();
		t.y = (float)((r + r - 1.0) * 0.0);
		t.z = (float)(Math_Random() * 8.0 - 4.0);
		Vec4_Scale(&t, &f5, &t1);
		Vec4_Scale(&t1, &k16, &t2);
		Vec4_Scale(&t2, &zero, &s->a);
		s->a.y = (float)(pow_int(Math_Random() * 1.4, 5) + 3.0);
		Vec4_Scale(&s->a, &zero, &s->a);
		t.x = (float)(Math_Random() * 4.0 - 2.0);
		t.y = (float)(Math_Random() * 4.0 - 2.0);
		t.z = (float)(Math_Random() * 4.0 - 2.0);
		Vec4_Scale(&t, &zero, &s->a);
		t.x = (float)(Math_Random() * 4.0 - 2.0);
		t.y = (float)(Math_Random() * 4.0 - 2.0);
		t.z = (float)(Math_Random() * 4.0 - 2.0);
		Vec4_Scale(&t, &zero, &s->v);
		for (k = 0; k < 4; k++) {
			s->turned[k] = s->c[k];
		}
		for (k = 0; k < 4; k++) {
			s->turnedN[k] = s->n[k];
		}
	}
}

/* ORIGIN 0x004503b0 Obj_CentreShards. */
void Obj_CentreShards(std::vector<Shard> *shards)
{
	size_t i;

	for (i = 0; i < shards->size(); i++) {
		Shard_Centre(&(*shards)[i]);
	}
}

/* ORIGIN 0x00455b10 Obj_SpinShards. */
void Obj_SpinShards(std::vector<Shard> *shards, double k)
{
	size_t i;

	for (i = 0; i < shards->size(); i++) {
		Shard_RandomSpin(&(*shards)[i], k);
	}
}

/* ORIGIN 0x00450410 Obj_ShardNormals. */
void Obj_ShardNormals(std::vector<Shard> *shards)
{
	size_t i;

	for (i = 0; i < shards->size(); i++) {
		Shard_Normals(&(*shards)[i]);
	}
}

/*
 * ORIGIN 0x0045f810 Obj_BakeShards. n * m records per shard, dt =
 * 0.5 / m (k_dbl_0_5) and two steps apart; the first is written 11
 * times, so the path holds still at its start.
 */
void Obj_BakeShards(std::vector<Shard> *shards, int n, int m)
{
	size_t i;
	int    k, j;
	double dt;

	for (i = 0; i < shards->size(); i++) {
		Shard *s = &(*shards)[i];

		if (0 < n * m) {
			dt = 0.5 / (double)m;
			for (k = 0; k < n * m; k++) {
				Shard_Record(s);
				if (k == 0) {
					for (j = 0; j < 5; j++) {
						Shard_Record(s);
						Shard_Record(s);
					}
				}
				Shard_Step(s, dt);
				Shard_Step(s, dt);
			}
		}
	}
}
