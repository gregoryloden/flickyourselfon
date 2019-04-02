#ifndef ENTITY_STATE_H
#define ENTITY_STATE_H
#include "Util/PooledReferenceCounter.h"

#define newStaticCameraAnchor(x, y) produceWithArgs(StaticCameraAnchor, x, y)

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

	void copyEntityState(EntityState* other);
	float getRenderCenterWorldX(int ticksTime);
	float getRenderCenterWorldY(int ticksTime);
	//set the camera on the next game state, based on this being the previous game state's camera
	virtual void setNextCamera(GameState* nextGameState, int ticksTime) = 0;
	void setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime);
	//begin a sprite animation
	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime) = 0;
};
class StaticCameraAnchor: public EntityState {
public:
	StaticCameraAnchor(objCounterParameters());
	~StaticCameraAnchor();

	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime) {}
	static StaticCameraAnchor* produce(objCounterParametersComma() float pX, float pY);
	virtual void release();
	virtual void setNextCamera(GameState* nextGameState, int ticksTime);
};
#endif
