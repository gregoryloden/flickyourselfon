#include "KickAction.h"

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
	short pRailSwitchId)
{
	initializeWithNewFromPool(k, KickAction)
	k->type = pType;
	k->targetPlayerX = pTargetPlayerX;
	k->targetPlayerY = pTargetPlayerY;
	k->fallHeight = pFallHeight;
	k->railSwitchId = pRailSwitchId;
	return k;
}
pooledReferenceCounterDefineRelease(KickAction)
