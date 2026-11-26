#include "video.h"

#include <stdio.h>
#include <string.h>

#include "file.h"
#include "gl1.h"
#include "shader.h"
#include "../../third_party/pl_mpeg/pl_mpeg.h"

/*
 * OURS. Reads the file once and makes the texture, empty until the
 * first Video_Show. GL_LINEAR and GL_CLAMP_TO_EDGE are FUN_0044eba0's
 * and part_video::Init's for every frame.
 */
bool Video_Open(Video *v, const char *path)
{
	plm_t *plm;

	v->plm = NULL;
	v->texture = 0;
	v->shown = -1;
	if (!File_Read(path, v->file) || v->file.empty()) {
		return false;
	}
	plm = plm_create_with_memory(&v->file[0], v->file.size(), 0);
	if (plm == NULL || !plm_probe(plm, 5000 * 1024)) {
		if (plm != NULL) {
			plm_destroy(plm);
		}
		return false;
	}
	plm_set_audio_enabled(plm, 0);
	v->plm = plm;
	v->width = plm_get_width(plm);
	v->height = plm_get_height(plm);
	v->frames = (int)(plm_get_duration(plm) * plm_get_framerate(plm) + 0.5);
	v->rows.resize((size_t)v->width * v->height);
	glGenTextures(1, &v->texture);
	gl1::BatchFlush();
	glBindTexture(GL_TEXTURE_2D, v->texture);
	Shader_Touch();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, v->width, v->height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	return true;
}

/*
 * OURS. Puts frame `frame` in the texture. The next frame is one
 * decode; any other is an exact seek, from the intra frame before it.
 * The rows go in bottom up because FUN_0044eba0 cvFlips every frame
 * before its upload.
 */
void Video_Show(Video *v, int frame)
{
	plm_t       *plm = (plm_t *)v->plm;
	plm_frame_t *f;
	double       fps;
	int          y;

	if (plm == NULL || frame == v->shown) {
		return;
	}
	if (frame == v->shown + 1) {
		f = plm_decode_video(plm);
	} else {
		fps = plm_get_framerate(plm);
		f = plm_seek_frame(plm, (frame - 0.5) / fps, 1);
	}
	if (f == NULL) {
		return;
	}
	for (y = 0; y < v->height; y++) {
		memcpy(&v->rows[(size_t)(v->height - 1 - y) * v->width],
		       f->y.data + (size_t)y * f->y.width, (size_t)v->width);
	}
	gl1::BatchFlush();
	glBindTexture(GL_TEXTURE_2D, v->texture);
	Shader_Touch();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, v->width, v->height, GL_RED, GL_UNSIGNED_BYTE,
	                &v->rows[0]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	v->shown = frame;
}

/* OURS */
void Video_Close(Video *v)
{
	if (v->plm != NULL) {
		plm_destroy((plm_t *)v->plm);
		v->plm = NULL;
	}
	if (v->texture != 0) {
		glDeleteTextures(1, &v->texture);
		v->texture = 0;
	}
	v->file.clear();
}
