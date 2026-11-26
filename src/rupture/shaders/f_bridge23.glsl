/*
 * cgeffects/f_bridge23.cpp, transcribed: a fragment stage of the
 * v_roadcut3 pair. Two discards: where 20 times decal's noise over
 * world xz, less 3 * (world y + 4), passes 15; and where
 * sin(object z * 0.17) falls below half the light-space z less 12.3.
 * Then HCOL2 plus a dark blue floor, raised by the world speckle mul1
 * and cooled to a blue-grey. red is 0 in this file, so its tint
 * multiplies by 1; the statement is kept, as the Cg has it.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: multt1, thestyle, d2, dd (both values, so its pow of a
 * negative base is never evaluated), basetexture, normal, nrr, cll1,
 * cll4, coll and the first oColor2 assignment, and with them the
 * samplers texture, texture2 and light. Left out, declared in the Cg
 * and never read: alpha, alpha2, alphaplane, alphaplane2, style,
 * focus, fireplace, fireplacealpha, ttime and the samplers cube and
 * oned.
 */

uniform sampler2D decal;

in vec4 TEXCOORD1;   /* oPosition, the light-space position */
in vec4 TEXCOORD5;   /* oPosition2, the object-space position */
in vec4 TEXCOORD6;   /* oThePos, the world position */
in vec4 COLOR1;      /* HCOL2 */
in vec4 TEXCOORD7;   /* Ramp */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	vec4  oPosition = TEXCOORD1;
	vec4  oPosition2 = TEXCOORD5;
	vec4  oThePos = TEXCOORD6;
	float multtt, ress, red, mul1;

	multtt = texture(decal, oThePos.xz * 0.095).x;
	ress = multtt * 20.0 - 3.0 * (oThePos.y + 4.0) - 1.0 - 2.0 * 3.0;
	if (ress > 8.0) {
		discard;
	}
	if (sin(oPosition2.z * 0.17) < 0.5 * oPosition.z - 12.3) {
		discard;
	}
	red = 0.0;
	mul1 = texture(decal, oThePos.xz * 0.02).z * texture(decal, oThePos.zy * 0.02).y;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1 * 1.0;
	oColor2 += vec4(0.015, 0.023, 0.04, 1.0) * 0.8;
	oColor2 *= 1.2 + 1.0 * vec4(0.5, 0.5, 0.5, 1.0) * 7000.0
	         * clamp(pow(mul1, 6.0) - 0.0, 0.0, 1.0);
	oColor2 *= 1.0 + vec4(1.0, 0.1, 0.02, 1.0) * red;
	oColor2 *= vec4(0.3, 0.3, 0.4, 1.0) * 1.1;
	oColor2.w = TEXCOORD7.x * 2.0;
}
