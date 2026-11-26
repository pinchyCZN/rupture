/*
 * cgeffects/f_bridge.cpp, transcribed: the fragment stage of the
 * v_roadcut2 pair. Two discards - a band in world x whose threshold
 * dd falls with the cube of object y + 36, and the lattice of
 * effects/look.txt 5.1 over world y, x at k 2.8 - then f_roadcut's
 * world speckle mul1 on HCOL2, without its burning front.
 *
 * pow(oPosition2.y + 36, 3) is written as b * b * b: the base goes
 * negative, where GLSL's pow is undefined, and the release's cg.dll
 * compiles it (fp40) to two multiplies, which keep the sign.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: dd's first value 0.06, basetexture, normal, nrr, cll1,
 * cll4 and coll, and with them the samplers texture, texture2 and
 * light. Left out, declared in the Cg and never read: alpha, alpha2,
 * alphaplane, alphaplane2, planecut, ttime and the samplers cube and
 * oned.
 */

uniform sampler2D decal;

in vec4 TEXCOORD5;   /* oPosition2, the object-space position */
in vec4 TEXCOORD6;   /* oThePos, the world position */
in vec4 COLOR1;      /* HCOL2 */
in vec4 TEXCOORD7;   /* Ramp */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	vec4  oPosition2 = TEXCOORD5;
	vec4  oThePos = TEXCOORD6;
	float b, dd, mul1;

	b = oPosition2.y + 36.0;
	dd = -1.1 - b * b * b * 0.00002;
	if (dd > 0.5) {
		dd = 0.5;
	}
	if (cos(oThePos.x * 0.06) < dd) {
		discard;
	}
	if (cos(oThePos.y * 2.8) * cos(oThePos.x * 2.8) < -0.7) {
		discard;
	}
	mul1 = texture(decal, oThePos.xz * 0.03).x * texture(decal, oThePos.xy * 0.03).y;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1;
	oColor2 *= 0.2 + 0.05 * vec4(0.5, 0.5, 0.5, 1.0) * 4000.0
	         * clamp(pow(mul1, 6.0) - 0.003, 0.0, 1.0);
	oColor2 += vec4(0.015, 0.023, 0.04, 1.0) * 0.5;
	oColor2.w = TEXCOORD7.x * 8.0;
}
