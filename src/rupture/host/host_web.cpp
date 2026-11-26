/*
 * OURS. The browser host: a WebGL2 context on #canvas, the browser's
 * animation frame as the loop, and its clock. Its page is index.html
 * beside it, which calls the web_* functions below and is called back
 * through Module.demoReady.
 */
#include <emscripten.h>
#include <emscripten/html5.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "../core/glapi.h"
#include "host.h"

static double      g_start;
static bool        g_loaded;
static bool        g_started;
static bool        g_dragging;
static std::string g_title;

/*
 * OURS. The demo has ended: no more frames and no more input, and the
 * page told (Module.demoEnded), which closes the audio and the WebGL
 * context and offers to reload. The browser cannot close the tab.
 */
static void park(void)
{
	emscripten_cancel_main_loop();
	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, NULL);
	emscripten_set_mousedown_callback("#canvas", NULL, EM_TRUE, NULL);
	emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, NULL);
	emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, NULL);
	emscripten_set_wheel_callback("#canvas", NULL, EM_TRUE, NULL);
	emscripten_run_script("Module.demoEnded && Module.demoEnded()");
}

/*
 * OURS. One frame, called by the browser. The first loads the parts,
 * main having drawn the loading screen, and tells the page. Then
 * nothing until the page calls web_start, on a click: the browser lets
 * audio start only after one, so the clock starts there and the music
 * with it.
 */
static void frame(void)
{
	std::string error;

	if (!g_loaded) {
		g_loaded = true;
		if (!App_Load(error)) {
			printf("%s\n", error.c_str());
			emscripten_cancel_main_loop();
			return;
		}
		emscripten_run_script("Module.demoReady && Module.demoReady()");
		return;
	}
	if (!g_started) {
		return;
	}
	if (!App_Frame((emscripten_get_now() - g_start) / 1000.0)) {
		park();
		return;
	}
	if (g_title != App_Title()) {
		g_title = App_Title();
		emscripten_set_window_title(g_title.c_str());
	}
}

extern "C" {

/* OURS. The page's start: the demo's clock begins at the next frame. */
EMSCRIPTEN_KEEPALIVE void web_start(void)
{
	if (!g_started) {
		g_started = true;
		g_start = emscripten_get_now();
	}
}

/* OURS. The page's mute box, read every half second and set when clicked. */
EMSCRIPTEN_KEEPALIVE int web_muted(void)
{
	return App_Muted() ? 1 : 0;
}

/* OURS */
EMSCRIPTEN_KEEPALIVE void web_set_muted(int muted)
{
	App_SetMuted(muted != 0);
}

/* OURS. The page's volume slider, in tenths, the same way. */
EMSCRIPTEN_KEEPALIVE int web_volume(void)
{
	return App_Volume();
}

/* OURS */
EMSCRIPTEN_KEEPALIVE void web_set_volume(int tenths)
{
	App_SetVolume(tenths);
}

}

/* OURS. A key as the character App_Key takes; with Ctrl, Alt or Meta held it is the browser's. F is the page's fullscreen. */
static EM_BOOL on_key(int type, const EmscriptenKeyboardEvent *e, void *user)
{
	(void)type;
	(void)user;
	if (e->ctrlKey || e->altKey || e->metaKey) {
		return EM_FALSE;
	}
	if (strlen(e->key) == 1 && toupper((unsigned char)e->key[0]) == 'F') {
		return EM_FALSE;
	}
	if (strlen(e->key) == 1) {
		App_Key(toupper((unsigned char)e->key[0]));
		return EM_TRUE;
	}
	if (strcmp(e->key, "Home") == 0) {
		App_Key(KEY_HOME);
		return EM_TRUE;
	}
	return EM_FALSE;
}

/*
 * OURS. Up and move are the window's, so a look-drag carries on off
 * the canvas; only a look-drag's are claimed. A claimed event's default
 * is prevented, and the page's volume slider then cannot be dragged.
 */
static EM_BOOL on_mouse(int type, const EmscriptenMouseEvent *e, void *user)
{
	(void)user;
	if (type == EMSCRIPTEN_EVENT_MOUSEDOWN && e->button == 1) {
		App_Key('C');
		return EM_TRUE;
	}
	if (type == EMSCRIPTEN_EVENT_MOUSEDOWN && e->button != 0) {
		App_ViewReset();
		return EM_TRUE;
	}
	if (type == EMSCRIPTEN_EVENT_MOUSEDOWN) {
		g_dragging = true;
		return EM_TRUE;
	}
	if (type == EMSCRIPTEN_EVENT_MOUSEUP) {
		if (g_dragging) {
			g_dragging = false;
			App_DragEnd();
		}
		return EM_FALSE;
	}
	if (g_dragging) {
		App_Drag((int)e->movementX, (int)e->movementY);
		return EM_TRUE;
	}
	return EM_FALSE;
}

/* OURS */
static EM_BOOL on_wheel(int type, const EmscriptenWheelEvent *e, void *user)
{
	(void)type;
	(void)user;
	App_Wheel(e->deltaY < 0 ? 1 : -1, e->mouse.ctrlKey != 0);
	return EM_TRUE;
}

/* OURS. The render size is the page's choice (index.html), 1280x720 if it made none. */
int main(void)
{
	EmscriptenWebGLContextAttributes attr;
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE  gl;
	std::string                      error;
	int                              width = emscripten_run_script_int("Module.rupWidth | 0");
	int                              height = emscripten_run_script_int("Module.rupHeight | 0");

	if (width < 320 || height < 200 || width > 4096 || height > 4096) {
		width = HOST_WIDTH;
		height = HOST_HEIGHT;
	}
	emscripten_set_canvas_element_size("#canvas", width, height);
	emscripten_webgl_init_context_attributes(&attr);
	attr.majorVersion = 2;
	attr.minorVersion = 0;
	attr.alpha = EM_FALSE;
	attr.depth = EM_TRUE;
	attr.antialias = EM_FALSE;
	gl = emscripten_webgl_create_context("#canvas", &attr);
	if (gl <= 0) {
		printf("this browser has no WebGL2\n");
		return 1;
	}
	emscripten_webgl_make_context_current(gl);
	if (!App_Init(width, height, error)) {
		printf("%s\n", error.c_str());
		return 1;
	}
	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, on_key);
	emscripten_set_mousedown_callback("#canvas", NULL, EM_TRUE, on_mouse);
	emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, on_mouse);
	emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, on_mouse);
	emscripten_set_wheel_callback("#canvas", NULL, EM_TRUE, on_wheel);
	emscripten_set_main_loop(frame, 0, 1);
	return 0;
}
