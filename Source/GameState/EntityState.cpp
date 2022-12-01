#include "EntityState.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/GameState.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

//////////////////////////////// EntityState ////////////////////////////////
EntityState::EntityState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, x(newConstantValue(0.0f))
, renderInterpolatedX(true)
, y(newConstantValue(0.0f))
, renderInterpolatedY(true)
, entityAnimation(nullptr)
, lastUpdateTicksTime(0) {
}
EntityState::~EntityState() {}
void EntityState::copyEntityState(EntityState* other) {
	x.set(other->x.get());
	renderInterpolatedX = other->renderInterpolatedX;
	y.set(other->y.get());
	renderInterpolatedY = other->renderInterpolatedY;
	entityAnimation.set(other->entityAnimation.get());
	lastUpdateTicksTime = other->lastUpdateTicksTime;
}
float EntityState::getRenderCenterWorldX(int ticksTime) {
	return x.get()->getValue(renderInterpolatedX ? ticksTime - lastUpdateTicksTime : 0);
}
float EntityState::getRenderCenterWorldY(int ticksTime) {
	return y.get()->getValue(renderInterpolatedY ? ticksTime - lastUpdateTicksTime : 0);
}
float EntityState::getRenderCenterScreenX(EntityState* camera, int ticksTime) {
	return getRenderCenterScreenXFromWorldX(getRenderCenterWorldX(ticksTime), camera, ticksTime);
}
float EntityState::getRenderCenterScreenXFromWorldX(float worldX, EntityState* camera, int ticksTime) {
	//convert these to ints first to align with the map in case the camera is not this entity
	int screenXOffset = (int)worldX - (int)camera->getRenderCenterWorldX(ticksTime);
	return (float)screenXOffset + (float)Config::gameScreenWidth * 0.5f;
}
float EntityState::getRenderCenterScreenY(EntityState* camera, int ticksTime) {
	return getRenderCenterScreenYFromWorldY(getRenderCenterWorldY(ticksTime), camera, ticksTime);
}
float EntityState::getRenderCenterScreenYFromWorldY(float worldY, EntityState* camera, int ticksTime) {
	//convert these to ints first to align with the map in case the camera is not this entity
	int screenYOffset = (int)worldY - (int)camera->getRenderCenterWorldY(ticksTime);
	return (float)screenYOffset + (float)Config::gameScreenHeight * 0.5f;
}
int EntityState::getAnimationTicksDuration() {
	return entityAnimation.get() != nullptr ? entityAnimation.get()->getTotalTicksDuration() : 0;
}
SpriteDirection EntityState::getSpriteDirection(float x, float y) {
	return abs(y) >= abs(x)
		? y >= 0 ? SpriteDirection::Down : SpriteDirection::Up
		: x >= 0 ? SpriteDirection::Right : SpriteDirection::Left;
}
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
void EntityState::setVelocity(DynamicValue* vx, DynamicValue* vy, int pLastUpdateTicksTime) {
	int timediff = pLastUpdateTicksTime - lastUpdateTicksTime;
	x.set(vx->copyWithConstantValue(x.get()->getValue(timediff)));
	y.set(vy->copyWithConstantValue(y.get()->getValue(timediff)));
	lastUpdateTicksTime = pLastUpdateTicksTime;
}
void EntityState::beginEntityAnimation(
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components, int ticksTime)
{
	entityAnimation.set(newEntityAnimation(ticksTime, components));
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
DynamicCameraAnchor* DynamicCameraAnchor::produce(objCounterParameters()) {
	initializeWithNewFromPool(d, DynamicCameraAnchor)
	d->screenOverlayR.set(newConstantValue(0.0f));
	d->screenOverlayG.set(newConstantValue(0.0f));
	d->screenOverlayB.set(newConstantValue(0.0f));
	d->screenOverlayA.set(newConstantValue(0.0f));
	d->shouldSwitchToPlayerCamera = false;
	return d;
}
void DynamicCameraAnchor::copyDynamicCameraAnchor(DynamicCameraAnchor* other) {
	copyEntityState(other);
	screenOverlayR.set(other->screenOverlayR.get());
	screenOverlayG.set(other->screenOverlayG.get());
	screenOverlayB.set(other->screenOverlayB.get());
	screenOverlayA.set(other->screenOverlayA.get());
}
pooledReferenceCounterDefineRelease(DynamicCameraAnchor)
void DynamicCameraAnchor::updateWithPreviousDynamicCameraAnchor(DynamicCameraAnchor* prev, int ticksTime) {
	copyDynamicCameraAnchor(prev);
	if (entityAnimation.get() != nullptr) {
		if (entityAnimation.get()->update(this, ticksTime))
			return;
		entityAnimation.set(nullptr);
	}
}
void DynamicCameraAnchor::setScreenOverlayColor(
	DynamicValue* r, DynamicValue* g, DynamicValue* b, DynamicValue* a, int pLastUpdateTicksTime)
{
	screenOverlayR.set(r);
	screenOverlayG.set(g);
	screenOverlayB.set(b);
	screenOverlayA.set(a);
	lastUpdateTicksTime = pLastUpdateTicksTime;
}
void DynamicCameraAnchor::setNextCamera(GameState* nextGameState, int ticksTime) {
	if (shouldSwitchToPlayerCamera)
		nextGameState->setPlayerCamera();
	else
		nextGameState->setDynamicCamera();
}
void DynamicCameraAnchor::render(int ticksTime) {
	int timediff = ticksTime - lastUpdateTicksTime;
	GLfloat r = (GLfloat)screenOverlayR.get()->getValue(timediff);
	GLfloat g = (GLfloat)screenOverlayG.get()->getValue(timediff);
	GLfloat b = (GLfloat)screenOverlayB.get()->getValue(timediff);
	GLfloat a = (GLfloat)screenOverlayA.get()->getValue(timediff);
	if (a > 0)
		SpriteSheet::renderFilledRectangle(r, g, b, a, 0, 0, (GLint)Config::gameScreenWidth, (GLint)Config::gameScreenHeight);
}
