/*
 * cgeffects/fragment_boat.cpp, transcribed: a fragment stage of the
 * vertex_boat pair. A discard where ten times decal's noise over world
 * xz passes 8; HCOL2 tinted blue-grey, plus a pale blue 0.4 where ten
 * times decal's fine noise over object xz plus object z passes 11.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: both basetexture assignments and its *= cll4, normal,
 * maxx, mx, cll1, cll4, nrr, coll and the first oColor2 assignment,
 * and with them the samplers texture, texture2 and light. Left out,
 * declared in the Cg and never read: alpha, alpha2, planecut and the
 * samplers cube and oned.
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
	float multt;

	if (texture(decal, oThePos.xz * 0.035).x * 10.0 + 0.0 * oThePos.y > 8.0) {
		discard;
	}
	multt = texture(decal, oPosition2.xz * 0.135).x * 10.0 + oPosition2.z;
	if (multt > 11.0) {
		multt = 0.4;
	} else {
		multt = 0.0;
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = (COLOR1) * vec4(0.2, 0.3, 0.4, 1.0) + multt * vec4(0.3, 0.6, 1.0, 1.0);
	oColor2.w = TEXCOORD7.x;
}
