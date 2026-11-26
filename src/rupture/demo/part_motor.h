/*
 * part_motor, the motorcycle on the road (effects/motor.txt): slot 1
 * of the relay. Members keep the demo's offsets in their comments; the
 * layout itself is ours.
 */
#ifndef RUPTURE_PART_MOTOR_H
#define RUPTURE_PART_MOTOR_H

#include <vector>

#include "../core/obj.h"
#include "../core/path.h"
#include "../core/track.h"
#include "demo.h"

class part_motor : public parts {
public:
	part_motor();
	virtual void Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq);
	virtual void Init(Rupture_Ctx *ctx);
	void         DrawScene(Rupture_Ctx *ctx, int pass);

	Path            path;                 /* +0x20, meshes/try1.sav */
	Obj_Mesh        room;                 /* +0x194, meshes/room.obj */
	Obj_Mesh        hex2;                 /* +0x2ec, meshes/hex2.obj */
	Obj_Mesh        kthrio1;              /* +0x10f0, meshes/kthrio1.obj */
	Track           road;                 /* +0x1188 */
	double          t;                    /* +0x210, the part's clock */
	double          tLocal;               /* +0x218 */
	double          dtCopy;               /* +0x2a8 */
	int             frames;               /* +0x228 */
	double          seqHead[3];           /* +0x328, copied from seq+0 */
	double          u;                    /* +0x1200, 25 t */
	int             texBase;              /* +0x2a0, 1000 */
	int             model;                /* +0x1154, 1000 */
	int             texNormal;            /* +0x1138 */
	int             texColour;            /* +0x113c */
	int             texQuarter;           /* +0x1144 */
	int             texGlow;              /* +0x1e4 */
	int             pairBlocks;           /* +0x1180 */
	int             pairRoad;             /* +0x1184 */
	int             pairMotor;            /* +0x11e8 */
	int             pairBuilding;         /* +0x11ec */
	int             pairHex;              /* +0x11f8 */
	int             pairKthrio1;          /* +0x112c */
	int             passBlur;             /* +0x1ec */
	int             passBend;             /* +0x1f4 */
	int             passWire;             /* +0x1fc */
	Vec4            vec1d4;               /* +0x1d4, written, not read here */
	std::vector<u8> bouno;                /* +0x1d0, textures/bouno1.png in grey, rows top down */
	int             bounoW, bounoH;
};

extern Obj_Mesh g_motorcycleObj;   /* 0x00b1e65c */

#endif
