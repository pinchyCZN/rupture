/*
 * The one place the core opens files (CLAUDE.md rule 20). Paths are
 * the release's own, relative to the working directory on Windows and
 * to the preloaded filesystem in the browser.
 */
#ifndef RUPTURE_FILE_H
#define RUPTURE_FILE_H

#include <string>
#include <vector>

#include "types.h"

bool File_Read(const char *path, std::vector<u8> &out);
bool File_ReadText(const char *path, std::string &out);

/* RGBA8, rows bottom up as glTexImage2D wants them. */
struct Image {
	int             width;
	int             height;
	std::vector<u8> rgba;
};

bool Image_Load(const char *path, Image &out);

#endif
