#include "General/General.h"

class SpriteAnimation;

class EntityState onlyInDebug(: public ObjCounter) {
protected:
	//TODO: make position an object that can do more math than just linear interpolation
	float xPosition;
	float xVelocityPerTick;
	bool renderInterpolatedXPosition;
	float yPosition;
	float yVelocityPerTick;
	bool renderInterpolatedYPosition;
	char z;
	int lastUpdateTicksTime;

public:
	EntityState(objCounterParametersComma() float pXPosition, float pYPosition);
	~EntityState();

	float getCameraCenterX(int ticksTime);
	float getCameraCenterY(int ticksTime);
protected:
	float getInterpolatedX(int ticksTime);
	float getInterpolatedY(int ticksTime);
public:
	void setPosition(float pXPosition, float pYPosition, char pZ, int pLastUpdateTicksTime);
	void setVelocity(float pXVelocityPerTick, float pYVelocityPerTick);
	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime) = 0;
	void copyEntityState(EntityState* other);
};
class StaticCameraAnchor: public EntityState {
public:
	StaticCameraAnchor(objCounterParametersComma() float cameraX, float cameraY);
	~StaticCameraAnchor();

	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pAnimationStartTicksTime) {}
};
