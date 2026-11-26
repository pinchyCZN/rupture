/*
 * cgeffects/fragment_city.cpp, transcribed: a fragment stage of the
 * vertex_boat2 pair. Between world y 25 and 260 it cuts vertical slots
 * where sin(0.9 * world x) passes 0.9, raises the flat band multt from
 * 0.1 to 0.4 where sin(0.9 * world y) passes 0.92, and adds a pale
 * 0.1 mulc where decal's coarse noise over object zx passes 0.64.
 * Below tick 0.99999 a second discard dissolves the city where
 * sin(world z * fl) * cos(world x * fl) passes tick. HCOL plus both
 * terms; alpha is 850 times Ramp.x.
 *
 * length(oThePos) is the Cg's length of the float4, w included.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: all three basetexture assignments and its *= cll4,
 * normal, maxx, mx, cll1, cll4, ms's first value (the saturate, set
 * to 1 straight after), nrr, coll and the first oColor2 assignment,
 * and with them the samplers texture, texture2 and light and the
 * inputs oPosition, oUV, HCOL2, oLight, oHalf and uvl. Left out,
 * declared in the Cg and never read: alpha, alpha2, planecut and the
 * samplers cube and oned.
 */

uniform float     tick;
uniform sampler2D decal;

in vec4 TEXCOORD5;   /* oPosition2, the object-space position */
in vec4 TEXCOORD6;   /* oThePos, the world position */
in vec4 COLOR0;      /* HCOL */
in vec4 TEXCOORD7;   /* Ramp */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	vec4  oPosition2 = TEXCOORD5;
	vec4  oThePos = TEXCOORD6;
	float mtt2, multt, ms, mulc, fl;

	mtt2 = texture(decal, oPosition2.zx * 0.00195).x * 1.0;
	ms = 1.0;
	mulc = 0.0;
	multt = 0.1 * ms;
	if (oThePos.y > 25.0) {
		if (oThePos.y < 260.0) {
			if (sin(oThePos.x * 0.9) > 0.90) {
				discard;
			}
			if (sin(oThePos.y * 0.9) > 0.92) {
				multt = 0.4 * ms;
			}
			if (mtt2 > 0.64) {
				mulc = 0.1;
			}
		}
	}
	if (tick < 0.99999) {
		fl = 0.01 - length(oThePos) * 0.00001;
		if ((sin(oThePos.z * fl) * cos(oThePos.x * fl)) > tick) {
			discard;
		}
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = multt * vec4(0.1, 0.2, 0.25, 1.0) + COLOR0 + mulc * vec4(1.0, 1.0, 0.9, 1.0);
	oColor2.w = TEXCOORD7.x * 850.0;
}
