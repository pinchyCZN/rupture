/*
 * part_truck, the motorcycle passing the truck and the tunnel
 * (effects/truck.txt): slot 2 of the relay. Members keep the demo's
 * offsets in their comments; the layout itself is ours.
 */
#ifndef RUPTURE_PART_TRUCK_H
#define RUPTURE_PART_TRUCK_H

#include <vector>

#include "../core/obj.h"
#include "../core/path.h"
#include "../core/shard.h"
#include "../core/track.h"
#include "demo.h"

class part_truck : public parts {
public:
	part_truck();
	virtual void Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq);
	virtual void Init(Rupture_Ctx *ctx);
	void         DrawScene(Rupture_Ctx *ctx, int pass);

	Path               path;           /* +0x20, meshes/try1.sav */
	Obj_Mesh           hex3;           /* +0x1d0 */
	Obj_Mesh           sign;           /* +0x20c */
	Obj_Mesh           tholos;         /* +0x248 */
	Obj_Mesh           tholos2;        /* +0x284 */
	Obj_Mesh           hex2;           /* +0x3ec */
	Obj_Mesh           destruction;    /* +0x428, meshes/truckdestruction.obj */
	std::vector<Shard> shards;         /* +0x454, destruction's shards */
	Obj_Mesh           lights;         /* +0x464, meshes/trucklights.obj */
	Obj_Mesh           empty2;         /* +0x4a0, meshes/truck_empty2.obj */
	Obj_Mesh           trainlong;      /* +0x4dc */
	Track              road;           /* +0x1378 */
	Track              cross;          /* +0x13d8 */
	Track              rails[5];       /* +0x1438, stride 0x60 */
	double             t;              /* +0x310, the part's clock */
	double             tLocal;         /* +0x318 */
	int                frames;         /* +0x328 */
	double             dtCopy;         /* +0x3a8 */
	double             seqHead[3];     /* +0x518, copied from seq+0 */
	double             u;              /* +0x1640, 27 t */
	unsigned char      flicker;        /* +0x2c0 */
	Vec4               vec2d4;         /* +0x2d4, written, not read here */
	int                texBase;        /* +0x3a0, 2000 */
	int                model;          /* +0x1344, 2000 */
	int                texNormal;      /* +0x1328 */
	int                texColour;      /* +0x132c */
	int                texQuarter;     /* +0x1334 */
	int                texGlow;        /* +0x2e4 */
	int                pairRoad;       /* +0x1374, v_roadcut2 + f_roadcut */
	int                pairBridge;     /* +0x304, v_roadcut2 + f_bridge */
	int                pairMotor;      /* +0x1620 */
	int                pairTruck;      /* +0x2c8 */
	int                pairDestruction;/* +0x2cc */
	int                pairLight;      /* +0x2d0, v_truck + f_truck_light */
	int                pairHex2;       /* +0x1630 */
	int                pairHex22;      /* +0x1634 */
	int                pairHex222;     /* +0x1638 */
	int                pairBoat;       /* +0x1618 */
	int                pairTrain;      /* +0x161c */
	int                pairMotion;     /* +0x1628 */
	int                pairS1;         /* +0x1624, 0x7f6, never loaded */
	int                pairS2;         /* +0x1370, 0x7f0, never loaded */
	int                passBlur;       /* +0x2ec */
	int                passBend;       /* +0x2f4 */
	int                passWire;       /* +0x2fc */
};

extern Obj_Mesh g_truckObj;   /* 0x00b1e5a8 */
extern Obj_Mesh g_boatObj;    /* 0x00b1e5e4 */

/* ORIGIN 0x0049f9b0 maybe_Track_DrawRails, part_truck_draw.cpp; part_boat calls it too. */
void maybe_Track_DrawRails(Rupture_Ctx *ctx, Track *T, double depth);

#endif
