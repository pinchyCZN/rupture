/*
 * cgeffects/vertex_projection_anisotropic_hex.cpp, transcribed: the
 * vertex stage of the fragment_projection_anisotropic_hex pair. It is
 * vertex_projection_anisotropic with one addition: within 10 units of
 * world (x, z) = (0, 430), a vertex below world y = 25 sinks by the
 * cube of its distance inside that circle, times Reduction.x. The
 * sunk position feeds the clip and light-space positions and L and V;
 * oThePos keeps the world position from before it.
 *
 * Dropped, overwritten before use: oPosition2's clip position (it
 * ends as the sphere-map coordinate), the first m from ref, the
 * specular and diffuse terms, the earlier HCOL / HCOL2 chain and
 * oLight's rotated value. COLOR0 and COLOR1 are clamped to [0, 1], as
 * an ARB vertex program's colour outputs are.
 */

in vec4 position;
in vec3 normal;
in vec4 diffuse;
in vec2 uvs;
in vec3 tangent;

uniform vec3 LightPos;
uniform vec3 CameraPos;
uniform vec3 Reduction;
uniform mat4 ModelViewLightProj;
uniform mat4 ModelViewObject;
uniform mat4 ModelViewProj;

out vec4 COLOR0;      /* HCOL */
out vec4 COLOR1;      /* HCOL2 */
out vec4 TEXCOORD7;   /* Ramp */
out vec4 TEXCOORD1;   /* oPosition */
out vec4 TEXCOORD5;   /* oPosition2 */
out vec4 TEXCOORD6;   /* oThePos */
out vec4 TEXCOORD0;   /* oUV */
out vec4 TEXCOORD3;   /* oNormal */
out vec4 TEXCOORD4;   /* oHalf */
out vec4 TEXCOORD2;   /* oLight */

void main()
{
	vec4  P = ModelViewObject * position;
	vec3  N = mat3(ModelViewObject) * normal;
	vec3  T = mat3(ModelViewObject) * tangent;
	vec4  oThePos;
	vec3  LightP, L, V, H, ref, binormal;
	float dd, m, mm, mult;
	mat3  rotation;

	oThePos = P * 1.0;
	TEXCOORD6 = oThePos;
	if (length(oThePos.xz - vec2(0.0, 430.0)) < 10.0) {
		dd = 10.0 - length(oThePos.xz - vec2(0.0, 430.0));
		dd *= dd * dd;
		if (P.y < 25.0) {
			P.y -= dd * Reduction.x;
		}
	}
	gl_Position = ModelViewProj * P;
	TEXCOORD0 = vec4(uvs * 1.0, 0.0, 1.0);
	TEXCOORD1 = ModelViewLightProj * P;
	LightP = LightPos;
	L = normalize(LightP - P.xyz);
	V = normalize(CameraPos - P.xyz);
	H = normalize(L + V);
	ref = reflect(V, N);
	ref.z += 5.0;
	mm = 2.0 * sqrt(ref.x * ref.x + ref.y * ref.y + (ref.z + 1.0) * (ref.z + 1.0));
	ref.x = 2.0 * ref.x / mm + 0.5;
	ref.y = 2.0 * ref.y / mm + 0.5;
	TEXCOORD5 = ref.xyzz;
	m = max(dot(N, V), 0.0);
	mm = pow(m, 15.0) * 2.0;
	m = 1.0 - m;
	/* m < 0 where a scaled ModelViewObject makes N longer than 1. vp30 compiles
	 * the pow as EX2(5 * LG2(m)), NaN there, and the clamped colour outputs
	 * turn NaN into 1; GLSL's pow is undefined, so the 3.4e38 stands in. */
	m = m < 0.0 ? 3.4e38 : pow(m, 5.0);
	TEXCOORD2 = vec4(0.0, 0.0, -1.0, 0.0);
	TEXCOORD3 = vec4(N, 0.0);
	binormal = cross(N, T);
	rotation = transpose(mat3(T, binormal, N));
	mult = 1.0 / (1.0 + 0.005 * length(CameraPos - P.xyz) * length(CameraPos - P.xyz));
	TEXCOORD4 = vec4(normalize(rotation * H), 0.0);
	COLOR1 = clamp(diffuse * m * mult, 0.0, 1.0);
	COLOR0 = clamp(diffuse * mm * mult, 0.0, 1.0);
	TEXCOORD7 = vec4(vec3(1.0, 1.0, 1.0) * mult, 0.0);
}
