/*
 * cgeffects/vertex_boat_test2building2.cpp, transcribed: the vertex
 * stage of the fragment_boat_testhex2, fragment_boat_testhex22 and
 * fragment_boat_testhex222 pairs. It is vertex_boat_test2building with
 * HCOL2 the vertex colour as it comes, unlit and unfaded. mul(M, v)
 * is M * v; Cg's float3x3(a, b, c) takes ROWS, so it is
 * transpose(mat3(a, b, c)) here. Reduction is declared and unused, as
 * in the Cg.
 *
 * Dropped, read by nothing or overwritten before the return: the
 * specular, diffuse and sphere-map terms (AllWhite, specularL, diff,
 * ref and the first m and mm from it); the rim term m once mm is taken
 * from it (1 - m, pow(m, 5), m + 0.5), which HCOL2 no longer reads;
 * the chain of earlier HCOL / HCOL2 assignments; oLight's rotated
 * value (it ends as (0, 0, -1)). COLOR0 and COLOR1 are clamped to
 * [0, 1], as an ARB vertex program's colour outputs are; a GLSL
 * varying is not.
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
	vec4  P = position;
	vec3  N = normal;
	vec3  T = tangent;
	float m, mm, mult;
	vec3  LightP, L, V, H, binormal;
	mat3  rotation;

	TEXCOORD5 = P;
	P = ModelViewObject * P;
	TEXCOORD6 = P * 1.0;
	N = mat3(ModelViewObject) * N;
	T = mat3(ModelViewObject) * T;
	TEXCOORD0 = vec4(uvs * 1.0, 0.0, 1.0);
	gl_Position = ModelViewProj * P;
	TEXCOORD1 = ModelViewLightProj * P;
	LightP = LightPos;
	L = normalize(LightP - P.xyz);
	V = normalize(CameraPos - P.xyz);
	H = normalize(L + V);
	m = max(dot(N, V), 0.0);
	mm = pow(m, 15.0) * 2.0;
	TEXCOORD3 = vec4(N, 0.0);
	binormal = cross(N, T);
	rotation = transpose(mat3(T, binormal, N));
	mult = 1.0 / (1.0 + 0.00018 * length(CameraPos - P.xyz) * length(CameraPos - P.xyz));
	TEXCOORD2 = vec4(0.0, 0.0, -1.0, 0.0);
	TEXCOORD4 = vec4(normalize(rotation * H), 0.0);
	COLOR1 = clamp(diffuse, 0.0, 1.0);
	COLOR0 = clamp(diffuse * mm * mult * 1.5, 0.0, 1.0);
	TEXCOORD7 = vec4(vec3(1.0, 1.0, 1.0) * mult, 0.0);
}
