#ifndef ENTITY_STATE_H
#define ENTITY_STATE_H
#include "Util/PooledReferenceCounter.h"

#define newDynamicCameraAnchor() produceWithoutArgs(DynamicCameraAnchor)

class DynamicValue;
class GameState;
class EntityAnimation;
class SpriteAnimation;

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
	~EntityState();

	//begin a sprite animation if applicable
	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime) {}
	//mark that the player camera should be used as the next camera
	virtual void setShouldSwitchToPlayerCamera() {}
	void copyEntityState(EntityState* other);
	float getRenderCenterWorldX(int ticksTime);
	float getRenderCenterWorldY(int ticksTime);
	void setPosition(float pX, float pY, int pLastUpdateTicksTime);
	void setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime);
	//set the camera on the next game state, based on this being the previous game state's camera
	virtual void setNextCamera(GameState* nextGameState, int ticksTime) = 0;
};
class DynamicCameraAnchor: public EntityState {
private:
	bool shouldSwitchToPlayerCamera;

public:
	DynamicCameraAnchor(objCounterParameters());
	~DynamicCameraAnchor();

	virtual void setShouldSwitchToPlayerCamera() { shouldSwitchToPlayerCamera = true; }
	static DynamicCameraAnchor* produce(objCounterParameters());
	virtual void release();
	void updateWithPreviousDynamicCameraAnchor(DynamicCameraAnchor* prev, int ticksTime);
	virtual void setNextCamera(GameState* nextGameState, int ticksTime);
};
#endif
