/*
 * cgeffects/fragment_wireframe.cpp, transcribed: the edge pass
 * (effects/look.txt 3.2). decal and decal2 are normal images, decal3
 * the colour. Dropped: V..V3, never read, and two empty ifs.
 */

uniform sampler2D decal;
uniform sampler2D decal2;
uniform sampler2D decal3;
uniform vec3      alpha;

in vec4 TEXCOORD0;

layout(location = 0) out vec4 COLOR0;

void main()
{
	vec2  oPosition = TEXCOORD0.xy;
	vec3  Col1 = texture(decal, oPosition).xyz;
	vec3  Col2 = texture(decal2, oPosition).xyz;
	float theta;
	vec4  BC;

	Col1 = Col1 * 2.0 - 1.0;
	Col2 = Col2 * 2.0 - 1.0;
	theta = 0.0;
	theta = length(Col1 - Col2) * 0.3;
	theta = max(theta, 1.0 * (1.0 - abs(dot(Col1, Col2))));
	theta *= 1.0;
	theta = clamp(theta, 0.0, 1.0);
	BC = texture(decal3, oPosition);
	COLOR0 = theta * (0.3 + 30.0 * BC) + BC;
	COLOR0 *= BC.w;
}
