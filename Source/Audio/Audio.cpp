#include "Audio.h"

int Audio::sampleRate = 44100;
Uint16 Audio::format = AUDIO_S16SYS;
int Audio::channels = 1;
void Audio::setUp() {
	Mix_Init(0);
	Mix_OpenAudio(sampleRate, format, channels, 2048);
	Mix_QuerySpec(&sampleRate, &format, &channels);
}
void Audio::tearDown() {
	Mix_Quit();
}
void Audio::generateTone(Waveform waveform, float frequency, float duration, float volume, Mix_Chunk* outChunk) {
	int sampleCount = (int)(duration * sampleRate);
	char bitsize = (char)SDL_AUDIO_BITSIZE(format);
	float valMax = volume * ((1 << bitsize) - 1);
	float fadeOutStart = duration - fadeInOutDuration;
	outChunk->allocated = 1;
	outChunk->alen = sampleCount * (bitsize / 8) * channels;
	delete[] outChunk->abuf;
	outChunk->abuf = new Uint8[outChunk->alen];
	outChunk->volume = MIX_MAX_VOLUME;
	for (int i = 0; i < sampleCount; i++) {
		float moment = (float)i / sampleRate;
		float waveSpot = fmodf(moment * frequency, 1.0f);
		float val = 0;
		switch (waveform) {
			case Waveform::Square:
				val = waveSpot < 0.5f ? valMax : -valMax;
				break;
			case Waveform::Triangle:
			case Waveform::Saw:
			case Waveform::Sine:
			default:
				break;
		}
		if (moment < fadeInOutDuration)
			val *= moment / fadeInOutDuration;
		else if (moment > fadeOutStart)
			val *= (duration - moment) / fadeInOutDuration;
		if (bitsize == 16) {
			for (int j = 0; j < channels; j++)
				((short*)(outChunk->abuf))[i * channels + j] = (short)val;
		} else if (bitsize == 8) {
			for (int j = 0; j < channels; j++)
				((char*)(outChunk->abuf))[i * channels + j] = (char)val;
		} else {
			for (int j = 0; j < channels; j++)
				((int*)(outChunk->abuf))[i * channels + j] = (int)val;
		}
	}
}
