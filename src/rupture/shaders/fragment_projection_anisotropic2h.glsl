/*
 * cgeffects/fragment_projection_anisotropic2h.cpp, transcribed: a
 * fragment stage of the vertex_projection_anisotropic pair, loaded by
 * part_city::Init. It is fragment_projection_anisotropic2hh without
 * the ripple: the same oval discard, length(world.xz * (0.6, 1)) >
 * 29.5, then decal laid flat over world xz, plus the band multt from
 * texture2 in light space.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: basetexture, normal, maxx, mx, cll1, cll4, mm, nrr, coll
 * and the first oColor2 assignment, and with them the samplers light,
 * cube, texture and oned and the inputs oUV, HCOL, HCOL2, Ramp,
 * oLight, oHalf and uvl. Left out, declared in the Cg and never read:
 * alpha, alpha2 and planecut.
 */

uniform sampler2D decal;
uniform sampler2D texture2;

in vec4 TEXCOORD1;   /* oPosition, the light-space position */
in vec4 TEXCOORD5;   /* oPosition2 */
in vec4 TEXCOORD6;   /* oThePos, the world position */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	vec4  oPosition = TEXCOORD1;
	vec4  oPosition2 = TEXCOORD5;
	vec4  oThePos = TEXCOORD6;
	float lf = length(vec2(0.0, 0.0) + oThePos.xz * vec2(0.6, 1.0));
	float cc, multt;

	if (lf > 29.5) {
		discard;
	}
	cc = texture(decal, oThePos.xz * 0.016 + 0.5 + vec2(0.0, -0.1)).x;
	multt = texture(texture2, oPosition.xy * 0.01).x * 10.0 - oPosition2.y * 4.2;
	if (multt > 3.05) {
		multt = 0.1;
	} else {
		multt = 0.0;
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = multt * 0.5 * vec4(0.7, 1.0, 1.3, 1.0) * 0.3;
	oColor2 += cc * vec4(1.0, 1.4, 1.9, 1.0) * 0.3 * 0.8;
	oColor2.w = 0.4;
}
