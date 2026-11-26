/*
 * cgeffects/fragment_trainold2.cpp, transcribed: the fragment stage of
 * the vertex_train pair. It is fragment_trainold with three changes:
 * a lattice cut inside an ellipse about object (y, z) = (2.3, -0.8/0.7)
 * for object x under 1, opening as alphaengine * lf rises; the depth
 * discard reads decal over world xz, offset by upper, and its margin
 * ress also adds a flame tint scaled by flames; HCOL2's gain is 1.3.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: basetexture, normal, maxx, mx, cll1, cll4, nrr, coll,
 * multt's first value 0, the first oColor2 assignment and the closing
 * multt1 += 0.2, and with them the samplers texture, texture2 and
 * light. Left out, declared in the Cg and never read: alpha, alpha2,
 * planecut and the samplers cube and oned.
 */

uniform float     alphaengine;
uniform float     flames;
uniform float     upper;
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
	float lf, multt1, multtt, ress, multt2, multt;

	lf = length(vec2(-2.3, 0.8) + oPosition2.yz * vec2(1.0, 0.7));
	if (lf < 1.5) {
		if (oPosition2.x < 1.0) {
			if ((sin(oPosition2.y * 18.35) + cos(oPosition2.z * 18.35)) > -2.0 + alphaengine * lf) {
				discard;
			}
		}
	}
	multt1 = texture(decal, oPosition2.xz * 0.095).x;
	multtt = texture(decal, oThePos.xz * 0.095).x;
	ress = multtt * 20.0 - 3.0 * (oThePos.y + 69.0 + upper) - 1.0 - 2.0 * 3.0;
	if (ress > 8.0) {
		discard;
	}
	multt2 = texture(decal, oThePos.yz * 0.00195).x;
	if (multt1 * 10.0 + (oPosition2.y + 50.0) > 60.0) {
		multt = 0.1;
	} else {
		multt = 0.0;
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = pow(multt2 + 0.3, 8.0) * 1.3 * (COLOR1) + multt + COLOR0 * 0.1
	        + ress * vec4(1.0 + ress * 0.06, 0.3, 0.1, 0.0) * 0.01 * flames;
	oColor2.w = TEXCOORD7.x * 4.0;
}
