/*
 * cgeffects/fragment_motion.cpp, transcribed: the motion pass. The
 * velocity is decal2's xy * 2 - 1, and where the stored value is not
 * zero, twelve taps of decal at 0 to
 * -0.1 times the velocity, each weighted 1/11, so the sum is 12/11 of
 * the mean; elsewhere decal as it is. The zero test is on the stored
 * value doubled, before the 1 is taken off. alpha is declared and
 * unused, as in the Cg.
 *
 * The output is float3 in the Cg, so cgc writes result.color.xyz and
 * leaves w unwritten; COLOR0 is a vec3 here, and the alpha written is
 * undefined in both. No statement is dropped.
 */

uniform sampler2D decal;
uniform sampler2D decal2;
uniform float     alpha;

in vec4 TEXCOORD0;

layout(location = 0) out vec3 COLOR0;

void main()
{
	vec2  oPosition = TEXCOORD0.xy;
	vec2  velocity = texture(decal2, oPosition).xy * 2.0;
	vec4  outcol = vec4(0.0, 0.0, 0.0, 0.0);
	vec4  sm;
	float x;
	int   t;

	if (length(velocity) > 0.001) {
		velocity -= vec2(1.0, 1.0);
		sm = vec4(0.0, 0.0, 0.0, 0.0);
		for (t = 0; t < 12; t++) {
			x = -0.1 * float(t) / 11.0;
			sm = texture(decal, oPosition + velocity * x) * (1.0 / 11.0);
			outcol += sm;
		}
	} else {
		outcol = texture(decal, oPosition);
	}
	COLOR0 = outcol.xyz;
}
