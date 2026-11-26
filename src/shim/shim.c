/*
 * OURS. A stand-in glut32.dll that patches the running demo.
 *
 * Windows loads it from the exe's folder before WinMain runs. On attach
 * it opens a console, checks every byte it is about to change against
 * the database's listing, and only if all of them match applies:
 *
 *   Timer_Seconds   replaced by a copy that can hold the clock still;
 *                   the keys are polled from it, so they work only
 *                   while the demo is drawing frames
 *   the exits       Demo_Frame's two _exit(0) calls, and the CRT's
 *                   exit after WinMain returns, go to ExitProcess,
 *                   past the static destructor that faults
 *                   (docs/subsystems/startup.txt 5)
 *   the dialog      preselects 800x600 instead of 1280x720, and ticks
 *                   Windowed (docs/subsystems/startup.txt 2)
 *
 * README.md says how to set up the run folder. agent.c is the pipe a
 * client drives it through, started with the first frame.
 *
 * Keys: Space pause, M mute. Both act on the music through FMOD's own
 * FSOUND_SetPaused and FSOUND_SetMute, which the demo never imports.
 * The demo starts muted: FSOUND_PlaySound's import slot goes to a
 * wrapper that starts each sound paused, mutes it, then lets it run.
 * SwapBuffers' import slot goes to a wrapper that can read the frame
 * back first, for the agent's DUMP.
 *
 * FIXED-STEP MODE, with RUPTURE_SHIM_FIXED=1 in the environment (as
 * tools/agentctl.py --shim launch starts it): the setup dialog runs
 * itself, the music stays paused and g_musicSyncOn is held at 0, and
 * Timer_Seconds advances by exactly one step per frame - so the demo
 * clock and every part clock move by dt per frame from 0, as the
 * port's agent mode does. The demo is held at the start of each frame,
 * inside Timer_Seconds, serving the agent and the window, until the
 * agent gives it a step; it starts held before its first frame.
 */
#include <windows.h>
#include <intrin.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/gl.h>

#include "agent.h"
#include "shim.h"

/*
 * The exe imports exactly these two names from glut32.dll [verified,
 * its import table]. Both are forwarded to the release's own DLL,
 * renamed glut32_orig.dll in the run folder, so no shim code runs on
 * either call.
 */
#pragma comment(linker, "/EXPORT:glutSolidCube=glut32_orig.glutSolidCube")
#pragma comment(linker, "/EXPORT:glutWireCube=glut32_orig.glutWireCube")

/* The demo's addresses. Fixed: the image has no relocations. */
#define DEMO_IMAGE_BASE       0x00400000
#define DEMO_TIMER_SECONDS    0x004028b0
#define DEMO_FRAME_TIMER_RA   0x00447b76   /* after Demo_Frame's first of two calls */
#define DEMO_LAST_QPC         0x00510758   /* LONGLONG, g_timerLastQpc */
#define DEMO_TIMER_COPY       0x00510768   /* double, g_timerSecondsCopy */
#define DEMO_TIMER_TOTAL      0x00510770   /* double, g_timerSeconds */
#define DEMO_CONSOLE_REDIRECT 0x00405dd0   /* Console_Redirect, no caller */
#define DEMO_EXIT_AT_END      0x0044857f   /* CALL _exit, clock past 401 */
#define DEMO_EXIT_AFTER_FADE  0x004485b0   /* CALL _exit, end of the Esc fade */
#define DEMO_EXIT_AFTER_MAIN  0x004c3fdd   /* CALL _exit, WinMain returned (WM_QUIT) */
#define DEMO_MODE_WIDTH_IMM   0x0043e054   /* CMP [mode].w, 0x500 */
#define DEMO_MODE_HEIGHT_IMM  0x0043e072   /* CMP [mode].h, 0x2d0 */
#define DEMO_IAT_CHECKDLGBTN  0x004f3330   /* CheckDlgButton; read only by Setup_DialogProc */
#define DEMO_IAT_PLAYSOUND    0x004f3420   /* FSOUND_PlaySound; read only by its thunk 0x004befc4 */
#define DEMO_IAT_SWAPBUFFERS  0x004f3000   /* SwapBuffers; WinMain loads it into EBX at 0x00448d7e */
#define DEMO_MUSIC_SYNC_ON    0x0050bbda   /* byte, g_musicSyncOn */
#define DEMO_WINDOW_ACTIVE    0x0050bbdb   /* byte, g_windowActive */
#define DLG_ID_RUN            1
#define DLG_ID_WINDOWED       0x3f4
#define DLG_ID_FORCE16X9      0x3f5

