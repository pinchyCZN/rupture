/*
 * cgeffects/fragment_city2.cpp, transcribed: a fragment stage of the
 * vertex_boat2 pair. It is fragment_city with the y-band block and
 * the tick dissolve commented out in the Cg: no discard, HCOL plus the
 * flat 0.1 band multt (mulc stays 0); alpha is 850 times Ramp.x.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: all three basetexture assignments and its *= cll4,
 * normal, maxx, mx, cll1, cll4, mtt2 (its readers are in the
 * commented-out block), ms's first value (the saturate, set to 1
 * straight after), nrr, coll and the first oColor2 assignment, and
 * with them the samplers decal, texture, texture2 and light and the
 * inputs oPosition, oPosition2, oThePos, oUV, HCOL2, oLight, oHalf and
 * uvl. Left out, declared in the Cg and never read: alpha, alpha2,
 * planecut, tick and the samplers cube and oned.
 */

in vec4 COLOR0;      /* HCOL */
in vec4 TEXCOORD7;   /* Ramp */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	float multt, ms, mulc;

	ms = 1.0;
	mulc = 0.0;
	multt = 0.1 * ms;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = multt * vec4(0.1, 0.2, 0.25, 1.0) + COLOR0 + mulc * vec4(1.0, 1.0, 0.9, 1.0);
	oColor2.w = TEXCOORD7.x * 850.0;
}
