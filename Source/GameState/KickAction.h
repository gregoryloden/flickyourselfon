#include "Util/PooledReferenceCounter.h"

#define newKickAction(type, targetPlayerX, targetPlayerY, fallHeight, railSwitchId, railSegmentIndex) \
	produceWithArgs(KickAction, type, targetPlayerX, targetPlayerY, fallHeight, railSwitchId, railSegmentIndex)

enum class KickActionType: int {
	None = -1,
	Climb = 0,
	Fall = 1,
	Rail = 2,
	NoRail = 3,
	Switch = 4,
	//radio tower square switch
	Square = 5,
	//radio tower triangle switch
	Triangle = 6,
	//radio tower saw switch
	Saw = 7,
	//radio tower sine switch
	Sine = 8,
	ResetSwitch
};
class KickAction: public PooledReferenceCounter {
private:
	KickActionType type;
	float targetPlayerX;
	float targetPlayerY;
	char fallHeight;
	short railSwitchId;
	int railSegmentIndex;

public:
	KickAction(objCounterParameters());
	virtual ~KickAction();

	KickActionType getType() { return type; }
	float getTargetPlayerX() { return targetPlayerX; }
	float getTargetPlayerY() { return targetPlayerY; }
	char getFallHeight() { return fallHeight; }
	short getRailSwitchId() { return railSwitchId; }
	int getRailSegmentIndex() { return railSegmentIndex; }
	//initialize and return a KickAction
	static KickAction* produce(
		objCounterParametersComma()
		KickActionType pType,
		float pTargetPlayerX,
		float pTargetPlayerY,
		char pFallHeight,
		short pRailSwitchId,
		int pRailSegmentIndex);
	//release a reference to this KickAction and return it to the pool if applicable
	virtual void release();
	//render this kick action
	void render(float centerX, float bottomY, bool hasRailsToReset);
};
