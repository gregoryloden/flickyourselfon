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
	ReferenceCounterHolder<EntityAnimation> entityAnimation;
	int lastUpdateTicksTime;

public:
	EntityState(objCounterParameters());
	virtual ~EntityState();

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
	//copy the state of the other EntityState
	void copyEntityState(EntityState* other);
	//return the entity's x coordinate at the given time that we should use for rendering the world
	float getRenderCenterWorldX(int ticksTime);
	//return the entity's y coordinate at the given time that we should use for rendering the world
	float getRenderCenterWorldY(int ticksTime);
	//return the entity's screen x coordinate at the given time
	float getRenderCenterScreenX(EntityState* camera, int ticksTime);
	//return the screen x coordinate at the given time from the given world x coordinate and camera
	static float getRenderCenterScreenXFromWorldX(float worldX, EntityState* camera, int ticksTime);
	//return the entity's screen y coordinate at the given time
	float getRenderCenterScreenY(EntityState* camera, int ticksTime);
	//return the screen y coordinate at the given time from the given world y coordinate and camera
	static float getRenderCenterScreenYFromWorldY(float worldX, EntityState* camera, int ticksTime);
	//get the duration of our entity animation, if we have one
	int getAnimationTicksDuration();
	//get a sprite direction based on movement velocity
	static SpriteDirection getSpriteDirection(float x, float y);
	//set the position to the given position at the given time, preserving the velocity
	void setPosition(float pX, float pY, int pLastUpdateTicksTime);
	//set the velocity to the given velocity at the given time, preserving the position
	void setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime);
	//start the given entity animation
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
	//initialize and return a DynamicCameraAnchor
	static DynamicCameraAnchor* produce(objCounterParameters());
	//copy the state of the other DynamicCameraAnchor
	void copyDynamicCameraAnchor(DynamicCameraAnchor* other);
	//release a reference to this DynamicCameraAnchor and return it to the pool if applicable
	virtual void release();
	//update this DynamicCameraAnchor by reading from the previous state
	void updateWithPreviousDynamicCameraAnchor(DynamicCameraAnchor* prev, int ticksTime);
	//use this color to render an overlay on the screen
	virtual void setScreenOverlayColor(
		DynamicValue* r, DynamicValue* g, DynamicValue* b, DynamicValue* a, int pLastUpdateTicksTime);
	//use this anchor unless we've been told to switch to the player camera
	virtual void setNextCamera(GameState* nextGameState, int ticksTime);
	//render an overlay over the screen if applicable
	void render(int ticksTime);
};
#endif
