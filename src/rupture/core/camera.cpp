#include "camera.h"

/* OURS. A kind-1 Vec4 from three doubles narrowed to float. */
static Vec4 vec4_from(const double *x, const double *y, const double *z)
{
	Vec4 v;

	v.kind = 1;
	v.x = (float)*x;
	v.y = (float)*y;
	v.z = (float)*z;
	return v;
}

/*
 * ORIGIN 0x004697e0 Camera_Set. The demo's is a method of the render
 * context and writes its eye camera, ctx+0x1848; here the camera is
 * passed.
 */
void Camera_Set(Camera *c, const double *ex, const double *ey, const double *ez,
                const double *tx, const double *ty, const double *tz,
                const double *ux, const double *uy, const double *uz)
{
	c->eye = vec4_from(ex, ey, ez);
	c->target = vec4_from(tx, ty, tz);
	c->up = vec4_from(ux, uy, uz);
}

/* ORIGIN 0x00469490 Camera_Init. */
void Camera_Init(Camera *c, const double *ex, const double *ey, const double *ez,
                 const double *tx, const double *ty, const double *tz,
                 const double *ux, const double *uy, const double *uz)
{
	c->eye = vec4_from(ex, ey, ez);
	c->target = vec4_from(tx, ty, tz);
	c->up = vec4_from(ux, uy, uz);
}

/* OURS. Camera_Lerp's step: a + (b - a) * f through the demo's temporaries. */
static void lerp_vec(Vec4 *out, float f, const Vec4 *a, const Vec4 *b)
{
	Vec4 d, s, r;

	Vec4_Sub(b, a, &d);
	Vec4_Scale(&d, &f, &s);
	Vec4_Add(a, &s, &r);
	*out = r;
}

/* ORIGIN 0x00469550 Camera_Lerp. out may be a, as part_video::Frame calls it. */
void Camera_Lerp(Camera *out, const double *f, const Camera *a, const Camera *b)
{
	lerp_vec(&out->eye, (float)*f, &a->eye, &b->eye);
	lerp_vec(&out->target, (float)*f, &a->target, &b->target);
	lerp_vec(&out->up, (float)*f, &a->up, &b->up);
}
