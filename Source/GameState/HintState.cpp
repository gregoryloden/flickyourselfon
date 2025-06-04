#include "HintState.h"
#include "GameState/MapState/Level.h"
#include "GameState/MapState/MapState.h"
#include "GameState/MapState/Rail.h"
#include "GameState/MapState/Switch.h"
#include "GameState/MapState/ResetSwitch.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Logger.h"

#define newHintStatePotentialLevelState(priorStateAndDraftState, steps) \
	produceWithArgs(HintState::PotentialLevelState, priorStateAndDraftState, steps)

//////////////////////////////// Hint ////////////////////////////////
Hint Hint::none (Hint::Type::None);
Hint Hint::calculatingHint (Hint::Type::CalculatingHint);
Hint Hint::searchCanceledEarly (Hint::Type::SearchCanceledEarly);
Hint::Hint(Type pType)
: type(pType)
, data() {
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
, railByteMasks(new unsigned int[maxRailByteMaskCount])
, railByteMasksHash(0)
, steps(0)
, plane(nullptr)
, hint(nullptr) {
}
HintState::PotentialLevelState::~PotentialLevelState() {
	//don't delete the prior state, it's being tracked separately
	//don't delete the plane, it's owned by a Level
	delete[] railByteMasks;
	//don't delete the hint, something else owns it
}
HintState::PotentialLevelState* HintState::PotentialLevelState::produce(
	objCounterParametersComma() PotentialLevelState* priorStateAndDraftState, int pSteps)
{
	initializeWithNewFromPool(p, PotentialLevelState)
	p->priorState = priorStateAndDraftState;
	for (int i = currentRailByteMaskCount - 1; i >= 0; i--)
		p->railByteMasks[i] = priorStateAndDraftState->railByteMasks[i];
	p->railByteMasksHash = priorStateAndDraftState->railByteMasksHash;
	p->steps = pSteps;
	p->retain();
	return p;
}
pooledReferenceCounterDefineRelease(HintState::PotentialLevelState)
void HintState::PotentialLevelState::setHash() {
	unsigned int val = 0;
	for (int i = currentRailByteMaskCount - 1; i >= 0; i--)
		val = val ^ railByteMasks[i];
	railByteMasksHash = val;
}
HintState::PotentialLevelState* HintState::PotentialLevelState::addNewState(
	vector<PotentialLevelState*>& potentialLevelStates, int pSteps)
{
	#ifdef TRACK_HINT_SEARCH_STATS
		Level::hintSearchActionsChecked++;
	#endif
	//look through every other state, and see if it matches this one
	int size = (int)potentialLevelStates.size();
	for (int i = 0; i < size; i++) {
		PotentialLevelState* potentialLevelState = potentialLevelStates[i];
		#ifdef TRACK_HINT_SEARCH_STATS
			Level::hintSearchComparisonsPerformed++;
		#endif
		//they can't be the same if their hashes don't match
		if (railByteMasksHash != potentialLevelState->railByteMasksHash)
			continue;
		for (int j = currentRailByteMaskCount - 1; true; j--) {
			//the bytes are not the same, so the states are not the same, move on to check the next PotentialLevelState
			if (railByteMasks[j] != potentialLevelState->railByteMasks[j])
				break;
			//if we've looked at every byte and they're all the same, this state is not new
			if (j == 0) {
				//however, if the new state takes fewer steps than the other state, replace it
				//set the steps for the other state to -1, add it to Level's list of replaced states, and replace that state in
				//	the list before returning the new state
				if (pSteps < potentialLevelState->steps) {
					potentialLevelState->steps = -1;
					Level::replacedPotentialLevelStates.push_back(potentialLevelState);
					return (potentialLevelStates[i] = newHintStatePotentialLevelState(this, pSteps));
				}
				return nullptr;
			}
		}
	}

	//all states have at least one byte difference, so this is a new state
	PotentialLevelState* newState = newHintStatePotentialLevelState(this, pSteps);
	potentialLevelStates.push_back(newState);
	return newState;
}
Hint* HintState::PotentialLevelState::getHint() {
	PotentialLevelState* hintLevelState = this;
	Level::foundHintSearchTotalHintSteps = 1;
	while (hintLevelState->priorState->priorState != nullptr) {
		hintLevelState = hintLevelState->priorState;
		Level::foundHintSearchTotalHintSteps++;
	}
	return hintLevelState->hint;
}
#ifdef LOG_FOUND_HINT_STEPS
	void HintState::PotentialLevelState::logSteps() {
		stringstream stepsMessage;
		if (priorState == nullptr) {
			stepsMessage << "start at plane " << plane->getIndexInOwningLevel();
			logRailByteMasks(stepsMessage);
		} else {
			priorState->logSteps();
			stepsMessage << " -> ";
			logHint(stepsMessage);
			stepsMessage << " -> plane " << plane->getIndexInOwningLevel();
			if (plane->isMilestoneSwitchHint(hint))
				stepsMessage << " (milestone)";
		}
		Logger::debugLogger.logString(stepsMessage.str());
	}
	void HintState::PotentialLevelState::logHint(stringstream& stepsMessage) {
		if (hint->type == Hint::Type::Plane)
			stepsMessage << "plane " << hint->data.plane->getIndexInOwningLevel();
		else if (hint->type == Hint::Type::Rail) {
			Rail::Segment* segment = hint->data.rail->getSegment(0);
			stepsMessage << "rail " << MapState::getRailSwitchId(segment->x, segment->y);
			MapState::logRailDescriptor(hint->data.rail, &stepsMessage);
		} else if (hint->type == Hint::Type::Switch) {
			Switch* switch0 = hint->data.switch0;
			stepsMessage << "switch " << MapState::getRailSwitchId(switch0->getLeftX(), switch0->getTopY());
			logRailByteMasks(stepsMessage);
			MapState::logSwitchDescriptor(hint->data.switch0, &stepsMessage);
		} else if (hint->type == Hint::Type::None)
			stepsMessage << "none";
		else if (hint->type == Hint::Type::UndoReset)
			stepsMessage << "undo/reset";
		else if (hint->type == Hint::Type::CalculatingHint)
			stepsMessage << "calculatingHint";
		else if (hint->type == Hint::Type::SearchCanceledEarly)
			stepsMessage << "searchCanceledEarly";
		else
			stepsMessage << "[unknown]";
	}
	void HintState::PotentialLevelState::logRailByteMasks(stringstream& stepsMessage) {
		for (int i = 0; i < currentRailByteMaskCount; i++)
			stepsMessage << std::hex << std::uppercase << "  " << railByteMasks[i] << std::dec;
	}
#endif

//////////////////////////////// HintState ////////////////////////////////
HintState::HintState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, hint(nullptr)
, animationEndTicksTime(0)
, offscreenArrowAlpha(0)
, renderLeftWorldX(0)
, renderTopWorldY(0)
, renderRightWorldX(0)
, renderBottomWorldY(0) {
}
HintState::~HintState() {
	//don't delete the hint, something else owns it
}
HintState* HintState::produce(objCounterParametersComma() Hint* pHint, int animationStartTicksTime) {
	initializeWithNewFromPool(h, HintState)
	h->hint = pHint;
	h->animationEndTicksTime = animationStartTicksTime + totalDisplayTicks;
	switch (pHint->type) {
		case Hint::Type::Plane:
			pHint->data.plane->getHintRenderBounds(
				&h->renderLeftWorldX, &h->renderTopWorldY, &h->renderRightWorldX, &h->renderBottomWorldY);
			break;
		case Hint::Type::Rail:
			pHint->data.rail->getHintRenderBounds(
				&h->renderLeftWorldX, &h->renderTopWorldY, &h->renderRightWorldX, &h->renderBottomWorldY);
			break;
		case Hint::Type::Switch:
			pHint->data.switch0->getHintRenderBounds(
				&h->renderLeftWorldX, &h->renderTopWorldY, &h->renderRightWorldX, &h->renderBottomWorldY);
			break;
		case Hint::Type::UndoReset:
			pHint->data.resetSwitch->getHintRenderBounds(
				&h->renderLeftWorldX, &h->renderTopWorldY, &h->renderRightWorldX, &h->renderBottomWorldY);
			break;
		default:
			h->offscreenArrowAlpha = 0;
			break;
	}
	return h;
}
pooledReferenceCounterDefineRelease(HintState)
void HintState::render(int screenLeftWorldX, int screenTopWorldY, int ticksTime) {
	//this is a temporary hint, always show this and at full alpha
	if (hint->type == Hint::Type::CalculatingHint) {
		MapState::renderControlsTutorial("(calculating hint...)", {});
		//text doesn't have an offscreen arrow
		offscreenArrowAlpha = 0;
		return;
	}
	//animation is over, don't render the hint or offscreen arrow
	if (ticksTime >= animationEndTicksTime) {
		offscreenArrowAlpha = 0;
		return;
	}
	int progressTicks = ticksTime + totalDisplayTicks - animationEndTicksTime;
	float progress = (float)progressTicks / totalDisplayTicks;
	float alpha = 0.5f - (progress + progress * progress) * 0.25f;
	//if we couldn't find a hint, show a tutorial-area message
	if (hint->type == Hint::Type::SearchCanceledEarly) {
		glColor4f(1.0f, 1.0f, 1.0f, alpha * 2.0f);
		MapState::renderControlsTutorial("(unable to calculate hint)", {});
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		//text doesn't have an offscreen arrow
		offscreenArrowAlpha = 0;
		return;
	}
	bool isOn = (progressTicks % flashOnOffTotalTicks) < flashOnOffTicks;
	if (!isOn) {
		offscreenArrowAlpha = 0;
		return;
	}
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
		case Hint::Type::UndoReset:
			hint->data.resetSwitch->renderHint(screenLeftWorldX, screenTopWorldY, alpha);
			break;
		default:
			return;
	}
	offscreenArrowAlpha = alpha;
}
void HintState::renderOffscreenArrow(int screenLeftWorldX, int screenTopWorldY) {
	if (offscreenArrowAlpha == 0)
		return;
	static constexpr int offscreenEdgeSpacing = 12;
	int renderScreenLeftX = renderLeftWorldX - screenLeftWorldX;
	int renderScreenTopY = renderTopWorldY - screenTopWorldY;
	int renderScreenRightX = renderRightWorldX - screenLeftWorldX;
	int renderScreenBottomY = renderBottomWorldY - screenTopWorldY;
	if (renderScreenLeftX <= Config::gameScreenWidth - offscreenEdgeSpacing
			&& renderScreenTopY <= Config::gameScreenHeight - offscreenEdgeSpacing
			&& renderScreenRightX >= offscreenEdgeSpacing
			&& renderScreenBottomY >= offscreenEdgeSpacing)
		return;
	//figure out where to place the arrow
	static constexpr int offscreenArrowMinEdgeSpacing = 1;
	static constexpr int offscreenArrowMaxEdgeSpacing = offscreenArrowMinEdgeSpacing + offscreenEdgeSpacing;
	static constexpr int offscreenArrowToHintSpacing = 1;
	int arrowSpriteHorizontalIndex = 1;
	int arrowSpriteVerticalIndex = 1;
	GLint drawArrowLeftX;
	GLint drawArrowTopY;
	if (renderScreenRightX < offscreenEdgeSpacing) {
		arrowSpriteHorizontalIndex = 0;
		drawArrowLeftX = (GLint)MathUtils::max(offscreenArrowMinEdgeSpacing, renderScreenRightX + offscreenArrowToHintSpacing);
	} else if (renderScreenLeftX > Config::gameScreenWidth - offscreenEdgeSpacing) {
		arrowSpriteHorizontalIndex = 2;
		int arrowRightX = MathUtils::min(
			Config::gameScreenWidth - offscreenArrowMinEdgeSpacing, renderScreenLeftX - offscreenArrowToHintSpacing);
		drawArrowLeftX = (GLint)(arrowRightX - SpriteRegistry::borderArrows->getSpriteWidth());
	} else {
		int baseArrowLeftX = (renderScreenLeftX + renderScreenRightX - SpriteRegistry::borderArrows->getSpriteWidth()) / 2;
		int maxArrowLeftX =
			Config::gameScreenWidth - offscreenArrowMaxEdgeSpacing - SpriteRegistry::borderArrows->getSpriteWidth();
		drawArrowLeftX = (GLint)(MathUtils::max(offscreenArrowMaxEdgeSpacing, MathUtils::min(maxArrowLeftX, baseArrowLeftX)));
	}
	if (renderScreenBottomY < offscreenEdgeSpacing) {
		arrowSpriteVerticalIndex = 0;
		drawArrowTopY = (GLint)MathUtils::max(offscreenArrowMinEdgeSpacing, renderScreenBottomY + offscreenArrowToHintSpacing);
	} else if (renderScreenTopY > Config::gameScreenHeight - offscreenEdgeSpacing) {
		arrowSpriteVerticalIndex = 2;
		int arrowBottomY = MathUtils::min(
			Config::gameScreenHeight - offscreenArrowMinEdgeSpacing, renderScreenTopY - offscreenArrowToHintSpacing);
		drawArrowTopY = (GLint)(arrowBottomY - SpriteRegistry::borderArrows->getSpriteHeight());
	} else {
		int baseArrowTopY = (renderScreenTopY + renderScreenBottomY - SpriteRegistry::borderArrows->getSpriteHeight()) / 2;
		int maxArrowTopY =
			Config::gameScreenHeight - offscreenArrowMaxEdgeSpacing - SpriteRegistry::borderArrows->getSpriteHeight();
		drawArrowTopY = (GLint)(MathUtils::max(offscreenArrowMaxEdgeSpacing, MathUtils::min(maxArrowTopY, baseArrowTopY)));
	}
	glColor4f(1.0f, 1.0f, 1.0f, offscreenArrowAlpha);
	SpriteRegistry::borderArrows->renderSpriteAtScreenPosition(
		arrowSpriteHorizontalIndex, arrowSpriteVerticalIndex, drawArrowLeftX, drawArrowTopY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
