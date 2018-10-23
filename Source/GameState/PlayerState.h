#include "General/General.h"

class PositionState;
enum class PlayerSpriteDirection: int;

class PlayerState onlyInDebug(: public ObjCounter) {
private:
	PositionState* position;
	int walkingAnimationStartTicksTime;
	PlayerSpriteDirection spriteDirection;

public:
	PlayerState(objCounterParameters());
	~PlayerState();

	PositionState* getPosition() { return position; }
	void updateWithPreviousPlayerState(PlayerState* prev, int ticksTime);
	void render();
};
