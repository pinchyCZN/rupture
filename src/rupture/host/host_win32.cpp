/*
 * OURS. The Windows host: a fixed-size window, an OpenGL 3.3 core
 * context, the message loop and a QueryPerformanceCounter clock. With
 * --agent (rupture's Debug build, RUPTURE_AGENT) the clock is the
 * agent's fixed step instead, and the agent pipe drives the loop
 * (host/agent_port.h). --size=WxH sets the client area, 1280x720 by
 * default, to match the shimmed original's 794x421 when the two are
 * compared. --nobatch turns gl1's hot-spot batching off, to compare.
 * --part=NAME[@T] starts at a part, or at time T in it (App_SetJump).
 * --perflog=FILE logs each frame's times (perf_open). --fullscreen is
 * borderless fullscreen on the primary monitor at its own resolution
 * (present). With no arguments at all the setup dialog comes first, as
 * the original's does, and its choices open on the monitor it was left
 * on; any argument skips it, and gfx_test, whose exe has no dialog
 * resource, never shows it. When the demo ends, as the original's at
 * clock 401, the window closes; under the agent it keeps drawing.
 */
#include <windows.h>
#include <stdio.h>
#include <windowsx.h>
#include <math.h>
#include <string>
#include <vector>

#include "../core/post.h"

#include "../core/gl1.h"
#include "../core/glapi.h"
#include "host.h"
#ifdef RUPTURE_AGENT
#include "agent_port.h"
#endif

#define WGL_CONTEXT_MAJOR_VERSION_ARB    0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB    0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB     0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x0001

typedef HGLRC (WINAPI *PFN_wglCreateContextAttribsARB)(HDC dc, HGLRC share, const int *attribs);

static HWND g_hWnd;
static HDC  g_hDC;
static bool g_quit;
static bool g_dragging;
static int  g_mouseX, g_mouseY;
static int  g_width = HOST_WIDTH, g_height = HOST_HEIGHT;   /* the view: what the demo draws */

struct DisplayMode {
	int w, h;
};

static std::vector<DisplayMode> g_displayModes;   /* g_displayModes_begin 0x00b1e4fc */
static bool        g_fullscreen;       /* g_fullscreen 0x00514663: here borderless, at the monitor's resolution */
static bool        g_force16x9;        /* g_force16x9 0x0050bbd8 */
static bool        g_fromDialog;       /* the dialog chose: a window opens on its monitor */
static HMONITOR    g_monitor;          /* the dialog's monitor, or the primary one */
static RECT        g_monitorRect, g_monitorWork;
static char        g_monitorDevice[CCHDEVICENAME];
static DisplayMode g_native;           /* the monitor's own resolution */
static int         g_outWidth, g_outHeight;   /* the window's client area */
static GLuint      g_screenFbo;        /* fullscreen: the view, copied to the window by present() */

enum { PERF_LAG = 4 };

struct PerfFrame {
	GLuint      query;
	const char *part;
	double      t, dt, cpu, swap;
};

static FILE     *g_perf;               /* --perflog's file */
static PerfFrame g_perfRing[PERF_LAG];
static unsigned  g_perfFrames;

/* OURS */
static void fail(const std::string &what)
{
	MessageBoxA(NULL, what.c_str(), "rupture", MB_ICONERROR);
	ExitProcess(1);
}

/* OURS. A virtual key as the character App_Key takes, or 0. */
static int key_char(WPARAM vk)
{
	if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9') || vk == ' ') {
		return (int)vk;
	}
	switch (vk) {
	case VK_OEM_4:  return '[';
	case VK_OEM_6:  return ']';
	case VK_ESCAPE: return KEY_ESCAPE;
	case VK_HOME:   return KEY_HOME;
	case VK_OEM_PLUS:
	case VK_ADD:    return '+';
	case VK_OEM_MINUS:
	case VK_SUBTRACT: return '-';
	case VK_OEM_PERIOD:
	case VK_DECIMAL:  return '.';
	}
	return 0;
}

