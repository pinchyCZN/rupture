/*
 * OURS. The port's side of the agent pipe (src/agent/agent_core.h), on
 * \\.\pipe\rupture_port. Windows Debug builds only, and only when the
 * exe is started with --agent; tools/agentctl.py launches it so.
 */
#ifndef RUPTURE_AGENT_PORT_H
#define RUPTURE_AGENT_PORT_H

/*
 * The host's frame in two halves: draw it at a clock, then show it,
 * reading it back first if capture is set - a STEP or RUN_TO reads back
 * only the frame it stops on.
 */
typedef void (*AgentPort_Draw)(double now);
typedef void (*AgentPort_Present)(bool capture);

bool   AgentPort_Start(AgentPort_Draw draw, AgentPort_Present present, int width, int height, double now);
void   AgentPort_Pass(void);
bool   AgentPort_Paused(void);
bool   AgentPort_Quit(void);
double AgentPort_Advance(void);
double AgentPort_Step(void);
void   AgentPort_Capture(void);
void   AgentPort_FrameDone(void);

#endif
