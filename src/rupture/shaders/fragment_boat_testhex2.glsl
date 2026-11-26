/*
 * cgeffects/fragment_boat_testhex2.cpp, transcribed: a fragment stage
 * of the vertex_boat_test2building2 pair. Five discards: below world
 * y -85; a coarse lattice over object y, z at k 0.1 whose threshold
 * -1 + doit opens it (effects/look.txt 5.1); a fine lattice at k 2.79,
 * c -0.9; decal's noise over world zx against object y; decal's noise
 * over world xz against a threshold that rises below world x 100.
 * Then the object-space speckle mul1 on HCOL2, tinted blue.
 *
 * Dropped, overwritten before use: the first two oColor2 assignments,
 * and with them HCOL. Left out, declared in the Cg and never read:
 * alpha, alpha2, alphaplane, alphaplane2, planecut, ttime and the
 * samplers light, cube, texture, oned and texture2.
 */

uniform float     doit;
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
	float st, mul1;

	if (oThePos.y < -85.0) {
		discard;
	}
	if (cos(oPosition2.y * 0.1) * cos(oPosition2.z * 0.1) < -1.0 + doit) {
		discard;
	}
	if (cos(oPosition2.y * 2.79) * cos(oPosition2.z * 2.79) < -0.9) {
		discard;
	}
	if (texture(decal, oThePos.zx * 0.004).x * 10.0 < oPosition2.y * 0.8 + 4.0) {
		discard;
	}
	st = clamp(-oThePos.x * 0.001 + 0.1, 0.0, 1.0);
	if (texture(decal, oThePos.xz * 0.0005).x * 1.0 < 0.3 + st) {
		discard;
	}
	mul1 = texture(decal, oPosition2.xy * 0.028).y;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = vec4(0.12, 0.17, 0.25, 1.0) * 0.3 * COLOR1;
	oColor2 *= 1.0 + vec4(0.3, 0.39, 0.5, 1.0) * 7000.0 * TEXCOORD7.x
	         * clamp(pow(mul1, 8.0) - 0.0600, 0.0, 1.0);
	oColor2.w = TEXCOORD7.x * 0.6;
}
