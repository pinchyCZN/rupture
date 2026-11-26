/*
 * cgeffects/f_kthrio1.cpp, transcribed: the fragment stage of the
 * vertex_boat_test2building pair in this+0x112c (effects/motor.txt
 * 4). Two spheres cut out in object space, two world hole lattices,
 * the first moved by alphaplane2, a fine lattice inside a third
 * sphere (effects/look.txt 5.1), and a glint from decal's x channel
 * above 0.65 raised to the twelfth power. oPosition2 + float3 is
 * oPosition2.xyz + vec3: cgc takes the length of three components.
 *
 * Dropped, read by nothing or overwritten before use: st, and the
 * first two oColor2 assignments. Left out, declared in the Cg and
 * never read: alpha, alpha2, doit, alphaplane, planecut, ttime and
 * the samplers light, cube, texture, oned and texture2.
 */

uniform float     alphaplane2;
uniform sampler2D decal;

in vec4 TEXCOORD5;   /* oPosition2, the object-space position */
in vec4 TEXCOORD6;   /* oThePos, the world position */
in vec4 COLOR0;      /* HCOL */
in vec4 TEXCOORD7;   /* Ramp */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	vec4  oPosition2 = TEXCOORD5;
	vec4  oThePos = TEXCOORD6;
	float mul1;

	if (length(oPosition2.xyz + vec3(0.0, -2.5, -1.5)) < 3.5) {
		discard;
	}
	if (length(oPosition2.xyz + vec3(0.0, +7.5, -1.5)) < 4.0) {
		discard;
	}
	if (cos(1.0 + oThePos.x * 0.8) * cos(oThePos.y * 0.8) < -0.6 + alphaplane2) {
		discard;
	}
	if (cos(1.0 + oThePos.z * 0.8) * cos(oThePos.y * 0.8) < 0.0) {
		discard;
	}
	if (length(oPosition2.xyz + vec3(0.0, +1.0, 0.0)) < 2.0) {
		if (cos(oThePos.x * 5.8) * cos(oThePos.y * 5.8) < -0.6) {
			discard;
		}
	}
	mul1 = texture(decal, oThePos.xy * 0.019).x;
	if (mul1 > 0.65) {
		mul1 = pow(mul1 - 0.25, 12.0);
	} else {
		mul1 = 0.0;
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = vec4(0.015, 0.023, 0.04, 1.0) * 1.2;
	oColor2 += COLOR0 * 0.06 + 8000.0 * mul1 * vec4(0.5, 0.8, 1.3, 1.0);
	oColor2.w = TEXCOORD7.x * 0.5;
}
