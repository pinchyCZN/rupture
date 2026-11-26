/*
 * cgeffects/fragment_projection_anisotropic2hh.cpp, transcribed: the
 * TV pair's fragment stage (effects/video.txt 3.1 and 4). One discard,
 * the ellipse length(world.xz * (0.6, 1)) > 29.5 that is the TV's
 * oval; inside it, the video frame decal laid over world xz and
 * rippled by dist, plus a band multt from texture2 in light space.
 *
 * Dropped, read by nothing that reaches an output: basetexture,
 * normal, maxx, mx, cll1, cll4, mm, nrr, coll, the first oColor2
 * assignment, and with them the samplers light, cube, texture and
 * oned and the inputs oUV, HCOL, HCOL2, Ramp, oLight, oHalf and uvl.
 * planecut is declared by the Cg file and read by no live statement.
 */

uniform float     multip;
uniform float     ttime;
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
	float fly, dist, dist2, cc, multt;

	if (lf > 29.5) {
		discard;
	}
	fly = oThePos.z * 1.4;
	dist = sin(fly - ttime * 1.0 * 3.3) * sin(1.5 * fly + ttime * 2.0 * 2.4)
	     * sin(1.32 * fly - ttime * 3.0 * 1.0 * 3.0) * sin(1.1 * fly + ttime * 1.0 * 1.3);
	dist2 = dist;
	/* pow(1.13 * dist, 3): GLSL's pow is undefined below 0, and cgc
	 * 2.1 expands this one to signed multiplies (arbfp1 and fp40). */
	dist = (1.13 * dist) * (1.13 * dist) * (1.13 * dist);
	dist *= 0.04 * multip;
	cc = texture(decal, oThePos.xz * 0.016 * vec2(0.8, 1.0 - dist2 * 0.0) + vec2(dist, 0.0) + 0.5
	             + vec2(0.0, -0.1)).x;
	multt = texture(texture2, oPosition.xy * 0.01).x * 10.0 - oPosition2.y * 4.2;
	if (multt > 3.05) {
		multt = 0.1;
	} else {
		multt = 0.0;
	}
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = multt * 0.5 * vec4(0.7, 1.0, 1.3, 1.0) * 0.3;
	oColor2 += cc * vec4(1.0, 1.4 - dist * 20.0, 1.9, 1.0) * 0.3 * 1.2;
	oColor2.w = 0.4;
}
