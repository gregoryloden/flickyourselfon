#include "Audio.h"
#include "Util/FileUtils.h"
#include "Util/StringUtils.h"

#define newMusic(waveform, filename) newWithArgs(Music, waveform, filename)

//////////////////////////////// Audio::Music::Note ////////////////////////////////
Audio::Music::Note::Note(float pFrequency, int pBeats)
: frequency(pFrequency)
, beats(pBeats) {
}
Audio::Music::Note::~Note() {
}

//////////////////////////////// Audio::Music ////////////////////////////////
Audio::Music::Music(objCounterParametersComma() Waveform pWaveform, const char* pFilename)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
chunk()
, waveform(pWaveform)
, filename(pFilename) {
}
Audio::Music::~Music() {
	delete[] chunk.abuf;
}

//////////////////////////////// Audio ////////////////////////////////
int Audio::sampleRate = 44100;
Uint16 Audio::format = AUDIO_S16SYS;
int Audio::channels = 1;
Audio::Music* Audio::musicSquare = nullptr;
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

	musicSquare = newMusic(Waveform::Square, "square");
	vector<Music*> musics ({ musicSquare });

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
				float frequency = noteFrequencies[noteIndex * 3];
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
				}
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
		music->chunk.allocated = 1;
		music->chunk.alen = totalSampleCount * bytesPerSample;
		music->chunk.abuf = new Uint8[music->chunk.alen];
		music->chunk.volume = MIX_MAX_VOLUME;

		//finally, go through all the notes and write their samples
		int beatsProcessed = 0;
		int samplesProcessed = 0;
		for (Music::Note& note : notes) {
			int sampleStart = samplesProcessed;
			beatsProcessed += note.beats;
			samplesProcessed = (int)((float)beatsProcessed * 60 / bpm * sampleRate);
			int sampleCount = samplesProcessed - sampleStart;
			Uint8* samples = music->chunk.abuf + sampleStart * bytesPerSample;
			if (note.frequency == 0)
				memset(samples, 0, sampleCount * bytesPerSample);
			else
				writeTone(music->waveform, note.frequency, sampleCount, musicVolume, samples);
		}
	}
}
void Audio::unloadMusic() {
	delete musicSquare;
}
void Audio::writeTone(Waveform waveform, float frequency, int sampleCount, float volume, Uint8* outSamples) {
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
		if (moment < fadeInOutDuration)
			val *= moment / fadeInOutDuration;
		else if (moment > fadeOutStart)
			val *= (duration - moment) / fadeInOutDuration;
		if (bitsize == 16) {
			for (int j = 0; j < channels; j++)
				((short*)outSamples)[i * channels + j] = (short)val;
		} else if (bitsize == 8) {
			for (int j = 0; j < channels; j++)
				((char*)outSamples)[i * channels + j] = (char)val;
		} else {
			for (int j = 0; j < channels; j++)
				((int*)outSamples)[i * channels + j] = (int)val;
		}
	}
}
