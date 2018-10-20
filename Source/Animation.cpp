#include "Animation.h"
#include "SpriteSheet.h"

Animation::Frame::Frame(objCounterParametersComma() int pSpriteHorizontalIndex, int pSpriteVerticalIndex, int pTicksDuration)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
spriteHorizontalIndex(pSpriteHorizontalIndex)
, spriteVerticalIndex(pSpriteVerticalIndex)
, ticksDuration(pTicksDuration) {
}
Animation::Frame::~Frame() {}
Animation::Animation(objCounterParametersComma() SpriteSheet* pSprite, vector<Frame*> pFrames)
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
Animation::~Animation() {
	for (Frame* frame : frames) {
		delete frame;
	}
	delete[] frameSearchPredecingTicksDurations;
}
//binary search for the referenced frame
Animation::Frame* Animation::findFrame(int animationTicksDuration) {
	animationTicksDuration %= totalTicksDuration;
	int low = 0;
	int high = frames.size();
	while (low + 1 < high) {
		int mid = (low + high) / 2;
		//this frame hasn't happened yet
		if (frameSearchPredecingTicksDurations[mid] > animationTicksDuration)
			high = mid;
		else
			low = mid;
	}
	return frames[low];
}
void Animation::renderUsingCenterWithVerticalIndex(
	int animationTicksDuration, int spriteVerticalIndex, float centerX, float centerY)
{
	sprite->renderUsingCenter(
		findFrame(animationTicksDuration)->getSpriteHorizontalIndex(), spriteVerticalIndex, centerX, centerY);
}
