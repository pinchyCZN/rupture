#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../third_party/jpgd/jpgd.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <windows.h>
#include <wincodec.h>
#endif

/* OURS */
bool File_Read(const char *path, std::vector<u8> &out)
{
	FILE *f = fopen(path, "rb");
	long  size;

	if (f == NULL) {
		return false;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	out.resize((size_t)size);
	if (size > 0 && fread(&out[0], 1, (size_t)size, f) != (size_t)size) {
		fclose(f);
		return false;
	}
	fclose(f);
	return true;
}

/* OURS */
bool File_ReadText(const char *path, std::string &out)
{
	std::vector<u8> data;

	if (!File_Read(path, data)) {
		return false;
	}
	out.assign(data.begin(), data.end());
	return true;
}

#ifdef __EMSCRIPTEN__

/*
 * OURS. The browser decodes the image while the page preloads it
 * (emcc --use-preload-plugins); this takes its RGBA and turns it
 * bottom up.
 */
static bool image_load_host(const char *path, Image &out)
{
	int   w = 0, h = 0, y;
	char *px = emscripten_get_preloaded_image_data(path, &w, &h);

	if (px == NULL) {
		return false;
	}
	out.width = w;
	out.height = h;
	out.rgba.resize((size_t)w * h * 4);
	for (y = 0; y < h; y++) {
		memcpy(&out.rgba[(size_t)y * w * 4], px + (size_t)(h - 1 - y) * w * 4, (size_t)w * 4);
	}
	free(px);
	return true;
}

#else

/* OURS. Windows decodes with WIC. */
static bool image_load_host(const char *path, Image &out)
{
	WCHAR                  wpath[MAX_PATH];
	IWICImagingFactory    *wic = NULL;
	IWICBitmapDecoder     *dec = NULL;
	IWICBitmapFrameDecode *frame = NULL;
	IWICFormatConverter   *conv = NULL;
	UINT                   w = 0, h = 0, y;
	bool                   ok = false;

	MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);
	CoInitialize(NULL);
	if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
	                               IID_IWICImagingFactory, (void **)&wic))
	 && SUCCEEDED(wic->CreateDecoderFromFilename(wpath, NULL, GENERIC_READ,
	                                             WICDecodeMetadataCacheOnDemand, &dec))
	 && SUCCEEDED(dec->GetFrame(0, &frame))
	 && SUCCEEDED(wic->CreateFormatConverter(&conv))
	 && SUCCEEDED(conv->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone,
	                               NULL, 0.0, WICBitmapPaletteTypeCustom))
	 && SUCCEEDED(conv->GetSize(&w, &h))) {
		out.width = (int)w;
		out.height = (int)h;
		out.rgba.resize((size_t)w * h * 4);
		ok = true;
		for (y = 0; y < h && ok; y++) {
			WICRect r;

			r.X = 0;
			r.Y = (INT)(h - 1 - y);
			r.Width = (INT)w;
			r.Height = 1;
			ok = SUCCEEDED(conv->CopyPixels(&r, w * 4, w * 4, &out.rgba[(size_t)y * w * 4]));
		}
	}
	if (conv != NULL) {
		conv->Release();
	}
	if (frame != NULL) {
		frame->Release();
	}
	if (dec != NULL) {
		dec->Release();
	}
	if (wic != NULL) {
		wic->Release();
	}
	return ok;
}

#endif

/*
 * OURS. The demo's JPEG decoder is jpgd (overview.txt 4): its IDCT,
 * colour tables and box chroma converters match the current jpgd's
 * with SIMD off and box chroma filtering, so JPEGs decode through it
 * on both hosts and every other file through the host's decoder.
 */
static bool image_load_jpeg(const char *path, Image &out)
{
	std::vector<u8> data;
	unsigned char  *px;
	int             w = 0, h = 0, comps = 0, y;

	if (!File_Read(path, data) || data.empty()) {
		return false;
	}
	px = jpgd::decompress_jpeg_image_from_memory(&data[0], (int)data.size(), &w, &h, &comps, 4,
	                                             jpgd::jpeg_decoder::cFlagBoxChromaFiltering
	                                             | jpgd::jpeg_decoder::cFlagDisableSIMD);
	if (px == NULL) {
		return false;
	}
	out.width = w;
	out.height = h;
	out.rgba.resize((size_t)w * h * 4);
	for (y = 0; y < h; y++) {
		memcpy(&out.rgba[(size_t)y * w * 4], px + (size_t)(h - 1 - y) * w * 4, (size_t)w * 4);
	}
	free(px);
	return true;
}

/* OURS */
bool Image_Load(const char *path, Image &out)
{
	size_t n = strlen(path);

	if (n > 4 && strcmp(path + n - 4, ".jpg") == 0) {
		return image_load_jpeg(path, out);
	}
	return image_load_host(path, out);
}
