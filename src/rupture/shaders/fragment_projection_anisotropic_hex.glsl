/*
 * cgeffects/fragment_projection_anisotropic_hex.cpp, transcribed: the
 * fragment stage of the vertex_projection_anisotropic_hex pair. It is
 * fragment_projection_anisotropic with a second discard: decal's noise
 * over world xz, scrolled in v by ft, times 750 plus |10.2 * world x|
 * passing sides. Within 14 units of world (x, z) = (0, 430) ft runs
 * with ttime and sides is raised by 60, within 10 by only 20. The
 * blue-grey band is scaled by blue, and (blue - 1) adds a faint tint
 * of basetexture.
 *
 * sides is a uniform, which GLSL cannot assign; the local sidesl
 * carries the Cg's += and -=. tex2Dproj(s, float3) divides xy by z,
 * as textureProj(s, vec3) does.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: fc, the first two basetexture assignments, normal, maxx,
 * mx, cll1, nrr and coll, and with them the samplers texture and
 * texture2. Left out, declared in the Cg and never read: alpha,
 * alpha2 and the samplers cube and oned.
 */

uniform float     ttime;
uniform float     blue;
uniform float     planecut;
uniform float     sides;
uniform sampler2D decal;
uniform sampler2D light;

in vec4 TEXCOORD1;   /* oPosition, the light-space position */
in vec4 TEXCOORD5;   /* oPosition2, the sphere-map coordinate */
in vec4 TEXCOORD6;   /* oThePos, the world position */
in vec4 COLOR0;      /* HCOL */
in vec4 COLOR1;      /* HCOL2 */
in vec4 TEXCOORD7;   /* Ramp */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	vec4  oPosition = TEXCOORD1;
	vec4  oPosition2 = TEXCOORD5;
	vec4  oThePos = TEXCOORD6;
	vec4  cll4, basetexture;
	float sidesl = sides;
	float ft, multt;

	ft = 0.0;
	if (length(oThePos.xz - vec2(0.0, 430.0)) < 14.0) {
		ft = -ttime * 0.24;
		sidesl += 60.0;
		if (length(oThePos.xz - vec2(0.0, 430.0)) < 10.0) {
			sidesl -= 40.0;
		}
	}
	cll4 = textureProj(decal, oPosition.xyw);
	if (texture(decal, oThePos.xz * 0.025).x * 15.0 + 3.0 * oThePos.y > planecut) {
		discard;
	}
	if (texture(decal, vec2(0.0, ft) + oThePos.xz * 0.055).x * 750.0 + abs(10.2 * oThePos.x)
	    > sidesl) {
		discard;
	}
	multt = texture(decal, oThePos.xz * 0.004).x * 10.0 - oPosition2.y * 3.0;
	if (multt > 5.5) {
		multt = 0.4;
	} else {
		multt = 0.0;
	}
	basetexture = 2.0 * (texture(light, -4.0 * oPosition2.xy)) + 0.1;
	basetexture *= cll4;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = (COLOR0 * basetexture + COLOR1 * vec4(0.2, 0.3, 0.89, 0.1))
	        + multt * vec4(0.2, 0.4, 0.5, 1.0) * blue
	        + (blue - 1.0) * basetexture * 0.015 * vec4(0.1, 0.3, 0.4, 0.0);
	oColor2.w = TEXCOORD7.x;
}
