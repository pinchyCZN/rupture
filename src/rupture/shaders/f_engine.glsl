/*
 * cgeffects/f_engine.cpp, transcribed: a fragment stage of the
 * v_roadcut3 pair. No discard: HCOL2 plus a dark blue floor, darkened
 * to 0.3 except in a band one unit wide about world y = -66.4, where
 * the world speckle mul1 (decal over world xz and zy at 0.03) lights
 * it orange; then a flat grey-blue and a grey speckle added.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: d2, dd (both values, so its pow of a negative base is
 * never evaluated), basetexture, normal, nrr, cll1, cll4, coll and the
 * first oColor2 assignment, and with them the samplers texture,
 * texture2 and light. Left out, declared in the Cg and never read:
 * alpha, alpha2, alphaplane, alphaplane2, planecut, ttime and the
 * samplers cube and oned.
 */

uniform sampler2D decal;

in vec4 TEXCOORD6;   /* oThePos, the world position */
in vec4 COLOR1;      /* HCOL2 */
in vec4 TEXCOORD7;   /* Ramp */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	vec4  oThePos = TEXCOORD6;
	float mul1, sf;

	mul1 = texture(decal, oThePos.xz * 0.03).z * texture(decal, oThePos.zy * 0.03).y;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1 * 1.0;
	oColor2 += vec4(0.016, 0.020, 0.03, 1.0) * 1.5;
	sf = clamp(1.0 - 2.0 * abs(oThePos.y + 66.4), 0.0, 1.0);
	oColor2 *= 0.3 + sf * vec4(4.8, 1.0, 0.2, 1.0) * 4000.0
	         * clamp(pow(mul1, 6.0) - 0.0002, 0.0, 1.0);
	oColor2 += vec4(0.1 + sf * 0.0, 0.1, 0.15, 1.0) * 0.1;
	oColor2 += vec4(0.5, 0.5, 0.5, 1.0) * 30.0 * clamp(pow(mul1, 6.0) - 0.002, 0.0, 1.0);
	oColor2.w = TEXCOORD7.x * 0.8;
}