/* OURS */
static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int k;

	switch (msg) {
	case WM_CLOSE:
		g_quit = true;
		return 0;
	case WM_KEYDOWN:
		k = key_char(wParam);
		if (k == KEY_ESCAPE) {
			g_quit = true;
		} else if (k != 0) {
			App_Key(k);
		}
		return 0;
	case WM_LBUTTONDOWN:
		g_dragging = true;
		g_mouseX = GET_X_LPARAM(lParam);
		g_mouseY = GET_Y_LPARAM(lParam);
		SetCapture(hWnd);
		return 0;
	case WM_LBUTTONUP:
		g_dragging = false;
		ReleaseCapture();
		App_DragEnd();
		return 0;
	case WM_MOUSEMOVE:
		if (g_dragging) {
			App_Drag(GET_X_LPARAM(lParam) - g_mouseX, GET_Y_LPARAM(lParam) - g_mouseY);
			g_mouseX = GET_X_LPARAM(lParam);
			g_mouseY = GET_Y_LPARAM(lParam);
		}
		return 0;
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		App_ViewReset();
		return 0;
	case WM_MOUSEWHEEL:
		App_Wheel(GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1 : -1, (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) != 0);
		return 0;
	}
	return DefWindowProcA(hWnd, msg, wParam, lParam);
}

/*
 * ORIGIN 0x00442630 Setup_EnumDisplayModes: every display mode once,
 * landscape, under 2048 each way, neither side over 1.8 (k_dbl_1_8
 * 0x004f3df8) times the other, in the driver's order. The last call's
 * failure leaves the previous mode, which is then a repeat.
 * Divergence: the modes are the named display's, the dialog's monitor,
 * where the original asks the primary one; and dmSize is set, where
 * the original passes a zeroed DEVMODE.
 */
static void Setup_EnumDisplayModes(const char *device)
{
	DEVMODEA    dm;
	DisplayMode m;
	DWORD       i = 0;
	BOOL        more;
	size_t      k;
	bool        seen;
	double      w, h;

	memset(&dm, 0, sizeof dm);
	dm.dmSize = sizeof dm;
	g_displayModes.clear();
	do {
		more = EnumDisplaySettingsA(device, i, &dm);
		m.w = (int)dm.dmPelsWidth;
		m.h = (int)dm.dmPelsHeight;
		seen = false;
		for (k = 0; k < g_displayModes.size(); k++) {
			if (g_displayModes[k].w == m.w && g_displayModes[k].h == m.h) {
				seen = true;
			}
		}
		w = (double)m.w;
		h = (double)m.h;
		if (!seen && m.h > 0 && h <= w && m.h < 2048 && m.w < 2048 && h / w <= 1.8 && w / h <= 1.8) {
			g_displayModes.push_back(m);
		}
		i++;
	} while (more);
}

/*
 * OURS. Sizes in real pixels on every monitor, so a fullscreen window
 * is the monitor's resolution exactly. Per-monitor where Windows has it
 * (10, 1703 on: -4 is DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2), else
 * system-wide.
 */
static void dpi_aware(void)
{
	typedef BOOL (WINAPI *Context_Fn)(HANDLE context);
	Context_Fn set = (Context_Fn)GetProcAddress(GetModuleHandleA("user32.dll"),
	                                            "SetProcessDpiAwarenessContext");

	if (set == NULL || !set((HANDLE)-4)) {
		SetProcessDPIAware();
	}
}

