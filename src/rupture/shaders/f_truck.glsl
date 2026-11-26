/*
 * cgeffects/f_truck.cpp, transcribed: the fragment stage of the
 * v_truck pair. A discard where the object-space noise mul1 plus
 * 0.4 |world z| falls under 0.4 ffx, ffx rising behind alphaplane2
 * (effects/look.txt 5.2 at 0.12); the speckle mul1 read again at a
 * position pulled 70% of the way back to object space; a burning edge
 * where ffx passes 2 cond2; HCOL added tinted blue.
 *
 * No statement is dropped. Left out, declared in the Cg and never
 * read: alpha, alpha2, alphaplane, planecut, ttime and the samplers
 * light, cube, texture, oned and texture2.
 */

uniform float     alphaplane2;
uniform sampler2D decal;

in vec4 TEXCOORD5;   /* oPosition2, the object-space position */
in vec4 TEXCOORD6;   /* oThePos, the world position */
in vec4 COLOR0;      /* HCOL */
in vec4 COLOR1;      /* HCOL2 */
in vec4 TEXCOORD7;   /* Ramp */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	vec4  oPosition2 = TEXCOORD5;
	vec4  oThePos = TEXCOORD6;
	float mul1, cond2, ffx;

	mul1 = 0.8 * texture(decal, oPosition2.xz * 0.038).x * texture(decal, oPosition2.xy * 0.038).y;
	cond2 = mul1 + abs(oThePos.z) * 0.400;
	ffx = clamp(0.12 * (alphaplane2 - oThePos.x), 0.0, 1.0);
	if (cond2 < 0.4 * ffx) {
		discard;
	}
	oThePos = oPosition2 + (oThePos - oPosition2) * 0.3;
	mul1 = texture(decal, oThePos.xz * 0.06).x * texture(decal, oThePos.xy * 0.05).y;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1 * vec4(0.2, 0.3, 0.4, 0.1) * 0.2;
	oColor2 *= 1.0 + vec4(0.3, 0.3, 0.4, 1.0) * 40000.0 * TEXCOORD7.x
	         * clamp(pow(mul1, 8.0) - 0.0000, 0.0, 1.0);
	oColor2 += clamp(1.0 * ffx - 2.0 * cond2, 0.0, 1.0) * 2.0 * vec4(1.0, 0.2, 0.1, 1.0);
	oColor2 += vec4(0.2, 0.3, 0.4, 1.0) * 0.2 * COLOR0;
	oColor2.w = TEXCOORD7.x;
}
