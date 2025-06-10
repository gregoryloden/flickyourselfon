#ifndef ENTITY_STATE_H
#define ENTITY_STATE_H
#include "Util/PooledReferenceCounter.h"

#define newDynamicCameraAnchor() produceWithoutArgs(DynamicCameraAnchor)
#define newParticle(x, y, r, g, b, isAbovePlayer) produceWithArgs(Particle, x, y, r, g, b, isAbovePlayer)

class DynamicValue;
class EntityAnimation;
class GameState;
class Hint;
class KickResetSwitchUndoState;
class SpriteAnimation;
namespace EntityAnimationTypes {
	class Component;
}

enum class SpriteDirection: int {
	Right = 0,
	Up = 1,
	Left = 2,
	Down = 3
};
class EntityState: public PooledReferenceCounter {
private:
	static GLuint postZoomFrameBufferId;
	static GLuint postZoomRenderBufferId;
	static GLuint preZoomFrameBufferId;
	static GLuint preZoomTextureId;

protected:
	ReferenceCounterHolder<DynamicValue> x;
	bool renderInterpolatedX;
	ReferenceCounterHolder<DynamicValue> y;
	bool renderInterpolatedY;
	ReferenceCounterHolder<EntityAnimation> entityAnimation;
	ReferenceCounterHolder<DynamicValue> zoom;
	int lastUpdateTicksTime;

public:
	EntityState(objCounterParameters());
	virtual ~EntityState();

	bool hasAnimation() { return entityAnimation.get() != nullptr; }
	//begin a sprite animation if applicable
	virtual void setSpriteAnimation(SpriteAnimation* spriteAnimation, int pSpriteAnimationStartTicksTime) {}
	//set the direction for this state's sprite, if it has one
	virtual void setDirection(SpriteDirection pSpriteDirection) {}
	//set the position of a ghost sprite that this entity state should render
	virtual void setGhostSprite(bool show, float pX, float pY, SpriteDirection direction, int ticksTime) {}
	//mark that the player camera should be used as the next camera
	virtual void setShouldSwitchToPlayerCamera() {}
	//set a dynamic color that should be used to render an overlay over the screen
	virtual void setScreenOverlayColor(
		DynamicValue* r, DynamicValue* g, DynamicValue* b, DynamicValue* a, int pLastUpdateTicksTime) {}
	//tell the map to kick a switch at the given time
	virtual void mapKickSwitch(short switchId, bool moveRailsForward, bool allowRadioTowerAnimation, int ticksTime) {}
	//tell the map to kick a reset switch at the given time
	virtual void mapKickResetSwitch(short resetSwitchId, KickResetSwitchUndoState* kickResetSwitchUndoState, int ticksTime) {}
	//spawn a particle with the given SpriteAnimation
	virtual void spawnParticle(
		float pX, float pY, SpriteAnimation* pAnimation, SpriteDirection pDirection, int particleStartTicksTime) {}
	//generate a hint based on our current state, or use the given state if present
	virtual void generateHint(Hint* useHint, int ticksTime) {}
	//copy the state of the other EntityState
	void copyEntityState(EntityState* other);
	//load the framebuffers to use for zooming
	static void setupZoomFrameBuffers();
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
	static float getRenderCenterScreenYFromWorldY(float worldY, EntityState* camera, int ticksTime);
	//get the duration of our entity animation, if we have one
	int getAnimationTicksDuration();
	//get a sprite direction based on movement velocity
	static SpriteDirection getSpriteDirection(float pX, float pY);
	//get a sprite direction in the opposite direction of the given sprite direction
	static SpriteDirection getOppositeDirection(SpriteDirection spriteDirection);
	//set the position to the given position at the given time, preserving the velocity
	void setPosition(float pX, float pY, int pLastUpdateTicksTime);
	//set the velocity to the given velocity at the given time, preserving the position
	void setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime);
	//start the given entity animation
	void beginEntityAnimation(vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components, int ticksTime);
	//setup rendering to render a zoomed image
	//returns the zoom level being used; if the value is 1, zoom is not applied and endZoom() is not neeeded
	float renderBeginZoom(int ticksTime);
	//finish rendering the zoomed image at the given zoom value (returned by beginZoom()), and render it to the screen
	//not needed if beginZoom() returned 1
	void renderEndZoom(float zoomValue);
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
	void updateWithPreviousDynamicCameraAnchor(DynamicCameraAnchor* prev, bool hasKeyboardControl, int ticksTime);
	//use this color to render an overlay on the screen
	virtual void setScreenOverlayColor(
		DynamicValue* r, DynamicValue* g, DynamicValue* b, DynamicValue* a, int pLastUpdateTicksTime);
	//use this anchor unless we've been told to switch to the player camera
	virtual void setNextCamera(GameState* nextGameState, int ticksTime);
	//render an overlay over the screen if applicable
	void render(int ticksTime);
};
class Particle: public EntityState {
private:
	SpriteAnimation* spriteAnimation;
	int spriteAnimationStartTicksTime;
	SpriteDirection spriteDirection;
	float r;
	float g;
	float b;
	bool isAbovePlayer;

public:
	Particle(objCounterParameters());
	virtual ~Particle();

	bool getIsAbovePlayer() { return isAbovePlayer; }
	//don't do anything based on a camera change
	virtual void setNextCamera(GameState* nextGameState, int ticksTime) {}
	//initialize and return a Particle
	static Particle* produce(objCounterParametersComma() float pX, float pY, float pR, float pG, float pB, bool pIsAbovePlayer);
	//copy the state of the other Particle
	void copyParticle(Particle* other);
	//release a reference to this Particle and return it to the pool if applicable
	virtual void release();
	//set the animation to the given animation at the given time
	virtual void setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime);
	//set the sprite direction, and also set x and y directions to match
	virtual void setDirection(SpriteDirection pSpriteDirection);
	//update this Particle by reading from the previous state
	//returns whether the animation is still running
	bool updateWithPreviousParticle(Particle* prev, int ticksTime);
	//render the sprite animation if applicable
	void render(EntityState* camera, int ticksTime);
};
#endif
