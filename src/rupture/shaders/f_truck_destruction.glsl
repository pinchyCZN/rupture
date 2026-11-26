/*
 * cgeffects/f_truck_destruction.cpp, transcribed: the fragment stage
 * of the v_truck_destruction pair. It is f_truck with no discard and
 * no burning edge, and HCOL2 at 0.3 in place of 0.2: the speckle mul1
 * read at a position pulled 70% of the way back to object space, and
 * HCOL added tinted blue.
 *
 * Dropped, read by nothing that reaches an output: the first mul1
 * (object space), cond2 and ffx, and with them alphaplane2. Left out,
 * declared in the Cg and never read: direction, alpha, alpha2,
 * alphaplane, planecut, ttime and the samplers light, cube, texture,
 * oned and texture2.
 */

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
	float mul1;

	oThePos = oPosition2 + (oThePos - oPosition2) * 0.3;
	mul1 = texture(decal, oThePos.xz * 0.06).x * texture(decal, oThePos.xy * 0.05).y;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1 * vec4(0.2, 0.3, 0.4, 0.1) * 0.3;
	oColor2 *= 1.0 + vec4(0.3, 0.3, 0.4, 1.0) * 40000.0 * TEXCOORD7.x
	         * clamp(pow(mul1, 8.0) - 0.0000, 0.0, 1.0);
	oColor2 += vec4(0.2, 0.3, 0.4, 1.0) * 0.2 * COLOR0;
	oColor2.w = TEXCOORD7.x;
}
