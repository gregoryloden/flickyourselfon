#include "General/General.h"
#include "GameState/CameraAnchor.h"

class PositionState;
class SpriteAnimation;
enum class PlayerSpriteDirection: int;

class PlayerState: public CameraAnchor {
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
	SpriteAnimation* animation;
	int animationStartTicksTime;
	PlayerSpriteDirection spriteDirection;
	bool hasBoot;

public:
	PlayerState(objCounterParameters());
	~PlayerState();

	float getCameraCenterX(int ticksTime);
	float getCameraCenterY(int ticksTime);
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
	void render(CameraAnchor* camera);
};
