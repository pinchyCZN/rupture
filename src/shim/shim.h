/*
 * OURS. What shim.c offers the agent: its log, the pause and mute it
 * drives from the keyboard, the fixed-step mode and the frame capture.
 */
#ifndef SHIM_H
#define SHIM_H

#define DEMO_CLOCK 0x005146c8   /* double, g_demoClock */

void shim_log(const char *fmt, ...);
void shim_pause(int on);
void shim_mute(int on);
int  shim_paused(void);
int  shim_muted(void);
int  shim_fixed(void);
int  shim_held(void);
void shim_quit_soon(void);

/* The frame SwapBuffers last showed, read back first when asked. */
void shim_capture_next(void);
int  shim_captured(void);
void shim_capture_get(const unsigned char **pixels, int *w, int *h);

#endif
