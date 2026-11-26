/*
 * part_city, slot 5 of the relay: hands over to part_end at t = 59.
 * Members keep the demo's offsets in their comments; the layout itself
 * is ours.
 */
#ifndef RUPTURE_PART_CITY_H
#define RUPTURE_PART_CITY_H

#include <vector>

#include "../core/camera.h"
#include "../core/obj.h"
#include "../core/path.h"
#include "../core/shard.h"
#include "demo.h"

class part_city : public parts {
public:
	part_city();
	virtual void Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq);
	virtual void Init(Rupture_Ctx *ctx);
	void         DrawScene(Rupture_Ctx *ctx, int pass);

	Path               path;            /* +0x20, meshes/try1.sav */
	Obj_Mesh           boat;            /* +0x194, meshes/boat.obj */
	Obj_Mesh           tablet;          /* +0x1d0, meshes/tablet.obj */
	Obj_Mesh           helicopter;      /* +0x20c, meshes/helicopter.obj */
	Obj_Mesh           hellicrash2;     /* +0x248, meshes/hellicrash2.obj */
	Obj_Mesh           hellidist;       /* +0x284, meshes/Hellidist.obj */
	std::vector<Shard> hellidistShards; /* hellidist's +0x20, this+0x2a4 */
	Obj_Mesh           helix;           /* +0x2c0, meshes/helix.obj */
	Obj_Mesh           city1;           /* +0x4b8, meshes/city1.obj */
	Obj_Mesh           cityLess;        /* +0x4f4, meshes/city_less.obj */
	Obj_Mesh           citycrash;       /* +0x530, meshes/citycrash.obj */
	std::vector<Shard> citycrashShards; /* citycrash's +0x20, this+0x550 */
	Obj_Mesh           citywater;       /* +0x56c, meshes/citywater.obj */
	double             d380;            /* +0x380 */
	Camera             cam388;          /* +0x388, three Vec4s built inline */
	Vec4               vec1358;         /* +0x1358 */
	Vec4               vec13ac;         /* +0x13ac */
	Vec4               vec13e0;         /* +0x13e0 */
	double             t;               /* +0x328, the part's clock */
	double             tLocal;          /* +0x330 */
	int                frames;          /* +0x340 */
	double             dtCopy;          /* +0x3c0 */
	double             seqHead[3];      /* +0x5a8, copied from seq+0 */
	double             seqPartClock;    /* +0xe60: maybe_Seq_Copy's copy of seq->partClock, last frame's */
	double             t10;             /* +0x13d8, 10 t, for DrawScene */
	int                val1370;         /* +0x1370, 0x70 */
	int                val1374;         /* +0x1374, 0x70 */
	int                texBase;         /* +0x3b8, 0 */
	int                model;           /* +0x1394, 0 */
	int                texSlot;         /* +0x1388, 1 */
	int                val13f0;         /* +0x13f0, 200 */
	int                texNormal;       /* +0x1378 */
	int                texColour;       /* +0x137c */
	int                slot1380;        /* +0x1380, 0x6f, no target made */
	int                texQuarter;      /* +0x1384 */
	int                texGlow;         /* +0x2fc */
	int                pairAniso2h;     /* +0x13bc, vertex_projection_anisotropic + fragment_..._anisotropic2h */
	int                pairBoat;        /* +0x13c4, vertex_boat + fragment_boat */
	int                pairCity;        /* +0x13c8, vertex_boat2 + fragment_city */
	int                pairCity2;       /* +0x318, vertex_boat2 + fragment_city2 */
	int                pairHeli;        /* +0x13cc, vertex_boat + fragment_heli */
	int                pairTrain;       /* +0x13d0, vertex_boat + fragment_train */
	int                slot13c0;        /* +0x13c0, 0x20, never loaded */
	int                passBlurCirc;    /* +0x300 */
	int                passBlur;        /* +0x304 */
	int                passBend;        /* +0x308 */
	int                passBlurFOV;     /* +0x30c */
	int                passWire;        /* +0x310 */
	int                slot314;         /* +0x314, 0x19, never loaded */
};

#endif
