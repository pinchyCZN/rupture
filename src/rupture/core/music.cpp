#include "music.h"

#include <string.h>
#include <vector>

#include "file.h"

#ifdef __EMSCRIPTEN__

/* OURS. core/music.js, linked by tools/build_web.sh. */
extern "C" {
	void   music_web_load(const unsigned char *p, int n);
	int    music_web_play(double at);
	double music_web_seconds(void);
	double music_web_length(void);
	void   music_web_volume(double v);
	void   music_web_pause(int on);
	void   music_web_seek(double at);
}

/* OURS */
bool Music_Load(const char *file)
{
	std::vector<u8> buf;

	if (!File_Read(file, buf) || buf.empty()) {
		return false;
	}
	music_web_load(&buf[0], (int)buf.size());
	return true;
}

/* OURS */
bool Music_Play(double seconds)
{
	return music_web_play(seconds) != 0;
}

/* OURS */
bool Music_Playing(void)
{
	return music_web_seconds() >= 0.0;
}

/* OURS */
double Music_Seconds(void)
{
	return music_web_seconds();
}

/* OURS */
double Music_Length(void)
{
	return music_web_length();
}

/* OURS */
void Music_SetVolume(double volume)
{
	music_web_volume(volume);
}

/* OURS */
void Music_Pause(bool on)
{
	music_web_pause(on ? 1 : 0);
}

/* OURS */
void Music_Seek(double seconds)
{
	music_web_seek(seconds);
}

#else

#include <windows.h>
#include <mmsystem.h>

#include "../../third_party/dr_mp3/dr_mp3.h"

enum { MUSIC_BUFFERS = 6, MUSIC_CHUNK = 2048 };

static drmp3_int16  *g_pcm;
static drmp3_uint64  g_frames;
static unsigned      g_channels, g_rate;
static HWAVEOUT      g_out;
static HANDLE        g_event;
static WAVEHDR       g_hdr[MUSIC_BUFFERS];
static short         g_chunk[MUSIC_BUFFERS][MUSIC_CHUNK * 2];
static drmp3_uint64  g_start, g_next;
static volatile LONG g_volume;
static bool          g_playing;
static CRITICAL_SECTION g_lock;   /* the feeder's fills against Music_Seek's */

/* OURS. Decode the whole file once, at startup (CLAUDE.md rule 15). */
bool Music_Load(const char *file)
{
	std::vector<u8> buf;
	drmp3_config    cfg;

	if (!File_Read(file, buf) || buf.empty()) {
		return false;
	}
	g_pcm = drmp3_open_memory_and_read_pcm_frames_s16(&buf[0], buf.size(), &cfg, &g_frames, NULL);
	if (g_pcm == NULL || cfg.channels < 1 || cfg.channels > 2) {
		return false;
	}
	g_channels = cfg.channels;
	g_rate = cfg.sampleRate;
	return true;
}

/* OURS. The next chunk into buffer i, scaled by the volume; silence past the end. */
static void fill(int i)
{
	LONG          vol = g_volume;
	unsigned      k, c;
	drmp3_uint64  f;
	short        *out = g_chunk[i];

	for (k = 0; k < MUSIC_CHUNK; k++) {
		f = g_next + k;
		for (c = 0; c < 2; c++) {
			out[k * 2 + c] = f < g_frames ? (short)((g_pcm[f * g_channels + (c % g_channels)] * vol) >> 8) : 0;
		}
	}
	g_next += MUSIC_CHUNK;
	g_hdr[i].dwFlags &= ~WHDR_DONE;
	waveOutWrite(g_out, &g_hdr[i], sizeof g_hdr[i]);
}

/* OURS. Refills each buffer WinMM hands back, until the music ends. */
static DWORD WINAPI feeder(LPVOID arg)
{
	int i;

	(void)arg;
	for (;;) {
		WaitForSingleObject(g_event, INFINITE);
		EnterCriticalSection(&g_lock);
		for (i = 0; i < MUSIC_BUFFERS; i++) {
			if ((g_hdr[i].dwFlags & WHDR_DONE) && g_next < g_frames) {
				fill(i);
			}
		}
		LeaveCriticalSection(&g_lock);
	}
}

/* OURS. Start playing at a position; once only. */
bool Music_Play(double seconds)
{
	WAVEFORMATEX fmt;
	int          i;

	if (g_playing) {
		return true;
	}
	if (g_pcm == NULL) {
		return false;
	}
	memset(&fmt, 0, sizeof fmt);
	fmt.wFormatTag = WAVE_FORMAT_PCM;
	fmt.nChannels = 2;
	fmt.nSamplesPerSec = g_rate;
	fmt.wBitsPerSample = 16;
	fmt.nBlockAlign = 4;
	fmt.nAvgBytesPerSec = g_rate * 4;
	InitializeCriticalSection(&g_lock);
	g_event = CreateEventA(NULL, FALSE, FALSE, NULL);
	if (waveOutOpen(&g_out, WAVE_MAPPER, &fmt, (DWORD_PTR)g_event, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
		return false;
	}
	g_start = g_next = (drmp3_uint64)(seconds * g_rate);
	for (i = 0; i < MUSIC_BUFFERS; i++) {
		memset(&g_hdr[i], 0, sizeof g_hdr[i]);
		g_hdr[i].lpData = (LPSTR)g_chunk[i];
		g_hdr[i].dwBufferLength = sizeof g_chunk[i];
		waveOutPrepareHeader(g_out, &g_hdr[i], sizeof g_hdr[i]);
		fill(i);
	}
	CloseHandle(CreateThread(NULL, 0, feeder, NULL, 0, NULL));
	g_playing = true;
	return true;
}

/* OURS */
bool Music_Playing(void)
{
	return g_playing;
}

/* OURS. FSOUND_GetCurrentPosition over FSOUND_GetFrequency: the device's own count. */
double Music_Seconds(void)
{
	MMTIME t;

	if (!g_playing) {
		return -1.0;
	}
	t.wType = TIME_SAMPLES;
	waveOutGetPosition(g_out, &t, sizeof t);
	if (t.wType != TIME_SAMPLES) {
		return -1.0;
	}
	return (double)(g_start + t.u.sample) / g_rate;
}

/* OURS */
double Music_Length(void)
{
	return g_rate != 0 ? (double)g_frames / g_rate : 0.0;
}

/* OURS. 0 to 1, applied as the feeder fills; it takes effect within a few chunks. */
void Music_SetVolume(double volume)
{
	InterlockedExchange(&g_volume, (LONG)(volume * 256.0 + 0.5));
}

/* OURS. Hold the device where it is, or let it run on. */
void Music_Pause(bool on)
{
	if (g_playing) {
		if (on) {
			waveOutPause(g_out);
		} else {
			waveOutRestart(g_out);
		}
	}
}

/*
 * OURS. Play on from another position: the queued buffers are dropped
 * and refilled from there. A paused device stays paused.
 */
void Music_Seek(double seconds)
{
	int i;

	if (!g_playing) {
		return;
	}
	EnterCriticalSection(&g_lock);
	waveOutReset(g_out);
	g_start = g_next = (drmp3_uint64)(seconds * g_rate);
	for (i = 0; i < MUSIC_BUFFERS; i++) {
		if (g_next < g_frames) {
			fill(i);
		}
	}
	LeaveCriticalSection(&g_lock);
}

#endif
