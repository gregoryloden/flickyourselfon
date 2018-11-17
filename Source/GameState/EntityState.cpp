#include "EntityState.h"
#include "GameState/DynamicValue.h"

EntityState::EntityState(objCounterParametersComma() float xPosition, float yPosition)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
x(callNewFromPool(CompositeLinearValue)->set(xPosition, 0.0f))
, renderInterpolatedX(true)
, y(callNewFromPool(CompositeLinearValue)->set(xPosition, 0.0f))
, renderInterpolatedY(true)
, z(0)
, lastUpdateTicksTime(0) {
}
EntityState::~EntityState() {
	x->release();
	y->release();
}
//return the entity's x coordinate at the given time that we should use for rendering the world
float EntityState::getRenderCenterX(int ticksTime) {
	return x->getValue(renderInterpolatedX ? ticksTime - lastUpdateTicksTime : 0);
}
//return the entity's y coordinate at the given time that we should use for rendering the world
float EntityState::getRenderCenterY(int ticksTime) {
	return y->getValue(renderInterpolatedY ? ticksTime - lastUpdateTicksTime : 0);
}
//set the position to the given position at the given time
void EntityState::setVelocity(float xVelocityPerTick, float yVelocityPerTick, int pLastUpdateTicksTime) {
	x->release();
	x = callNewFromPool(CompositeLinearValue)->set(x->getValue(0), xVelocityPerTick);
	y->release();
	y = callNewFromPool(CompositeLinearValue)->set(y->getValue(0), xVelocityPerTick);
	lastUpdateTicksTime = pLastUpdateTicksTime;
}
//copy the state of the other entity
void EntityState::copyEntityState(EntityState* other) {
	x->release();
	x = other->x;
	x->retain();
	renderInterpolatedX = other->renderInterpolatedX;
	y->release();
	y = other->y;
	y->retain();
	renderInterpolatedY = other->renderInterpolatedY;
	z = other->z;
	lastUpdateTicksTime = other->lastUpdateTicksTime;
}
StaticCameraAnchor::StaticCameraAnchor(objCounterParametersComma() float cameraX, float cameraY)
: EntityState(objCounterArgumentsComma() cameraX, cameraY) {
}
StaticCameraAnchor::~StaticCameraAnchor() {}
