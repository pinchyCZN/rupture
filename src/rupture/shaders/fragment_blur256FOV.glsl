/*
 * cgeffects/fragment_Blur256FOV.cpp, transcribed: fragment_blur256's
 * thirteen-tap cross at 1, 2 and 4 steps of 0.006 instead of 0.0011,
 * scaled by 0.078 instead of divided by 13. The file is named as the
 * code names it, "cgeffects/fragment_blur256FOV.cpp".
 *
 * The output is float3 in the Cg, so cgc writes result.color.xyz and
 * leaves w unwritten; COLOR0 is a vec3 here, and the alpha written is
 * undefined in both. decal2 and alpha are declared and unused, as in
 * the Cg. Dropped: ccl1, never read.
 */

uniform sampler2D decal;
uniform sampler2D decal2;
uniform vec3      alpha;

in vec4 TEXCOORD0;

layout(location = 0) out vec3 COLOR0;

void main()
{
	vec2  oPosition = TEXCOORD0.xy;
	vec4  cl;
	float v = 0.006;
	vec2  MX = vec2(v, 0.0);
	vec2  MU = vec2(0.0, v);

	cl = 1.0 * texture(decal, oPosition);
	cl += 1.0 * texture(decal, oPosition + MX);
	cl += 1.0 * texture(decal, oPosition - MX);
	cl += 1.0 * texture(decal, oPosition + MU);
	cl += 1.0 * texture(decal, oPosition - MU);
	cl += 1.0 * texture(decal, oPosition + 2.0 * MX);
	cl += 1.0 * texture(decal, oPosition - 2.0 * MX);
	cl += 1.0 * texture(decal, oPosition + 2.0 * MU);
	cl += 1.0 * texture(decal, oPosition - 2.0 * MU);
	cl += 1.0 * texture(decal, oPosition + 4.0 * MX);
	cl += 1.0 * texture(decal, oPosition - 4.0 * MX);
	cl += 1.0 * texture(decal, oPosition + 4.0 * MU);
	cl += 1.0 * texture(decal, oPosition - 4.0 * MU);
	COLOR0 = (cl * 0.078).xyz;
}
