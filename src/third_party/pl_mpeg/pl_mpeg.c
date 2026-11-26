/*
 * OURS. The one translation unit that builds pl_mpeg, compiled as C:
 * the library is C99 and the rupture C++ is C++98, so it cannot be
 * built inside a .cpp. pl_mpeg.h is Dominic Szablewski's, MIT, copied
 * unmodified from pl_mpeg-master. Its declarations name FILE before it
 * includes <stdio.h>, so that comes first.
 */
#include <stdio.h>

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"