/* OURS. The monitor a window is mostly on, or the primary one: its rectangles and its own resolution. */
static void monitor_of(HWND hWnd)
{
	MONITORINFOEXA mi;
	DEVMODEA       dm;
	POINT          origin = { 0, 0 };

	g_monitor = hWnd != NULL ? MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST)
	                         : MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
	mi.cbSize = sizeof mi;
	GetMonitorInfoA(g_monitor, (MONITORINFO *)&mi);
	g_monitorRect = mi.rcMonitor;
	g_monitorWork = mi.rcWork;
	strcpy(g_monitorDevice, mi.szDevice);
	memset(&dm, 0, sizeof dm);
	dm.dmSize = sizeof dm;
	if (EnumDisplaySettingsA(g_monitorDevice, ENUM_CURRENT_SETTINGS, &dm)) {
		g_native.w = (int)dm.dmPelsWidth;
		g_native.h = (int)dm.dmPelsHeight;
	} else {
		g_native.w = mi.rcMonitor.right - mi.rcMonitor.left;
		g_native.h = mi.rcMonitor.bottom - mi.rcMonitor.top;
	}
}

/*
 * OURS. The dialog's list for its monitor. Windowed, the modes, with the
 * one the original preselects - 1280x720, else 1024x768 - or the one
 * kept from before. Fullscreen, one entry, the monitor's own
 * resolution, and the list disabled.
 */
static void setup_fill(HWND dlg, int keepW, int keepH)
{
	HWND   combo = GetDlgItem(dlg, 1014);
	char   label[64];
	size_t i, pick = 0;
	bool   kept = false;
	double aspect;

	SendMessageA(combo, CB_RESETCONTENT, 0, 0);
	if (IsDlgButtonChecked(dlg, 1012) == 0) {
		sprintf(label, " %d x %d  (this monitor)", g_native.w, g_native.h);
		SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)label);
		SendMessageA(combo, CB_SETCURSEL, 0, 0);
		EnableWindow(combo, FALSE);
		return;
	}
	EnableWindow(combo, TRUE);
	for (i = 0; i < g_displayModes.size(); i++) {
		aspect = (double)g_displayModes[i].w / (double)g_displayModes[i].h;
		if (fabs(aspect - 1.6) < 0.01) {
			sprintf(label, " %d x %d  (16:10)", g_displayModes[i].w, g_displayModes[i].h);
		} else if (fabs(aspect - 1.7777777777777777) < 0.01) {
			sprintf(label, " %d x %d  (16:9)", g_displayModes[i].w, g_displayModes[i].h);
		} else {
			sprintf(label, " %d x %d       ", g_displayModes[i].w, g_displayModes[i].h);
		}
		if (g_displayModes[i].w == keepW && g_displayModes[i].h == keepH) {
			pick = i;
			kept = true;
		} else if (!kept && g_displayModes[i].w == 1280 && g_displayModes[i].h == 720) {
			pick = i;
		}
		SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)label);
	}
	if (pick == 0 && !kept) {
		for (i = 0; i < g_displayModes.size(); i++) {
			if (g_displayModes[i].w == 1024 && g_displayModes[i].h == 768) {
				pick = i;
			}
		}
	}
	SendMessageA(combo, CB_SETCURSEL, (WPARAM)pick, 0);
}

/* OURS. The windowed mode the list shows, to keep it across a refill. */
static void setup_chosen(HWND dlg, int *w, int *h)
{
	LRESULT sel = SendDlgItemMessageA(dlg, 1014, CB_GETCURSEL, 0, 0);

	*w = 0;
	*h = 0;
	if (IsWindowEnabled(GetDlgItem(dlg, 1014)) && sel >= 0 && (size_t)sel < g_displayModes.size()) {
		*w = g_displayModes[sel].w;
		*h = g_displayModes[sel].h;
	}
}

