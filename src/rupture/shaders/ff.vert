// OURS. The fixed-function vertex stage, for draws the demo makes with
// its Cg profiles disabled and for the post passes, which bind only a
// fragment program: position through projection times modelview, the
// colour and the texture coordinate passed on under their Cg semantics.
// The colour is clamped to [0, 1], as GL_CLAMP_VERTEX_COLOR (on by
// default) clamps it: the demo passes negative alphas and 1.1 reds.

in vec4 position;
in vec4 diffuse;
in vec2 uvs;

uniform mat4  ffModelViewProj;
uniform float ffPointSize;

out vec4 TEXCOORD0;
out vec4 COLOR0;

void main()
{
	gl_Position = ffModelViewProj * position;
	TEXCOORD0 = vec4(uvs, 0.0, 1.0);
	COLOR0 = clamp(diffuse, 0.0, 1.0);
	gl_PointSize = ffPointSize;
}
