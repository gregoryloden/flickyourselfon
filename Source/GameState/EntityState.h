#include "Util/PooledReferenceCounter.h"

class DynamicValue;
class SpriteAnimation;

class EntityState onlyInDebug(: public ObjCounter) {
protected:
	ReferenceCounterHolder<DynamicValue> x;
	bool renderInterpolatedX;
	ReferenceCounterHolder<DynamicValue> y;
	bool renderInterpolatedY;
	char z;
	int lastUpdateTicksTime;

public:
	EntityState(objCounterParametersComma() float xPosition, float yPosition);
	~EntityState();

	void copyEntityState(EntityState* other);
	float getRenderCenterWorldX(int ticksTime);
	float getRenderCenterWorldY(int ticksTime);
	//if an animation changes the camera anchor, return it here
	virtual EntityState* getNextCameraAnchor(int ticksTime) = 0;
	void setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime);
	//begin a sprite animation
	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime) = 0;
};
class StaticCameraAnchor: public EntityState {
public:
	StaticCameraAnchor(objCounterParametersComma() float cameraX, float cameraY);
	~StaticCameraAnchor();

	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime) {}
};
