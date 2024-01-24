#include "HintState.h"
#include "GameState/MapState/Level.h"
#include "GameState/MapState/Rail.h"
#include "GameState/MapState/Switch.h"

//////////////////////////////// HintStateTypes::PotentialLevelState ////////////////////////////////
newInPlaceWithoutArgs(HintStateTypes::PotentialLevelState, HintStateTypes::PotentialLevelState::draftState);
int HintStateTypes::PotentialLevelState::railByteMaskCount = 0;
HintStateTypes::PotentialLevelState::PotentialLevelState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, priorState(nullptr)
, plane(nullptr)
, railByteMasks(new unsigned int[railByteMaskCount])
, railByteMasksHash(0)
, type(Type::None)
, data() {
}
HintStateTypes::PotentialLevelState::~PotentialLevelState() {
	//don't delete the prior state, it's being tracked separately
	//don't delete the currentPlane, it's owned by a Level
	delete[] railByteMasks;
}
HintStateTypes::PotentialLevelState* HintStateTypes::PotentialLevelState::produce(
	objCounterParametersComma() PotentialLevelState* pPriorState, LevelTypes::Plane* pPlane, PotentialLevelState* draftState)
{
	initializeWithNewFromPool(p, PotentialLevelState)
	p->priorState = pPriorState;
	p->plane = pPlane;
	for (int i = railByteMaskCount - 1; i >= 0; i--)
		p->railByteMasks[i] = draftState->railByteMasks[i];
	p->railByteMasksHash = draftState->railByteMasksHash;
	return p;
}
pooledReferenceCounterDefineRelease(HintStateTypes::PotentialLevelState)
void HintStateTypes::PotentialLevelState::setHash() {
	unsigned int val = 0;
	for (int i = railByteMaskCount - 1; i >= 0; i--)
		val = val ^ railByteMasks[i];
	railByteMasksHash = val;
}
bool HintStateTypes::PotentialLevelState::isNewState(vector<PotentialLevelState*>& potentialLevelStates) {
	//look through every other state, and see if it matches this one
	for (PotentialLevelState* potentialLevelState : potentialLevelStates) {
		#ifdef DEBUG
			Level::hintSearchComparisonsPerformed++;
		#endif
		//they can't be the same if their hashes don't match
		if (railByteMasksHash != potentialLevelState->railByteMasksHash)
			continue;
		for (int i = railByteMaskCount - 1; true; i--) {
			//the bytes are not the same, so the states are not the same, move on to check the next PotentialLevelState
			if (railByteMasks[i] != potentialLevelState->railByteMasks[i])
				break;
			//if we've looked at every byte and they're all the same, this state is not new
			if (i == 0)
				return false;
		}
	}

	//all states have at least one byte difference, so this is a new state
	return true;
}
HintState* HintStateTypes::PotentialLevelState::getHint() {
	PotentialLevelState* hintLevelState = this;
	#ifdef DEBUG
		Level::foundHintSearchTotalSteps = 1;
	#endif
	while (hintLevelState->priorState->priorState != nullptr) {
		hintLevelState = hintLevelState->priorState;
		#ifdef DEBUG
			Level::foundHintSearchTotalSteps++;
		#endif
	}
	return newHintState(hintLevelState->type, hintLevelState->data);
}
using namespace HintStateTypes;

//////////////////////////////// HintState ////////////////////////////////
HintState::HintState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, type(Type::None)
, data()
, animationEndTicksTime(0) {
}
HintState::~HintState() {}
HintState* HintState::produce(objCounterParametersComma() Type pType, Data pData) {
	initializeWithNewFromPool(h, HintState)
	h->type = pType;
	h->data = pData;
	//don't show it as an animation until requested
	h->animationEndTicksTime = 0;
	return h;
}
pooledReferenceCounterDefineRelease(HintState)
void HintState::render(int screenLeftWorldX, int screenTopWorldY, int ticksTime) {
	int progressTicks = ticksTime + totalDisplayTicks - animationEndTicksTime;
	bool isOn = (progressTicks % flashOnOffTotalTicks) < flashOnOffTicks;
	if (!isOn)
		return;
	float progress = (float)progressTicks / totalDisplayTicks;
	float alpha = 0.5f - (progress + progress * progress) * 0.25f;
	switch (type) {
		case HintStateTypes::Type::Plane:
			data.plane->renderHint(screenLeftWorldX, screenTopWorldY, alpha);
			break;
		case HintStateTypes::Type::Rail:
			data.rail->renderHint(screenLeftWorldX, screenTopWorldY, alpha);
			break;
		case HintStateTypes::Type::Switch:
			data.switch0->renderHint(screenLeftWorldX, screenTopWorldY, alpha);
			break;
		default:
			break;
	}
}
