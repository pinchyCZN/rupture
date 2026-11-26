#include "glapi.h"

#ifdef __EMSCRIPTEN__

/* OURS. The browser's GL needs no loading. */
bool GlApi_Load(void)
{
	return true;
}

#else

#define GLAPI_DEFINE(ret, name, params) PFN_##name name;
GLAPI_FUNCS(GLAPI_DEFINE)
#undef GLAPI_DEFINE

/* OURS. Each entry point through wglGetProcAddress. */
bool GlApi_Load(void)
{
	bool ok = true;

#define GLAPI_LOAD(ret, name, params) \
    name = (PFN_##name)wglGetProcAddress(#name); \
    if (name == NULL) { \
        ok = false; \
    }
	GLAPI_FUNCS(GLAPI_LOAD)
#undef GLAPI_LOAD
	return ok;
}

#endif
