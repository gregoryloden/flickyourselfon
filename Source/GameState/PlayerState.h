#include "GameState/EntityState.h"
#include "Util/PooledReferenceCounter.h"

#define newPlayerState() newWithoutArgs(PlayerState)

class PositionState;
class SpriteAnimation;
class EntityAnimation;
enum class PlayerSpriteDirection: int;

class PlayerState: public EntityState {
private:
	static const float playerStartingXPosition;
	static const float playerStartingYPosition;
	static const float playerWidth;
	static const float playerHeight;
	static const float boundingBoxLeftOffset;
	static const float boundingBoxRightOffset;
	static const float boundingBoxTopOffset;
	static const float boundingBoxBottomOffset;

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

	void copyPlayerState(PlayerState* other);
	virtual EntityState* getNextCameraAnchor(int ticksTime);
	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime);
	void updateWithPreviousPlayerState(PlayerState* prev, int ticksTime);
private:
	void updatePositionWithPreviousPlayerState(PlayerState* prev, int ticksTime);
	void collideWithEnvironmentWithPreviousPlayerState(PlayerState* prev);
	void updateSpriteWithPreviousPlayerState(PlayerState* prev, int ticksTime, bool usePreviousStateSpriteAnimation);
public:
	void beginKicking(int ticksTime);
	void render(EntityState* camera, int ticksTime);
};