/* FMOD 3: FSOUND_ALL names every channel; F_API is __stdcall. */
#define FSOUND_ALL (-3)

typedef signed char (__stdcall *FSOUND_SetFlag)(int channel, signed char on);
typedef int (__stdcall *FSOUND_PlaySound_Fn)(int channel, void *sample);
typedef int (__stdcall *FSOUND_PlaySoundEx_Fn)(int channel, void *sample, void *dsp,
                                               signed char startPaused);
typedef void (__cdecl *Demo_Void)(void);
typedef BOOL (WINAPI *CheckDlgButton_Fn)(HWND dlg, int id, UINT check);
typedef BOOL (WINAPI *SwapBuffers_Fn)(HDC dc);

typedef struct Patch {
	DWORD               addr;
	int                 len;
	unsigned char       expect[8];
	unsigned char       write[8];      /* filled in by shim_install */
} Patch;

static FILE                 *g_log;
static int                   g_paused;
static int                   g_muted = 1;     /* the shim starts the demo quiet */
static int                   g_titleDirty = 1;
static int                   g_demoConsole;
static int                   g_keyDown[256];
static FSOUND_SetFlag        g_setPaused;
static FSOUND_SetFlag        g_setMute;
static FSOUND_PlaySound_Fn   g_playSound;
static FSOUND_PlaySoundEx_Fn g_playSoundEx;
static char                  g_title[256];
static HWND                  g_titleWnd;
static CheckDlgButton_Fn     g_checkDlgButton;
static SwapBuffers_Fn        g_swapBuffers;
static int                   g_fixed;
static int                   g_held;
static int                   g_captureNext;
static int                   g_captured;
static unsigned char        *g_pixels;
static int                   g_pixelsW;
static int                   g_pixelsH;

/* Every byte the shim changes, as the database lists it. */
static Patch g_patches[] = {
	{ DEMO_TIMER_SECONDS,   5, { 0x83, 0xec, 0x10, 0x56, 0x8d } },
	{ DEMO_EXIT_AT_END,     5, { 0xe8, 0x9a, 0xad, 0x07, 0x00 } },
	{ DEMO_EXIT_AFTER_FADE, 5, { 0xe8, 0x69, 0xad, 0x07, 0x00 } },
	{ DEMO_EXIT_AFTER_MAIN, 5, { 0xe8, 0x3c, 0xf3, 0xff, 0xff } },
	{ DEMO_MODE_WIDTH_IMM,  4, { 0x00, 0x05, 0x00, 0x00 } },
	{ DEMO_MODE_HEIGHT_IMM, 4, { 0xd0, 0x02, 0x00, 0x00 } },
	{ DEMO_IAT_CHECKDLGBTN, 4, { 0 } },   /* expect: GetProcAddress, at install */
	{ DEMO_IAT_PLAYSOUND,   4, { 0 } },   /* expect: GetProcAddress, at install */
	{ DEMO_IAT_SWAPBUFFERS, 4, { 0 } }    /* expect: GetProcAddress, at install */
};
#define PATCH_COUNT (sizeof g_patches / sizeof g_patches[0])

/* OURS. One line to the console and to shim.log. */
void shim_log(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	printf("shim: ");
	vprintf(fmt, ap);
	printf("\n");
	fflush(stdout);
	va_end(ap);
	if (g_log != NULL) {
		va_start(ap, fmt);
		vfprintf(g_log, fmt, ap);
		fputc('\n', g_log);
		fflush(g_log);
		va_end(ap);
	}
}

/*
 * OURS. A console for the shim's own CRT. The demo's CRT is not
 * started yet at attach time; it is bound later, by console_demo.
 */
static void console_open(void)
{
	if (!AllocConsole()) {
		return;
	}
	SetConsoleTitleA("rupture shim - closing this window ends the demo");
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
}

