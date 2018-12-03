#include "GameState/EntityState.h"
#include "Util/PooledReferenceCounter.h"

class PositionState;
class SpriteAnimation;
class EntityAnimation;
enum class PlayerSpriteDirection: int;

class PlayerState: public EntityState {
public:
	static const float boundingBoxLeftOffset;
	static const float boundingBoxRightOffset;
	static const float boundingBoxTopOffset;
	static const float boundingBoxBottomOffset;

private:
	char xDirection;
	char yDirection;
	SpriteAnimation* animation;
	int animationStartTicksTime;
	PlayerSpriteDirection spriteDirection;
	bool hasBoot;
	ReferenceCounterHolder<EntityAnimation> kickingAnimation;

public:
	PlayerState(objCounterParameters());
	~PlayerState();

	virtual EntityState* getNextCameraAnchor(int ticksTime);
	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime);
	void updateWithPreviousPlayerState(PlayerState* prev, int ticksTime);
private:
	void updatePositionWithPreviousPlayerState(PlayerState* prev, int ticksTime);
	void collideWithEnvironmentWithPreviousPlayerState(PlayerState* prev);
	void updateSpriteWithPreviousPlayerState(PlayerState* prev, int ticksTime, bool usePreviousSpriteAnimation);
public:
	void beginKicking(int ticksTime);
	void render(EntityState* camera, int ticksTime);
};
