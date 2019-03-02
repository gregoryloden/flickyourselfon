#include "EntityState.h"
#include "GameState/DynamicValue.h"

EntityState::EntityState(objCounterParametersComma() float xPosition, float yPosition)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
x(newCompositeQuarticValue(xPosition, 0.0f, 0.0f, 0.0f, 0.0f))
, renderInterpolatedX(true)
, y(newCompositeQuarticValue(yPosition, 0.0f, 0.0f, 0.0f, 0.0f))
, renderInterpolatedY(true)
, z(0)
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
StaticCameraAnchor::StaticCameraAnchor(objCounterParametersComma() float cameraX, float cameraY)
: EntityState(objCounterArgumentsComma() cameraX, cameraY) {
}
StaticCameraAnchor::~StaticCameraAnchor() {}