/*
 * OURS. Binds the demo's own stdin, stdout and stderr to the console by
 * calling its dead Console_Redirect. Its AllocConsole fails harmlessly,
 * the console already being there.
 */
static void console_demo(void)
{
	g_demoConsole = 1;
	((Demo_Void)DEMO_CONSOLE_REDIRECT)();
	shim_log("demo stdout bound to this console (Console_Redirect 0x00405dd0)");
}

/*
 * ORIGIN 0x004028b0 Timer_Seconds. The performance-counter delta over
 * the frequency, accumulated into g_timerSeconds and copied to
 * g_timerSecondsCopy, returned on the x87 stack as a cdecl double is.
 *
 * Divergence: while g_paused the delta is read and dropped, so the
 * demo clock and every part clock receive a zero frame time.
 */
static double timer_seconds(void)
{
	LARGE_INTEGER now;
	LARGE_INTEGER freq;
	LONGLONG      delta;
	LONGLONG     *lastQpc = (LONGLONG *)DEMO_LAST_QPC;
	double       *total = (double *)DEMO_TIMER_TOTAL;

	QueryPerformanceCounter(&now);
	QueryPerformanceFrequency(&freq);
	delta = now.QuadPart - *lastQpc;
	*lastQpc = now.QuadPart;
	if (g_paused) {
		delta = 0;
	}
	*total = (double)delta / (double)freq.QuadPart + *total;
	*(double *)DEMO_TIMER_COPY = *total;
	return *total;
}

/* OURS. FMOD is a static import of the exe, so it is mapped by now. */
static int fmod_bind(void)
{
	HMODULE fmod;

	if (g_setPaused != NULL) {
		return 1;
	}
	fmod = GetModuleHandleA("fmod.dll");
	if (fmod == NULL) {
		shim_log("fmod.dll is not loaded");
		return 0;
	}
	g_setPaused = (FSOUND_SetFlag)GetProcAddress(fmod, "_FSOUND_SetPaused@8");
	g_setMute = (FSOUND_SetFlag)GetProcAddress(fmod, "_FSOUND_SetMute@8");
	g_playSound = (FSOUND_PlaySound_Fn)GetProcAddress(fmod, "_FSOUND_PlaySound@8");
	g_playSoundEx = (FSOUND_PlaySoundEx_Fn)GetProcAddress(fmod, "_FSOUND_PlaySoundEx@16");
	if (g_setPaused == NULL || g_setMute == NULL || g_playSound == NULL
	 || g_playSoundEx == NULL) {
		shim_log("fmod.dll lacks a FSOUND entry point the shim uses");
		g_setPaused = NULL;
		return 0;
	}
	return 1;
}

/* OURS. Holds the demo clock and the music still, or lets them run; fixed-step keeps the music held. */
void shim_pause(int on)
{
	if (on == g_paused) {
		return;
	}
	g_paused = on;
	if (!g_fixed && fmod_bind()) {
		g_setPaused(FSOUND_ALL, (signed char)on);
	}
	shim_log("%s at g_demoClock %.3f", on ? "pause" : "resume", *(double *)DEMO_CLOCK);
	g_titleDirty = 1;
}

/* OURS */
void shim_mute(int on)
{
	if (on == g_muted) {
		return;
	}
	g_muted = on;
	if (fmod_bind()) {
		g_setMute(FSOUND_ALL, (signed char)on);
	}
	shim_log(on ? "mute" : "unmute");
	g_titleDirty = 1;
}

/* OURS */
int shim_paused(void)
{
	return g_paused;
}

/* OURS */
int shim_muted(void)
{
	return g_muted;
}

/* OURS */
int shim_fixed(void)
{
	return g_fixed;
}

/* OURS. True while fixed-step mode holds the demo at a frame's start. */
int shim_held(void)
{
	return g_held;
}

/* OURS. Reads back the next frame SwapBuffers shows. */
void shim_capture_next(void)
{
	g_captured = 0;
	g_captureNext = 1;
}

/* OURS */
int shim_captured(void)
{
	return g_captured;
}

/* OURS */
void shim_capture_get(const unsigned char **pixels, int *w, int *h)
{
	*pixels = g_pixels;
	*w = g_pixelsW;
	*h = g_pixelsH;
}

