#ifndef ENTITY_STATE_H
#define ENTITY_STATE_H
#include "Util/PooledReferenceCounter.h"

#define newDynamicCameraAnchor() produceWithoutArgs(DynamicCameraAnchor)

class DynamicValue;
class EntityAnimation;
class GameState;
class Holder_EntityAnimationComponentVector;
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
	//set the position of a ghost sprite that this entity state should render
	virtual void setGhostSprite(bool show, float x, float y, int ticksTime) {}
	//mark that the player camera should be used as the next camera
	virtual void setShouldSwitchToPlayerCamera() {}
	//set a dynamic color that should be used to render an overlay over the screen
	virtual void setScreenOverlayColor(
		DynamicValue* r, DynamicValue* g, DynamicValue* b, DynamicValue* a, int pLastUpdateTicksTime)
	{
	}
	//tell the map to kick a switch at the given time
	virtual void mapKickSwitch(short switchId, bool allowRadioTowerAnimation, int ticksTime) {}
	//tell the map to kick a reset switch at the given time
	virtual void mapKickResetSwitch(short resetSwitchId, int ticksTime) {}
	void copyEntityState(EntityState* other);
	float getRenderCenterWorldX(int ticksTime);
	float getRenderCenterWorldY(int ticksTime);
	float getRenderCenterScreenX(EntityState* camera, int ticksTime);
	static float getRenderCenterScreenXFromWorldX(float worldX, EntityState* camera, int ticksTime);
	float getRenderCenterScreenY(EntityState* camera, int ticksTime);
	static float getRenderCenterScreenYFromWorldY(float worldX, EntityState* camera, int ticksTime);
	int getAnimationTicksDuration();
	static SpriteDirection getSpriteDirection(float x, float y);
	void setPosition(float pX, float pY, int pLastUpdateTicksTime);
	void setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime);
	void beginEntityAnimation(Holder_EntityAnimationComponentVector* componentsHolder, int ticksTime);
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
