/*
Copyright (C) 2003 Cedric Cellier, Dominique Lavault

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <mikmod.h>
#include <assert.h>
#include "log.h"

int sound_on=0;

struct {
	const char *file;
	MODULE *mod;
} musics[]= {
	{ "chrono.s3m" },
	{ "gf_delit.s3m" },
	{ "gf_heave.s3m" },
	{ "fatmoist.s3m" },
	{ "gf_willb.s3m" },
	{ "gf_triba.s3m" },
	{ "gf_depen.s3m" },
	{ "gf_srace.s3m" }
};
unsigned nb_musics = (sizeof(musics)/sizeof(*musics));

struct {
	const char *file;
	SAMPLE *sample;
} samples[] = {
	{ "clic.wav" },
	{ "pop.wav" },
	{ "boing.wav" },
	{ "tougoudou.wav" },
	{ "frrrr.wav" },
	{ "driiing.wav" },
	{ "poum.wav" }
};
unsigned nb_samples = (sizeof(samples)/sizeof(*samples));

void sound_init(int active) {
	unsigned i;
	sound_on = active;
	if (!sound_on) return;
//	MikMod_RegisterAllDrivers();
#ifdef _WINDOWS
	MikMod_RegisterDriver(&drv_win);
#else
	MikMod_RegisterDriver(&drv_oss);
#endif
	MikMod_RegisterAllLoaders();
	md_device = 1;
	if (MikMod_Init("buffer=10")) {
		gltv_log_fatal("MikMod : Cannot initialize sound: %s", MikMod_strerror(MikMod_errno));
	}
	for (i=0; i<nb_musics; i++) {
		musics[i].mod = Player_Load(musics[i].file, 16, 0);
		if (NULL==musics[i].mod) {
			gltv_log_fatal("MikMod : Cannot load module '%s' : %s", musics[i].file, MikMod_strerror(MikMod_errno));
		}
		musics[i].mod->wrap = 1;
	}
	for (i=0; i<nb_samples; i++) {
		samples[i].sample = Sample_Load(samples[i].file);
		if (NULL==samples[i].sample) {
			gltv_log_fatal("MikMod : Cannot load sample '%s' : %s", samples[i].file, MikMod_strerror(MikMod_errno));
		}
	}
	MikMod_SetNumVoices(-1,2);	/* voices for samples */
	MikMod_EnableOutput();
	Player_SetVolume(100);
/*	if (!MikMod_InitThreads()) {
		gltv_log_fatal("MikMod : not thread safe");
	} else {
		gltv_log_warning(GLTV_LOG_DEBUG, "Mikmod init ok");
	}*/
}

void sound_music_start(unsigned zik) {
	assert(zik<nb_musics);
	if (!sound_on) return;
	gltv_log_warning(GLTV_LOG_DEBUG, "start music %d", zik);
//	MikMod_Lock();
	Player_SetPosition(0);
	Player_Start(musics[zik].mod);
//	MikMod_Unlock();
}

void sound_update() {
//	MikMod_Lock();
	MikMod_Update();
//	MikMod_Unlock();
}

void sound_sample_start(unsigned s, unsigned volume, float freq, unsigned pan) {
	/* 0-255 for volume and pan,
	 * 0->n for frequency (1 beeing normal)
	 */
	int si;
	assert(s<nb_samples);
	if (!sound_on) return;
//	MikMod_Lock();
	si = Sample_Play(samples[s].sample, 0,0);
	Voice_SetVolume(si, volume);
	Voice_SetFrequency(si, (ULONG)((float)samples[s].sample->speed * freq));
	Voice_SetPanning(si, pan);
//	MikMod_Unlock();
}

void sound_music_stop() {
//	MikMod_Lock();
	if (!sound_on) return;
	Player_Stop();
//	MikMod_Unlock();
}

void sound_end() {
	unsigned i;
	if (!sound_on) return;
	MikMod_DisableOutput();
	for (i=0; i<nb_musics; i++) {
		Player_Free(musics[i].mod);
	}
	for (i=0; i<nb_samples; i++) {
		Sample_Free(samples[i].sample);
	}
	MikMod_Exit();
}