/* OURS. The foreground window, if it belongs to this process. */
static HWND demo_window(void)
{
	HWND  wnd = GetForegroundWindow();
	DWORD pid = 0;

	if (wnd == NULL) {
		return NULL;
	}
	GetWindowThreadProcessId(wnd, &pid);
	return pid == GetCurrentProcessId() ? wnd : NULL;
}

/* OURS. Tags the window caption with the shim's state. */
static void title_update(HWND wnd)
{
	char text[320];

	if (g_titleWnd != wnd) {
		g_titleWnd = wnd;
		GetWindowTextA(wnd, g_title, sizeof g_title);
	}
	strcpy(text, g_title);
	if (g_fixed) {
		strcat(text, "  [fixed step]");
	}
	if (g_paused) {
		strcat(text, "  [paused]");
	}
	if (g_muted) {
		strcat(text, "  [muted]");
	}
	SetWindowTextA(wnd, text);
}

/* OURS. True once per press of vk. */
static int key_pressed(int vk)
{
	int down = (GetAsyncKeyState(vk) & 0x8000) != 0;
	int pressed = down && !g_keyDown[vk];

	g_keyDown[vk] = down;
	return pressed;
}

/* OURS */
static void keys_poll(void)
{
	HWND wnd = demo_window();

	if (wnd != NULL) {
		if (key_pressed(VK_SPACE)) {
			shim_pause(!g_paused);
		}
		if (key_pressed('M')) {
			shim_mute(!g_muted);
		}
	}
	if (g_titleDirty && (wnd != NULL || g_titleWnd != NULL)) {
		g_titleDirty = 0;
		title_update(wnd != NULL ? wnd : g_titleWnd);
	}
}

static void __cdecl shim_exit(int code);

/* OURS. The window's messages while the demo is held; a WM_QUIT ends it here. */
static void messages_pump(void)
{
	MSG msg;

	while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			shim_exit(0);
		}
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
}

/*
 * OURS. Fixed-step mode at the start of a frame: hold here, serving
 * the agent, the keys and the window, until the agent gives a step;
 * then advance g_timerSeconds by it. g_windowActive is held at 1 so
 * WinMain keeps calling Demo_Frame with the window in the background.
 */
static void fixed_frame_start(void)
{
	double step;

	*(BYTE *)DEMO_MUSIC_SYNC_ON = 0;
	g_held = 1;
	for (;;) {
		*(BYTE *)DEMO_WINDOW_ACTIVE = 1;
		agent_service();
		keys_poll();
		step = agent_fixed_advance();
		if (step > 0.0) {
			break;
		}
		messages_pump();
		Sleep(1);
	}
	g_held = 0;
	*(double *)DEMO_TIMER_TOTAL += step;
	agent_frame();
	shim_capture_next();
}

/*
 * OURS. What the patched entry of Timer_Seconds jumps to. Its first
 * call comes from Demo_Init, after the exe's CRT is up. Demo_Frame
 * calls it twice; the agent and the keys run on the first, once a
 * frame. In fixed-step mode only that first call moves the clock, so
 * Demo_Frame's delta between the two frames' second calls is the step.
 */
static double __cdecl timer_hook(void)
{
	double *total = (double *)DEMO_TIMER_TOTAL;

	if (!g_demoConsole) {
		console_demo();
		agent_start();
	}
	if ((DWORD)_ReturnAddress() == DEMO_FRAME_TIMER_RA) {
		if (g_fixed) {
			fixed_frame_start();
		} else {
			agent_frame();
			agent_service();
			keys_poll();
		}
	}
	if (g_fixed) {
		*(double *)DEMO_TIMER_COPY = *total;
		return *total;
	}
	return timer_seconds();
}

/*
 * OURS. Takes the place of _exit (0x004c331e) at Demo_Frame's two
 * exits and at ___tmainCRTStartup's call after WinMain returns, the
 * route a WM_CLOSE takes. FMOD is closed or about to be torn down and
 * the window is gone by then; ending the process
 * here skips the atexit list, whose render-context destructor
 * (0x004424e0) frees a pointer that is not a heap block.
 */
