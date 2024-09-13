#include "HintState.h"
#include "GameState/MapState/Level.h"
#include "GameState/MapState/Rail.h"
#include "GameState/MapState/Switch.h"

//////////////////////////////// Hint ////////////////////////////////
Hint Hint::none (Hint::Type::None, 0);
Hint Hint::undoReset (Hint::Type::UndoReset, 0);
Hint::Hint(Type pType, int pLevelN)
: type(pType)
, data()
, levelN(pLevelN) {
}
Hint::~Hint() {
	//don't delete anything in data, it's owned by something else
}

//////////////////////////////// HintState::PotentialLevelState ////////////////////////////////
newInPlaceWithoutArgs(HintState::PotentialLevelState, HintState::PotentialLevelState::draftState);
int HintState::PotentialLevelState::maxRailByteMaskCount = 0;
int HintState::PotentialLevelState::currentRailByteMaskCount = 0;
HintState::PotentialLevelState::PotentialLevelState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, priorState(nullptr)
, plane(nullptr)
, railByteMasks(new unsigned int[maxRailByteMaskCount])
, railByteMasksHash(0)
, hint(nullptr) {
}
HintState::PotentialLevelState::~PotentialLevelState() {
	//don't delete the prior state, it's being tracked separately
	//don't delete the plane, it's owned by a Level
	delete[] railByteMasks;
	//don't delete the hint, something else owns it
}
HintState::PotentialLevelState* HintState::PotentialLevelState::produce(
	objCounterParametersComma()
	PotentialLevelState* pPriorState,
	LevelTypes::Plane* pPlane,
	PotentialLevelState* pDraftState,
	Hint* pHint)
{
	initializeWithNewFromPool(p, PotentialLevelState)
	p->priorState = pPriorState;
	p->plane = pPlane;
	for (int i = currentRailByteMaskCount - 1; i >= 0; i--)
		p->railByteMasks[i] = pDraftState->railByteMasks[i];
	p->railByteMasksHash = pDraftState->railByteMasksHash;
	p->hint = pHint;
	return p;
}
pooledReferenceCounterDefineRelease(HintState::PotentialLevelState)
void HintState::PotentialLevelState::setHash() {
	unsigned int val = 0;
	for (int i = currentRailByteMaskCount - 1; i >= 0; i--)
		val = val ^ railByteMasks[i];
	railByteMasksHash = val;
}
bool HintState::PotentialLevelState::isNewState(vector<PotentialLevelState*>& potentialLevelStates) {
	//look through every other state, and see if it matches this one
	for (PotentialLevelState* potentialLevelState : potentialLevelStates) {
		#ifdef TRACK_HINT_SEARCH_STATS
			Level::hintSearchComparisonsPerformed++;
		#endif
		//they can't be the same if their hashes don't match
		if (railByteMasksHash != potentialLevelState->railByteMasksHash)
			continue;
		for (int i = currentRailByteMaskCount - 1; true; i--) {
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
Hint* HintState::PotentialLevelState::getHint() {
	PotentialLevelState* hintLevelState = this;
	#ifdef TRACK_HINT_SEARCH_STATS
		Level::foundHintSearchTotalSteps = 1;
	#endif
	while (hintLevelState->priorState->priorState != nullptr) {
		hintLevelState = hintLevelState->priorState;
		#ifdef TRACK_HINT_SEARCH_STATS
			Level::foundHintSearchTotalSteps++;
		#endif
	}
	return hintLevelState->hint;
}

//////////////////////////////// HintState ////////////////////////////////
HintState::HintState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, hint(nullptr)
, animationEndTicksTime(0) {
}
HintState::~HintState() {
	//don't delete the hint, something else owns it
}
HintState* HintState::produce(objCounterParametersComma() Hint* pHint, int animationStartTicksTime) {
	initializeWithNewFromPool(h, HintState)
	h->hint = pHint;
	h->animationEndTicksTime = animationStartTicksTime + totalDisplayTicks;
	return h;
}
pooledReferenceCounterDefineRelease(HintState)
void HintState::render(int screenLeftWorldX, int screenTopWorldY, bool belowRails, int ticksTime) {
	if (ticksTime >= animationEndTicksTime
			//only render planes below rails, and only rails and switches above rails
			|| (belowRails
				? hint->type != Hint::Type::Plane
				: hint->type != Hint::Type::Rail && hint->type != Hint::Type::Switch))
		return;
	int progressTicks = ticksTime + totalDisplayTicks - animationEndTicksTime;
	bool isOn = (progressTicks % flashOnOffTotalTicks) < flashOnOffTicks;
	if (!isOn)
		return;
	float progress = (float)progressTicks / totalDisplayTicks;
	float alpha = 0.5f - (progress + progress * progress) * 0.25f;
	switch (hint->type) {
		case Hint::Type::Plane:
			hint->data.plane->renderHint(screenLeftWorldX, screenTopWorldY, alpha);
			break;
		case Hint::Type::Rail:
			hint->data.rail->renderHint(screenLeftWorldX, screenTopWorldY, alpha);
			break;
		case Hint::Type::Switch:
			hint->data.switch0->renderHint(screenLeftWorldX, screenTopWorldY, alpha);
			break;
		default:
			break;
	}
}
