#include "EntityState.h"
#include "GameState/DynamicValue.h"
#include "GameState/GameState.h"
#include "GameState/EntityAnimation.h"

//////////////////////////////// EntityState ////////////////////////////////
EntityState::EntityState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, x(nullptr)
, renderInterpolatedX(true)
, y(nullptr)
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
//set the position to the given position at the given time
void EntityState::setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime) {
	int timediff = pLastUpdateTicksTime - lastUpdateTicksTime;
	x.set(vx->copyWithConstantValue(x.get()->getValue(timediff)));
	y.set(vy->copyWithConstantValue(y.get()->getValue(timediff)));
	lastUpdateTicksTime = pLastUpdateTicksTime;
}

//////////////////////////////// StaticCameraAnchor ////////////////////////////////
StaticCameraAnchor::StaticCameraAnchor(objCounterParameters())
: EntityState(objCounterArguments()) {
}
StaticCameraAnchor::~StaticCameraAnchor() {}
//initialize and return a StaticCameraAnchor
StaticCameraAnchor* StaticCameraAnchor::produce(objCounterParametersComma() float pX, float pY) {
	initializeWithNewFromPool(s, StaticCameraAnchor)
	s->x.set(newCompositeQuarticValue(pX, 0.0f, 0.0f, 0.0f, 0.0f));
	s->y.set(newCompositeQuarticValue(pY, 0.0f, 0.0f, 0.0f, 0.0f));
	return s;
}
pooledReferenceCounterDefineRelease(StaticCameraAnchor)
//TODO: descibe this
//TODO: what causes this to stop being the camera anchor?
void StaticCameraAnchor::setNextCamera(GameState* nextGameState, int ticksTime) {
	//TODO: set a different camera anchor at the end of our animation
	nextGameState->setProvidedCamera(newStaticCameraAnchor(x.get()->getValue(0), y.get()->getValue(0)));
}
