#include "Util/PooledReferenceCounter.h"

#define newKickAction(type, targetPlayerX, targetPlayerY, fallHeight, railSwitchId) \
	produceWithArgs(KickAction, type, targetPlayerX, targetPlayerY, fallHeight, railSwitchId)

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
	short railSwitchId;

public:
	KickAction(objCounterParameters());
	virtual ~KickAction();

	KickActionType getType() { return type; }
	float getTargetPlayerX() { return targetPlayerX; }
	float getTargetPlayerY() { return targetPlayerY; }
	char getFallHeight() { return fallHeight; }
	short getRailSwitchId() { return railSwitchId; }
	virtual void release();
	static KickAction* produce(
		objCounterParametersComma()
		KickActionType pType,
		float pTargetPlayerX,
		float pTargetPlayerY,
		char pFallHeight,
		short pRailSwitchId);
};
