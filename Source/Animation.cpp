#include "Animation.h"
#include "SpriteSheet.h"

Animation::Frame::Frame(objCounterParametersComma() int pSpriteHorizontalIndex, int pSpriteVerticalIndex, int pDuration)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
spriteHorizontalIndex(pSpriteHorizontalIndex)
, spriteVerticalIndex(pSpriteVerticalIndex)
, duration(pDuration) {
}
Animation::Frame::~Frame() {}
Animation::Animation(objCounterParametersComma() SpriteSheet* pSprite, vector<Frame*> pFrames)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
sprite(pSprite)
, frames(pFrames)
, frameSearchPredecingDurations(new int[pFrames.size()])
, totalDuration(0) {
	int frameCount = pFrames.size();
	int frameSearchPredecingDuration = 0;
	for (int i = 0; i < frameCount; i++) {
		frameSearchPredecingDurations[i] = frameSearchPredecingDuration;
		frameSearchPredecingDuration += pFrames[i]->getDuration();
	}
	totalDuration = frameSearchPredecingDuration;
}
Animation::~Animation() {
	for (Frame* frame : frames) {
		delete frame;
	}
	delete[] frameSearchPredecingDurations;
}
//binary search for the referenced frame
Animation::Frame* Animation::findFrame(int frameNum) {
	frameNum %= totalDuration;
	int low = 0;
	int high = frames.size();
	while (low + 1 < high) {
		int mid = (low + high) / 2;
		//this frame hasn't happened yet
		if (frameSearchPredecingDurations[mid] > frameNum)
			high = mid;
		else
			low = mid;
	}
	return frames[low];
}
void Animation::renderUsingCenterWithVerticalIndex(int frameNum, int spriteVerticalIndex, float centerX, float centerY) {
	sprite->renderUsingCenter(findFrame(frameNum)->getSpriteHorizontalIndex(), spriteVerticalIndex, centerX, centerY);
}
