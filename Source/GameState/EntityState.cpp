#include "EntityState.h"
#include "GameState/DynamicValue.h"
#include "GameState/GameState.h"
#include "GameState/EntityAnimation.h"

//////////////////////////////// EntityState ////////////////////////////////
EntityState::EntityState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, x(newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f))
, renderInterpolatedX(true)
, y(newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f))
, renderInterpolatedY(true)
, z(0)
, entityAnimation(nullptr)
, lastUpdateTicksTime(0) {
}
EntityState::~EntityState() {}
//copy the state of the other entity
void EntityState::copyEntityState(EntityState* other) {
	x.set(other->x.get());
	renderInterpolatedX = other->renderInterpolatedX;
	y.set(other->y.get());
	renderInterpolatedY = other->renderInterpolatedY;
	z = other->z;
	entityAnimation.set(other->entityAnimation.get());
	lastUpdateTicksTime = other->lastUpdateTicksTime;
}
//return the entity's x coordinate at the given time that we should use for rendering the world
float EntityState::getRenderCenterWorldX(int ticksTime) {
	return x.get()->getValue(renderInterpolatedX ? ticksTime - lastUpdateTicksTime : 0);
}
//return the entity's y coordinate at the given time that we should use for rendering the world
float EntityState::getRenderCenterWorldY(int ticksTime) {
	return y.get()->getValue(renderInterpolatedY ? ticksTime - lastUpdateTicksTime : 0);
}
//set the position to the given position at the given time, preserving the velocity
void EntityState::setPosition(float pX, float pY, int pLastUpdateTicksTime) {
	//get our original position
	float originalXPosition = x.get()->getValue(0);
	float originalYPosition = y.get()->getValue(0);
	//find how far we have to go
	int timediff = pLastUpdateTicksTime - lastUpdateTicksTime;
	float addX = pX - x.get()->getValue(timediff);
	float addY = pY - y.get()->getValue(timediff);
	//set a new position offset by how far we have to move
	x.set(x.get()->copyWithConstantValue(originalXPosition + addX));
	y.set(y.get()->copyWithConstantValue(originalYPosition + addY));
}
//set the velocity to the given velocity at the given time, preserving the position
void EntityState::setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime) {
	int timediff = pLastUpdateTicksTime - lastUpdateTicksTime;
	x.set(vx->copyWithConstantValue(x.get()->getValue(timediff)));
	y.set(vy->copyWithConstantValue(y.get()->getValue(timediff)));
	lastUpdateTicksTime = pLastUpdateTicksTime;
}

//////////////////////////////// DynamicCameraAnchor ////////////////////////////////
DynamicCameraAnchor::DynamicCameraAnchor(objCounterParameters())
: EntityState(objCounterArguments())
, shouldSwitchToPlayerCamera(false) {
}
DynamicCameraAnchor::~DynamicCameraAnchor() {}
//initialize and return a DynamicCameraAnchor
DynamicCameraAnchor* DynamicCameraAnchor::produce(objCounterParameters()) {
	initializeWithNewFromPool(s, DynamicCameraAnchor)
	s->shouldSwitchToPlayerCamera = false;
	return s;
}
pooledReferenceCounterDefineRelease(DynamicCameraAnchor)
//update this dynamic camera anchor
void DynamicCameraAnchor::updateWithPreviousDynamicCameraAnchor(DynamicCameraAnchor* prev, int ticksTime) {
	//TODO: advance our animation
	copyEntityState(prev);
}
//TODO: descibe this
//TODO: what causes this to stop being the camera anchor?
void DynamicCameraAnchor::setNextCamera(GameState* nextGameState, int ticksTime) {
	//TODO: set a different camera anchor at the end of our animation
	if (shouldSwitchToPlayerCamera)
		nextGameState->setPlayerCamera();
	else
		nextGameState->setDynamicCamera();
}
