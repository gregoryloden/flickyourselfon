#include "KickAction.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Sprites/Text.h"
#include "Util/Config.h"

KickAction::KickAction(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, type(KickActionType::None)
, targetPlayerX(0.0f)
, targetPlayerY(0.0f)
, fallHeight(0)
, railSwitchId(0) {
}
KickAction::~KickAction() {}
KickAction* KickAction::produce(
	objCounterParametersComma()
	KickActionType pType,
	float pTargetPlayerX,
	float pTargetPlayerY,
	char pFallHeight,
	short pRailSwitchId,
	int pRailSegmentIndex)
{
	initializeWithNewFromPool(k, KickAction)
	k->type = pType;
	k->targetPlayerX = pTargetPlayerX;
	k->targetPlayerY = pTargetPlayerY;
	k->fallHeight = pFallHeight;
	k->railSwitchId = pRailSwitchId;
	k->railSegmentIndex = pRailSegmentIndex;
	return k;
}
pooledReferenceCounterDefineRelease(KickAction)
//render this kick action
void KickAction::render(float centerX, float bottomY, bool hasRailsToReset) {
	bool showKickIndicator = false;
	switch (type) {
		case KickActionType::Climb:
			showKickIndicator = Config::kickIndicators.climb;
			break;
		case KickActionType::Fall:
			showKickIndicator = Config::kickIndicators.fall;
			break;
		case KickActionType::Rail:
		case KickActionType::NoRail:
			showKickIndicator = Config::kickIndicators.rail;
			break;
		case KickActionType::Switch:
		case KickActionType::Square:
		case KickActionType::Triangle:
		case KickActionType::Saw:
		case KickActionType::Sine:
			showKickIndicator = Config::kickIndicators.switch0;
			break;
		case KickActionType::ResetSwitch: {
			if (!Config::kickIndicators.resetSwitch || !hasRailsToReset)
				return;
			Text::Metrics metrics = Text::getMetrics("Reset", 1.0f);
			Text::render("Reset", centerX - metrics.charactersWidth * 0.5f, bottomY - 2.0f, 1.0f);
			return;
		}
		default:
			return;
	}
	if (!showKickIndicator)
		return;
	glEnable(GL_BLEND);
	SpriteRegistry::kickIndicator->renderSpriteCenteredAtScreenPosition(
		(int)type, 0, centerX, bottomY - 1.0f - (float)SpriteRegistry::kickIndicator->getSpriteHeight() / 2.0f);
}
