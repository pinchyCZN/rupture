/*
 * cgeffects/fragment_heli2r.cpp, transcribed: a fragment stage of the
 * vertex_boat pair, loaded by part_end::Init. fragment_heli2 with one
 * discard: where world x passes 2 and decal's noise over world zx,
 * scaled by 0.035 and squeezed fifty times in z, passes 0.72. The
 * rest is fragment_heli2's: multt, 0.06 where fifteen times decal's
 * noise over world xz passes 10.5, tripled in pale blue; multt2, 0.1
 * where decal's noise over object xz less 0.15 * object x passes
 * ttime - 1, in orange at 1.5 scaled by strong; both over HCOL2
 * tinted blue and a hundredth of HCOL. Alpha is 100 times Ramp.x.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: all three basetexture assignments and its *= cll4,
 * normal, maxx, mx, cll1, cll4 (zero here, not a projected read), nrr,
 * coll and the first oColor2 assignment, and with them the samplers
 * texture, texture2 and light and the inputs oPosition, oUV, oLight,
 * oHalf and uvl. Left out, declared in the Cg and never read: alpha,
 * alpha2, planecut and the samplers cube and oned.
 */

uniform float     ttime;
uniform float     strong;
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
	float vall, multt, multt2;

	vall = texture(decal, oThePos.zx * 0.035 * vec2(0.02, 1.0)).x;
	if (oThePos.x > 2.0) {
		if (vall > 0.72) {
			discard;
		}
	}
	multt = texture(decal, oThePos.xz * 0.135).x * 15.0;
	if (multt > 10.5) {
		multt = 0.06;
	} else {
		multt = 0.0;
	}
	multt2 = 0.0;
	if (texture(decal, oPosition2.xz * 0.135).x * 1.0 - 0.15 * oPosition2.x > -1.0 + ttime) {
		multt2 = 0.1;
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR0 * 0.01 + (COLOR1) * 1.0 * vec4(0.2, 0.4, 0.6, 1.0)
	        + multt * vec4(0.4, 0.6, 1.1, 1.0) * 3.0
	        + multt2 * vec4(1.0, 0.3, 0.1, 1.0) * 1.5 * strong;
	oColor2.w = TEXCOORD7.x * 100.0;
}
