/*
 * part_end::DrawScene's sections, split across two files:
 * part_end_draw.cpp (from 0x0047a770, and DrawScene itself) and
 * part_end_draw2.cpp (from the boundary below). No value crosses
 * from one file's sections to the other's through DrawScene's stack
 * frame, so there is no shared struct (effects/end.txt 4).
 *
 * The boundary is 0x0047c503, the lit set-up after the glPopMatrix at
 * 0x0047c4fd; 0x0047b6ab jumps straight to it when the section before
 * is skipped.
 */
#ifndef RUPTURE_PART_END_DRAW_H
#define RUPTURE_PART_END_DRAW_H

#include "part_end.h"

/* part_end_draw2.cpp, from 0x0047c503 to DrawScene's end */
void end_logo(Rupture_Ctx *ctx, part_end *p);
void end_stars(Rupture_Ctx *ctx, part_end *p);

#endif
