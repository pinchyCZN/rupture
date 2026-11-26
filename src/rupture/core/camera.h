/*
 * The demo's camera object: three flagged Vec4s, eye, target and up
 * (0x30 bytes). The render context holds the eye camera at ctx+0x1848
 * and the light camera at ctx+0x18a8; maybe_Scene_DrawLit makes the
 * view and the light projection from them.
 */
#ifndef RUPTURE_CAMERA_H
#define RUPTURE_CAMERA_H

#include "obj.h"

struct Camera {
	Vec4 eye;      /* +0x00 */
	Vec4 target;   /* +0x10 */
	Vec4 up;       /* +0x20 */
};

void Camera_Set(Camera *c, const double *ex, const double *ey, const double *ez,
                const double *tx, const double *ty, const double *tz,
                const double *ux, const double *uy, const double *uz);
void Camera_Init(Camera *c, const double *ex, const double *ey, const double *ez,
                 const double *tx, const double *ty, const double *tz,
                 const double *ux, const double *uy, const double *uz);
void Camera_Lerp(Camera *out, const double *f, const Camera *a, const Camera *b);

#endif
