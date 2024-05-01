#include "Audio.h"
#include "Util/FileUtils.h"
#include "Util/StringUtils.h"

#define newMusicWithReverb(filename, waveform, volume, volumeEffect, reverbRepetitions, reverbSingleDelay, reverbFalloff) \
	newWithArgs(Music, filename, waveform, volume, volumeEffect, reverbRepetitions, reverbSingleDelay, reverbFalloff)
#define newMusic(filename, waveform, volume) newWithArgs(Music, filename, waveform, volume)

//////////////////////////////// Audio::Music::Note ////////////////////////////////
Audio::Music::Note::Note(float pFrequency, int pBeats)
: frequency(pFrequency)
, beats(pBeats) {
}
Audio::Music::Note::~Note() {
}

//////////////////////////////// Audio::Music ////////////////////////////////
Audio::Music::Music(
	objCounterParametersComma()
	const char* pFilename,
	Waveform pWaveform,
	float pVolume,
	VolumeEffect pVolumeEffect,
	int pReverbRepetitions,
	float pReverbSingleDelay,
	float pReverbFalloff)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
filename(pFilename)
, waveform(pWaveform)
, volume(pVolume)
, volumeEffect(pVolumeEffect)
, reverbRepetitions(pReverbRepetitions)
, reverbSingleDelay(pReverbSingleDelay)
, reverbFalloff(pReverbFalloff)
, chunk() {
}
Audio::Music::Music(objCounterParametersComma() const char* pFilename, Waveform pWaveform, float pVolume)
: Music(objCounterArgumentsComma() pFilename, pWaveform, pVolume, VolumeEffect::Full, 0, 0, 0) {
}
Audio::Music::~Music() {
	delete[] chunk.abuf;
}

//////////////////////////////// Audio ////////////////////////////////
int Audio::sampleRate = 44100;
Uint16 Audio::format = AUDIO_S16SYS;
int Audio::channels = 1;
Audio::Music* Audio::musicSquare = nullptr;
Audio::Music* Audio::radioWavesSoundSquare = nullptr;
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
	//these are ordered by note name, rather than absolute frequency; they loop around at C
	constexpr float noteFrequencies[] = {
		frequencyA4, frequencyAS4, frequencyGS4,
		frequencyB4, frequencyC4, frequencyAS4,
		frequencyC4, frequencyCS4, frequencyB4,
		frequencyD4, frequencyDS4, frequencyCS4,
		frequencyE4, frequencyF4, frequencyDS4,
		frequencyF4, frequencyFS4, frequencyE4,
		frequencyG4, frequencyGS4, frequencyFS4,
	};
	int bytesPerSample = SDL_AUDIO_BITSIZE(format) / 8 * channels;

	vector<Music*> musics ({
		musicSquare = newMusic("square", Waveform::Square, musicVolume),
		radioWavesSoundSquare = newMusicWithReverb(
			"radiowaves",
			Waveform::Square,
			radioWavesVolume,
			VolumeEffect::SquareDecay,
			radioWavesReverbRepetitions,
			radioWavesReverbSingleDelay,
			radioWavesReverbFalloff),
	});

	for (Music* music : musics) {
		ifstream file;
		string path = string("audio/") + music->filename + ".smus";
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
		int totalSampleCount = (int)(totalDuration * sampleRate);
		int reverbExtraSamples = (int)(music->reverbRepetitions * music->reverbSingleDelay * sampleRate);
		music->chunk.allocated = 1;
		music->chunk.alen = (totalSampleCount + reverbExtraSamples) * bytesPerSample;
		music->chunk.abuf = new Uint8[music->chunk.alen]();
		music->chunk.volume = MIX_MAX_VOLUME;

		//finally, go through all the notes and write their samples
		int beatsProcessed = 0;
		int samplesProcessed = 0;
		for (Music::Note& note : notes) {
			int sampleStart = samplesProcessed;
			beatsProcessed += note.beats;
			samplesProcessed = (int)((float)beatsProcessed * 60 / bpm * sampleRate);
			if (note.frequency == 0)
				continue;

			int sampleCount = samplesProcessed - sampleStart;
			Uint8* samples = music->chunk.abuf + sampleStart * bytesPerSample;
			writeTone(
				music->waveform,
				music->volume,
				music->volumeEffect,
				music->reverbRepetitions,
				music->reverbSingleDelay,
				music->reverbFalloff,
				note.frequency,
				sampleCount,
				samples);
		}
	}
}
void Audio::unloadMusic() {
	delete musicSquare;
	delete radioWavesSoundSquare;
}
void Audio::writeTone(
	Waveform waveform,
	float volume,
	VolumeEffect volumeEffect,
	int reverbRepetitions,
	float reverbSingleDelay,
	float reverbFalloff,
	float frequency,
	int sampleCount,
	Uint8* outSamples)
{
	float duration = (float)sampleCount / sampleRate;
	char bitsize = (char)SDL_AUDIO_BITSIZE(format);
	float valMax = volume * ((1 << bitsize) - 1);
	float fadeOutStart = duration - fadeInOutDuration;
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
		switch (volumeEffect) {
			case VolumeEffect::SquareDecay: {
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
			for (int reverb = 0; reverb <= reverbRepetitions; reverb++) {
				int sampleOffset = i + (int)(reverb * reverbSingleDelay * sampleRate);
				for (int j = 0; j < channels; j++)
					((short*)outSamples)[sampleOffset * channels + j] += (short)val;
				val *= reverbFalloff;
			}
		} else if (bitsize == 8) {
			for (int j = 0; j < channels; j++)
				((char*)outSamples)[i * channels + j] = (char)val;
		} else {
			for (int j = 0; j < channels; j++)
				((int*)outSamples)[i * channels + j] = (int)val;
		}
	}
}
