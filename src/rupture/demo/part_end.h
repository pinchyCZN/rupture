/*
 * part_end, slot 6 of the relay and the last part: it sets its own
 * started flag at t = 54 rather than a next one. Members keep the
 * demo's offsets in their comments; the layout itself is ours.
 */
#ifndef RUPTURE_PART_END_H
#define RUPTURE_PART_END_H

#include <vector>

#include "../core/camera.h"
#include "../core/obj.h"
#include "../core/path.h"
#include "../core/shard.h"
#include "../core/track.h"
#include "demo.h"

class part_end : public parts {
public:
	part_end();
	virtual void Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq);
	virtual void Init(Rupture_Ctx *ctx);
	void         DrawScene(Rupture_Ctx *ctx, int pass);

	Path               path;             /* +0x20, meshes/try1.sav */
	Track              track;            /* +0x198, 1000 points (0,0,1)..(0,0,1000) */
	Obj_Mesh           helicopter;       /* +0x1f8, meshes/helicopter.obj */
	Obj_Mesh           hellicrash2;      /* +0x234, meshes/hellicrash2.obj */
	Obj_Mesh           hellidist;        /* +0x270, meshes/Hellidist.obj */
	std::vector<Shard> hellidistShards;  /* hellidist's +0x20, this+0x290 */
	Obj_Mesh           helix;            /* +0x2ac, meshes/helix.obj */
	Obj_Mesh           outputobj;        /* +0x2e8, meshes/outputobj.obj */
	std::vector<Shard> outputobjShards;  /* outputobj's +0x20, this+0x308 */
	Obj_Mesh           city1;            /* +0x4d8, meshes/city1.obj */
	Obj_Mesh           cityLess;         /* +0x514, meshes/city_less.obj */
	Obj_Mesh           citywater;        /* +0x58c, meshes/citywater.obj */
	double             d3a0;             /* +0x3a0 */
	Camera             cam3a8;           /* +0x3a8, three Vec4s built inline */
	Vec4               vec1378;          /* +0x1378 */
	Vec4               vec13cc;          /* +0x13cc */
	Vec4               vec1408;          /* +0x1408 */
	double             t;                /* +0x348, the part's clock */
	double             tLocal;           /* +0x350 */
	int                frames;           /* +0x360 */
	double             dtCopy;           /* +0x3e0 */
	double             seqHead[3];       /* +0x5c8, copied from seq+0 */
	double             seqPartClock;     /* +0xe80: maybe_Seq_Copy's copy of seq->partClock, last frame's */
	double             t10;              /* +0x1400, 10 t, for DrawScene */
	int                val1390;          /* +0x1390, 0x70 */
	int                val1394;          /* +0x1394, 0x70 */
	int                texBase;          /* +0x3d8, 5000 */
	int                model;            /* +0x13b4, 5000 */
	int                texSlot;          /* +0x13a8, 1 */
	int                val1418;          /* +0x1418, 200 */
	int                texNormal;        /* +0x1398 */
	int                texColour;        /* +0x139c */
	int                slot13a0;         /* +0x13a0, 0x6f, no target made */
	int                texQuarter;       /* +0x13a4 */
	int                texGlow;          /* +0x324 */
	int                pairAniso2h;      /* +0x13dc, vertex_projection_anisotropic + fragment_..._anisotropic2h */
	int                pairBoat;         /* +0x13e4, vertex_boat + fragment_boat */
	int                pairCity;         /* +0x13e8, vertex_boat2 + fragment_city */
	int                pairHeli2;        /* +0x13ec, vertex_boat + fragment_heli2 */
	int                pairHeli2r;       /* +0x13f0, vertex_boat + fragment_heli2r */
	int                pairTrain;        /* +0x13f4, vertex_boat + fragment_train */
	int                slot13e0;         /* +0x13e0, 0x13a8, never loaded */
	int                passBlurCirc;     /* +0x328 */
	int                passBlur;         /* +0x32c */
	int                passBend;         /* +0x330 */
	int                passBlurFOV;      /* +0x334 */
	int                passWire;         /* +0x338 */
	int                slot33c;          /* +0x33c, 0x13a1, never loaded */
};

#endif
