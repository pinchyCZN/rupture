/*
 * cgeffects/fragment_bend3.cpp, transcribed: the lens bend, the colour
 * fringes and the motion smear (effects/look.txt 3.3). Dropped: cl and
 * PI, never read, and VV2, computed and never read.
 */

uniform sampler2D decal;
uniform sampler2D decal2;
uniform float     alpha;
uniform float     RightV;
uniform float     UpV;

in vec4 TEXCOORD0;

layout(location = 0) out vec4 COLOR0;

void main()
{
	vec2  UV = TEXCOORD0.xy;
	vec3  DirV;
	float lgn, rec;
	vec2  VV1, Clt;
	vec4  color, Cl;
	int   r;

	UV -= vec2(0.5, 0.5);
	UV *= 2.0;
	DirV = vec3(UV.x, UV.y, 0.6);
	lgn = length(DirV);
	DirV /= lgn;
	VV1 = vec2(0.0030, 0.0) * UV;
	UV.xy = UV.xy + (DirV.xy - UV.xy) * alpha;
	UV *= 0.5;
	UV += vec2(0.5, 0.5);
	color = texture(decal, UV) * vec4(1.0, 0.0, 0.0, 1.0);
	color += texture(decal, UV + VV1) * vec4(0.0, 1.0, 0.0, 1.0);
	color += texture(decal, UV + VV1 * 2.0) * vec4(0.0, 0.0, 1.0, 1.0);
	rec = clamp(1.0 - 0.4 * lgn, 0.0, 1.0);
	color = texture(decal2, UV) + (color - texture(decal2, UV)) * rec;
	VV1 *= 2.0;
	color += texture(decal2, UV) * vec4(1.0, 0.0, 0.0, 1.0);
	color += texture(decal2, UV + VV1) * vec4(0.0, 1.0, 0.0, 1.0);
	color += texture(decal2, UV + VV1 * 2.0) * vec4(0.0, 0.0, 1.0, 1.0);
	Clt = vec2(RightV, UpV) * 1.0 / 8.0;
	if (length(Clt) > 0.0035) {
		Clt *= 0.6;
		Cl = vec4(0.0, 0.0, 0.0, 0.0);
		for (r = 0; r < 4; r++) {
			Cl += texture(decal, UV + Clt * float(r));
		}
		for (r = 0; r < 4; r++) {
			Cl += texture(decal2, UV + Clt * float(r));
		}
		Cl *= 1.0 / 8.0;
		color *= 0.6;
		color += Cl * 0.8;
	}
	COLOR0 = color;
}
