/*
 * maybe_Scene_DrawLit, the set-up every lit pair gets before its
 * meshes: view, light projection, CameraPos, LightPos and samplers
 * (effects/video.txt 3); and the small pieces every part's Frame and
 * DrawScene repeat.
 */
#ifndef RUPTURE_SCENE_H
#define RUPTURE_SCENE_H

#include "../core/track.h"
#include "demo.h"

void maybe_Scene_DrawLit(Rupture_Ctx *ctx, int model, int slotA, int slotB, int prog,
                         const Camera *eye, const Camera *light, double time);

/* glPushAttrib(GL_ALL_ATTRIB_BITS) / glPopAttrib for what the parts change. */
void Attrib_Push(void);
void Attrib_Pop(void);

double Clamp01(double v);
void   Cam9(Camera *c, double ex, double ey, double ez, double tx, double ty, double tz,
            double ux, double uy, double uz);
void   maybe_Camera_LookAtEye(Rupture_Ctx *ctx);
void   Scene_LookOffset(const Rupture_Ctx *ctx);
Vec4   Vec4_Make(float x, float y, float z);
Vec4   Vec4_AddScaled(const Vec4 *a, const Vec4 *b, float s);
Vec4   Vec4_RandomDir(bool absY);
void   Scene_RoadFrame(const Track *T, int k, Vec4 *P, Vec4 *D, Vec4 *R, Vec4 *U);
void   Scene_DrawRailsCached(Rupture_Ctx *ctx, Track *T, double depth,
                             void (*draw)(Rupture_Ctx *, Track *, double));
void   Scene_Lit(Rupture_Ctx *ctx, int model, int texBase, int texNormal, int pair, double time);

#endif
