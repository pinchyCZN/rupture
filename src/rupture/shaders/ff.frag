// OURS. The fixed-function fragment stage: the colour, times the
// texture on unit 0 when GL_TEXTURE_2D is enabled (GL_MODULATE),
// written to both draw buffers, as fixed-function GL writes every
// enabled one. Writing only the first leaves the second undefined: a
// part's stars and billboards would black out its colour image.

uniform sampler2D ffTexture;
uniform int       ffTextured;

in vec4 TEXCOORD0;
in vec4 COLOR0;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragColor1;

void main()
{
	if (ffTextured != 0) {
		fragColor = texture(ffTexture, TEXCOORD0.xy) * COLOR0;
	} else {
		fragColor = COLOR0;
	}
	fragColor1 = fragColor;
}
