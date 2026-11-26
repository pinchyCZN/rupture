/*
 * cgeffects/fragment_boat_test3.cpp, transcribed: a fragment stage of
 * the vertex_boat_test2building pair in this+0x11ec (effects/motor.txt
 * 4). Two hole lattices (effects/look.txt 5.1), one in world space
 * and one in object space, the world speckle mul1, and HCOL added
 * back as a factor after the alpha is set, so the alpha is scaled
 * too.
 *
 * No statement is dropped. Left out, declared in the Cg and never
 * read: alpha, alpha2, alphaplane, alphaplane2, planecut, ttime and
 * the samplers light, cube, texture, oned and texture2.
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

	if (cos(oThePos.y * 0.7) * cos(oThePos.x * 0.7) < -0.7) {
		discard;
	}
	if (cos(oPosition2.y * 1.7) * cos(oPosition2.x * 1.7) < -0.5) {
		discard;
	}
	mul1 = texture(decal, oThePos.xz * 0.03).x * texture(decal, oThePos.xy * 0.03).y;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1;
	oColor2 *= 1.0 + vec4(0.3, 0.4, 0.7, 1.0) * 4000.0 * clamp(pow(mul1, 6.0) - 0.001, 0.0, 1.0);
	oColor2.w = TEXCOORD7.x * 1.0;
	oColor2 *= 1.0 + 3.0 * (COLOR0);
}
