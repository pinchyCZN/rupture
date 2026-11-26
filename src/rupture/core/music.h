/*
 * The music: the release's one MP3, decoded and played by the platform
 * in place of FMOD (subsystems/clock.txt 2). Windows decodes it whole
 * with dr_mp3 at load and plays it through WinMM; the browser decodes
 * it with Web Audio and can start only after a key or a click.
 */
#ifndef RUPTURE_MUSIC_H
#define RUPTURE_MUSIC_H

bool   Music_Load(const char *file);
bool   Music_Play(double seconds);
bool   Music_Playing(void);
double Music_Seconds(void);
double Music_Length(void);
void   Music_SetVolume(double volume);
void   Music_Pause(bool on);
void   Music_Seek(double seconds);

#endif
