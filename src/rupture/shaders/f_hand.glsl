/*
 * cgeffects/f_hand.cpp, transcribed: a fragment stage of the
 * v_roadcut3 pair. No discard: HCOL2 plus a dark blue floor, scaled by
 * the world speckle mul1 (decal over world xz and zy at 0.02).
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
	float mul1;

	mul1 = texture(decal, oThePos.xz * 0.02).z * texture(decal, oThePos.zy * 0.02).y;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1 * 1.0;
	oColor2 += vec4(0.015, 0.023, 0.04, 1.0) * 0.5;
	oColor2 *= 1.2 + 1.0 * vec4(0.5, 0.5, 0.5, 1.0) * 4000.0
	         * clamp(pow(mul1, 6.0) - 0.001, 0.0, 1.0);
	oColor2.w = TEXCOORD7.x * 8.0;
}