/* OURS. F1's list of the keys and the mouse, as the browser page shows them. */
static void help_show(HWND owner)
{
	static const char text[] =
	    "Space\tpause and resume\n"
	    ".\tstep one frame (held: keeps stepping)\n"
	    "F\tfour times speed, on and off\n"
	    "]\tskip to the next part, and reset the look\n"
	    "M\tmute and unmute\n"
	    "+ -\tvolume up and down\n"
	    "Esc\tquit\n"
	    "\n"
	    "Drag\tlook around; let go and it eases back\n"
	    "\t(paused, the look stays)\n"
	    "C\thold the look while running too, on and off\n"
	    "Wheel\tzoom in and out; it stays (Ctrl: finer)\n"
	    "Middle or right click\n"
	    "\tback to the demo's own view and lens\n"
	    "\n"
	    "Options: --size=WxH  --fullscreen  --part=NAME[@T]\n"
	    "         --perflog=FILE  --nobatch";

	MessageBoxA(owner, text, "RUPTURE - keys", MB_OK | MB_ICONINFORMATION);
}

/*
 * ORIGIN 0x0043dca0 Setup_DialogProc. Opening: Force 16:9 ticked,
 * Windowed not, the modes listed and labelled by aspect within 0.01
 * (k_dbl_0_01 0x004f3dd0) of 1.6 (k_dbl_1_6 0x004f3dc8) or 16/9
 * (k_dbl_16_9 0x004f3dd8), 1280x720 chosen, else 1024x768. Run: the
 * view is the chosen mode, or with Force 16:9 the width over 16/9, as
 * a float, truncated; windowed, the window follows the view. Quit
 * exits. Divergences, ours: Antialiased, which changed nothing
 * (effects/look.txt), is replaced by Muted, ticked as the port starts
 * muted; fullscreen is the dialog's monitor at its own resolution, the
 * list showing only that and disabled, and a 16:9 view too tall for it
 * is narrowed to fit; the list follows the monitor the dialog is moved
 * to; F1 lists the keys. Left out: a
 * radio group, 1002 to 1004, the template does not have, and a
 * one-time flag around the list.
 */
static INT_PTR CALLBACK Setup_DialogProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HMONITOR was;
	int      w, h;

	(void)lParam;
	switch (msg) {
	case WM_INITDIALOG:
		CheckDlgButton(dlg, 1013, BST_CHECKED);
		CheckDlgButton(dlg, 1015, App_Muted() ? BST_CHECKED : BST_UNCHECKED);
		monitor_of(dlg);
		Setup_EnumDisplayModes(g_monitorDevice);
		setup_fill(dlg, 0, 0);
		return TRUE;
	case WM_HELP:
		help_show(dlg);
		return TRUE;
	case WM_EXITSIZEMOVE:
		was = g_monitor;
		monitor_of(dlg);
		if (g_monitor != was) {
			setup_chosen(dlg, &w, &h);
			Setup_EnumDisplayModes(g_monitorDevice);
			setup_fill(dlg, w, h);
		}
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == 1012 && HIWORD(wParam) == BN_CLICKED) {
			setup_fill(dlg, 0, 0);
			return TRUE;
		}
		if (LOWORD(wParam) == IDOK) {
			g_fullscreen = IsDlgButtonChecked(dlg, 1012) == 0;
			g_force16x9 = IsDlgButtonChecked(dlg, 1013) != 0;
			App_SetMuted(IsDlgButtonChecked(dlg, 1015) != 0);
			monitor_of(dlg);
			if (g_fullscreen) {
				g_width = g_native.w;
				g_height = g_native.h;
			} else {
				setup_chosen(dlg, &w, &h);
				if (w > 0) {
					g_width = w;
					g_height = h;
				}
			}
			if (g_force16x9) {
				g_height = (int)(float)((double)g_width / 1.7777777777777777);
				if (g_fullscreen && g_height > g_native.h) {
					g_height = g_native.h;
					g_width = (int)(float)((double)g_height * 1.7777777777777777);
				}
			}
			g_fromDialog = true;
			EndDialog(dlg, IDOK);
			return TRUE;
		}
		if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(dlg, IDCANCEL);
			ExitProcess(0);
		}
		return FALSE;
	}
	return FALSE;
}

/*
 * OURS. Borderless fullscreen: the view is drawn into an FBO of its own
 * size (Fbo_SetScreen), and present() copies it, unscaled, to the
 * middle of the monitor.
 */
