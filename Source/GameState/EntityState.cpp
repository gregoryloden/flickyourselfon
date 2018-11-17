#include "EntityState.h"

EntityState::EntityState(objCounterParametersComma() float pXPosition, float pYPosition)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
xPosition(pXPosition)
, xVelocityPerTick(0.0f)
, renderInterpolatedXPosition(true)
, yPosition(pYPosition)
, yVelocityPerTick(0.0f)
, renderInterpolatedYPosition(true)
, z(0)
, lastUpdateTicksTime(0) {
}
EntityState::~EntityState() {}
//return the entity's x coordinate at the given time that we should use for rendering the world
float EntityState::getCameraCenterX(int ticksTime) {
	return renderInterpolatedXPosition ? getInterpolatedX(ticksTime) : xPosition;
}
//return the entity's y coordinate at the given time that we should use for rendering the world
float EntityState::getCameraCenterY(int ticksTime) {
	return renderInterpolatedYPosition ? getInterpolatedY(ticksTime) : yPosition;
}
//return the entity's x coordinate at the given time after accounting for velocity
float EntityState::getInterpolatedX(int ticksTime) {
	return xPosition + (float)xVelocityPerTick * (float)(ticksTime - lastUpdateTicksTime);
}
//return the entity's y coordinate at the given time after accounting for velocity
float EntityState::getInterpolatedY(int ticksTime) {
	return yPosition + (float)yVelocityPerTick * (float)(ticksTime - lastUpdateTicksTime);
}
//set the position to the given position at the given time
void EntityState::setPosition(float pXPosition, float pYPosition, char pZ, int pLastUpdateTicksTime) {
	xPosition = pXPosition;
	yPosition = pYPosition;
	z = pZ;
	lastUpdateTicksTime = pLastUpdateTicksTime;
}
//set the position to the given position at the given time
void EntityState::setVelocity(float pXVelocityPerTick, float pYVelocityPerTick) {
	xVelocityPerTick = pXVelocityPerTick;
	yVelocityPerTick = pYVelocityPerTick;
}
//copy the state of the other entity
void EntityState::copyEntityState(EntityState* other) {
	xPosition = other->xPosition;
	xVelocityPerTick = other->xVelocityPerTick;
	renderInterpolatedXPosition = other->renderInterpolatedXPosition;
	yPosition = other->yPosition;
	yVelocityPerTick = other->yVelocityPerTick;
	renderInterpolatedYPosition = other->renderInterpolatedYPosition;
	z = other->z;
	lastUpdateTicksTime = other->lastUpdateTicksTime;
}
StaticCameraAnchor::StaticCameraAnchor(objCounterParametersComma() float cameraX, float cameraY)
: EntityState(objCounterArgumentsComma() cameraX, cameraY) {
}
StaticCameraAnchor::~StaticCameraAnchor() {}
