/*
 * part_start, the loading screen (subsystems/startup.txt 4): built
 * and drawn once by Demo_Init, before the seven parts, and never in
 * the relay. Members keep the demo's offsets in their comments.
 */
#ifndef RUPTURE_PART_START_H
#define RUPTURE_PART_START_H

#include "demo.h"

class part_start : public parts {
public:
	part_start();
	virtual void Frame(double *dt, Rupture_Ctx *ctx, Rupture_Seq *seq);
	virtual void Init(Rupture_Ctx *ctx);

	double d1d0;      /* +0x1d0, zeroed by Init; reader [open] */
	double d1d8;      /* +0x1d8, the same */
	int    i1e8;      /* +0x1e8, the same */
	int    texBase;   /* +0x260, 0 */
};

#endif
