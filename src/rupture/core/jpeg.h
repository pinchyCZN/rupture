/*
 * The demo's JPEG arithmetic that the current jpgd lacks: the MMX
 * fast IDCT and the quantisation scaling it needs (overview.txt 4,
 * formats/jpeg.txt). src/third_party/jpgd/jpgd.cpp calls both.
 */
#ifndef RUPTURE_JPEG_H
#define RUPTURE_JPEG_H

#include "types.h"

s16  Jpeg_QuantMmx(unsigned q, int pos);
void Jpeg_IdctMmx(const s16 *coef, u8 *out);

#endif