static void __cdecl shim_exit(int code)
{
	shim_log("exit %d - skipping the static destructors", code);
	if (g_log != NULL) {
		fclose(g_log);
		g_log = NULL;
	}
	ExitProcess((UINT)code);
}

/*
 * OURS. Stands in for CheckDlgButton in the exe's import table. The
 * setup dialog's WM_INITDIALOG ticks Antialiased then Force 16:9 and
 * leaves Windowed clear; this ticks Windowed with Force 16:9. The Run
 * button reads the box, so the player can still untick it.
 */
static BOOL WINAPI check_dlg_button(HWND dlg, int id, UINT check)
{
	BOOL ok = g_checkDlgButton(dlg, id, check);

	if (id == DLG_ID_FORCE16X9) {
		g_checkDlgButton(dlg, DLG_ID_WINDOWED, BST_CHECKED);
		if (g_fixed) {
			PostMessageA(dlg, WM_COMMAND, DLG_ID_RUN, 0);
		}
	}
	return ok;
}

/*
 * OURS. Stands in for SwapBuffers in the exe's import table: when a
 * capture is asked for, reads the back buffer of the window's client
 * area first, as RGBA rows bottom up.
 */
static BOOL WINAPI swap_buffers(HDC dc)
{
	RECT r;
	int  w;
	int  h;

	if (g_captureNext && GetClientRect(WindowFromDC(dc), &r)) {
		w = r.right - r.left;
		h = r.bottom - r.top;
		if (w > 0 && h > 0 && (w != g_pixelsW || h != g_pixelsH || g_pixels == NULL)) {
			free(g_pixels);
			g_pixels = (unsigned char *)malloc((size_t)w * h * 4);
			g_pixelsW = w;
			g_pixelsH = h;
		}
		if (g_pixels != NULL) {
			glReadBuffer(GL_BACK);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(0, 0, g_pixelsW, g_pixelsH, GL_RGBA, GL_UNSIGNED_BYTE, g_pixels);
			g_captured = 1;
		}
		g_captureNext = 0;
	}
	return g_swapBuffers(dc);
}

#pragma warning(push)
#pragma warning(disable: 4702)

/* OURS. The quitting thread: lets the QUIT reply reach the client first. */
static DWORD WINAPI quit_thread(void *param)
{
	(void)param;
	Sleep(300);
	shim_exit(0);
	return 0;
}
#pragma warning(pop)

/* OURS. QUIT: ends the process shortly, past the static destructors. */
void shim_quit_soon(void)
{
	HANDLE thread = CreateThread(NULL, 0, quit_thread, NULL, 0, NULL);

	if (thread != NULL) {
		CloseHandle(thread);
	}
}

/*
 * OURS. Stands in for FSOUND_PlaySound in the exe's import table. While
 * muted, the sound starts paused, is muted, then is released, so no
 * sample of it is heard; the pause state the shim holds is kept.
 */
static int __stdcall play_sound(int channel, void *sample)
{
	int ch;

	if (!g_muted && !g_fixed) {
		return g_playSound(channel, sample);
	}
	ch = g_playSoundEx(channel, sample, NULL, 1);
	if (ch >= 0) {
		g_setMute(ch, (signed char)g_muted);
		g_setPaused(ch, (signed char)(g_paused || g_fixed));
	}
	shim_log("FSOUND_PlaySound(%d) started muted on channel %d", channel, ch);
	return ch;
}

/* OURS. A 5-byte E8 / E9 rel32 from addr to target. */
static void rel32_encode(unsigned char *out, unsigned char op, DWORD addr, const void *target)
{
	LONG rel = (LONG)((DWORD)target - (addr + 5));

	out[0] = op;
	memcpy(out + 1, &rel, 4);
}

/* OURS. A little-endian dword. */
static void imm32_encode(unsigned char *out, DWORD value)
{
	memcpy(out, &value, 4);
}

/* OURS */
static int patch_write(const Patch *p)
{
	DWORD old;

	if (!VirtualProtect((void *)p->addr, p->len, PAGE_EXECUTE_READWRITE, &old)) {
		return 0;
	}
	memcpy((void *)p->addr, p->write, p->len);
	VirtualProtect((void *)p->addr, p->len, old, &old);
	FlushInstructionCache(GetCurrentProcess(), (void *)p->addr, p->len);
	return 1;
}

