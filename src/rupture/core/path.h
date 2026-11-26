/*
 * A recorded path read from a .sav file: 28-byte records of float x,
 * y, z and two doubles, plus three tracks smoothed from them
 * (Path_Construct 0x00441ed0, Path_Load 0x0045caf0). part_video holds
 * one at this+0x20 and loads meshes/try1.sav into it; its Frame shakes
 * the camera with the raw points (effects/video.txt 5).
 */
#ifndef RUPTURE_PATH_H
#define RUPTURE_PATH_H

#include <vector>

#include "obj.h"
#include "track.h"

struct Path {
	std::vector<Vec4>   points;      /* +0x00, first at +0xc */
	std::vector<double> a;           /* +0x18, first at +0x24 */
	std::vector<double> b;           /* +0x30, first at +0x3c */
	Track               pointTrack;  /* +0x48 */
	Track               aTrack;      /* +0xa8 */
	Track               bTrack;      /* +0x108 */
};

Path *Path_Construct(Path *p);
void  Path_Load(Path *p, const char *file);

#endif
