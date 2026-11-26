/*
 * part_boat, slot 4 of the relay: hands over to part_city at t = 54.
 * Members keep the demo's offsets in their comments; the layout itself
 * is ours.
 */
#ifndef RUPTURE_PART_BOAT_H
#define RUPTURE_PART_BOAT_H

#include "../core/camera.h"
#include "../core/obj.h"
#include "../core/path.h"
#include "../core/track.h"
#include "demo.h"

class part_boat : public parts {
public:
	part_boat();
	virtual void Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq);
	virtual void Init(Rupture_Ctx *ctx);
	void         DrawScene(Rupture_Ctx *ctx, int pass);

	Path          path;           /* +0x20, meshes/try1.sav */
	Track         track;          /* +0x198, 600 points (0,0,1)..(0,0,600) */
	Obj_Mesh      cylinders;      /* +0x21c, meshes/cylinders.obj, lists 1000..1004 */
	Obj_Mesh      trainlong;      /* +0x258, meshes/trainlong.obj */
	Obj_Mesh      boat;           /* +0x294, meshes/boat.obj */
	Obj_Mesh      obj378;         /* +0x378, constructed, never loaded */
	Obj_Mesh      hex;            /* +0x3b4, meshes/hex.obj */
	double        d330;           /* +0x330 */
	Camera        cam338;         /* +0x338, three Vec4s built inline */
	Vec4          vec11a0;        /* +0x11a0 */
	Vec4          vec11f4;        /* +0x11f4 */
	double        t;              /* +0x2d8, the part's clock */
	double        tLocal;         /* +0x2e0 */
	int           frames;         /* +0x2f0 */
	double        dtCopy;         /* +0x370 */
	double        seqHead[3];     /* +0x3f0, copied from seq+0 */
	double        t10;            /* +0x1218, 10 t, read by DrawScene */
	double        seqPartClock;   /* +0xca8: maybe_Seq_Copy's copy of seq->partClock, last frame's */
	int           val11b8;        /* +0x11b8, 0x70 */
	int           val11bc;        /* +0x11bc, 0x70 */
	int           texBase;        /* +0x368, 4000 */
	int           model;          /* +0x11dc, 4000 */
	int           texSlot;        /* +0x11d0, 1 */
	int           texNormal;      /* +0x11c0 */
	int           texColour;      /* +0x11c4 */
	int           slot11c8;       /* +0x11c8, 0x6f, no target made */
	int           texQuarter;     /* +0x11cc */
	int           texGlow;        /* +0x1f8 */
	int           pairAnisoHex;   /* +0x1204, the projection_anisotropic_hex pair */
	int           pairAnisoHex2;  /* +0x218, vertex_projection_anisotropic + fragment_..._hex2 */
	int           pairBoat;       /* +0x1208, vertex_boat + fragment_boat */
	int           pairBoat2;      /* +0x120c, vertex_boat + fragment_boat2 */
	int           pairTrain2;     /* +0x1210, vertex_boat + fragment_train2 */
	int           pairBridge23;   /* +0x200, v_roadcut3 + f_bridge23 */
	int           passBlurCirc;   /* +0x1fc */
	int           passBlur;       /* +0x204 */
	int           passBend;       /* +0x208 */
	int           passBlurFOV;    /* +0x20c */
	int           passWire;       /* +0x210 */
	int           slot214;        /* +0x214, 0xfb9, never loaded */
};

#endif
