#include "EntityState.h"
#include "GameState/DynamicValue.h"

EntityState::EntityState(objCounterParametersComma() float xPosition, float yPosition)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
x(callNewFromPool(CompositeQuarticValue)->set(xPosition, 0.0f, 0.0f, 0.0f, 0.0f))
, renderInterpolatedX(true)
, y(callNewFromPool(CompositeQuarticValue)->set(xPosition, 0.0f, 0.0f, 0.0f, 0.0f))
, renderInterpolatedY(true)
, z(0)
, lastUpdateTicksTime(0) {
}
EntityState::~EntityState() {}
//return the entity's x coordinate at the given time that we should use for rendering the world
float EntityState::getRenderCenterX(int ticksTime) {
	return x.get()->getValue(renderInterpolatedX ? ticksTime - lastUpdateTicksTime : 0);
}
//return the entity's y coordinate at the given time that we should use for rendering the world
float EntityState::getRenderCenterY(int ticksTime) {
	return y.get()->getValue(renderInterpolatedY ? ticksTime - lastUpdateTicksTime : 0);
}
//set the position to the given position at the given time
void EntityState::setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime) {
	x.set(vx->copyWithConstantValue(x.get()->getValue(0)));
	y.set(vy->copyWithConstantValue(y.get()->getValue(0)));
	lastUpdateTicksTime = pLastUpdateTicksTime;
}
//copy the state of the other entity
void EntityState::copyEntityState(EntityState* other) {
	x.set(other->x.get());
	renderInterpolatedX = other->renderInterpolatedX;
	y.set(other->y.get());
	renderInterpolatedY = other->renderInterpolatedY;
	z = other->z;
	lastUpdateTicksTime = other->lastUpdateTicksTime;
}
StaticCameraAnchor::StaticCameraAnchor(objCounterParametersComma() float cameraX, float cameraY)
: EntityState(objCounterArgumentsComma() cameraX, cameraY) {
}
StaticCameraAnchor::~StaticCameraAnchor() {}
