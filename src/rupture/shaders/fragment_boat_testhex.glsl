/*
 * cgeffects/fragment_boat_testhex.cpp, transcribed: a fragment stage
 * of the vertex_boat_test2building pair in this+0x11f8
 * (effects/motor.txt 4). Two discards against decal's noise over
 * world zx, by a threshold st that rises with world x below 550 and,
 * scaled by doit, with object x below 300; the world speckle mul1
 * and HCOL added at half weight.
 *
 * No statement is dropped. Left out, declared in the Cg and never
 * read: alpha, alpha2, alphaplane, planecut, ttime and the samplers
 * light, cube, texture, oned and texture2. alphaplane2 is read only
 * times zero and is kept.
 */

uniform float     doit;
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
	float st, mul1;

	st = clamp((-alphaplane2 * 0.0 - oThePos.x + 550.0) * 0.001, 0.0, 1.0);
	st += clamp((-alphaplane2 * 0.0 - oPosition2.x + 300.0) * 0.001, 0.0, 1.0) * doit;
	if (texture(decal, oThePos.zx * 0.001).y < st - 0.02) {
		discard;
	}
	if (texture(decal, oThePos.zx * 0.0004).x < st + 0.1) {
		discard;
	}
	mul1 = texture(decal, oThePos.xz * 0.005).x * texture(decal, oThePos.xy * 0.005).y;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1 * vec4(0.2, 0.2, 0.22, 0.1) * 10.0;
	oColor2 *= 1.0 + vec4(0.3, 0.35, 0.5, 1.0) * 300000.0 * TEXCOORD7.x
	         * clamp(pow(mul1, 8.0) - 0.0000, 0.0, 1.0);
	oColor2 += 0.5 * COLOR0;
	oColor2.w = TEXCOORD7.x * 2.0;
}
