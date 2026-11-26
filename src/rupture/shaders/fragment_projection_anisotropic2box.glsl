/*
 * cgeffects/fragment_projection_anisotropic2box.cpp, transcribed: the
 * room pair's fragment stage (effects/video.txt 4). Three discards,
 * every one by position, none by the mesh's own coordinates:
 *
 *   a disc in the sphere-map coordinate oPosition2 (view dependent),
 *   the SWISS CHEESE - a sine lattice in world space above y = 5,
 *   the DISSOLVE - decal's noise laid over world xz and xy, against
 *   the threshold ttime.
 *
 * oursCheeseOff is OURS, a teaching switch for gfx_test: unset it is
 * 0 and the lattice cuts exactly as the demo's does.
 *
 * Dropped, read by nothing that reaches an output: multtt, d2, dd,
 * fl, mul1, basetexture, normal, nrr, cll1, cll4, coll, and the
 * earlier oColor2 assignments.
 */

uniform float     ttime;
uniform float     oursCheeseOff;
uniform sampler2D decal;

in vec4 TEXCOORD5;   /* oPosition2 */
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
	float lf = length(vec2(14.0, -4.0) + oPosition2.yx);

	if (lf < 7.5) {
		discard;
	}
	if (oursCheeseOff == 0.0 && oThePos.y > 5.0) {
		if (sin(oThePos.z * 0.07) * cos(oThePos.x * 0.07 + oThePos.y * 0.1) < -0.3) {
			discard;
		}
	}
	if (400.0 * clamp(pow(texture(decal, oThePos.xz * 0.00003).y
	                      * texture(decal, oThePos.xy * 0.00005).y, 6.0) - 0.0, 0.0, 1.0) > ttime) {
		discard;
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1 * vec4(0.2, 0.3, 0.5, 1.0) * 0.3;
	oColor2.w = TEXCOORD7.x * 10.0;
}
