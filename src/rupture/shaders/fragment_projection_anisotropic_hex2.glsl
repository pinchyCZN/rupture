/*
 * cgeffects/fragment_projection_anisotropic_hex2.cpp, transcribed: a
 * fragment stage of the vertex_projection_anisotropic pair. It is
 * fragment_projection_anisotropic_hex with the planecut discard gone
 * and the second one fixed: decal's noise over world xz times 250
 * plus |10.2 * world x| passing 250. light at the sphere-map
 * coordinate times decal projected through oPosition, lit by HCOL,
 * plus HCOL2 tinted blue, all tripled. The multt band is computed and
 * its one reader in oColor2 is commented out in the Cg.
 *
 * tex2Dproj(s, float3) divides xy by z, as textureProj(s, vec3) does.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: the first two basetexture assignments, normal, maxx,
 * mx, cll1, multt, nrr and coll, and with them the samplers texture
 * and texture2. Left out, declared in the Cg and never read: alpha,
 * alpha2, blue, planecut, sides and the samplers cube and oned.
 */

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
	vec4 oPosition = TEXCOORD1;
	vec4 oPosition2 = TEXCOORD5;
	vec4 oThePos = TEXCOORD6;
	vec4 cll4, basetexture;

	cll4 = textureProj(decal, oPosition.xyw);
	if (texture(decal, oThePos.xz * 0.055).x * 250.0 + abs(10.2 * oThePos.x) > 250.0) {
		discard;
	}
	basetexture = 2.0 * (texture(light, -4.0 * oPosition2.xy)) + 0.1;
	basetexture *= cll4;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = 3.0 * (COLOR0 * basetexture + COLOR1 * vec4(0.2, 0.3, 0.89, 0.1));
	oColor2.w = TEXCOORD7.x;
}
