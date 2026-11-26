/*
 * cgeffects/fragment_boat_test2.cpp, transcribed (effects/motor.txt
 * 2.2): the object-space hole lattice, the world-space speckle mask
 * mul1 from decal, the dissolving front behind alphaplane2, and two
 * outputs - the normal (COLOR0) and the colour (COLOR1).
 *
 * Dropped, read only by values nothing uses: basetexture, normal,
 * nrr, cll1, cll4, coll, and with them the samplers texture,
 * texture2, light, cube and oned.
 */

uniform float     alpha2;
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
	float mul1, cond1, cond2, diff, ffx;

	if (cos(oPosition2.y * 0.7) * cos(oPosition2.z * 0.7) < -0.7) {
		discard;
	}
	mul1 = texture(decal, oThePos.xz * 0.03).x * texture(decal, oThePos.xy * 0.03).y;
	cond1 = mul1 * 0.400 + alpha2 * 2.0 + oThePos.y * 0.1 + 1.0;
	cond2 = mul1 + abs(oThePos.z) * 0.200;
	if (cond1 > 0.97) {
		discard;
	}
	diff = oThePos.x - alphaplane2;
	ffx = clamp(0.05 * (alphaplane2 - oThePos.x), 0.0, 1.0);
	if (oThePos.y > -25.0) {
		if (diff < 0.0) {
			if (cond2 < 0.73 * ffx) {
				discard;
			}
		}
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1;
	oColor2 *= 1.0 + vec4(0.2, 0.6, 1.0, 1.0) * 4000.0 * clamp(pow(mul1, 6.0) - 0.001, 0.0, 1.0);
	oColor2 += clamp(1.0 * ffx - cond2, 0.0, 1.0) * 2.0 * vec4(1.0, 0.2, 0.1, 1.0)
	         * clamp(1.0 - diff, 0.0, 1.0);
	oColor2.w = TEXCOORD7.x;
	oColor2 += COLOR0 * 0.4;
}
