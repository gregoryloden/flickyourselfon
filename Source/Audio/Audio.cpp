#include "Audio.h"
#ifdef ENABLE_SKIP_BEATS
	#include <algorithm>
#endif
#include "Util/Config.h"
#include "Util/FileUtils.h"
#include "Util/StringUtils.h"

#define newSound(filename, channel) newWithArgs(Sound, filename, channel)
#define newMusic(filename, channel, waveform, soundEffectSpecs) \
	newWithArgs(Music, filename, channel, waveform, soundEffectSpecs)

//////////////////////////////// AudioTypes::Sound ////////////////////////////////
AudioTypes::Sound::Sound(objCounterParametersComma() string pFilename, int pChannel)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
filepath(string("audio/") + pFilename)
, chunk(nullptr)
, channel(pChannel) {
}
AudioTypes::Sound::~Sound() {
	Mix_FreeChunk(chunk);
}
void AudioTypes::Sound::load() {
	chunk = Mix_LoadWAV(filepath.c_str());
}
void AudioTypes::Sound::play(int loops) {
	Audio::applyChannelVolume(Mix_PlayChannel(channel, chunk, loops));
}
#ifdef WRITE_WAV_FILES
	void AudioTypes::Sound::writeWavFile() {
		ofstream out;
		out.open(string(filepath) + ".wav", ios::out | ios::trunc | ios::binary);

		int bitSize = SDL_AUDIO_BITSIZE(Audio::format);
		int bytesPerSample = Audio::channels * bitSize / 8;
		int byteRate = Audio::sampleRate * bytesPerSample;
		char chunk1Header[] = {
			//constant
			'f', 'm', 't', ' ',
			//chunk 1 size, 16 bytes
			16, 0, 0, 0,
		};
		char chunk1[] = {
			//format: PCM == 1
			1, 0,
			//channels
			(char)Audio::channels, 0,
			//sample rate
			(char)Audio::sampleRate, (char)(Audio::sampleRate >> 8),
			(char)(Audio::sampleRate >> 16), (char)(Audio::sampleRate >> 24),
			//byte rate
			(char)byteRate, (char)(byteRate >> 8), (char)(byteRate >> 16), (char)(byteRate >> 24),
			//bytes per sample
			(char)bytesPerSample, 0,
			//bits per sample per channel
			(char)bitSize, (char)(bitSize >> 8),
		};
		char chunk2Header[] = {
			//constant
			'd', 'a', 't', 'a',
			//chunk 2 size
			(char)chunk->alen, (char)(chunk->alen >> 8), (char)(chunk->alen >> 16), (char)(chunk->alen >> 24),
		};
		int chunkSizes = sizeof(chunk1Header) + sizeof(chunk1) + sizeof(chunk2Header) + chunk->alen;
		char header[] = {
			//constant
			'R', 'I', 'F', 'F',
			//chunk sizes
			(char)chunkSizes, (char)(chunkSizes >> 8), (char)(chunkSizes >> 16), (char)(chunkSizes >> 24),
			//constant
			'W', 'A', 'V', 'E',
		};
		out.write(header, sizeof(header));
		out.write(chunk1Header, sizeof(chunk1Header));
		out.write(chunk1, sizeof(chunk1));
		out.write(chunk2Header, sizeof(chunk2Header));
		out.write((char*)chunk->abuf, chunk->alen);
		out.close();
	}
#endif

//////////////////////////////// AudioTypes::Music::SoundEffectSpecs ////////////////////////////////
AudioTypes::Music::SoundEffectSpecs::SoundEffectSpecs(
	float pVolume, VolumeEffect pVolumeEffect, int pReverbRepetitions, float pReverbSingleDelay, float pReverbFalloff)
: volume(pVolume)
, volumeEffect(pVolumeEffect)
, reverbRepetitions(pReverbRepetitions)
, reverbSingleDelay(pReverbSingleDelay)
, reverbFalloff(pReverbFalloff) {
}
AudioTypes::Music::SoundEffectSpecs::~SoundEffectSpecs() {}
AudioTypes::Music::SoundEffectSpecs AudioTypes::Music::SoundEffectSpecs::withVolume(float pVolume) {
	return SoundEffectSpecs(pVolume, volumeEffect, reverbRepetitions, reverbSingleDelay, reverbFalloff);
}

