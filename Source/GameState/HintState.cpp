#include "HintState.h"
#include "GameState/MapState/Level.h"

//////////////////////////////// HintState::PotentialLevelState ////////////////////////////////
HintStateTypes::PotentialLevelState HintStateTypes::PotentialLevelState::draftState (
	objCounterLocalArgumentsComma(HintStateTypes::PotentialLevelState) nullptr);
HintStateTypes::PotentialLevelState::PotentialLevelState(objCounterParametersComma() LevelTypes::Plane* pPlane)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
priorState(nullptr)
, plane(pPlane)
, railByteMaskCount(0)
, railByteMasks(nullptr)
, type(Type::None)
, data() {
	if (pPlane != nullptr)
		loadState(pPlane->getOwningLevel());
}
HintStateTypes::PotentialLevelState::PotentialLevelState(
	objCounterParametersComma() PotentialLevelState* pPriorState, LevelTypes::Plane* pPlane, PotentialLevelState* draftState)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
priorState(pPriorState)
, plane(pPlane)
, railByteMaskCount(draftState->railByteMaskCount)
, railByteMasks(new unsigned int[draftState->railByteMaskCount])
, type(Type::None)
, data() {
	for (int i = railByteMaskCount - 1; i >= 0; i--)
		railByteMasks[i] = draftState->railByteMasks[i];
}
HintStateTypes::PotentialLevelState::~PotentialLevelState() {
	//don't delete the prior state, it's being tracked separately
	//don't delete the currentPlane, it's owned by a Level
	delete[] railByteMasks;
}
void HintStateTypes::PotentialLevelState::loadState(Level* level) {
	delete[] railByteMasks;
	railByteMaskCount = level->getRailByteCount();
	railByteMasks = new unsigned int[railByteMaskCount] {};
}
bool HintStateTypes::PotentialLevelState::isNewState(vector<PotentialLevelState*>& potentialLevelStates) {
	//for speed, cache these values
	int cachedRailByteMaskCountMinus1 = railByteMaskCount - 1;
	unsigned int* cachedRailByteMasks = railByteMasks;

	//look through every other state, and see if it matches this one
	for (PotentialLevelState* potentialLevelState : potentialLevelStates) {
		unsigned int* otherRailByteMasks = potentialLevelState->railByteMasks;
		for (int i = cachedRailByteMaskCountMinus1; true; i--) {
			//the bytes are not the same, so the states are not the same, move on to check the next PotentialLevelState
			if (cachedRailByteMasks[i] != otherRailByteMasks[i])
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
	while (hintLevelState->priorState->priorState != nullptr)
		hintLevelState = hintLevelState->priorState;
	return newHintState(hintLevelState->type, hintLevelState->data);
}
using namespace HintStateTypes;

//////////////////////////////// HintState ////////////////////////////////
HintState::HintState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, type(Type::None)
, data()
, animationEndTicksTime() {
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