static void screen_create(void)
{
	GLuint rb[2];

	glGenFramebuffers(1, &g_screenFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, g_screenFbo);
	glGenRenderbuffers(2, rb);
	glBindRenderbuffer(GL_RENDERBUFFER, rb[0]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, g_width, g_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb[0]);
	glBindRenderbuffer(GL_RENDERBUFFER, rb[1]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, g_width, g_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb[1]);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fail("the fullscreen framebuffer is incomplete");
	}
	Fbo_SetScreen(g_screenFbo);
}

/*
 * OURS. The frame to the display, then swapped. Fullscreen, the view
 * is copied pixel for pixel to the middle of the monitor, black around
 * it when it is smaller. Divergence: the original changes the display
 * mode to the chosen one instead.
 */
static void present(void)
{
	GLfloat clear[4];
	int     x, y;

	if (g_screenFbo != 0) {
		x = (g_outWidth - g_width) / 2;
		y = (g_outHeight - g_height) / 2;
		glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, g_screenFbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glViewport(0, 0, g_outWidth, g_outHeight);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glBlitFramebuffer(0, 0, g_width, g_height, x, y, x + g_width, y + g_height, GL_COLOR_BUFFER_BIT,
		                  GL_NEAREST);
		glClearColor(clear[0], clear[1], clear[2], clear[3]);
		glBindFramebuffer(GL_FRAMEBUFFER, g_screenFbo);
		glViewport(0, 0, g_width, g_height);
	}
	SwapBuffers(g_hDC);
}

/* OURS. The window and a 3.3 core context, made through a throwaway legacy one. */
static void window_create(HINSTANCE hInst)
{
	static const int attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	WNDCLASSA                      wc;
	RECT                           r;
	PIXELFORMATDESCRIPTOR          pfd;
	DWORD                          style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	HGLRC                          legacy, core = NULL;
	PFN_wglCreateContextAttribsARB create;
	int                            pf, x, y;

	memset(&wc, 0, sizeof wc);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = wnd_proc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = "rupture";
	RegisterClassA(&wc);
	if (g_fullscreen) {
		r = g_monitorRect;
		g_outWidth = r.right - r.left;
		g_outHeight = r.bottom - r.top;
		g_hWnd = CreateWindowA("rupture", "rupture", WS_POPUP, r.left, r.top, g_outWidth, g_outHeight,
		                       NULL, NULL, hInst, NULL);
		ShowCursor(FALSE);
	} else {
		r.left = 0;
		r.top = 0;
		r.right = g_width;
		r.bottom = g_height;
		AdjustWindowRect(&r, style, FALSE);
		g_outWidth = g_width;
		g_outHeight = g_height;
		x = y = CW_USEDEFAULT;
		if (g_fromDialog) {
			x = (g_monitorWork.left + g_monitorWork.right - (r.right - r.left)) / 2;
			y = (g_monitorWork.top + g_monitorWork.bottom - (r.bottom - r.top)) / 2;
			if (y < g_monitorWork.top) {
				y = g_monitorWork.top;
			}
		}
		g_hWnd = CreateWindowA("rupture", "rupture", style, x, y, r.right - r.left, r.bottom - r.top, NULL,
		                       NULL, hInst, NULL);
	}
	if (g_hWnd == NULL) {
		fail("CreateWindow failed");
	}
	g_hDC = GetDC(g_hWnd);
	memset(&pfd, 0, sizeof pfd);
	pfd.nSize = sizeof pfd;
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 24;
	pf = ChoosePixelFormat(g_hDC, &pfd);
	if (pf == 0 || !SetPixelFormat(g_hDC, pf, &pfd)) {
		fail("no suitable pixel format");
	}
	legacy = wglCreateContext(g_hDC);
	wglMakeCurrent(g_hDC, legacy);
	create = (PFN_wglCreateContextAttribsARB)wglGetProcAddress("wglCreateContextAttribsARB");
	if (create == NULL || (core = create(g_hDC, NULL, attribs)) == NULL) {
		fail("the driver cannot make an OpenGL 3.3 core context");
	}
	wglMakeCurrent(g_hDC, core);
	wglDeleteContext(legacy);
	if (!GlApi_Load()) {
		fail("the driver lacks an OpenGL 3.3 entry point");
	}
	if (g_fullscreen) {
		screen_create();
	}
	ShowWindow(g_hWnd, SW_SHOW);
}

