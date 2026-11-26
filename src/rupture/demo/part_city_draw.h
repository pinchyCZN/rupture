/*
 * part_city::DrawScene's sections, split across two files:
 * part_city_draw.cpp (from 0x00471a30, and DrawScene itself) and
 * part_city_draw2.cpp (from the lit set-up at 0x004739c7). City_Draw
 * carries the values that cross from one section to a later one
 * through DrawScene's stack frame; each field names its stack offset.
 */
#ifndef RUPTURE_PART_CITY_DRAW_H
#define RUPTURE_PART_CITY_DRAW_H

#include "part_city.h"

struct City_Draw {
	/* set by A, part_city_draw.cpp, at DrawScene's entry */
	bool t22;   /* byte [ESP+0x4f], t >= 22 */
	Vec4 s80;   /* [ESP+0x80], {1, 0, 0, 0} */

	/* set by B, part_city_draw2.cpp */
	Vec4 s270;  /* [ESP+0x270], city_boat's translation, read by city_sparks */
};

/* part_city_draw.cpp's helpers, which both files' sections use. */
double city_max0(double x);
void   city_obj_begin(void);
void   city_obj_end(Shader_Ctx *s, int pair);
void   city_lit(Rupture_Ctx *ctx, part_city *p, int pair);

/* part_city_draw2.cpp, 0x004739c7..0x00476608 */
void city_boat(Rupture_Ctx *ctx, part_city *p, City_Draw *D);
void city_heli(Rupture_Ctx *ctx, part_city *p, City_Draw *D);
void city_heli_shards(Rupture_Ctx *ctx, part_city *p, City_Draw *D);
void city_stars(Rupture_Ctx *ctx, part_city *p);
void city_sparks(Rupture_Ctx *ctx, part_city *p, City_Draw *D);

#endif
