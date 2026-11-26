/*
 * OURS. The one translation unit that builds dr_mp3, compiled as C, as
 * pl_mpeg is. dr_mp3.h is David Reid's (public domain or MIT-0), from
 * dr_libs, copied unmodified. Only the Windows build uses it: the
 * browser decodes the MP3 itself (core/music.cpp).
 */
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