//////////////////////////////// AudioTypes::Music::Note ////////////////////////////////
AudioTypes::Music::Note::Note(float pFrequency, int pBeats)
: frequency(pFrequency)
, beats(pBeats) {
}
AudioTypes::Music::Note::~Note() {}

//////////////////////////////// AudioTypes::Music ////////////////////////////////
AudioTypes::Music::Music(
	objCounterParametersComma() string pFilename, int pChannel, Waveform pWaveform, SoundEffectSpecs& pSoundEffectSpecs)
: Sound(objCounterArgumentsComma() pFilename + ".smus", pChannel)
, waveform(pWaveform)
, soundEffectSpecs(pSoundEffectSpecs)
, notes() {
}
AudioTypes::Music::~Music() {
	delete[] chunk->abuf;
	delete chunk;
	chunk = nullptr;
}
void AudioTypes::Music::load() {
	//these are ordered by note name, rather than absolute frequency; they loop around at C
	static constexpr float noteFrequencies[] = {
		frequencyA4, frequencyAS4, frequencyGS4,
		frequencyB4, frequencyC4, frequencyAS4,
		frequencyC4, frequencyCS4, frequencyB4,
		frequencyD4, frequencyDS4, frequencyCS4,
		frequencyE4, frequencyF4, frequencyDS4,
		frequencyF4, frequencyFS4, frequencyE4,
		frequencyG4, frequencyGS4, frequencyFS4,
	};
	int bytesPerSample = SDL_AUDIO_BITSIZE(Audio::format) / 8 * Audio::channels;

	ifstream file;
	FileUtils::openFileForRead(&file, filepath.c_str());
	string line;

	//get the bpm
	getline(file, line);
	int bpm;
	const char* firstLine = StringUtils::parseNextInt(line.c_str(), &bpm);
	#ifdef ENABLE_SKIP_BEATS
		int blockBeatsCount, skipBeatBlocksCount, skipBeatsCount;
		firstLine = StringUtils::parseNextInt(firstLine, &blockBeatsCount);
		firstLine = StringUtils::parseNextInt(firstLine, &skipBeatBlocksCount);
		firstLine = StringUtils::parseNextInt(firstLine, &skipBeatsCount);
		skipBeatsDuration = (float)(skipBeatBlocksCount * blockBeatsCount + skipBeatsCount) * 60 / bpm;
	#endif

	//collect all the notes into one string
	string notesString;
	while (getline(file, line))
		notesString += line;
	file.close();

	//go through all the note strings and convert them into Notes
	const char* nextNotes = notesString.c_str();
	for (char c = *nextNotes; c != 0; c = *nextNotes) {
		if (c == '-') {
			if (notes.size() == 0 || notes.back().frequency != 0)
				notes.push_back(Music::Note(0, 1));
			else
				notes.back().beats++;
		} else if (c == '_') {
			if (notes.size() > 0)
				notes.back().beats++;
		} else if (c >= 'A' && c <= 'G') {
			char noteIndex = c - 'A';
			float frequency;
			nextNotes++;
			c = *nextNotes;
			if (c == '#') {
				frequency = noteFrequencies[noteIndex * 3 + 1];
				nextNotes++;
				c = *nextNotes;
			} else if (c == 'b') {
				frequency = noteFrequencies[noteIndex * 3 + 2];
				nextNotes++;
				c = *nextNotes;
			} else
				frequency = noteFrequencies[noteIndex * 3];
			if (c != '4')
				frequency *= pow(2.0f, c - '4');
			notes.push_back(Music::Note(frequency, 1));
		}
		nextNotes++;
	}

	//calculate the duration of the track
	int totalBeats = 0;
	for (Music::Note& note : notes)
		totalBeats += note.beats;
	float totalDuration = (float)totalBeats * 60 / bpm;

	//allocate the samples
	int totalSampleCount = (int)(totalDuration * Audio::sampleRate);
	int reverbExtraSamples = (int)(soundEffectSpecs.reverbRepetitions * soundEffectSpecs.reverbSingleDelay * Audio::sampleRate);
	chunk = new Mix_Chunk();
	chunk->allocated = 1;
	chunk->alen = (totalSampleCount + reverbExtraSamples) * bytesPerSample;
	chunk->abuf = new Uint8[chunk->alen]();
	chunk->volume = MIX_MAX_VOLUME;

	//finally, go through all the notes and write their samples
	int beatsProcessed = 0;
	int samplesProcessed = 0;
	for (Music::Note& note : notes) {
		int sampleStart = samplesProcessed;
		beatsProcessed += note.beats;
		samplesProcessed = (int)((float)beatsProcessed * 60 / bpm * Audio::sampleRate);
		if (note.frequency == 0)
			continue;

		int sampleCount = samplesProcessed - sampleStart;
		Uint8* samples = chunk->abuf + sampleStart * bytesPerSample;
		writeTone(note.frequency, sampleCount, samples);
	}
}
void AudioTypes::Music::writeTone(float frequency, int sampleCount, Uint8* outSamples) {
	float duration = (float)sampleCount / Audio::sampleRate;
	char bitsize = (char)SDL_AUDIO_BITSIZE(Audio::format);
	float valMax = soundEffectSpecs.volume * ((1 << bitsize) - 1);
	float fadeOutStart = duration - fadeInOutDuration;
	for (int i = 0; i < sampleCount; i++) {
		float moment = (float)i / Audio::sampleRate;
		float waveSpot = fmodf(moment * frequency, 1.0f);
		float val = 0;
		switch (waveform) {
			case Waveform::Square:
				val = waveSpot < 0.5f ? valMax : -valMax;
				break;
			case Waveform::Triangle:
				val = valMax
					* 4.0f
					* (waveSpot < 0.25f ? waveSpot
						: waveSpot < 0.75f ? (0.5f - waveSpot)
						: waveSpot - 1.0f);
				break;
			case Waveform::Saw:
				val = valMax * (1.0f - waveSpot * 2.0f);
				break;
			case Waveform::Sine:
				val = valMax * sinf(MathUtils::twoPi * waveSpot);
				break;
			default:
				break;
		}
		switch (soundEffectSpecs.volumeEffect) {
			case SoundEffectSpecs::VolumeEffect::SquareDecay:
				val *= MathUtils::fsqr((float)(sampleCount - i) / sampleCount);
				break;
			case SoundEffectSpecs::VolumeEffect::SquareInSquareOut: {
				float toneSpot = (float)(sampleCount - i) / sampleCount;
				val *= 4.0f * MathUtils::fsqr(toneSpot < 0.5f ? toneSpot : 1.0f - toneSpot);
				break;
			}
			default:
				break;
		}
		if (moment < fadeInOutDuration)
			val *= moment / fadeInOutDuration;
		else if (moment > fadeOutStart)
			val *= (duration - moment) / fadeInOutDuration;
		if (bitsize == 16) {
			for (int reverb = 0; reverb <= soundEffectSpecs.reverbRepetitions; reverb++) {
				int sampleOffset = i + (int)(reverb * soundEffectSpecs.reverbSingleDelay * Audio::sampleRate);
				for (int j = 0; j < Audio::channels; j++)
					((short*)outSamples)[sampleOffset * Audio::channels + j] += (short)val;
				val *= soundEffectSpecs.reverbFalloff;
			}
		} else if (bitsize == 8) {
			for (int j = 0; j < Audio::channels; j++)
				((char*)outSamples)[i * Audio::channels + j] = (char)val;
		} else {
			for (int j = 0; j < Audio::channels; j++)
				((int*)outSamples)[i * Audio::channels + j] = (int)val;
		}
	}
}
void AudioTypes::Music::overlay(Music* other) {
	char bitsize = (char)SDL_AUDIO_BITSIZE(Audio::format);
	int count = MathUtils::min(chunk->alen, other->chunk->alen);
	Uint8* dst = chunk->abuf;
	Uint8* src = other->chunk->abuf;
	if (bitsize == 16) {
		count /= 2;
		for (int i = 0; i < count; i++)
			((short*)dst)[i] += ((short*)src)[i];
	} else if (bitsize == 8) {
		for (int i = 0; i < count; i++)
			((char*)dst)[i] += ((char*)src)[i];
	} else {
		count /= 4;
		for (int i = 0; i < count; i++)
			((int*)dst)[i] += ((int*)src)[i];
	}
}
#ifdef ENABLE_SKIP_BEATS
	void AudioTypes::Music::skipBeats() {
		char bitsize = (char)SDL_AUDIO_BITSIZE(Audio::format);
		int skipSamples = (int)(skipBeatsDuration * Audio::sampleRate);
		rotate(chunk->abuf, chunk->abuf + (bitsize / 8 * Audio::channels * skipSamples), chunk->abuf + chunk->alen);
	}
