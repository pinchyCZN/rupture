/*
 * The developer camera rig the release still carries
 * (subsystems/devcam.txt). Transcribed and NOT WIRED: nothing calls
 * these yet. The host would hand DevCam_Message the messages
 * Window_Proc would pass on to DefWindowProc, and Demo_Frame would call
 * the three DevCam_Frame steps where the original has them.
 */
#ifndef RUPTURE_DEVCAM_H
#define RUPTURE_DEVCAM_H

#include <string>
#include <vector>

#include "../core/obj.h"
#include "../core/types.h"
#include "demo.h"

/* The window messages Window_Proc's developer cases test, by their Win32 values. */
enum {
	DEVCAM_LBUTTONDOWN = 0x201,
	DEVCAM_LBUTTONUP   = 0x202,
	DEVCAM_RBUTTONDOWN = 0x204,
	DEVCAM_RBUTTONUP   = 0x205,
	DEVCAM_MBUTTONDOWN = 0x207,
	DEVCAM_MOUSEWHEEL  = 0x20a
};

/*
 * The rig's state. In the original the first five are globals and the
 * rest live in g_seq; they are gathered here because Rupture_Seq is
 * cleared with memset and nothing in the relay reads them.
 */
struct Rupture_DevCam {
	u8                buttonL;    /* g_devButtonL 0x005146e4 */
	u8                buttonR;    /* g_devButtonR 0x005146e5 */
	double            wheel;      /* g_devWheel 0x00514698 */
	double            cursorX;    /* g_devCursorX 0x00514688, last frame's */
	double            cursorY;    /* g_devCursorY 0x00514690 */
	double            arrowX;     /* g_seqArrowX, seq+0x18 */
	double            arrowY;     /* g_seqArrowY, seq+0x20 */
	int               seqButtonR; /* g_seqButtonR, seq+0x2c */
	double            pnt[3];     /* g_seqPnt, seq+0x30 */
	std::vector<Vec4> points;     /* g_seqDevPoints, seq+0x90 */
	std::string       text;       /* g_clipboardText 0x0050bbe4 */
};

void DevCam_Init(Rupture_DevCam *dc);
void DevCam_Message(Rupture_DevCam *dc, const Rupture_Seq *seq, const Rupture_Ctx *ctx,
                    const u8 *keys, double frameDt, unsigned msg, unsigned wParam);
void DevCam_FrameStart(Rupture_DevCam *dc, Rupture_Seq *seq);
void DevCam_FrameScale(const Rupture_Seq *seq, double *frameDt);
void DevCam_FrameEnd(Rupture_DevCam *dc, double cursorX, double cursorY);

#endif
