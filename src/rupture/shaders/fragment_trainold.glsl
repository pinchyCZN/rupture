/*
 * cgeffects/fragment_trainold.cpp, transcribed: a fragment stage of
 * the vertex_boat pair. A discard where 20 times decal's noise over
 * object xz, less 3 * (world y + 68.5), passes 15; HCOL2 scaled by
 * the eighth power of decal's noise over world yz plus 0.3, plus a
 * flat 0.1 where ten times that object-space noise plus object y
 * passes 10, plus a tenth of HCOL.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: basetexture, normal, maxx, mx, cll1, cll4, nrr, coll,
 * multt's first value 0, the first oColor2 assignment and the closing
 * multt1 += 0.2, and with them the samplers texture, texture2 and
 * light. Left out, declared in the Cg and never read: alpha, alpha2,
 * planecut and the samplers cube and oned.
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
	float multt1, multt2, multt;

	multt1 = texture(decal, oPosition2.xz * 0.095).x;
	if (multt1 * 20.0 - 3.0 * (oThePos.y + 68.5) - 1.0 - 2.0 * 3.0 > 8.0) {
		discard;
	}
	multt2 = texture(decal, oThePos.yz * 0.00195).x;
	if (multt1 * 10.0 + (oPosition2.y + 50.0) > 60.0) {
		multt = 0.1;
	} else {
		multt = 0.0;
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = pow(multt2 + 0.3, 8.0) * 8.0 * (COLOR1) + multt + COLOR0 * 0.1;
	oColor2.w = TEXCOORD7.x;
}
