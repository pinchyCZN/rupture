/*
 * part_rail, the motorcycle on the rails and the bridge: slot 3 of the
 * relay. Members keep the demo's offsets in their comments; the layout
 * itself is ours.
 */
#ifndef RUPTURE_PART_RAIL_H
#define RUPTURE_PART_RAIL_H

#include "../core/camera.h"
#include "../core/obj.h"
#include "../core/path.h"
#include "../core/track.h"
#include "demo.h"

class part_rail : public parts {
public:
	part_rail();
	virtual void Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq);
	virtual void Init(Rupture_Ctx *ctx);
	void         DrawScene(Rupture_Ctx *ctx, int pass);

	Path          path;           /* +0x20, meshes/try1.sav */
	Obj_Mesh      cylinders;      /* +0x284, meshes/cylinders.obj, lists 1000..1005 */
	Obj_Mesh      human;          /* +0x3bc, meshes/human.obj */
	Obj_Mesh      hand;           /* +0x524, meshes/hand.obj */
	Obj_Mesh      trainlong;      /* +0x560, meshes/trainlong.obj, lists 2000..2002 */
	Track         track1400;      /* +0x1400, constructed, never built */
	Track         track;          /* +0x1460, the long track */
	Track         rails[5];       /* +0x14c0, stride 0x60 */
	Vec4          vec2d4;         /* +0x2d4 */
	double        d370;           /* +0x370 */
	Camera        cam378;         /* +0x378 */
	Vec4          vec1350;        /* +0x1350 */
	Vec4          vec13e4;        /* +0x13e4 */
	double        t;              /* +0x318, the part's clock */
	double        tLocal;         /* +0x320 */
	int           frames;         /* +0x330 */
	double        dtCopy;         /* +0x3b0 */
	double        seqHead[3];     /* +0x5a0, copied from seq+0 */
	double        u;              /* +0x16c8, 27 t */
	int           val13a8;        /* +0x13a8, 0x70 */
	int           val13ac;        /* +0x13ac, 0x70 */
	int           texBase;        /* +0x3a8, 3000 */
	int           model;          /* +0x13cc, 3000 */
	int           texSlot;        /* +0x13c0, 1 */
	int           texNormal;      /* +0x13b0 */
	int           texColour;      /* +0x13b4 */
	int           slot13b8;       /* +0x13b8, 6, no target made */
	int           texQuarter;     /* +0x13bc */
	int           texGlow;        /* +0x2e4 */
	int           pairAniso;      /* +0x13f4, the projection_anisotropic pair */
	int           pairBridge2;    /* +0x30c, v_roadcut3 + f_bridge2 */
	int           pairTrain;      /* +0x16a4, vertex_train + fragment_trainold2 */
	int           pairEngine;     /* +0x2ec, v_roadcut3 + f_engine */
	int           pairHand;       /* +0x300, v_roadcut3 + f_hand */
	int           pairHead;       /* +0x3b8, v_roadcut3 + f_head */
	int           passBlurCirc;   /* +0x2e8 */
	int           passBlur;       /* +0x2f0 */
	int           passMotion;     /* +0x2f4 */
	int           passBend;       /* +0x2f8 */
	int           passBlurFOV;    /* +0x2fc */
	int           passWire;       /* +0x304 */
	int           slot13f8;       /* +0x13f8, 0xbd8, never loaded */
	int           slot16ac;       /* +0x16ac, 0xbde, never loaded */
	int           slot16b0;       /* +0x16b0, 0xbdc, never loaded */
	int           slot16b4;       /* +0x16b4, 0xbda, never loaded */
	int           slot16b8;       /* +0x16b8, 0xbe4, never loaded */
	int           slot16bc;       /* +0x16bc, 0xbf0, never loaded */
	int           slot16c0;       /* +0x16c0, 0xbf6, never loaded */
	int           slot308;        /* +0x308, 0xbd1, never loaded */
	int           slot13fc;       /* +0x13fc, 0xbe6, never loaded */
	int           slot16a8;       /* +0x16a8, 0xbe8, never loaded */
	int           slot13a4;       /* +0x13a4, 0xbea, never loaded */
	int           slot2c8;        /* +0x2c8, 0xbec, never loaded */
	int           slot2cc;        /* +0x2cc, 0xc02, never loaded */
	int           slot2d0;        /* +0x2d0, 0xbee, never loaded */
	int           slot16a0;       /* +0x16a0, 0xbfe, never loaded */
};

extern int g_railHeadFlag;   /* 0x00b1e558, seqHead[2] > 0.1, set by DrawScene, read nowhere */

#endif
