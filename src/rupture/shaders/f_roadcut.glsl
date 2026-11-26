/*
 * cgeffects/f_roadcut.cpp, transcribed: the fragment stage of the
 * v_roadcut pair (effects/motor.txt 4): the world speckle mul1 and
 * the burning front behind alphaplane2 (effects/look.txt 5.2), with
 * cond2 lowered by oLight.x * 0.5, where oLight.x is v_roadcut's
 * ffx2 * 0.005. The only discard is the front's.
 *
 * Dropped, read by nothing that reaches an output: cond1,
 * basetexture, normal, nrr, cll1, cll4, coll, and with them alpha2
 * and the samplers texture, texture2 and light.
 */

uniform float     alphaplane2;
uniform sampler2D decal;

in vec4 TEXCOORD6;   /* oThePos, the world position */
in vec4 COLOR1;      /* HCOL2 */
in vec4 TEXCOORD7;   /* Ramp */
in vec4 TEXCOORD2;   /* oLight */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	vec4  oThePos = TEXCOORD6;
	vec4  oLight = TEXCOORD2;
	float mul1, cond2, diff, ffx;

	mul1 = texture(decal, oThePos.xz * 0.03).x * texture(decal, oThePos.xy * 0.03).y;
	cond2 = mul1 + abs(oThePos.z) * 0.200 - oLight.x * 0.5;
	diff = oThePos.x - alphaplane2;
	ffx = clamp(0.05 * (alphaplane2 - oThePos.x), 0.0, 1.0);
	if (diff < 0.0) {
		if (cond2 < 0.73 * ffx) {
			discard;
		}
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1;
	oColor2 *= 0.2 + 0.05 * vec4(0.5, 0.5, 0.5, 1.0) * 4000.0
	         * clamp(pow(mul1, 6.0) - 0.003, 0.0, 1.0);
	oColor2 += vec4(0.015, 0.023, 0.04, 1.0) * 0.5;
	oColor2 += clamp(1.0 * ffx - cond2, 0.0, 1.0) * 2.0 * vec4(1.0, 0.2, 0.1, 1.0)
	         * clamp(1.0 - diff, 0.0, 1.0);
	oColor2.w = TEXCOORD7.x * 8.0;
}
