#include "General/General.h"

class Audio {
public:
	enum class Waveform: unsigned char {
		Square,
		Triangle,
		Saw,
		Sine,
	};
	class Music onlyInDebug(: public ObjCounter) {
	public:
		//Should only be allocated within an object, on the stack, or as a static object
		class Note {
		public:
			float frequency;
			int beats;

			Note(float pFrequency, int pBeats);
			virtual ~Note();
		};

		const char* filename;
		Waveform waveform;
		float volume;
		Mix_Chunk chunk;

		Music(objCounterParametersComma() const char* pFilename, Waveform pWaveform, float pVolume);
		virtual ~Music();
	};

private:
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
	static constexpr float musicVolume = 1.0f / 64.0f;

	static int sampleRate;
	static Uint16 format;
	static int channels;
	static Music* musicSquare;

public:
	//Prevent allocation
	Audio() = delete;
	//prep everything so that we can start playing audio
	static void setUp();
	//stop audio, but don't complete cleanup just yet
	static void stopAudio();
	//completely clean up all audio handling
	static void tearDown();
	//load the music files
	static void loadMusic();
	//clean up the music files
	static void unloadMusic();
	//write a tone of the given waveform and frequency, for a certain number of samples at the given volume
	//adds a short fade-in and fade-out to avoid pops
	static void writeTone(Waveform waveform, float frequency, int sampleCount, float volume, Uint8* outSamples);
};
