/*
 * cgeffects/fragment_train.cpp, transcribed: the fragment stage of the
 * vertex_boat pair (effects/motor.txt 4). A discard where decal's
 * noise over world xz, less 3 * oThePos.y, passes 15; HCOL2 tinted
 * blue.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: basetexture, normal, maxx, mx, cll1, cll4, multt, nrr,
 * coll and the first oColor2 assignment, and with them HCOL and the
 * samplers texture, texture2 and light. Left out, declared in the Cg
 * and never read: alpha, alpha2, planecut and the samplers cube and
 * oned.
 */

uniform sampler2D decal;

in vec4 TEXCOORD6;   /* oThePos, the world position */
in vec4 COLOR1;      /* HCOL2 */
in vec4 TEXCOORD7;   /* Ramp */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	vec4 oThePos = TEXCOORD6;

	if (texture(decal, oThePos.xz * 0.035).x * 10.0 - 3.0 * oThePos.y - 1.0 - 2.0 * 3.0 > 8.0) {
		discard;
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = (COLOR1) * vec4(0.2, 0.3, 0.4, 1.0);
	oColor2.w = TEXCOORD7.x;
}
