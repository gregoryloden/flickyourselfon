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

	float getRenderCenterX(int ticksTime);
	float getRenderCenterY(int ticksTime);
	void setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime);
	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime) = 0;
	void copyEntityState(EntityState* other);
};
class StaticCameraAnchor: public EntityState {
public:
	StaticCameraAnchor(objCounterParametersComma() float cameraX, float cameraY);
	~StaticCameraAnchor();

	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime) {}
};
