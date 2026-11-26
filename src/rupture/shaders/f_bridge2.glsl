/*
 * cgeffects/f_bridge2.cpp, transcribed: a fragment stage of the
 * v_roadcut3 pair. style picks one of two discard sets over the world
 * position, both cut on the square of a sin * cos lattice whose phase
 * d2 swings with world z. Style 1 cuts below the deck height gefyra,
 * removes two slabs of the box z -970..-900, y -260..-70, and above
 * y = -70 burns red within about 33 units of z = fireplace, scaled by
 * fireplacealpha and decal's twelfth power. Style 0 cuts everywhere,
 * its threshold lowered by focus. Then HCOL2 takes the world speckle
 * mul1 and the red.
 *
 * int(style) truncates toward zero, as the release's cg.dll compiles
 * the Cg int conversion (fp40: FLR of |style|, sign restored). The two
 * pow(x, 2) of a sin * cos product are written as s * s: the base goes
 * negative, where GLSL's pow is undefined, and cg.dll compiles both to
 * one multiply.
 *
 * Dropped, read by nothing that reaches an output or overwritten
 * before use: dd (both values, so its pow of a negative base is never
 * evaluated), basetexture, normal, nrr, cll1, cll4, coll and the first
 * oColor2 assignment, and with them the samplers texture, texture2 and
 * light. Left out, declared in the Cg and never read: alpha, alpha2,
 * alphaplane, alphaplane2, ttime and the samplers cube and oned.
 */

uniform float     style;
uniform float     focus;
uniform float     fireplace;
uniform float     fireplacealpha;
uniform sampler2D decal;

in vec4 TEXCOORD6;   /* oThePos, the world position */
in vec4 COLOR1;      /* HCOL2 */
in vec4 TEXCOORD7;   /* Ramp */
in vec4 TEXCOORD3;   /* oNormal */

layout(location = 0) out vec4 oColor1;
layout(location = 1) out vec4 oColor2;

void main()
{
	vec4  oThePos = TEXCOORD6;
	int   thestyle;
	float d2, red, gefyra, sd, s, satt, mul1;

	thestyle = int(style);
	d2 = (-90.0 + 30.0 * sin(oThePos.z * 0.03));
	red = 0.0;
	if (thestyle == 1) {
		gefyra = -71.0;
		if (oThePos.z < 300.0) {
			gefyra = -70.0;
		}
		if (oThePos.y < gefyra) {
			if (oThePos.z - 250.0 > (oThePos.y + 71.0) * 4.0) {
				discard;
			}
			if (sin(oThePos.z * 0.2) > -0.95) {
				sd = sin(oThePos.y * 0.11);
				s = sin(oThePos.z * 0.07 + d2 * 0.1 + sd) * cos(d2 * 0.076 + oThePos.y * 0.1);
				if (s * s > 0.1) {
					discard;
				}
			}
		}
		if (oThePos.z < -900.0) {
			if (oThePos.z > -970.0) {
				if (oThePos.y < -70.0) {
					if (oThePos.y > -260.0) {
						if (oThePos.z < -900.0) {
							if (oThePos.z > -914.0) {
								discard;
							}
						}
						if (oThePos.z < -957.8) {
							discard;
						}
					}
				}
			}
		}
		if (oThePos.y > -70.0) {
			satt = 1.0 - clamp(0.03 * length(oThePos.z - fireplace), 0.0, 1.0);
			satt *= fireplacealpha;
			red = 40.0 * satt;
			red += (satt) * 140000.0 * pow(texture(decal, oThePos.zy * 0.08).y, 12.0);
		}
	} else if (thestyle == 0) {
		sd = sin(oThePos.y * 0.11);
		s = sin(oThePos.z * 0.04 + d2 * 0.1 + sd) * cos(d2 * 0.08 + oThePos.y * 0.07);
		if (s * s > 0.15 - focus) {
			discard;
		}
	}
	mul1 = texture(decal, oThePos.xz * 0.02).z * texture(decal, oThePos.zy * 0.02).y;
	oColor1 = (TEXCOORD3.xyzz + 1.0) * 0.5;
	oColor2 = COLOR1 * 1.0;
	oColor2 += vec4(0.015, 0.023, 0.04, 1.0) * 0.8;
	oColor2 *= 1.2 + 1.0 * vec4(0.5, 0.5, 0.5, 1.0) * 7000.0
	         * clamp(pow(mul1, 6.0) - 0.001, 0.0, 1.0);
	oColor2 *= 1.0 + vec4(1.0, 0.1, 0.02, 1.0) * red;
	oColor2.w = TEXCOORD7.x * 8.0;
}
