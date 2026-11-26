/*
 * cgeffects/fragment_Blur_circular.cpp, transcribed: a radial blur
 * toward the screen centre (0.5, 0.5) in seven taps, weight 0.191
 * falling by 0.05 a tap, multiplied by the unblurred sample sti and
 * by 4.5. The tap at 0.93 * 0.15 toward the centre scales oPosition
 * by 1 - 0.93 * 0.015, not 1 - 0.93 * 0.15; that is the Cg's, and
 * cgc compiles it so (0.98605).
 *
 * The output is float3 in the Cg, so cgc writes result.color.xyz and
 * leaves w unwritten; COLOR0 is a vec3 here, and the alpha written is
 * undefined in both. decal2 and alpha are declared and unused, as in
 * the Cg.
 *
 * Dropped: dss's first value, overwritten by 1 before use; v, MX and
 * MU, computed and never read; ccl1, never read.
 */

uniform sampler2D decal;
uniform sampler2D decal2;
uniform vec3      alpha;

in vec4 TEXCOORD0;

layout(location = 0) out vec3 COLOR0;

void main()
{
	vec2  oPosition = TEXCOORD0.xy;
	vec4  cl, sti;
	float dss;
	vec2  dirr = vec2(0.5, 0.5);

	dss = 1.0;
	dss *= 4.5;
	cl = vec4(0.0, 0.0, 0.0, 0.0);
	sti = texture(decal, oPosition);
	cl += 0.191 * texture(decal, oPosition);
	cl += 0.191 * (0.95) * texture(decal, 0.93 * 0.075 * dirr + (1.0 - 0.93 * 0.075) * oPosition);
	cl += 0.191 * (0.9) * texture(decal, 0.93 * 0.15 * dirr + (1.0 - 0.93 * 0.015) * oPosition);
	cl += 0.191 * (0.85) * texture(decal, 0.93 * 0.225 * dirr + (1.0 - 0.93 * 0.225) * oPosition);
	cl += 0.191 * (0.8) * texture(decal, 0.93 * 0.300 * dirr + (1.0 - 0.93 * 0.300) * oPosition);
	cl += 0.191 * (0.75) * texture(decal, 0.93 * 0.375 * dirr + (1.0 - 0.93 * 0.375) * oPosition);
	cl += 0.191 * (0.7) * texture(decal, 0.93 * 0.450 * dirr + (1.0 - 0.93 * 0.450) * oPosition);
	cl *= dss * sti;
	COLOR0 = cl.xyz;
}
