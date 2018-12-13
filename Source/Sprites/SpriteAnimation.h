#include "General/General.h"

#define newSpriteAnimation(sprite, frames) newWithArgs(SpriteAnimation, sprite, frames)
#define newSpriteAnimationFrame(spriteHorizontalIndex, spriteVerticalIndex, ticksDuration) \
	newWithArgs(SpriteAnimation::Frame, spriteHorizontalIndex, spriteVerticalIndex, ticksDuration)

class SpriteSheet;

class SpriteAnimation onlyInDebug(: public ObjCounter) {
public:
	class Frame onlyInDebug(: public ObjCounter) {
	private:
		int spriteHorizontalIndex;
		int spriteVerticalIndex;
		int ticksDuration;

	public:
		Frame(objCounterParametersComma() int pSpriteHorizontalIndex, int pSpriteVerticalIndex, int pTicksDuration);
		~Frame();

		int getSpriteHorizontalIndex() { return spriteHorizontalIndex; }
		int getSpriteVerticalIndex() { return spriteVerticalIndex; }
		int getTicksDuration() { return ticksDuration; }
	};

private:
	SpriteSheet* sprite;
	vector<Frame*> frames;
	int* frameSearchPredecingTicksDurations;
	int totalTicksDuration;

public:
	SpriteAnimation(objCounterParametersComma() SpriteSheet* pSprite, vector<Frame*> pFrames);
	~SpriteAnimation();

	int getTotalTicksDuration() { return totalTicksDuration; }
private:
	Frame* findFrame(int animationTicksElapsed);
public:
	void renderUsingCenter(
		float centerX,
		float centerY,
		int animationTicksElapsed,
		int fallbackSpriteHorizontalIndex,
		int fallbackSpriteVerticalIndex);
};
