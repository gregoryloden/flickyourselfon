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
filepath("audio/" + pFilename)
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
		char chunk1Header[] {
			//constant
			'f', 'm', 't', ' ',
			//chunk 1 size, 16 bytes
			16, 0, 0, 0,
		};
		char chunk1[] {
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
		char chunk2Header[] {
			//constant
			'd', 'a', 't', 'a',
			//chunk 2 size
			(char)chunk->alen, (char)(chunk->alen >> 8), (char)(chunk->alen >> 16), (char)(chunk->alen >> 24),
		};
		int chunkSizes = sizeof(chunk1Header) + sizeof(chunk1) + sizeof(chunk2Header) + chunk->alen;
		char header[] {
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
	static constexpr float frequencyA4 = 440.0f;
	static constexpr float frequencyC4 = frequencyA4 / 2 * (float)MathUtils::powConst(2.0, 3.0 / 12.0);
	static constexpr float frequencyCS4 = frequencyA4 / 2 * (float)MathUtils::powConst(2.0, 4.0 / 12.0);
	static constexpr float frequencyD4 = frequencyA4 / 2 * (float)MathUtils::powConst(2.0, 5.0 / 12.0);
	static constexpr float frequencyDS4 = frequencyA4 / 2 * (float)MathUtils::powConst(2.0, 6.0 / 12.0);
	static constexpr float frequencyE4 = frequencyA4 / 2 * (float)MathUtils::powConst(2.0, 7.0 / 12.0);
	static constexpr float frequencyF4 = frequencyA4 / 2 * (float)MathUtils::powConst(2.0, 8.0 / 12.0);
	static constexpr float frequencyFS4 = frequencyA4 / 2 * (float)MathUtils::powConst(2.0, 9.0 / 12.0);
	static constexpr float frequencyG4 = frequencyA4 / 2 * (float)MathUtils::powConst(2.0, 10.0 / 12.0);
	static constexpr float frequencyGS4 = frequencyA4 / 2 * (float)MathUtils::powConst(2.0, 11.0 / 12.0);
	static constexpr float frequencyAS4 = frequencyA4 * (float)MathUtils::powConst(2.0, 1.0 / 12.0);
	static constexpr float frequencyB4 = frequencyA4 * (float)MathUtils::powConst(2.0, 2.0 / 12.0);
	//these are ordered by note name, rather than absolute frequency; they loop around at C
	static constexpr float noteFrequencies[][3] {
		{ frequencyA4, frequencyAS4, frequencyGS4 },
		{ frequencyB4, frequencyC4, frequencyAS4 },
		{ frequencyC4, frequencyCS4, frequencyB4 },
		{ frequencyD4, frequencyDS4, frequencyCS4 },
		{ frequencyE4, frequencyF4, frequencyDS4 },
		{ frequencyF4, frequencyFS4, frequencyE4 },
		{ frequencyG4, frequencyGS4, frequencyFS4 },
	};

	ifstream file;
	FileUtils::openFileForRead(&file, filepath.c_str(), FileUtils::FileReadLocation::Installation);
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
				notes.push_back(Note(0, 1));
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
				frequency = noteFrequencies[noteIndex][1];
				nextNotes++;
				c = *nextNotes;
			} else if (c == 'b') {
				frequency = noteFrequencies[noteIndex][2];
				nextNotes++;
				c = *nextNotes;
			} else
				frequency = noteFrequencies[noteIndex][0];
			if (c != '4')
				frequency *= pow(2.0f, c - '4');
			notes.push_back(Note(frequency, 1));
		}
		nextNotes++;
	}

	//calculate the duration of the track
	int totalBeats = 0;
	for (Note& note : notes)
		totalBeats += note.beats;
	float totalDuration = (float)totalBeats * 60 / bpm;

	//allocate the samples
	int bytesPerSample = SDL_AUDIO_BITSIZE(Audio::format) / 8 * Audio::channels;
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
	for (Note& note : notes) {
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
	static constexpr float fadeInOutDuration = 1.0f / 256.0f;
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
Music::WaveformMusicSet Audio::musics;
Music::WaveformMusicSet Audio::radioWavesSounds;
Music::WaveformMusicSet Audio::switchesFadeInSounds;
Music::WaveformMusicSet Audio::railSwitchWavesSounds;
Music* Audio::victorySound = nullptr;
Sound* Audio::stepSounds[Audio::stepSoundsCount] {};
Sound* Audio::climbSound = nullptr;
Sound* Audio::jumpSound = nullptr;
Sound* Audio::landSound = nullptr;
Sound* Audio::kickSound = nullptr;
Sound* Audio::kickAirSound = nullptr;
Sound* Audio::switchOnSound = nullptr;
Sound* Audio::stepOnRailSound = nullptr;
Sound* Audio::stepOffRailSound = nullptr;
Sound* Audio::rideRailSounds[Audio::rideRailSoundsCount] {};
Sound* Audio::rideRailOutSounds[Audio::rideRailOutSoundsCount] {};
Sound* Audio::railSlideSound = nullptr;
Sound* Audio::railSlideSquareSound = nullptr;
Sound* Audio::selectSound = nullptr;
Sound* Audio::confirmSound = nullptr;
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
	static constexpr float musicSquareVolume = 3.0f / 64.0f;
	static constexpr float musicTriangleVolume = 13.5f / 64.0f;
	static constexpr float musicSawVolume = 3.0f / 64.0f;
	static constexpr float musicSineVolume = 13.5f / 64.0f;
	static constexpr float radioWavesSoundSquareVolume = 3.0f / 64.0f;
	static constexpr float radioWavesSoundTriangleVolume = 12.0f / 64.0f;
	static constexpr float radioWavesSoundSawVolume = 3.5f / 64.0f;
	static constexpr float radioWavesSoundSineVolume = 21.0f / 64.0f;
	static constexpr int radioWavesReverbRepetitions = 8;
	static constexpr float radioWavesReverbSingleDelay = 1.0f / 32.0f;
	static constexpr float radioWavesReverbFalloff = 3.0f / 8.0f;
	static constexpr float railSwitchWavesSoundSquareVolume = 2.0f / 64.0f;
	static constexpr float railSwitchWavesSoundTriangleVolume = 9.0f / 64.0f;
	static constexpr float railSwitchWavesSoundSawVolume = 3.0f / 64.0f;
	static constexpr float railSwitchWavesSoundSineVolume = 8.0f / 64.0f;
	static constexpr int railSwitchWavesReverbRepetitions = 16;
	static constexpr float railSwitchWavesReverbSingleDelay = 1.0f / 32.0f;
	static constexpr float railSwitchWavesReverbFalloff = 1.5f / 8.0f;
	static constexpr float victorySoundSquareVolume = 2.5f / 64.0f;
	static constexpr float victorySoundTriangleVolume = 12.0f / 64.0f;
	static constexpr float victorySoundSawVolume = 3.5f / 64.0f;
	static constexpr float victorySoundSineVolume = 13.5f / 64.0f;

	Music::SoundEffectSpecs musicSoundEffectSpecs (1, Music::SoundEffectSpecs::VolumeEffect::Full, 0, 0, 0);
	Music::SoundEffectSpecs radioWavesSoundEffectSpecs (
		1,
		Music::SoundEffectSpecs::VolumeEffect::SquareDecay,
		radioWavesReverbRepetitions,
		radioWavesReverbSingleDelay,
		radioWavesReverbFalloff);
	Music::SoundEffectSpecs switchesFadeInSoundEffectSpecs (
		1, Music::SoundEffectSpecs::VolumeEffect::SquareInSquareOut, 0, 0, 0);
	Music::SoundEffectSpecs railSwitchWavesSoundEffectSpecs (
		1,
		Music::SoundEffectSpecs::VolumeEffect::SquareDecay,
		railSwitchWavesReverbRepetitions,
		railSwitchWavesReverbSingleDelay,
		railSwitchWavesReverbFalloff);
	Music::WaveformMusicSet victorySounds;
	vector<Sound*> sounds ({
		climbSound = newSound("climb.wav", -1),
		jumpSound = newSound("jump.wav", -1),
		landSound = newSound("land.wav", -1),
		kickSound = newSound("kick.wav", -1),
		kickAirSound = newSound("kick air.wav", -1),
		switchOnSound = newSound("switch on.wav", -1),
		stepOnRailSound = newSound("step on rail.wav", -1),
		stepOffRailSound = newSound("step off rail.wav", -1),
		railSlideSound = newSound("rail slide.wav", -1),
		railSlideSquareSound = newSound("rail slide square.wav", -1),
		selectSound = newSound("select.wav", -1),
		confirmSound = newSound("confirm.wav", -1),
	});

	for (Sound* sound : sounds)
		sound->load();

	loadWaveformMusicSet(
		"music",
		true,
		musicChannel,
		musicSoundEffectSpecs,
		{ musicSquareVolume, musicTriangleVolume, musicSawVolume, musicSineVolume },
		musics);
	loadWaveformMusicSet(
		"radiowaves",
		false,
		-1,
		radioWavesSoundEffectSpecs,
		{ radioWavesSoundSquareVolume, radioWavesSoundTriangleVolume, radioWavesSoundSawVolume, radioWavesSoundSineVolume },
		radioWavesSounds);
	loadWaveformMusicSet(
		"switchesfadein",
		false,
		-1,
		switchesFadeInSoundEffectSpecs,
		{ musicSquareVolume, musicTriangleVolume, musicSawVolume, musicSineVolume },
		switchesFadeInSounds);
	loadWaveformMusicSet(
		"railswitchwaves",
		false,
		-1,
		railSwitchWavesSoundEffectSpecs,
		{
			railSwitchWavesSoundSquareVolume,
			railSwitchWavesSoundTriangleVolume,
			railSwitchWavesSoundSawVolume,
			railSwitchWavesSoundSineVolume,
		},
		railSwitchWavesSounds);
	loadWaveformMusicSet(
		"victory",
		true,
		-1,
		musicSoundEffectSpecs,
		{ victorySoundSquareVolume, victorySoundTriangleVolume, victorySoundSawVolume, victorySoundSineVolume },
		victorySounds);

	loadSoundSet("step", stepSoundsCount, stepSounds);
	loadSoundSet("ride rail", rideRailSoundsCount, rideRailSounds);
	loadSoundSet("ride rail out", rideRailOutSoundsCount, rideRailOutSounds);

	musics[(int)Music::Waveform::Triangle]->overlay(musics[(int)Music::Waveform::Square]);
	musics[(int)Music::Waveform::Saw]->overlay(musics[(int)Music::Waveform::Triangle]);
	musics[(int)Music::Waveform::Sine]->overlay(musics[(int)Music::Waveform::Saw]);
	victorySound = victorySounds[(int)Music::Waveform::Square];
	victorySound->overlay(victorySounds[(int)Music::Waveform::Triangle]);
	victorySound->overlay(victorySounds[(int)Music::Waveform::Saw]);
	victorySound->overlay(victorySounds[(int)Music::Waveform::Sine]);
	delete victorySounds[(int)Music::Waveform::Triangle];
	delete victorySounds[(int)Music::Waveform::Saw];
	delete victorySounds[(int)Music::Waveform::Sine];
	#ifdef ENABLE_SKIP_BEATS
		musics[(int)Music::Waveform::Square]->skipBeats();
		musics[(int)Music::Waveform::Triangle]->skipBeats();
		musics[(int)Music::Waveform::Saw]->skipBeats();
		musics[(int)Music::Waveform::Sine]->skipBeats();
	#endif
}
void Audio::loadWaveformMusicSet(
	const char* prefix,
	bool includeSuffix,
	int channel,
	Music::SoundEffectSpecs soundEffectSpecs,
	array<float, (int)Music::Waveform::Count> volumes,
	Music::WaveformMusicSet& waveformMusicSet)
{
	static constexpr char* suffixes[] = { "square", "triangle", "saw", "sine" };
	for (int i = 0; i < (int)Music::Waveform::Count; i++) {
		string filename = string(prefix) + (includeSuffix ? suffixes[i] : "");
		waveformMusicSet[i] = newMusic(filename, channel, (Music::Waveform)i, soundEffectSpecs.withVolume(volumes[i]));
		waveformMusicSet[i]->load();
	}
}
void Audio::loadSoundSet(const char* prefix, int count, Sound** soundSet) {
	for (int i = 0; i < count; i++) {
		soundSet[i] = newSound(prefix + to_string(i + 1) + ".wav", -1);
		soundSet[i]->load();
	}
}
void Audio::unloadSounds() {
	unloadWaveformMusicSet(musics);
	unloadWaveformMusicSet(radioWavesSounds);
	unloadWaveformMusicSet(switchesFadeInSounds);
	unloadWaveformMusicSet(railSwitchWavesSounds);
	delete victorySound;
	unloadSoundSet(stepSoundsCount, stepSounds);
	delete climbSound;
	delete jumpSound;
	delete landSound;
	delete kickSound;
	delete kickAirSound;
	delete switchOnSound;
	delete stepOnRailSound;
	delete stepOffRailSound;
	unloadSoundSet(rideRailSoundsCount, rideRailSounds);
	unloadSoundSet(rideRailOutSoundsCount, rideRailOutSounds);
	delete railSlideSound;
	delete railSlideSquareSound;
	delete selectSound;
	delete confirmSound;
}
void Audio::unloadWaveformMusicSet(Music::WaveformMusicSet& waveformMusicSet) {
	for (int i = 0; i < (int)Music::Waveform::Count; i++)
		delete waveformMusicSet[i];
}
void Audio::unloadSoundSet(int count, Sound** soundSet) {
	for (int i = 0; i < count; i++)
		delete soundSet[i];
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
