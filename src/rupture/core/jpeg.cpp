/*
 * The demo's JPEG IDCT path (formats/jpeg.txt). Every value is kept
 * in 16 bits as the MMX registers keep it: sums wrap, the multiplies
 * keep the high or low half, and only the final add saturates.
 */
#include "jpeg.h"

/* libjpeg's aanscales, natural order, 0x0050b980. */
static const s16 k_aanScales[64] = {
	16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
	21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
	19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
	16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
	 8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
	 4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
};

/* OURS. A 16-bit lane: paddw, psubw and psllw wrap. */
static s16 w16(int v)
{
	return (s16)(u16)(unsigned)v;
}

/* OURS. pmulhw: the high half of the signed product. */
static s16 mulhi(s16 a, s16 b)
{
	return (s16)(((int)a * (int)b) >> 16);
}

/* OURS. psraw 5, then paddsw 128 and packuswb's unsigned saturation. */
static u8 descale(s16 v)
{
	int x = (v >> 5) + 128;

	if (x > 32767) {
		x = 32767;
	}
	if (x < 0) {
		return 0;
	}
	return (u8)(x > 255 ? 255 : x);
}

/*
 * ORIGIN 0x0041dd00 FUN_0041dd00 maybe_jpgd_read_dqt_marker, its MMX branch: a
 * quantisation value for natural position pos, scaled by aanscales
 * with 12 fraction bits and kept in 16 bits.
 */
s16 Jpeg_QuantMmx(unsigned q, int pos)
{
	return (s16)(u16)(((unsigned)k_aanScales[pos] * q + 0x800u) >> 12);
}

/*
 * ORIGIN 0x0041b400 jpgd_idct_mmx: libjpeg's fast integer IDCT in MMX.
 * The coefficients come in already multiplied by Jpeg_QuantMmx's
 * values, which is the pmullw the demo does first; the 8x8 result goes
 * to out with a stride of 8. The column pass runs four columns to a
 * register and the row pass four rows; each lane is the same sum, so
 * here it is one column or one row at a time.
 */
void Jpeg_IdctMmx(const s16 *coef, u8 *out)
{
	s16 ws[64];
	s16 z10, z11, z12, z13, t0, t1, t2, t3, t4, t5, t6, t7, t11, t12, e10, e11;
	int c, r;
	const s16 *in;
	s16 *w;

	for (c = 0; c < 8; c++) {
		in = coef + c;
		w = ws + c;
		z13 = w16(in[40] + in[24]);
		z10 = w16(in[40] - in[24]);
		z11 = w16(in[8] + in[56]);
		z12 = w16(in[8] - in[56]);
		t7 = w16(z11 + z13);
		t11 = mulhi(w16((z11 - z13) << 2), 23170);
		t6 = w16(w16(mulhi(w16(z10 << 2), -12540) + mulhi(w16(z12 << 2), 30273)) - t7);
		t5 = w16(t11 - t6);
		t4 = w16(w16(mulhi(w16(z10 << 2), -30353) + mulhi(w16(z12 << 2), -12540)) + t5);
		t3 = w16(in[16] + in[48]);
		t12 = w16(mulhi(w16((in[16] - in[48]) << 2), 23170) - t3);
		e10 = w16(in[0] + in[32]);
		e11 = w16(in[0] - in[32]);
		t0 = w16(e10 + t3);
		t1 = w16(e11 + t12);
		t2 = w16(e11 - t12);
		t3 = w16(e10 - t3);
		w[0] = w16(t0 + t7);
		w[56] = w16(t0 - t7);
		w[8] = w16(t1 + t6);
		w[48] = w16(t1 - t6);
		w[16] = w16(t2 + t5);
		w[40] = w16(t2 - t5);
		w[32] = w16(t3 + t4);
		w[24] = w16(t3 - t4);
	}
	for (r = 0; r < 8; r++) {
		w = ws + r * 8;
		e10 = w16(w[0] + w[4]);
		e11 = w16(w[0] - w[4]);
		t3 = w16(w[2] + w[6]);
		t12 = w16(mulhi(w16((w[2] - w[6]) << 2), 23170) - t3);
		t0 = w16(e10 + t3);
		t3 = w16(e10 - t3);
		t1 = w16(e11 + t12);
		t2 = w16(e11 - t12);
		z10 = w16(w[5] - w[3]);
		z11 = w16(w[1] + w[7]);
		z12 = w16(w[1] - w[7]);
		z13 = w16(w[5] + w[3]);
		t7 = w16(z11 + z13);
		t11 = mulhi(w16((z11 - z13) << 2), 23170);
		t6 = w16(w16(mulhi(w16(z12 << 2), 30273) + mulhi(w16(z10 << 2), -12540)) - t7);
		t5 = w16(t11 - t6);
		t4 = w16(w16(mulhi(w16(z10 << 2), -30353) + mulhi(w16(z12 << 2), -12540)) + t5);
		out[r * 8 + 0] = descale(w16(t0 + t7));
		out[r * 8 + 7] = descale(w16(t0 - t7));
		out[r * 8 + 1] = descale(w16(t1 + t6));
		out[r * 8 + 6] = descale(w16(t1 - t6));
		out[r * 8 + 2] = descale(w16(t2 + t5));
		out[r * 8 + 5] = descale(w16(t2 - t5));
		out[r * 8 + 4] = descale(w16(t3 + t4));
		out[r * 8 + 3] = descale(w16(t3 - t4));
	}
}
