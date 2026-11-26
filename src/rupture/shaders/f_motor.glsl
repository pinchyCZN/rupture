/*
 * cgeffects/f_motor.cpp, transcribed: the motorcycle's fragment stage
 * (effects/motor.txt 3.3). alphaplane2, which the part sets, is
 * declared and never read, as in the Cg. No statement is dropped.
 * Left out, declared in the Cg and never read: alpha, alpha2,
 * alphaplane, planecut, ttime and the samplers light, cube, texture,
 * oned and texture2.
 */

uniform float     alphaplane2;
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
	float mul1 = texture(decal, oThePos.xz * 0.08).x * texture(decal, oThePos.xy * 0.09).y;

	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1 * vec4(0.2, 0.3, 0.4, 0.1) * 0.2;
	oColor2 *= 1.0 + vec4(0.3, 0.3, 0.4, 1.0) * 100000.0 * TEXCOORD7.x
	         * clamp(pow(mul1, 8.0) - 0.0000, 0.0, 1.0);
	oColor2.w = TEXCOORD7.x;
}
