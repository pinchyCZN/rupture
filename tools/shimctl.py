"""Talk to the shim's agent in the running demo: tools/agentctl.py --shim.

The shim (src/shim/agent.c) opens \\\\.\\pipe\\rupture_shim once the
demo draws its first frame. Same verbs and options as agentctl.py;
tools/README-agent.txt lists them.

    py -3.10 tools/shimctl.py STATUS
    py -3.10 tools/shimctl.py READ addr=g_demoClock len=8 -f d
    py -3.10 tools/shimctl.py HOOK add addr=Demo_Frame log=ra,absd+g_demoClock
    py -3.10 tools/shimctl.py TRACE
    py -3.10 tools/shimctl.py PAUSE on
"""
import sys

import agentctl

if __name__ == '__main__':
    sys.exit(agentctl.main(default_target='shim'))
