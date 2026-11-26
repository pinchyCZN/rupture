/*
 * cgeffects/fragment_boat_test_motion.cpp, transcribed: the fragment
 * stage of the vertex_boat_test_motion pair (effects/motor.txt 4).
 * One output, COLOR0, which is the vertex stage's HCOL - the screen
 * motion as (dP + 1) * 0.5. No statement is dropped. Left out,
 * declared in the Cg and never read: alpha, alpha2, planecut and the
 * samplers decal, light, cube, texture, oned and texture2.
 */

in vec4 COLOR0;      /* HCOL */

layout(location = 0) out vec4 oColor3;

void main()
{
	oColor3 = COLOR0;
}
