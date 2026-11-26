/*
 * The shards walls.obj breaks into (effects/video.txt 3.4): one
 * tetrahedron per group, launched, flown once at load and recorded
 * into two tracks, then replayed at a per-shard index s. The record
 * is 0x228 bytes in the demo and the offsets below are its.
 *
 * The demo keeps the shard vector in the mesh, at Obj_Mesh+0x20; here
 * it is a vector of its own, which the Obj_*Shards loops take, as
 * theirs take the mesh's.
 */
#ifndef RUPTURE_SHARD_H
#define RUPTURE_SHARD_H

#include <vector>

#include "obj.h"
#include "track.h"

struct Shard {
	Vec4  c[4];        /* +0x00, relative to pos once centred */
	Vec4  pos;         /* +0x40 */
	Vec4  rest;        /* +0x50, the centre at rest */
	Vec4  axis;        /* +0x60, unit */
	Vec4  v;           /* +0x70 */
	Vec4  a;           /* +0x80 */
	float angle;       /* +0x90 */
	float spin;        /* +0x94 */
	Vec4  turned[4];   /* +0x98 */
	Vec4  n[4];        /* +0xd8, one per face */
	Vec4  turnedN[4];  /* +0x118 */
	Track posTrack;    /* +0x160 */
	Track angleTrack;  /* +0x1c0, (angle, 0, 0) */
	float s;           /* +0x220 */
};

Shard *Shard_Construct(Shard *sh, Vec4 c0, Vec4 c1, Vec4 c2, Vec4 c3);
void   Shard_Centre(Shard *sh);
void   Shard_Normals(Shard *sh);
void   Shard_RandomSpin(Shard *sh, double k);
void   Shard_Step(Shard *sh, double dt);
void   Shard_Record(Shard *sh);
void   Shard_Seek(Shard *sh, double s);
void   Shard_Rotate(Shard *sh);

void Obj_MakeShards(const Obj_Mesh *m, std::vector<Shard> *shards, double spin);
void Obj_CentreShards(std::vector<Shard> *shards);
void Obj_SpinShards(std::vector<Shard> *shards, double k);
void Obj_ShardNormals(std::vector<Shard> *shards);
void Obj_BakeShards(std::vector<Shard> *shards, int n, int m);

double pow_int(double x, unsigned n);

#endif
