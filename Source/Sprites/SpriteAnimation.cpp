#include "SpriteAnimation.h"
#include "Sprites/SpriteSheet.h"

SpriteAnimation::Frame::Frame(objCounterParametersComma() int pSpriteHorizontalIndex, int pSpriteVerticalIndex, int pTicksDuration)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
spriteHorizontalIndex(pSpriteHorizontalIndex)
, spriteVerticalIndex(pSpriteVerticalIndex)
, ticksDuration(pTicksDuration) {
}
SpriteAnimation::Frame::~Frame() {}
SpriteAnimation::SpriteAnimation(objCounterParametersComma() SpriteSheet* pSprite, vector<Frame*> pFrames)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
sprite(pSprite)
, frames(pFrames)
, frameSearchPredecingTicksDurations(new int[pFrames.size()])
, totalTicksDuration(0) {
	int frameCount = pFrames.size();
	int frameSearchPredecingTicksDuration = 0;
	for (int i = 0; i < frameCount; i++) {
		frameSearchPredecingTicksDurations[i] = frameSearchPredecingTicksDuration;
		frameSearchPredecingTicksDuration += pFrames[i]->getTicksDuration();
	}
	totalTicksDuration = frameSearchPredecingTicksDuration;
}
SpriteAnimation::~SpriteAnimation() {
	for (Frame* frame : frames) {
		delete frame;
	}
	delete[] frameSearchPredecingTicksDurations;
}
//binary search for the referenced frame
SpriteAnimation::Frame* SpriteAnimation::findFrame(int animationTicksElapsed) {
	animationTicksElapsed %= totalTicksDuration;
	int low = 0;
	int high = frames.size();
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
	if (spriteHoritontalIndex == -1)
		spriteHoritontalIndex = fallbackSpriteHorizontalIndex;
	if (spriteVerticalIndex == -1)
		spriteVerticalIndex = fallbackSpriteVerticalIndex;
	sprite->renderUsingCenter(centerX, centerY, spriteHoritontalIndex, spriteVerticalIndex);
}
