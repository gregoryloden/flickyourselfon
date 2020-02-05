#include "KickAction.h"

KickAction::KickAction(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, type(KickActionType::None)
, targetPlayerX(0.0f)
, targetPlayerY(0.0f)
, fallHeight(0)
, railId(0)
, rail(nullptr)
, useRailStart(false)
, switchId(0)
, resetSwitchId(0) {
}
KickAction::~KickAction() {}
KickAction* KickAction::produce(
	objCounterParametersComma()
	KickActionType pType,
	float pTargetPlayerX,
	float pTargetPlayerY,
	char pFallHeight,
	short pRailId,
	Rail* pRail,
	bool pUseRailStart,
	short pSwitchId,
	short pResetSwitchId)
{
	initializeWithNewFromPool(k, KickAction)
	k->type = pType;
	k->targetPlayerX = pTargetPlayerX;
	k->targetPlayerY = pTargetPlayerY;
	k->fallHeight = pFallHeight;
	k->railId = pRailId;
	k->rail = pRail;
	k->useRailStart = pUseRailStart;
	k->switchId = pSwitchId;
	k->resetSwitchId = pResetSwitchId;
	return k;
}
pooledReferenceCounterDefineRelease(KickAction)