#endif
using namespace AudioTypes;

//////////////////////////////// Audio ////////////////////////////////
int Audio::sampleRate = 44100;
Uint16 Audio::format = AUDIO_S16SYS;
int Audio::channels = 1;
Music* Audio::musicSquare = nullptr;
Music* Audio::musicTriangle = nullptr;
Music* Audio::musicSaw = nullptr;
Music* Audio::musicSine = nullptr;
Music* Audio::radioWavesSoundSquare = nullptr;
Music* Audio::radioWavesSoundTriangle = nullptr;
Music* Audio::radioWavesSoundSaw = nullptr;
Music* Audio::radioWavesSoundSine = nullptr;
Music* Audio::switchesFadeInSoundSquare = nullptr;
Music* Audio::switchesFadeInSoundTriangle = nullptr;
Music* Audio::switchesFadeInSoundSaw = nullptr;
Music* Audio::switchesFadeInSoundSine = nullptr;
Music* Audio::victorySound = nullptr;
void Audio::setUp() {
	Mix_Init(0);
	Mix_OpenAudio(sampleRate, format, channels, 2048);
	Mix_QuerySpec(&sampleRate, &format, &channels);
	Mix_ReserveChannels(1);
}
void Audio::stopAudio() {
	Mix_CloseAudio();
}
void Audio::tearDown() {
	Mix_Quit();
}
void Audio::loadSounds() {
	Music::SoundEffectSpecs musicSoundEffectSpecs (1, Music::SoundEffectSpecs::VolumeEffect::Full, 0, 0, 0);
	Music::SoundEffectSpecs radioWavesSoundEffectSpecs (
		1,
		Music::SoundEffectSpecs::VolumeEffect::SquareDecay,
		radioWavesReverbRepetitions,
		radioWavesReverbSingleDelay,
		radioWavesReverbFalloff);
	Music::SoundEffectSpecs switchesFadeInSoundEffectSpecs (
		1, Music::SoundEffectSpecs::VolumeEffect::SquareInSquareOut, 0, 0, 0);
	Music *victorySoundTriangle, *victorySoundSaw, *victorySoundSine;
	vector<Sound*> sounds ({
		musicSquare =
			newMusic("musicsquare", musicChannel, Music::Waveform::Square, musicSoundEffectSpecs.withVolume(musicSquareVolume)),
		musicTriangle = newMusic(
			"musictriangle", musicChannel, Music::Waveform::Triangle, musicSoundEffectSpecs.withVolume(musicTriangleVolume)),
		musicSaw = newMusic("musicsaw", musicChannel, Music::Waveform::Saw, musicSoundEffectSpecs.withVolume(musicSawVolume)),
		musicSine =
			newMusic("musicsine", musicChannel, Music::Waveform::Sine, musicSoundEffectSpecs.withVolume(musicSineVolume)),
		radioWavesSoundSquare = newMusic(
			"radiowaves", -1, Music::Waveform::Square, radioWavesSoundEffectSpecs.withVolume(radioWavesSoundSquareVolume)),
		radioWavesSoundTriangle = newMusic(
			"radiowaves", -1, Music::Waveform::Triangle, radioWavesSoundEffectSpecs.withVolume(radioWavesSoundTriangleVolume)),
		radioWavesSoundSaw =
			newMusic("radiowaves", -1, Music::Waveform::Saw, radioWavesSoundEffectSpecs.withVolume(radioWavesSoundSawVolume)),
		radioWavesSoundSine =
			newMusic("radiowaves", -1, Music::Waveform::Sine, radioWavesSoundEffectSpecs.withVolume(radioWavesSoundSineVolume)),
		switchesFadeInSoundSquare = newMusic(
			"switchesfadein", -1, Music::Waveform::Square, switchesFadeInSoundEffectSpecs.withVolume(musicSquareVolume)),
		switchesFadeInSoundTriangle = newMusic(
			"switchesfadein", -1, Music::Waveform::Triangle, switchesFadeInSoundEffectSpecs.withVolume(musicTriangleVolume)),
		switchesFadeInSoundSaw =
			newMusic("switchesfadein", -1, Music::Waveform::Saw, switchesFadeInSoundEffectSpecs.withVolume(musicSawVolume)),
		switchesFadeInSoundSine =
			newMusic("switchesfadein", -1, Music::Waveform::Sine, switchesFadeInSoundEffectSpecs.withVolume(musicSineVolume)),
		victorySound =
			newMusic("victorysquare", -1, Music::Waveform::Square, musicSoundEffectSpecs.withVolume(victorySoundSquareVolume)),
		victorySoundTriangle = newMusic(
			"victorytriangle", -1, Music::Waveform::Triangle, musicSoundEffectSpecs.withVolume(victorySoundTriangleVolume)),
		victorySoundSaw =
			newMusic("victorysaw", -1, Music::Waveform::Saw, musicSoundEffectSpecs.withVolume(victorySoundSawVolume)),
		victorySoundSine =
			newMusic("victorysine", -1, Music::Waveform::Sine, musicSoundEffectSpecs.withVolume(victorySoundSineVolume)),
	});

	for (Sound* sound : sounds)
		sound->load();

	musicTriangle->overlay(musicSquare);
	musicSaw->overlay(musicTriangle);
	musicSine->overlay(musicSaw);
	victorySound->overlay(victorySoundTriangle);
	victorySound->overlay(victorySoundSaw);
	victorySound->overlay(victorySoundSine);
	delete victorySoundTriangle;
	delete victorySoundSaw;
	delete victorySoundSine;
	#ifdef ENABLE_SKIP_BEATS
		musicSquare->skipBeats();
		musicTriangle->skipBeats();
		musicSaw->skipBeats();
		musicSine->skipBeats();
	#endif
}
void Audio::unloadSounds() {
	delete musicSquare;
	delete musicTriangle;
	delete musicSaw;
	delete musicSine;
	delete radioWavesSoundSquare;
	delete radioWavesSoundTriangle;
	delete radioWavesSoundSaw;
	delete radioWavesSoundSine;
	delete switchesFadeInSoundSquare;
	delete switchesFadeInSoundTriangle;
	delete switchesFadeInSoundSaw;
	delete switchesFadeInSoundSine;
	delete victorySound;
}
void Audio::applyVolume() {
	applyChannelVolume(-1);
	applyChannelVolume(musicChannel);
}
void Audio::applyChannelVolume(int channel) {
	float masterVolume = (float)Config::masterVolume.volume / ConfigTypes::VolumeSetting::maxVolume;
	float channelVolume =
		(float)(channel == musicChannel ? Config::musicVolume : Config::soundsVolume).volume
			/ ConfigTypes::VolumeSetting::maxVolume;
	Mix_Volume(channel, (int)(masterVolume * channelVolume * MIX_MAX_VOLUME));
}
void Audio::pauseAll() {
	Mix_Pause(-1);
}
void Audio::resumeAll() {
	Mix_Resume(-1);
}
void Audio::fadeOutAll(int ms) {
	Mix_FadeOutChannel(-1, ms);
}
void Audio::stopAll() {
	Mix_HaltChannel(-1);
}
