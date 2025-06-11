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
void KickAction::render(float centerX, float bottomY, bool hasRailsToReset) {
	bool showKickIndicator = false;
	switch (type) {
		case KickActionType::Climb:
			showKickIndicator = Config::climbKickIndicator.isOn();
			break;
		case KickActionType::Fall:
		case KickActionType::FallBig:
			showKickIndicator = Config::fallKickIndicator.isOn();
			break;
		case KickActionType::Rail:
		case KickActionType::NoRail:
			showKickIndicator = Config::railKickIndicator.isOn();
			break;
		case KickActionType::Switch:
		case KickActionType::NoSwitch:
		case KickActionType::Square:
		case KickActionType::Triangle:
		case KickActionType::Saw:
		case KickActionType::Sine:
			showKickIndicator = Config::switchKickIndicator.isOn();
			break;
		case KickActionType::Undo:
		case KickActionType::Redo:
		case KickActionType::EndGame:
			showKickIndicator = true;
			break;
		case KickActionType::ResetSwitch: {
			if (!Config::resetSwitchKickIndicator.isOn())
				return;
			if (!hasRailsToReset)
				Text::setRenderColor(1.0f, 1.0f, 1.0f, 0.5f);
			static constexpr char* resetText = "Reset";
			Text::render(resetText, centerX - Text::getMetrics(resetText, 1.0f).charactersWidth * 0.5f, bottomY - 2.0f, 1.0f);
			if (!hasRailsToReset) {
				Text::setRenderColor(1.0f, 1.0f, 1.0f, 1.0f);
				static constexpr char* xText = u8"❌";
				Text::render(xText, centerX - Text::getMetrics(xText, 1.0f).charactersWidth * 0.5f, bottomY - 2.0f, 1.0f);
			}
			return;
		}
		default:
			return;
	}
	if (!showKickIndicator)
		return;
	SpriteRegistry::kickIndicator->renderSpriteCenteredAtScreenPosition((int)type, 0, centerX, bottomY - 1.0f);
}
