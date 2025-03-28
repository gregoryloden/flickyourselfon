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
		virtual ~Frame();

		int getSpriteHorizontalIndex() { return spriteHorizontalIndex; }
		int getSpriteVerticalIndex() { return spriteVerticalIndex; }
		int getTicksDuration() { return ticksDuration; }
	};

	static constexpr int absentSpriteIndex = -1;

private:
	SpriteSheet* sprite;
	vector<Frame*> frames;
	int* frameSearchPredecingTicksDurations;
	int totalTicksDuration;
	bool loopAnimation;

public:
	SpriteAnimation(objCounterParametersComma() SpriteSheet* pSprite, vector<Frame*> pFrames);
	virtual ~SpriteAnimation();

	int getTotalTicksDuration() { return totalTicksDuration; }
	void disableLooping() { loopAnimation = false; }
private:
	//binary search for the referenced frame
	Frame* findFrame(int animationTicksElapsed);
public:
	//render the appropriate sprite for this frame, using the given center
	void renderUsingCenter(
		float centerX,
		float centerY,
		int animationTicksElapsed,
		int fallbackSpriteHorizontalIndex,
		int fallbackSpriteVerticalIndex);
};
