/*
 * The demo's key track (Rupture_Track, 0x60 bytes): a vector of Vec4
 * read back at a fractional index by Track_Eval. part_motor's road and
 * every shard's two recorded paths are tracks. The demo's two further
 * vectors at +0x18 and +0x30 are never read by anything ported here
 * and are left out.
 */
#ifndef RUPTURE_TRACK_H
#define RUPTURE_TRACK_H

#include <vector>

#include "obj.h"

struct Track {
	std::vector<Vec4> points;  /* +0x00, first/last at +0xc/+0x10 */
	int               mode;    /* +0x48, 3 after Track_Clear */
	char              loop;    /* +0x4c */
	int               count;   /* +0x50 */
	double            unk58;   /* +0x58 */
};

Track *Track_Construct(Track *t);
Track *Track_ConstructMode(Track *t, int mode);
void   Track_Clear(Track *t);
void   Track_Eval(const Track *t, float s, Vec4 *out);
void   Track_Resample(Track *t, int n, int sub);
Track *Track_Copy(Track *t, const Track *src);

#endif
