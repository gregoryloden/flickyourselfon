#include "General.h"

class SpriteSheet;

class Animation onlyInDebug(: public ObjCounter) {
public:
	class Frame onlyInDebug(: public ObjCounter) {
	private:
		int spriteHorizontalIndex;
		int spriteVerticalIndex;
		int duration;

	public:
		Frame(objCounterParametersComma() int pSpriteHorizontalIndex, int pSpriteVerticalIndex, int pDuration);
		~Frame();

		int getSpriteHorizontalIndex() { return spriteHorizontalIndex; }
		int getSpriteVerticalIndex() { return spriteVerticalIndex; }
		int getDuration() { return duration; }
	};

private:
	SpriteSheet* sprite;
	vector<Frame*> frames;
	int* frameSearchPredecingDurations;
	int totalDuration;

public:
	Animation(objCounterParametersComma() SpriteSheet* pSprite, vector<Frame*> pFrames);
	~Animation();

private:
	Frame* findFrame(int frameNum);
public:
	void renderUsingCenterWithVerticalIndex(int frameNum, int spriteVerticalIndex, float centerX, float centerY);
};
