/*
 * cgeffects/fragment_blur256.cpp, transcribed: thirteen taps in a
 * cross at 1, 2 and 4 steps of 0.0011, averaged. alpha is declared
 * and unused, as in the Cg.
 */

uniform sampler2D decal;
uniform vec3      alpha;

in vec4 TEXCOORD0;

layout(location = 0) out vec4 COLOR0;

void main()
{
	vec2  oPosition = TEXCOORD0.xy;
	vec4  cl;
	float v = 0.0011;
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
	COLOR0 = cl / 13.0;
}
