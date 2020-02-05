#include "Util/PooledReferenceCounter.h"

#define newKickAction(type, targetPlayerX, targetPlayerY, fallHeight, railId, rail, useRailStart, switchId, resetSwitchId) \
	produceWithArgs(\
		KickAction, type, targetPlayerX, targetPlayerY, fallHeight, railId, rail, useRailStart, switchId, resetSwitchId)

class Rail;

enum class KickActionType: int {
	None,
	Climb,
	Fall,
	Rail,
	Switch,
	ResetSwitch
};
class KickAction: public PooledReferenceCounter {
private:
	KickActionType type;
	float targetPlayerX;
	float targetPlayerY;
	char fallHeight;
	short railId;
	Rail* rail;
	bool useRailStart;
	short switchId;
	short resetSwitchId;

public:
	KickAction(objCounterParameters());
	virtual ~KickAction();

	KickActionType getType() { return type; }
	float getTargetPlayerX() { return targetPlayerX; }
	float getTargetPlayerY() { return targetPlayerY; }
	char getFallHeight() { return fallHeight; }
	short getRailId() { return railId; }
	Rail* getRail() { return rail; }
	bool getUseRailStart() { return useRailStart; }
	short getSwitchId() { return switchId; }
	short getResetSwitchId() { return resetSwitchId; }
	virtual void release();
	static KickAction* produce(
		objCounterParametersComma()
		KickActionType pType,
		float pTargetPlayerX,
		float pTargetPlayerY,
		char pFallHeight,
		short pRailId,
		Rail* pRail,
		bool pUseRailStart,
		short pSwitchId,
		short pResetSwitchId);
};
