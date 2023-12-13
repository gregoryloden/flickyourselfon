#include "Util/PooledReferenceCounter.h"

#define newKickAction(type, targetPlayerX, targetPlayerY, fallHeight, railSwitchId, railSegmentIndex) \
	produceWithArgs(KickAction, type, targetPlayerX, targetPlayerY, fallHeight, railSwitchId, railSegmentIndex)

enum class KickActionType: int {
	None = -1,
	Climb = 0,
	Fall = 1,
	FallBig = 2,
	Rail = 3,
	NoRail = 4,
	Switch = 5,
	NoSwitch = 6,
	//radio tower square switch
	Square = 7,
	//radio tower triangle switch
	Triangle = 8,
	//radio tower saw switch
	Saw = 9,
	//radio tower sine switch
	Sine = 10,
	Undo = 11,
	Redo = 12,
	ResetSwitch = 100,
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
