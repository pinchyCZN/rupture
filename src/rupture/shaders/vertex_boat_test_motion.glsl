/*
 * cgeffects/vertex_boat_test_motion.cpp, transcribed: the vertex
 * stage of the fragment_boat_test_motion pair (effects/motor.txt 4).
 * Where dot(current - previous, mat3(ModelViewObject) * -oPosition2)
 * is positive the vertex is placed at its ModelViewLightProj
 * (previous) position, elsewhere at its ModelViewObject (current)
 * one, and HCOL carries the difference of the two after the divide
 * by w, as (dP + 1) * 0.5. LightPos, CameraPos and Reduction
 * are declared and unused, as in the Cg.
 *
 * Dropped, read by nothing: the transformed normal and tangent, smm,
 * and normalize(MotionBlurNormalDirection), whose result the Cg
 * discards. Only the outputs the Cg writes are declared; cgc writes
 * no other. COLOR0 is clamped to [0, 1], as an ARB vertex program's
 * colour outputs are; a GLSL varying is not.
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
out vec4 TEXCOORD5;   /* oPosition2 */
out vec4 TEXCOORD6;   /* oThePos */
out vec4 TEXCOORD0;   /* oUV */

void main()
{
	vec4  P = position;
	vec4  MotionBlurPos_current, MotionBlurPos_previous, MotionBlurstretch;
	vec3  MotionBlurmotionVector, MotionBlurNormalDirection, MotionBlur_dP;
	float MotionBlurflag;

	TEXCOORD5 = P;
	MotionBlurPos_current = ModelViewObject * P;
	MotionBlurPos_previous = ModelViewLightProj * P;
	P = ModelViewObject * P;
	TEXCOORD6 = P * 1.0;
	TEXCOORD0 = vec4(uvs * 1.0, 0.0, 1.0);
	MotionBlurmotionVector = MotionBlurPos_current.xyz - MotionBlurPos_previous.xyz;
	MotionBlurNormalDirection = vec3(0.0, 0.0, 0.0) - TEXCOORD5.xyz;
	MotionBlurNormalDirection = mat3(ModelViewObject) * MotionBlurNormalDirection;
	MotionBlurflag = dot(MotionBlurmotionVector, MotionBlurNormalDirection);
	if (MotionBlurflag > 0.0) {
		MotionBlurstretch = MotionBlurPos_current
		                  + (MotionBlurPos_previous - MotionBlurPos_current) * 1.0;
	} else {
		MotionBlurstretch = MotionBlurPos_current;
	}
	gl_Position = ModelViewProj * MotionBlurstretch;
	MotionBlurPos_current.xyz = MotionBlurPos_current.xyz / MotionBlurPos_current.w;
	MotionBlurstretch.xyz = MotionBlurstretch.xyz / MotionBlurstretch.w;
	MotionBlur_dP = MotionBlurstretch.xyz - MotionBlurPos_current.xyz;
	MotionBlur_dP += vec4(1.0, 1.0, 1.0, 1.0).xyz;
	MotionBlur_dP *= 0.5;
	COLOR0 = clamp(MotionBlur_dP.xyzz, 0.0, 1.0);
}