/*
 * OURS. Refuses to patch anything but the image the addresses came
 * from: the exe at its base, every expected byte in place. All the
 * patches go in, or none.
 */
static void shim_install(void)
{
	char                path[MAX_PATH];
	char               *slash;
	unsigned int        i;
	CheckDlgButton_Fn   iatValue = check_dlg_button;
	FSOUND_PlaySound_Fn playValue = play_sound;
	SwapBuffers_Fn      swapValue = swap_buffers;
	char                env[8];

	GetModuleFileNameA(NULL, path, sizeof path);
	slash = strrchr(path, '\\');
	strcpy(slash != NULL ? slash + 1 : path, "shim.log");
	g_log = fopen(path, "w");

	if ((DWORD)GetModuleHandleA(NULL) != DEMO_IMAGE_BASE) {
		shim_log("exe is not at 0x00400000 - not patching");
		return;
	}
	g_checkDlgButton = (CheckDlgButton_Fn)GetProcAddress(GetModuleHandleA("user32.dll"),
	                                                     "CheckDlgButton");
	if (g_checkDlgButton == NULL
	 || *(CheckDlgButton_Fn *)DEMO_IAT_CHECKDLGBTN != g_checkDlgButton) {
		shim_log("import slot 0x%08lx is not CheckDlgButton - not patching",
		         (DWORD)DEMO_IAT_CHECKDLGBTN);
		return;
	}
	memcpy(g_patches[6].expect, &g_checkDlgButton, 4);
	if (!fmod_bind()) {
		shim_log("not patching");
		return;
	}
	memcpy(g_patches[7].expect, &g_playSound, 4);
	g_swapBuffers = (SwapBuffers_Fn)GetProcAddress(GetModuleHandleA("gdi32.dll"), "SwapBuffers");
	if (g_swapBuffers == NULL) {
		shim_log("gdi32.dll has no SwapBuffers - not patching");
		return;
	}
	memcpy(g_patches[8].expect, &g_swapBuffers, 4);
	for (i = 0; i < PATCH_COUNT; i++) {
		if (memcmp((void *)g_patches[i].addr, g_patches[i].expect, g_patches[i].len) != 0) {
			shim_log("bytes at 0x%08lx differ - not ASD-Rapture.exe, not patching",
			         g_patches[i].addr);
			return;
		}
	}
	rel32_encode(g_patches[0].write, 0xe9, DEMO_TIMER_SECONDS, (const void *)timer_hook);
	rel32_encode(g_patches[1].write, 0xe8, DEMO_EXIT_AT_END, (const void *)shim_exit);
	rel32_encode(g_patches[2].write, 0xe8, DEMO_EXIT_AFTER_FADE, (const void *)shim_exit);
	rel32_encode(g_patches[3].write, 0xe8, DEMO_EXIT_AFTER_MAIN, (const void *)shim_exit);
	imm32_encode(g_patches[4].write, 800);
	imm32_encode(g_patches[5].write, 600);
	memcpy(g_patches[6].write, &iatValue, 4);
	memcpy(g_patches[7].write, &playValue, 4);
	memcpy(g_patches[8].write, &swapValue, 4);
	for (i = 0; i < PATCH_COUNT; i++) {
		if (!patch_write(&g_patches[i])) {
			shim_log("VirtualProtect failed at 0x%08lx", g_patches[i].addr);
			return;
		}
	}
	shim_log("patched: Timer_Seconds, three exits, dialog default 800x600 windowed");
	shim_log("audio starts muted; keys: Space pause, M mute");
	if (GetEnvironmentVariableA("RUPTURE_SHIM_FIXED", env, sizeof env) > 0 && env[0] == '1') {
		g_fixed = 1;
		g_paused = 1;
		shim_log("fixed-step mode: held before the first frame, music held");
	}
}

/* OURS */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
	(void)reserved;
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(inst);
		console_open();
		shim_install();
	} else if (reason == DLL_PROCESS_DETACH && g_log != NULL) {
		fclose(g_log);
		g_log = NULL;
	}
	return TRUE;
}
