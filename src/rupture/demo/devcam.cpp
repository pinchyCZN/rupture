/*
 * The developer camera rig (subsystems/devcam.txt). Window_Proc and
 * Demo_Frame are split here by rule 20: the window half of Window_Proc
 * is the host's and the rest of Demo_Frame is demo.cpp's, so each
 * function below is the developer lines of one original function.
 */
#include "devcam.h"

#include <stdio.h>

/*
 * OURS. The CRT's _gcvt(v, 6, buf), as "%.6g". Divergence: _gcvt's own
 * spelling of whole numbers and exponents is not reproduced.
 */
static std::string devcam_gcvt(double v)
{
	char buf[32];

	sprintf(buf, "%.6g", v);
	return std::string(buf);
}

/* ORIGIN 0x00447390 Demo_Init, its stores at 0x00447a70..0x00447aa9; the rest are zeroed globals. */
void DevCam_Init(Rupture_DevCam *dc)
{
	dc->buttonL = 0;
	dc->buttonR = 0;
	dc->wheel = 0.0;
	dc->cursorX = 0.0;
	dc->cursorY = 0.0;
	dc->pnt[0] = 0.0;
	dc->pnt[1] = 0.0;
	dc->pnt[2] = 0.0;
	dc->arrowX = 0.0;
	dc->arrowY = 0.0;
	dc->seqButtonR = 0;
	dc->points.clear();
	dc->text.clear();
}

/*
 * ORIGIN 0x00442810 Window_Proc, its developer cases. The caller hands
 * it every message Window_Proc would pass on to DefWindowProc: the key,
 * size, activate and close messages return before the arrow step, and
 * so does the wheel, here.
 */
void DevCam_Message(Rupture_DevCam *dc, const Rupture_Seq *seq, const Rupture_Ctx *ctx,
                    const u8 *keys, double frameDt, unsigned msg, unsigned wParam)
{
	std::string s;
	Vec4        p;

	switch (msg) {
	case DEVCAM_LBUTTONDOWN:
		dc->buttonL = 1;
		break;
	case DEVCAM_LBUTTONUP:
		dc->buttonL = 0;
		break;
	case DEVCAM_RBUTTONDOWN:
		dc->buttonR = 1;
		break;
	case DEVCAM_RBUTTONUP:
		dc->buttonR = 0;
		break;
	case DEVCAM_MBUTTONDOWN:
		p.kind = 1;
		p.x = (float)dc->pnt[0];
		p.y = (float)dc->pnt[1];
		p.z = (float)dc->pnt[2];
		dc->points.push_back(p);
		s = "Cam :";
		s += devcam_gcvt(seq->head[0]);
		s += ",";
		s += devcam_gcvt(seq->head[1]);
		s += ",";
		s += devcam_gcvt(seq->head[2]);
		s += "  \n";
		dc->text += s;
		s = "";
		s = "Pnt :";
		s += devcam_gcvt(dc->pnt[0]);
		s += ",";
		s += devcam_gcvt(dc->pnt[1]);
		s += ",";
		s += devcam_gcvt(dc->pnt[2]);
		s += "  \n \n";
		dc->text += s;
		s = "";
		s = "MCr :";
		s += devcam_gcvt((double)ctx->eye.eye.x);
		s += ",";
		s += devcam_gcvt((double)ctx->eye.eye.y);
		s += ",";
		s += devcam_gcvt((double)ctx->eye.eye.z);
		s += "  \n \n";
		dc->text += s;
		break;
	case DEVCAM_MOUSEWHEEL:
		dc->wheel = (double)(int)(short)(wParam >> 16) / 600.0 + dc->wheel;   /* 0x004f3e00 */
		return;
	}
	if (keys[0x26] == 1) {
		dc->arrowY = frameDt * 1.5 + dc->arrowY;                              /* 0x004f3758 */
	}
	if (keys[0x28] == 1) {
		dc->arrowY = dc->arrowY - 1.5 * frameDt;
	}
	if (keys[0x25] == 1) {
		dc->arrowX = dc->arrowX - frameDt;
	}
	if (keys[0x27] == 1) {
		dc->arrowX = frameDt + dc->arrowX;
	}
}

/* ORIGIN 0x00447b40 Demo_Frame, 0x00447d65..0x00447da1: before the clock advances. */
void DevCam_FrameStart(Rupture_DevCam *dc, Rupture_Seq *seq)
{
	dc->seqButtonR = dc->buttonR;
	seq->head[0] = 0.0;
	seq->head[1] = 0.0;
	seq->head[2] = 0.0;
	seq->override28 = 0;
}

/*
 * ORIGIN 0x00447b40 Demo_Frame, 0x00447ee7..0x00447f14: after the clock
 * has advanced by the unscaled delta, the delta the parts are handed.
 */
void DevCam_FrameScale(const Rupture_Seq *seq, double *frameDt)
{
	*frameDt = *frameDt * (seq->head[2] + 1.0);   /* 0x004f36b8 */
	if (seq->head[2] < 0.0) {
		*frameDt = 0.002;                         /* 0x004f40b0 */
	}
}

/* ORIGIN 0x00447b40 Demo_Frame, from 0x00448846 at its normal exit: the cursor its entry read. */
void DevCam_FrameEnd(Rupture_DevCam *dc, double cursorX, double cursorY)
{
	dc->cursorX = cursorX;
	dc->cursorY = cursorY;
}
