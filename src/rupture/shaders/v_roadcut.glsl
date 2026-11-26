/*
 * cgeffects/v_roadcut.cpp, transcribed: the vertex stage of the
 * f_roadcut pair (effects/motor.txt 4). It is vertex_boat_test2cut's
 * z push behind Reduction.x plus a drop in y that grows with the
 * square of the distance behind it, ffx2, which also scales HCOL2 and
 * leaves in oLight.x. mul(M, v) is M * v; Cg's float3x3(a, b, c)
 * takes ROWS, so it is transpose(mat3(a, b, c)) here.
 *
 * Dropped, read by nothing or overwritten before the return: the
 * specular, diffuse and sphere-map terms (AllWhite, specularL, diff,
 * ref and the first m and mm from it) and the chain of earlier HCOL /
 * HCOL2 assignments. COLOR0 and COLOR1 are clamped to [0, 1], as an
 * ARB vertex program's colour outputs are; a GLSL varying is not.
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
	vec4  oThePos;
	float ffx, ffx2, m, mm, mult;
	vec3  LightP, L, V, H, binormal;
	mat3  rotation;

	TEXCOORD5 = P;
	P = ModelViewObject * P;
	oThePos = P * 1.0;
	TEXCOORD6 = oThePos;
	ffx = clamp(0.05 * (Reduction.x - oThePos.x), 0.0, 1.0);
	if (P.z > 0.0) {
		P.z += ffx * 2.0;
	} else {
		P.z -= ffx * 2.0;
	}
	ffx2 = 0.14 * (Reduction.x - oThePos.x);
	if (ffx2 < 0.0) {
		ffx2 = 0.0;
	}
	ffx2 = pow(ffx2, 2.0);
	P.y -= ffx2 * 0.3;
	TEXCOORD2 = vec4(vec3(ffx2, 0.0, 0.0) * 0.005, 0.0);
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
	m = 1.0 - m;
	/* m < 0 where a scaled ModelViewObject makes N longer than 1. vp30 compiles
	 * the pow as EX2(5 * LG2(m)), NaN there, and the clamped colour outputs
	 * turn NaN into 1; GLSL's pow is undefined, so the 3.4e38 stands in. */
	m = m < 0.0 ? 3.4e38 : pow(m, 5.0);
	m += 0.5;
	TEXCOORD3 = vec4(N, 0.0);
	binormal = cross(N, T);
	rotation = transpose(mat3(T, binormal, N));
	mult = 1.0 / (1.0 + 0.0008 * length(CameraPos - P.xyz) * length(CameraPos - P.xyz));
	TEXCOORD4 = vec4(normalize(rotation * H), 0.0);
	COLOR1 = clamp(diffuse * m * mult * 0.5 * ffx2, 0.0, 1.0);
	COLOR0 = clamp(diffuse * mm * mult, 0.0, 1.0);
	TEXCOORD7 = vec4(vec3(1.0, 1.0, 1.0) * mult, 0.0);
}