/* OURS. Every pending window message. */
static void pump(void)
{
	MSG msg;

	while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
}

/* OURS. The title, when it has changed. */
static void title_update(const char *suffix)
{
	static std::string title;
	std::string        now = std::string(App_Title()) + suffix;

	if (title != now) {
		title = now;
		SetWindowTextA(g_hWnd, title.c_str());
	}
}

/*
 * OURS. --perflog=FILE: a line per frame, written PERF_LAG frames late,
 * when the frame's timer query can be read without waiting. All in ms:
 * dt from the previous frame's start, cpu in App_Frame, swap in
 * SwapBuffers, gpu from the first of the frame's commands to the last
 * done - idle gaps included, so it is GPU work only while the CPU is
 * ahead. tools/perflog.py summarises it.
 */
static void perf_open(const char *arg)
{
	char path[260];
	int  i;

	if (sscanf(arg, "%259s", path) != 1 || (g_perf = fopen(path, "w")) == NULL) {
		fail("--perflog=FILE, a file it can write");
	}
	for (i = 0; i < PERF_LAG; i++) {
		glGenQueries(1, &g_perfRing[i].query);
	}
}

/* OURS. The oldest frame's line out, and its slot's query started for this frame. */
static void perf_begin(void)
{
	PerfFrame *f = &g_perfRing[g_perfFrames % PERF_LAG];
	GLuint64   ns = 0;

	if (g_perfFrames >= PERF_LAG) {
		glGetQueryObjectui64v(f->query, GL_QUERY_RESULT, &ns);
		fprintf(g_perf, "%s %.4f dt %.2f cpu %.2f swap %.2f gpu %.2f\n", f->part, f->t, f->dt, f->cpu,
		        f->swap, (double)ns / 1e6);
	}
	glBeginQuery(GL_TIME_ELAPSED, f->query);
}

/* OURS. Milliseconds from a to b. */
static double ms(LARGE_INTEGER a, LARGE_INTEGER b, LARGE_INTEGER freq)
{
	return (double)(b.QuadPart - a.QuadPart) * 1000.0 / (double)freq.QuadPart;
}

/* OURS. This frame's times, into its slot. */
static void perf_end(double dt, double cpu, double swap)
{
	PerfFrame *f = &g_perfRing[g_perfFrames % PERF_LAG];

	App_Part(&f->part, &f->t);
	f->dt = dt;
	f->cpu = cpu;
	f->swap = swap;
	g_perfFrames++;
}

#ifdef RUPTURE_AGENT
/* OURS. The agent's frame, first half. */
static void agent_draw(double now)
{
	App_Frame(now);
	AgentPort_FrameDone();
}

/* OURS. The second half: read back if asked, swap, keep the window alive. */
static void agent_present(bool capture)
{
	if (capture) {
		AgentPort_Capture();
	}
	SwapBuffers(g_hDC);
	title_update("   [agent]");
	pump();
}

