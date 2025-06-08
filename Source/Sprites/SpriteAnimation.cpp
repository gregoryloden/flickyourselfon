#include "SpriteAnimation.h"
#include "Sprites/SpriteSheet.h"

//////////////////////////////// SpriteAnimation::Frame ////////////////////////////////
SpriteAnimation::Frame::Frame(
	objCounterParametersComma() int pSpriteHorizontalIndex, int pSpriteVerticalIndex, int pTicksDuration)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
spriteHorizontalIndex(pSpriteHorizontalIndex)
, spriteVerticalIndex(pSpriteVerticalIndex)
, ticksDuration(pTicksDuration) {
}
SpriteAnimation::Frame::~Frame() {}

//////////////////////////////// SpriteAnimation ////////////////////////////////
SpriteAnimation::SpriteAnimation(objCounterParametersComma() SpriteSheet* pSprite, vector<Frame*> pFrames)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
sprite(pSprite)
, frames(pFrames)
, frameSearchPredecingTicksDurations(new int[pFrames.size()])
, totalTicksDuration(0)
, loopAnimation(true) {
	int frameSearchPredecingTicksDuration = 0;
	for (int i = 0; i < (int)pFrames.size(); i++) {
		frameSearchPredecingTicksDurations[i] = frameSearchPredecingTicksDuration;
		frameSearchPredecingTicksDuration += pFrames[i]->getTicksDuration();
	}
	totalTicksDuration = frameSearchPredecingTicksDuration;
}
SpriteAnimation::~SpriteAnimation() {
	//don't delete the sprite, it's stored elsewhere
	for (Frame* frame : frames)
		delete frame;
	delete[] frameSearchPredecingTicksDurations;
}
SpriteAnimation::Frame* SpriteAnimation::findFrame(int animationTicksElapsed) {
	//if we aren't looping the animation, stop at the last frame
	if (animationTicksElapsed >= totalTicksDuration && !loopAnimation)
		return frames.back();

	animationTicksElapsed %= totalTicksDuration;
	int low = 0;
	int high = (int)frames.size();
	while (low + 1 < high) {
		int mid = (low + high) / 2;
		//this frame hasn't happened yet
		if (frameSearchPredecingTicksDurations[mid] > animationTicksElapsed)
			high = mid;
		else
			low = mid;
	}
	return frames[low];
}
void SpriteAnimation::renderUsingCenter(
	float centerX, float centerY, int animationTicksElapsed, int fallbackSpriteHorizontalIndex, int fallbackSpriteVerticalIndex)
{
	Frame* frame = findFrame(animationTicksElapsed);
	int spriteHoritontalIndex = frame->getSpriteHorizontalIndex();
	int spriteVerticalIndex = frame->getSpriteVerticalIndex();
	if (spriteHoritontalIndex == absentSpriteIndex)
		spriteHoritontalIndex = fallbackSpriteHorizontalIndex;
	if (spriteVerticalIndex == absentSpriteIndex)
		spriteVerticalIndex = fallbackSpriteVerticalIndex;
	sprite->renderSpriteCenteredAtScreenPosition(spriteHoritontalIndex, spriteVerticalIndex, centerX, centerY);
}
