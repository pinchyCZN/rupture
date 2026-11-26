/*
 * cgeffects/fragment_projection_anisotropic.cpp, transcribed: the
 * fragment stage of the pair FUN_004bb6c0 loads into this+0x117c with
 * vertex_projection_anisotropic. A discard where decal's noise over
 * world xz plus 3 * oThePos.y passes planecut; light sampled at the
 * sphere-map coordinate oPosition2, times decal projected through
 * oPosition, lit by HCOL; HCOL2 tinted blue; 0.4 of a blue-grey
 * where decal's coarse noise * 10 - 3 * oPosition2.y passes 5.5.
 *
 * tex2Dproj(s, float3) divides xy by z, as textureProj(s, vec3) does.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: the first two basetexture assignments, normal, maxx,
 * mx, cll1, nrr and coll, and with them the samplers texture and
 * texture2. Left out, declared in the Cg and never read: alpha,
 * alpha2 and the samplers cube and oned.
 */

uniform float     planecut;
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
	float multt;

	cll4 = textureProj(decal, oPosition.xyw);
	if (texture(decal, oThePos.xz * 0.025).x * 15.0 + 3.0 * oThePos.y > planecut) {
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
	        + multt * vec4(0.2, 0.4, 0.5, 1.0);
	oColor2.w = TEXCOORD7.x;
}
