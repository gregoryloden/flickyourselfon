#include "Audio.h"
#include "Util/FileUtils.h"
#include "Util/StringUtils.h"

#define newMusic(filename, waveform, soundEffectSpecs) newWithArgs(Music, filename, waveform, soundEffectSpecs)

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
	objCounterParametersComma()
	const char* pFilename,
	Waveform pWaveform,
	SoundEffectSpecs& pSoundEffectSpecs)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
filename(pFilename)
, waveform(pWaveform)
, soundEffectSpecs(pSoundEffectSpecs)
, chunk() {
}
AudioTypes::Music::~Music() {
	delete[] chunk.abuf;
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
	string path = string("audio/") + filename + ".smus";
	FileUtils::openFileForRead(&file, path.c_str());
	string line;

	//get the bpm
	getline(file, line);
	int bpm;
	StringUtils::parseNextInt(line.c_str(), &bpm);

	//collect all the notes into one string
	string notesString;
	while (getline(file, line))
		notesString += line;
	file.close();

	//go through all the note strings and convert them into Notes
	vector<Music::Note> notes;
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
	chunk.allocated = 1;
	chunk.alen = (totalSampleCount + reverbExtraSamples) * bytesPerSample;
	chunk.abuf = new Uint8[chunk.alen]();
	chunk.volume = MIX_MAX_VOLUME;

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
		Uint8* samples = chunk.abuf + sampleStart * bytesPerSample;
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
			case SoundEffectSpecs::VolumeEffect::SquareDecay: {
				val *= MathUtils::fsqr((float)(sampleCount - i) / sampleCount);
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
	if (bitsize == 16) {
		int count = MathUtils::min(chunk.alen, other->chunk.alen) / 2;
		for (int i = 0; i < count; i++)
			((short*)chunk.abuf)[i] += ((short*)other->chunk.abuf)[i];
	} else if (bitsize == 8) {
		int count = MathUtils::min(chunk.alen, other->chunk.alen);
		for (int i = 0; i < count; i++)
			((char*)chunk.abuf)[i] += ((char*)other->chunk.abuf)[i];
	} else {
		int count = MathUtils::min(chunk.alen, other->chunk.alen) / 4;
		for (int i = 0; i < count; i++)
			((int*)chunk.abuf)[i] += ((int*)other->chunk.abuf)[i];
	}
}
void AudioTypes::Music::play(int loops) {
	Mix_PlayChannel(-1, &chunk, loops);
}
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
void Audio::setUp() {
	Mix_Init(0);
	Mix_OpenAudio(sampleRate, format, channels, 2048);
	Mix_QuerySpec(&sampleRate, &format, &channels);
}
void Audio::stopAudio() {
	Mix_CloseAudio();
}
void Audio::tearDown() {
	Mix_Quit();
}
void Audio::loadMusic() {
	Music::SoundEffectSpecs musicSoundEffectSpecs (1, Music::SoundEffectSpecs::VolumeEffect::Full, 0, 0, 0);
	Music::SoundEffectSpecs radioWavesSoundEffectSpecs (
		1,
		Music::SoundEffectSpecs::VolumeEffect::SquareDecay,
		radioWavesReverbRepetitions,
		radioWavesReverbSingleDelay,
		radioWavesReverbFalloff);
	vector<Music*> musics ({
		musicSquare = newMusic("square", Music::Waveform::Square, musicSoundEffectSpecs.withVolume(musicSquareVolume)),
		musicTriangle = newMusic("triangle", Music::Waveform::Triangle, musicSoundEffectSpecs.withVolume(musicTriangleVolume)),
		musicSaw = newMusic("saw", Music::Waveform::Saw, musicSoundEffectSpecs.withVolume(musicSawVolume)),
		musicSine = newMusic("sine", Music::Waveform::Sine, musicSoundEffectSpecs.withVolume(musicSineVolume)),
		radioWavesSoundSquare =
			newMusic("radiowaves", Music::Waveform::Square, radioWavesSoundEffectSpecs.withVolume(radioWavesSoundSquareVolume)),
		radioWavesSoundTriangle = newMusic(
			"radiowaves", Music::Waveform::Triangle, radioWavesSoundEffectSpecs.withVolume(radioWavesSoundTriangleVolume)),
		radioWavesSoundSaw =
			newMusic("radiowaves", Music::Waveform::Saw, radioWavesSoundEffectSpecs.withVolume(radioWavesSoundSawVolume)),
		radioWavesSoundSine =
			newMusic("radiowaves", Music::Waveform::Sine, radioWavesSoundEffectSpecs.withVolume(radioWavesSoundSineVolume)),
	});

	for (Music* music : musics)
		music->load();

	musicTriangle->overlay(musicSquare);
	musicSaw->overlay(musicTriangle);
	musicSine->overlay(musicSaw);
}
void Audio::unloadMusic() {
	delete musicSquare;
	delete musicTriangle;
	delete musicSaw;
	delete musicSine;
	delete radioWavesSoundSquare;
	delete radioWavesSoundTriangle;
	delete radioWavesSoundSaw;
	delete radioWavesSoundSine;
}
void Audio::pauseAll() {
	Mix_Pause(-1);
}
void Audio::resumeAll() {
	Mix_Resume(-1);
}
void Audio::stopAll() {
	Mix_HaltChannel(-1);
}
