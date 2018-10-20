#include "General.h"

class SpriteSheet;

class Animation onlyInDebug(: public ObjCounter) {
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
	Animation(objCounterParametersComma() SpriteSheet* pSprite, vector<Frame*> pFrames);
	~Animation();

private:
	Frame* findFrame(int animationTicksDuration);
public:
	void renderUsingCenterWithVerticalIndex(int animationTicksDuration, int spriteVerticalIndex, float centerX, float centerY);
};
