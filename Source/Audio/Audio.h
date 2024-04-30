#include "General/General.h"

class Audio {
public:
	enum class Waveform: unsigned char {
		Square,
		Triangle,
		Saw,
		Sine,
	};

private:
	static constexpr float fadeInOutDuration = 1.0f / 256.0f;

	static int sampleRate;
	static Uint16 format;
	static int channels;

public:
	//Prevent allocation
	Audio() = delete;
	static void setUp();
	static void tearDown();
	static void generateTone(Waveform waveform, float frequency, float duration, float volume, Mix_Chunk* outChunk);
};
