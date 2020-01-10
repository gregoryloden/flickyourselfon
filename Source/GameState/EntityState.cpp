#include "EntityState.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/GameState.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

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
//return the entity's screen x coordinate at the given time
float EntityState::getRenderCenterScreenX(EntityState* camera, int ticksTime) {
	return getRenderCenterScreenXFromWorldX(getRenderCenterWorldX(ticksTime), camera, ticksTime);
}
//return the screen x coordinate at the given time from the given world x coordinate
float EntityState::getRenderCenterScreenXFromWorldX(float worldX, EntityState* camera, int ticksTime) {
	//convert these to ints first to align with the map in case the camera is not this entity
	int screenXOffset = (int)worldX - (int)camera->getRenderCenterWorldX(ticksTime);
	return (float)screenXOffset + (float)Config::gameScreenWidth * 0.5f;
}
//return the entity's screen y coordinate at the given time
float EntityState::getRenderCenterScreenY(EntityState* camera, int ticksTime) {
	return getRenderCenterScreenYFromWorldY(getRenderCenterWorldY(ticksTime), camera, ticksTime);
}
//return the screen y coordinate at the given time from the given world y coordinate
float EntityState::getRenderCenterScreenYFromWorldY(float worldY, EntityState* camera, int ticksTime) {
	//convert these to ints first to align with the map in case the camera is not this entity
	int screenYOffset = (int)worldY - (int)camera->getRenderCenterWorldY(ticksTime);
	return (float)screenYOffset + (float)Config::gameScreenHeight * 0.5f;
}
//get the duration of our entity animation, if we have one
int EntityState::getAnimationTicksDuration() {
	return entityAnimation.get() != nullptr ? entityAnimation.get()->getTotalTicksDuration() : 0;
}
//get a sprite direction based on movement velocity
SpriteDirection EntityState::getSpriteDirection(float x, float y) {
	return abs(y) >= abs(x)
		? y >= 0 ? SpriteDirection::Down : SpriteDirection::Up
		: x >= 0 ? SpriteDirection::Right : SpriteDirection::Left;
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
//start the given entity animation
void EntityState::beginEntityAnimation(Holder_EntityAnimationComponentVector* componentsHolder, int ticksTime) {
	entityAnimation.set(newEntityAnimation(ticksTime, componentsHolder->val));
	//update it once to get it started
	entityAnimation.get()->update(this, ticksTime);
}

//////////////////////////////// DynamicCameraAnchor ////////////////////////////////
DynamicCameraAnchor::DynamicCameraAnchor(objCounterParameters())
: EntityState(objCounterArguments())
, screenOverlayR(nullptr)
, screenOverlayG(nullptr)
, screenOverlayB(nullptr)
, screenOverlayA(nullptr)
, shouldSwitchToPlayerCamera(false) {
}
DynamicCameraAnchor::~DynamicCameraAnchor() {}
//initialize and return a DynamicCameraAnchor
DynamicCameraAnchor* DynamicCameraAnchor::produce(objCounterParameters()) {
	initializeWithNewFromPool(d, DynamicCameraAnchor)
	d->screenOverlayR.set(newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	d->screenOverlayG.set(newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	d->screenOverlayB.set(newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	d->screenOverlayA.set(newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	d->shouldSwitchToPlayerCamera = false;
	return d;
}
//copy the other state
void DynamicCameraAnchor::copyDynamicCameraAnchor(DynamicCameraAnchor* other) {
	copyEntityState(other);
	screenOverlayR.set(other->screenOverlayR.get());
	screenOverlayG.set(other->screenOverlayG.get());
	screenOverlayB.set(other->screenOverlayB.get());
	screenOverlayA.set(other->screenOverlayA.get());
}
pooledReferenceCounterDefineRelease(DynamicCameraAnchor)
//update this dynamic camera anchor
void DynamicCameraAnchor::updateWithPreviousDynamicCameraAnchor(DynamicCameraAnchor* prev, int ticksTime) {
	copyDynamicCameraAnchor(prev);
	if (entityAnimation.get() != nullptr) {
		if (entityAnimation.get()->update(this, ticksTime))
			return;
		entityAnimation.set(nullptr);
	}
}
//use this color to render an overlay on the screen
void DynamicCameraAnchor::setScreenOverlayColor(
	DynamicValue* r, DynamicValue* g, DynamicValue* b, DynamicValue* a, int pLastUpdateTicksTime)
{
	screenOverlayR.set(r);
	screenOverlayG.set(g);
	screenOverlayB.set(b);
	screenOverlayA.set(a);
	lastUpdateTicksTime = pLastUpdateTicksTime;
}
//use this anchor unless we've been told to switch to the player camera
void DynamicCameraAnchor::setNextCamera(GameState* nextGameState, int ticksTime) {
	if (shouldSwitchToPlayerCamera)
		nextGameState->setPlayerCamera();
	else
		nextGameState->setDynamicCamera();
}
//render an overlay over the screen if applicable
void DynamicCameraAnchor::render(int ticksTime) {
	int timediff = ticksTime - lastUpdateTicksTime;
	GLfloat r = (GLfloat)screenOverlayR.get()->getValue(timediff);
	GLfloat g = (GLfloat)screenOverlayG.get()->getValue(timediff);
	GLfloat b = (GLfloat)screenOverlayB.get()->getValue(timediff);
	GLfloat a = (GLfloat)screenOverlayA.get()->getValue(timediff);
	if (a > 0)
		SpriteSheet::renderFilledRectangle(r, g, b, a, 0, 0, (GLint)Config::gameScreenWidth, (GLint)Config::gameScreenHeight);
}
