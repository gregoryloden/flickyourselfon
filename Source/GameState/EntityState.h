#ifndef ENTITY_STATE_H
#define ENTITY_STATE_H
#include "Util/PooledReferenceCounter.h"

#define newDynamicCameraAnchor() produceWithoutArgs(DynamicCameraAnchor)

class DynamicValue;
class EntityAnimation;
class GameState;
class SpriteAnimation;

enum class SpriteDirection: int {
	Right = 0,
	Up = 1,
	Left = 2,
	Down = 3
};
class EntityState: public PooledReferenceCounter {
protected:
	ReferenceCounterHolder<DynamicValue> x;
	bool renderInterpolatedX;
	ReferenceCounterHolder<DynamicValue> y;
	bool renderInterpolatedY;
	char z;
	ReferenceCounterHolder<EntityAnimation> entityAnimation;
	int lastUpdateTicksTime;

public:
	EntityState(objCounterParameters());
	virtual ~EntityState();

	char getZ() { return z; }
	//begin a sprite animation if applicable
	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime) {}
	//set the direction for this state's sprite, if it has one
	virtual void setSpriteDirection(SpriteDirection pSpriteDirection) {}
	//mark that the player camera should be used as the next camera
	virtual void setShouldSwitchToPlayerCamera() {}
	//set a dynamic color that should be used to render an overlay over the screen
	virtual void setScreenOverlayColor(
		DynamicValue* r, DynamicValue* g, DynamicValue* b, DynamicValue* a, int pLastUpdateTicksTime)
	{
	}
	void copyEntityState(EntityState* other);
	float getRenderCenterWorldX(int ticksTime);
	float getRenderCenterWorldY(int ticksTime);
	float getRenderCenterScreenX(EntityState* camera, int ticksTime);
	float getRenderCenterScreenY(EntityState* camera, int ticksTime);
	int getAnimationTicksDuration();
	void setPosition(float pX, float pY, int pLastUpdateTicksTime);
	void setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime);
	void beginEntityAnimation(EntityAnimation* pEntityAnimation, int ticksTime);
	//set the camera on the next game state, based on this being the previous game state's camera
	virtual void setNextCamera(GameState* nextGameState, int ticksTime) = 0;
};
class DynamicCameraAnchor: public EntityState {
private:
	ReferenceCounterHolder<DynamicValue> screenOverlayR;
	ReferenceCounterHolder<DynamicValue> screenOverlayG;
	ReferenceCounterHolder<DynamicValue> screenOverlayB;
	ReferenceCounterHolder<DynamicValue> screenOverlayA;
	bool shouldSwitchToPlayerCamera;

public:
	DynamicCameraAnchor(objCounterParameters());
	virtual ~DynamicCameraAnchor();

	virtual void setShouldSwitchToPlayerCamera() { shouldSwitchToPlayerCamera = true; }
	static DynamicCameraAnchor* produce(objCounterParameters());
	void copyDynamicCameraAnchor(DynamicCameraAnchor* other);
	virtual void release();
	void updateWithPreviousDynamicCameraAnchor(DynamicCameraAnchor* prev, int ticksTime);
	virtual void setScreenOverlayColor(
		DynamicValue* r, DynamicValue* g, DynamicValue* b, DynamicValue* a, int pLastUpdateTicksTime);
	virtual void setNextCamera(GameState* nextGameState, int ticksTime);
	void render(int ticksTime);
};
#endif
