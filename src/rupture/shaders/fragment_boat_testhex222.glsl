/*
 * cgeffects/fragment_boat_testhex222.cpp, transcribed: a fragment
 * stage of the vertex_boat_test2building2 pair. fragment_boat_testhex22
 * with the third lattice at k 0.099, phase 3.2, and the speckle mul1
 * read at object xy * 0.0058 with ten times the gain and a 0.04 floor.
 *
 * Dropped, read by nothing: st. Left out, declared in the Cg and never
 * read: alpha, alpha2, doit, alphaplane, alphaplane2, planecut, ttime
 * and the samplers light, cube, texture, oned and texture2.
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
	float mul1;

	if (cos(oThePos.y * 4.5) * cos(oThePos.z * 4.5) < -0.7) {
		discard;
	}
	if (cos(oThePos.y * 0.8) * cos(oThePos.x * 0.8) < -0.1) {
		discard;
	}
	if (cos(oThePos.y * 0.099 + 3.2) * cos(oThePos.z * 0.099) < -0.5) {
		discard;
	}
	mul1 = texture(decal, oPosition2.xy * 0.0058).y;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = vec4(0.12, 0.17, 0.25, 1.0) * 0.3 * COLOR1;
	oColor2 *= 1.0 + 10.0 * vec4(0.3, 0.39, 0.5, 1.0) * 7000.0 * TEXCOORD7.x
	         * clamp(pow(mul1, 8.0) - 0.0400, 0.0, 1.0);
	oColor2.w = TEXCOORD7.x * 0.6;
}
