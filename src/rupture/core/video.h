/*
 * The TV's video, OURS in its whole carriage: part_video::Init uploads
 * the 838 frames of test_video2/ as 838 textures (effects/video.txt
 * 1). Here they are one MPEG-1 file, src/video/test_video2.mpg made by
 * tools/video_encode.py, held in memory and decoded into ONE texture as
 * the frame index changes. The luma plane is the frame; the texture is
 * GL_R8 swizzled to (r, r, r, 1), which is what the demo's GL_LUMINANCE
 * upload samples as. Video frame k is file k + 1, so DrawScene's index
 * picks the same picture.
 */
#ifndef RUPTURE_VIDEO_H
#define RUPTURE_VIDEO_H

#include <vector>

#include "glapi.h"
#include "types.h"

struct Video {
	std::vector<u8> file;      /* the whole .mpg; pl_mpeg reads it in place */
	void           *plm;       /* plm_t */
	GLuint          texture;
	int             width;
	int             height;
	int             frames;
	int             shown;     /* the frame in the texture, -1 for none */
	std::vector<u8> rows;      /* one frame turned bottom up */
};

bool Video_Open(Video *v, const char *path);
void Video_Show(Video *v, int frame);
void Video_Close(Video *v);

#endif
