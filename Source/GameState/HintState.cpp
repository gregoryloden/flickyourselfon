#include "HintState.h"
#include "GameState/MapState/Level.h"
#include "GameState/MapState/Rail.h"
#include "GameState/MapState/Switch.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

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
, animationEndTicksTime(0)
, renderAlpha(0)
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
		default:
			break;
	}
	return h;
}
pooledReferenceCounterDefineRelease(HintState)
void HintState::render(int screenLeftWorldX, int screenTopWorldY, bool belowRails, int ticksTime) {
	//animation is over, don't render the hint or offscreen arrow
	if (ticksTime >= animationEndTicksTime) {
		renderAlpha = 0;
		return;
	//only render planes below rails, and only rails and switches above rails
	} else if (belowRails
			? hint->type != Hint::Type::Plane
			: hint->type != Hint::Type::Rail && hint->type != Hint::Type::Switch)
		return;
	int progressTicks = ticksTime + totalDisplayTicks - animationEndTicksTime;
	bool isOn = (progressTicks % flashOnOffTotalTicks) < flashOnOffTicks;
	if (!isOn) {
		renderAlpha = 0;
		return;
	}
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
	renderAlpha = alpha;
}
void HintState::renderOffscreenArrow(int screenLeftWorldX, int screenTopWorldY) {
	if (renderAlpha == 0)
		return;
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
	glColor4f(1.0f, 1.0f, 1.0f, renderAlpha);
	SpriteRegistry::borderArrows->renderSpriteAtScreenPosition(
		arrowSpriteHorizontalIndex, arrowSpriteVerticalIndex, drawArrowLeftX, drawArrowTopY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
