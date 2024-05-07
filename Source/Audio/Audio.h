#include "General/General.h"

namespace AudioTypes {
	class Sound onlyInDebug(: public ObjCounter) {
	protected:
		string filepath;
		Mix_Chunk* chunk;

	public:
		Sound(objCounterParametersComma() string pFilename);
		virtual ~Sound();

		//load the sound
		virtual void load();
		//play this sound, looping the given amount of additional loops
		void play(int loops);
	};
	class Music: public Sound {
	public:
		enum class Waveform : unsigned char {
			Square,
			Triangle,
			Saw,
			Sine,
		};
		//Should only be allocated within an object, on the stack, or as a static object
		class SoundEffectSpecs {
		public:
			enum class VolumeEffect : unsigned char {
				Full,
				SquareDecay,
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

		static constexpr float fadeInOutDuration = 1.0f / 256.0f;
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

		Waveform waveform;
		SoundEffectSpecs soundEffectSpecs;

	public:
		Music(
			objCounterParametersComma()
			string pFilename,
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
	};
}
class Audio {
private:
	static constexpr float musicSquareVolume = 3.0f / 256.0f;
	static constexpr float musicTriangleVolume = 13.5f / 256.0f;
	static constexpr float musicSawVolume = 3.0f / 256.0f;
	static constexpr float musicSineVolume = 13.5f / 256.0f;
	static constexpr float radioWavesSoundSquareVolume = 3.0f / 256.0f;
	static constexpr float radioWavesSoundTriangleVolume = 12.0f / 256.0f;
	static constexpr float radioWavesSoundSawVolume = 3.5f / 256.0f;
	static constexpr float radioWavesSoundSineVolume = 21.0f / 256.0f;
	static constexpr int radioWavesReverbRepetitions = 8;
	static constexpr float radioWavesReverbSingleDelay = 1.0f / 32.0f;
	static constexpr float radioWavesReverbFalloff = 3.0f / 8.0f;

public:
	static int sampleRate;
	static Uint16 format;
	static int channels;
	static AudioTypes::Music* musicSquare;
	static AudioTypes::Music* musicTriangle;
	static AudioTypes::Music* musicSaw;
	static AudioTypes::Music* musicSine;
	static AudioTypes::Music* radioWavesSoundSquare;
	static AudioTypes::Music* radioWavesSoundTriangle;
	static AudioTypes::Music* radioWavesSoundSaw;
	static AudioTypes::Music* radioWavesSoundSine;

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
	//clean up the sound files
	static void unloadSounds();
	//pause all audio tracks
	static void pauseAll();
	//resume all audio tracks
	static void resumeAll();
	//fade out all audio tracks
	static void fadeOutAll(int ms);
	//clear all currently playing audio tracks
	static void stopAll();
};
