/*
 * What a program gives its host. A host (host_win32.cpp, host_web.cpp)
 * owns the window, the GL context, the input and the clock, and calls
 * App_Frame once per frame; a program never loops (CLAUDE.md rule 20).
 */
#ifndef RUPTURE_HOST_H
#define RUPTURE_HOST_H

#include <string>

enum { HOST_WIDTH = 1280, HOST_HEIGHT = 720 };

/* Keys arrive as characters: letters upper case, '[', ']', ' ', '+', '-', '.', and these. */
enum { KEY_ESCAPE = 27, KEY_HOME = 0x100 };

bool        App_Init(int width, int height, std::string &error);   /* draws the loading screen */
bool        App_Load(std::string &error);    /* after the host has shown it */
bool        App_SetJump(const char *spec);   /* before App_Init: NAME or NAME@T */
double      App_TimeBase(void);              /* after App_Init: where the host's clock starts */
bool        App_Frame(double seconds);       /* false: the demo has ended */
void        App_Key(int key);
void        App_Drag(int dx, int dy);
void        App_DragEnd(void);
void        App_Wheel(int notches, bool fine);  /* > 0 away from the user: zoom in; fine: Ctrl held */
void        App_ViewReset(void);
const char *App_Title(void);

/* The agent's view (host/agent_port.cpp): Windows Debug builds only. */
void        App_SetAgent(void);
double      App_Clock(void);
int         App_Part(const char **name, double *t);
bool        App_Muted(void);
void        App_SetMuted(bool muted);
int         App_Volume(void);                /* tenths, 0 to 10 */
void        App_SetVolume(int tenths);

#endif
