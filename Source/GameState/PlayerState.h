#include "General/General.h"

class PositionState;
enum class PlayerSpriteDirection: int;

class PlayerState onlyInDebug(: public ObjCounter) {
public:
	const float boundingBoxLeftOffset;
	const float boundingBoxRightOffset;
	const float boundingBoxTopOffset;
	const float boundingBoxBottomOffset;
private:
	float xPosition;
	char xDirection;
	float xVelocityPerTick;
	bool renderInterpolatedXPosition;
	float yPosition;
	char yDirection;
	float yVelocityPerTick;
	bool renderInterpolatedYPosition;
	char z;
	int lastUpdateTicksTime;
	int walkingAnimationStartTicksTime;
	PlayerSpriteDirection spriteDirection;

public:
	PlayerState(objCounterParameters());
	~PlayerState();

	float getRenderableX(int ticksTime);
	float getRenderableY(int ticksTime);
private:
	float getInterpolatedX(int ticksTime);
	float getInterpolatedY(int ticksTime);
public:
	void updateWithPreviousPlayerState(PlayerState* prev, int ticksTime);
private:
	void updatePositionWithPreviousPlayerState(PlayerState* prev, int ticksTime);
	void collideWithEnvironmentWithPreviousPlayerState(PlayerState* prev);
	void updateSpriteWithPreviousPlayerState(PlayerState* prev, int ticksTime);
public:
	void render();
};
