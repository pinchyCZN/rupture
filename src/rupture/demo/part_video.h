/*
 * part_video, the opening room (effects/video.txt): slot 0 of the
 * relay. Members keep the demo's offsets in their comments; the
 * layout itself is ours.
 */
#ifndef RUPTURE_PART_VIDEO_H
#define RUPTURE_PART_VIDEO_H

#include <vector>

#include "../core/obj.h"
#include "../core/path.h"
#include "../core/shard.h"
#include "../core/video.h"
#include "demo.h"

class part_video : public parts {
public:
	part_video();
	virtual void Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq);
	virtual void Init(Rupture_Ctx *ctx);
	void         DrawScene(Rupture_Ctx *ctx, int pass);

	Path               path;         /* +0x20, meshes/try1.sav */
	double             t;            /* +0x1b8, the part's clock */
	double             tCopy;        /* +0x1c0 */
	int                frames;       /* +0x1d0 */
	int                texBase;      /* +0x248, 7000 */
	double             dtCopy;       /* +0x250 */
	Obj_Mesh           hex4;         /* +0x294 */
	Obj_Mesh           walls2;       /* +0x2d0 */
	Obj_Mesh           walls;        /* +0x30c */
	std::vector<Shard> shards;       /* walls' +0x20, this+0x32c */
	double             seqHead[3];   /* +0x348, copied from seq+0 */
	double             tTen;         /* +0x1170, 10 t */
	int                videoFrames;  /* +0x1178, 838 */
	int                texNormal;    /* +0x1118 */
	int                texColour;    /* +0x111c */
	int                texQuarter;   /* +0x1124 */
	int                texGlow;      /* +0x194 */
	int                pairTv;       /* +0x115c */
	int                pairRoom;     /* +0x1164 */
	int                passBlur;     /* +0x19c */
	int                passBend;     /* +0x1a0 */
	int                passWire;     /* +0x1a8 */
	Video              video;        /* frames 10000 + i, one MPEG here */
};

extern Obj_Mesh g_room2Obj;   /* 0x00b1e620 */

#endif