/* OURS. The agent's loop: the pipe runs it, paused until told otherwise; running, one step per dt of wall time. */
static void agent_loop(void)
{
	typedef BOOL (WINAPI *SwapInterval_Fn)(int interval);
	SwapInterval_Fn swapInterval = (SwapInterval_Fn)wglGetProcAddress("wglSwapIntervalEXT");

	if (swapInterval != NULL) {
		swapInterval(0);
	}
	if (!AgentPort_Start(agent_draw, agent_present, g_width, g_height, App_TimeBase())) {
		fail("the agent pipe would not start");
	}
	LARGE_INTEGER freq, last, now;

	title_update("   [agent: paused, frame 0]");
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&last);
	while (!g_quit && !AgentPort_Quit()) {
		pump();
		AgentPort_Pass();
		QueryPerformanceCounter(&now);
		if (AgentPort_Paused()
		 || (double)(now.QuadPart - last.QuadPart) / (double)freq.QuadPart < AgentPort_Step()) {
			Sleep(1);
		} else {
			last = now;
			agent_draw(AgentPort_Advance());
			agent_present(true);
		}
	}
}
#endif

/* OURS */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
	std::string   error;
	LARGE_INTEGER freq, start, now, last, drawn, swapped;
	double        base;
	bool          agent = strstr(cmd, "--agent") != NULL;
	const char   *size = strstr(cmd, "--size=");
	const char   *part = strstr(cmd, "--part=");
	const char   *perflog = strstr(cmd, "--perflog=");
	const char   *c;
	bool          dialog;

	(void)hPrev;
	(void)show;
	c = cmd;
	while (*c == ' ' || *c == '\t') {
		c++;
	}
	dialog = *c == '\0' && FindResourceA(hInst, MAKEINTRESOURCEA(101), RT_DIALOG) != NULL;
	if (dialog || strstr(cmd, "--fullscreen") != NULL) {
		dpi_aware();
	}
	if (dialog && DialogBoxParamA(hInst, MAKEINTRESOURCEA(101), NULL, Setup_DialogProc, 0) <= 0) {
		fail("the setup dialog would not open");
	}
	if (size != NULL && sscanf(size + 7, "%dx%d", &g_width, &g_height) != 2) {
		fail("--size=WxH, as --size=794x421");
	}
	if (strstr(cmd, "--fullscreen") != NULL) {
		if (agent) {
			fail("--agent runs in a window, not --fullscreen");
		}
		g_fullscreen = true;
		monitor_of(NULL);
		g_width = g_native.w;
		g_height = g_native.h;
	}
#ifndef RUPTURE_AGENT
	if (agent) {
		fail("--agent needs the Debug build: build/rupture/Debug/rupture.exe "
		     "(tools/agentctl.py launch starts it)");
	}
#endif
	gl1::SetBatching(strstr(cmd, "--nobatch") == NULL);
	if (part != NULL) {
		char spec[64];

		if (sscanf(part + 7, "%63s", spec) != 1 || !App_SetJump(spec)) {
			fail("--part=NAME or --part=NAME@T, NAME one of video motor truck rail boat city end");
		}
	}
	window_create(hInst);
#ifdef RUPTURE_AGENT
	if (agent) {
		App_SetAgent();
	}
#endif
	if (!App_Init(g_width, g_height, error)) {
		fail(error);
	}
	present();
	if (!App_Load(error)) {
		fail(error);
	}
#ifdef RUPTURE_AGENT
	if (agent) {
		agent_loop();
		return 0;
	}
#endif
	base = App_TimeBase();
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);
	if (perflog != NULL) {
		perf_open(perflog + 10);
	}
	last = drawn = start;
	while (!g_quit) {
		pump();
		QueryPerformanceCounter(&now);
		if (g_perf != NULL) {
			perf_begin();
		}
		if (!App_Frame(base + (double)(now.QuadPart - start.QuadPart) / (double)freq.QuadPart)) {
			break;
		}
		if (g_perf != NULL) {
			glEndQuery(GL_TIME_ELAPSED);
			QueryPerformanceCounter(&drawn);
		}
		present();
		if (g_perf != NULL) {
			QueryPerformanceCounter(&swapped);
			perf_end(ms(last, now, freq), ms(now, drawn, freq), ms(drawn, swapped, freq));
			last = now;
		}
		title_update("");
	}
	if (g_perf != NULL) {
		fclose(g_perf);
	}
	return 0;
}
