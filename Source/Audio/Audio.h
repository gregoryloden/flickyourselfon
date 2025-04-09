#include "General/General.h"

#ifdef DEBUG
	#define WRITE_WAV_FILES
	#define ENABLE_SKIP_BEATS
#endif

namespace AudioTypes {
	class Sound onlyInDebug(: public ObjCounter) {
	protected:
		string filepath;
		Mix_Chunk* chunk;
		int channel;

	public:
		Sound(objCounterParametersComma() string pFilename, int pChannel);
		virtual ~Sound();

		//load the sound
		virtual void load();
		//play this sound, looping the given amount of additional loops (or -1 for infinite)
		void play(int loops);
		#ifdef WRITE_WAV_FILES
			//write this sound to a .wav file
			void writeWavFile();
		#endif
	};
	class Music: public Sound {
	public:
		enum class Waveform: unsigned char {
			Square,
			Triangle,
			Saw,
			Sine,
			Count,
		};
		typedef array<Music*, (int)Waveform::Count> WaveformMusicSet;
		//Should only be allocated within an object, on the stack, or as a static object
		class SoundEffectSpecs {
		public:
			enum class VolumeEffect: unsigned char {
				Full,
				SquareDecay,
				SquareInSquareOut,
			};

			float volume;
			VolumeEffect volumeEffect;
			int reverbRepetitions;
			float reverbSingleDelay;
			float reverbFalloff;

			SoundEffectSpecs(
				float pVolume,
				VolumeEffect pVolumeEffect,
				int pReverbRepetitions,
				float pReverbSingleDelay,
				float pReverbFalloff);
			virtual ~SoundEffectSpecs();

			//copy this SoundEffectSpecs with the given volume
			SoundEffectSpecs withVolume(float pVolume);
		};
	private:
		//Should only be allocated within an object, on the stack, or as a static object
		class Note {
		public:
			float frequency;
			int beats;

			Note(float pFrequency, int pBeats);
			virtual ~Note();
		};

		Waveform waveform;
		SoundEffectSpecs soundEffectSpecs;
		#ifdef ENABLE_SKIP_BEATS
			float skipBeatsDuration;
		#endif
		vector<Note> notes;

	public:
		Music(
			objCounterParametersComma()
			string pFilename,
			int pChannel,
			Waveform pWaveform,
			SoundEffectSpecs& pSoundEffectSpecs);
		virtual ~Music();

		//load the music file
		virtual void load();
		//write a tone of the given waveform and frequency, for a certain number of samples at the given volume
		//adds a short fade-in and fade-out to avoid pops
		void writeTone(float frequency, int sampleCount, Uint8* outSamples);
		//overlay the given music onto this music
		void overlay(Music* other);
		#ifdef ENABLE_SKIP_BEATS
			//start the music at a later sample
			void skipBeats();
		#endif
	};
}
class Audio {
private:
	static constexpr int musicChannel = 0;
public:
	static constexpr int stepSoundsCount = 6;
	static constexpr int rideRailSoundsCount = 6;
	static constexpr int rideRailOutSoundsCount = 3;

	static int sampleRate;
	static Uint16 format;
	static int channels;
	static AudioTypes::Music::WaveformMusicSet musics;
	static AudioTypes::Music::WaveformMusicSet radioWavesSounds;
	static AudioTypes::Music::WaveformMusicSet switchesFadeInSounds;
	static AudioTypes::Music::WaveformMusicSet railSwitchWavesSounds;
	static AudioTypes::Music::WaveformMusicSet resetSwitchWavesSounds;
	static AudioTypes::Music* victorySound;
	static array<AudioTypes::Sound*, stepSoundsCount> stepSounds;
	static AudioTypes::Sound* climbSound;
	static AudioTypes::Sound* jumpSound;
	static AudioTypes::Sound* landSound;
	static AudioTypes::Sound* kickSound;
	static AudioTypes::Sound* kickAirSound;
	static AudioTypes::Sound* switchOnSound;
	static AudioTypes::Sound* stepOnRailSound;
	static AudioTypes::Sound* stepOffRailSound;
	static array<AudioTypes::Sound*, rideRailSoundsCount> rideRailSounds;
	static array<AudioTypes::Sound*, rideRailOutSoundsCount> rideRailOutSounds;
	static AudioTypes::Sound* railSlideSound;
	static AudioTypes::Sound* railSlideSquareSound;
	static AudioTypes::Sound* selectSound;
	static AudioTypes::Sound* confirmSound;

	//Prevent allocation
	Audio() = delete;
	//prep everything so that we can start playing audio
	static void setUp();
	//stop audio, but don't complete cleanup just yet
	static void stopAudio();
	//completely clean up all audio handling
	static void tearDown();
	//load the sound files
	static void loadSounds();
private:
	//load the same music for each waveform
	static void loadWaveformMusicSet(
		const char* prefix,
		bool includeSuffix,
		int channel,
		AudioTypes::Music::SoundEffectSpecs soundEffectSpecs,
		array<float, (int)AudioTypes::Music::Waveform::Count> volumes,
		AudioTypes::Music::WaveformMusicSet& waveformMusicSet);
	//load multiple sound files with the same name prefix
	template <int count> static void loadSoundSet(const char* prefix, array<AudioTypes::Sound*, count>& soundSet);
public:
	//clean up the sound files
	static void unloadSounds();
private:
	//clean up musics from a music set
	static void unloadWaveformMusicSet(AudioTypes::Music::WaveformMusicSet& waveformMusicSet);
	//clean up sounds from a sound set
	template <int count> static void unloadSoundSet(array<AudioTypes::Sound*, count>& soundSet);
public:
	//apply audio volume settings for all channels
	static void applyVolume();
	//apply audio volume settings for a single channel
	static void applyChannelVolume(int channel);
	//pause all audio tracks
	static void pauseAll();
	//resume all audio tracks
	static void resumeAll();
	//fade out all audio tracks
	static void fadeOutAll(int ms);
	//clear all currently playing audio tracks
	static void stopAll();
};
