#include "EntityAnimation.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityState.h"
#include "Sprites/SpriteAnimation.h"

//////////////////////////////// EntityAnimation::Component ////////////////////////////////
EntityAnimation::Component::Component(objCounterParameters())
: PooledReferenceCounter(objCounterArguments()) {
}
EntityAnimation::Component::~Component() {}

//////////////////////////////// EntityAnimation::Delay ////////////////////////////////
EntityAnimation::Delay::Delay(objCounterParameters())
: Component(objCounterArguments())
, ticksDuration(0) {
}
EntityAnimation::Delay::~Delay() {}
EntityAnimation::Delay* EntityAnimation::Delay::produce(objCounterParametersComma() int pTicksDuration) {
	initializeWithNewFromPool(d, EntityAnimation::Delay)
	d->ticksDuration = pTicksDuration;
	return d;
}
pooledReferenceCounterDefineRelease(EntityAnimation::Delay)
bool EntityAnimation::Delay::handle(EntityState* entityState, int ticksTime) {
	return false;
}

//////////////////////////////// EntityAnimation::SetPosition ////////////////////////////////
EntityAnimation::SetPosition::SetPosition(objCounterParameters())
: Component(objCounterArguments())
, x(0.0f)
, y(0.0f) {
}
EntityAnimation::SetPosition::~SetPosition() {}
EntityAnimation::SetPosition* EntityAnimation::SetPosition::produce(objCounterParametersComma() float pX, float pY) {
	initializeWithNewFromPool(s, EntityAnimation::SetPosition)
	s->x = pX;
	s->y = pY;
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetPosition)
bool EntityAnimation::SetPosition::handle(EntityState* entityState, int ticksTime) {
	entityState->setPosition(x, y, ticksTime);
	return true;
}

//////////////////////////////// EntityAnimation::SetVelocity ////////////////////////////////
EntityAnimation::SetVelocity::SetVelocity(objCounterParameters())
: Component(objCounterArguments())
, vx(nullptr)
, vy(nullptr) {
}
EntityAnimation::SetVelocity::~SetVelocity() {}
EntityAnimation::SetVelocity* EntityAnimation::SetVelocity::produce(
	objCounterParametersComma() DynamicValue* pVx, DynamicValue* pVy)
{
	initializeWithNewFromPool(s, EntityAnimation::SetVelocity)
	s->vx.set(pVx);
	s->vy.set(pVy);
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetVelocity)
void EntityAnimation::SetVelocity::prepareReturnToPool() {
	vx.set(nullptr);
	vy.set(nullptr);
}
bool EntityAnimation::SetVelocity::handle(EntityState* entityState, int ticksTime) {
	entityState->setVelocity(vx.get(), vy.get(), ticksTime);
	return true;
}
EntityAnimation::SetVelocity* EntityAnimation::SetVelocity::cubicInterpolation(
	float xMoveDistance, float yMoveDistance, float ticksDuration)
{
	//vy = at(t-1) = at^2-at   (a < 0)
	//y = at^3/3-at^2/2
	//1 = a1^3/3-a1^2/2 = a(1/3 - 1/2) = a(-1/6)
	//a = 1/(-1/6) = -6
	//y = -2t^3+3t^2
	float ticksDurationSquared = ticksDuration * ticksDuration;
	float ticksDurationCubed = ticksDuration * ticksDurationSquared;
	return newEntityAnimationSetVelocity(
		newCompositeQuarticValue(
			0.0f, 0.0f, 3.0f * xMoveDistance / ticksDurationSquared, -2.0f * xMoveDistance / ticksDurationCubed, 0.0f),
		newCompositeQuarticValue(
			0.0f, 0.0f, 3.0f * yMoveDistance / ticksDurationSquared, -2.0f * yMoveDistance / ticksDurationCubed, 0.0f));
}

//////////////////////////////// EntityAnimation::SetSpriteAnimation ////////////////////////////////
EntityAnimation::SetSpriteAnimation::SetSpriteAnimation(objCounterParameters())
: Component(objCounterArguments())
, animation(nullptr) {
}
EntityAnimation::SetSpriteAnimation::~SetSpriteAnimation() {
	//don't delete the animation, something else owns it
}
EntityAnimation::SetSpriteAnimation* EntityAnimation::SetSpriteAnimation::produce(
	objCounterParametersComma() SpriteAnimation* pAnimation)
{
	initializeWithNewFromPool(s, EntityAnimation::SetSpriteAnimation)
	s->animation = pAnimation;
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetSpriteAnimation)
bool EntityAnimation::SetSpriteAnimation::handle(EntityState* entityState, int ticksTime) {
	entityState->setSpriteAnimation(animation, ticksTime);
	return true;
}

//////////////////////////////// EntityAnimation::SetSpriteDirection ////////////////////////////////
EntityAnimation::SetSpriteDirection::SetSpriteDirection(objCounterParameters())
: Component(objCounterArguments())
, direction(SpriteDirection::Right) {
}
EntityAnimation::SetSpriteDirection::~SetSpriteDirection() {}
EntityAnimation::SetSpriteDirection* EntityAnimation::SetSpriteDirection::produce(
	objCounterParametersComma() SpriteDirection pDirection)
{
	initializeWithNewFromPool(s, EntityAnimation::SetSpriteDirection)
	s->direction = pDirection;
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetSpriteDirection)
bool EntityAnimation::SetSpriteDirection::handle(EntityState* entityState, int ticksTime) {
	entityState->setSpriteDirection(direction);
	return true;
}

//////////////////////////////// EntityAnimation::SetGhostSprite ////////////////////////////////
EntityAnimation::SetGhostSprite::SetGhostSprite(objCounterParameters())
: Component(objCounterArguments())
, show(true)
, x(0.0f)
, y(0.0f) {
}
EntityAnimation::SetGhostSprite::~SetGhostSprite() {}
EntityAnimation::SetGhostSprite* EntityAnimation::SetGhostSprite::produce(
	objCounterParametersComma() bool pShow, float pX, float pY)
{
	initializeWithNewFromPool(s, EntityAnimation::SetGhostSprite)
	s->show = pShow;
	s->x = pX;
	s->y = pY;
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetGhostSprite)
bool EntityAnimation::SetGhostSprite::handle(EntityState* entityState, int ticksTime) {
	entityState->setGhostSprite(show, x, y, ticksTime);
	return true;
}

//////////////////////////////// EntityAnimation::SetScreenOverlayColor ////////////////////////////////
EntityAnimation::SetScreenOverlayColor::SetScreenOverlayColor(objCounterParameters())
: Component(objCounterArguments())
, r(nullptr)
, g(nullptr)
, b(nullptr)
, a(nullptr) {
}
EntityAnimation::SetScreenOverlayColor::~SetScreenOverlayColor() {}
EntityAnimation::SetScreenOverlayColor* EntityAnimation::SetScreenOverlayColor::produce(
	objCounterParametersComma() DynamicValue* pR, DynamicValue* pG, DynamicValue* pB, DynamicValue* pA)
{
	initializeWithNewFromPool(s, EntityAnimation::SetScreenOverlayColor)
	s->r.set(pR);
	s->g.set(pG);
	s->b.set(pB);
	s->a.set(pA);
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetScreenOverlayColor)
bool EntityAnimation::SetScreenOverlayColor::handle(EntityState* entityState, int ticksTime) {
	entityState->setScreenOverlayColor(r.get(), g.get(), b.get(), a.get(), ticksTime);
	return true;
}

//////////////////////////////// EntityAnimation::MapKickSwitch ////////////////////////////////
EntityAnimation::MapKickSwitch::MapKickSwitch(objCounterParameters())
: Component(objCounterArguments())
, switchId(0)
, allowRadioTowerAnimation(true) {
}
EntityAnimation::MapKickSwitch::~MapKickSwitch() {}
EntityAnimation::MapKickSwitch* EntityAnimation::MapKickSwitch::produce(
	objCounterParametersComma() short pSwitchId, bool pAllowRadioTowerAnimation)
{
	initializeWithNewFromPool(m, EntityAnimation::MapKickSwitch)
	m->switchId = pSwitchId;
	m->allowRadioTowerAnimation = pAllowRadioTowerAnimation;
	return m;
}
pooledReferenceCounterDefineRelease(EntityAnimation::MapKickSwitch)
bool EntityAnimation::MapKickSwitch::handle(EntityState* entityState, int ticksTime) {
	entityState->mapKickSwitch(switchId, allowRadioTowerAnimation, ticksTime);
	return true;
}

//////////////////////////////// EntityAnimation::MapKickResetSwitch ////////////////////////////////
EntityAnimation::MapKickResetSwitch::MapKickResetSwitch(objCounterParameters())
: Component(objCounterArguments())
, resetSwitchId(0) {
}
EntityAnimation::MapKickResetSwitch::~MapKickResetSwitch() {}
EntityAnimation::MapKickResetSwitch* EntityAnimation::MapKickResetSwitch::produce(
	objCounterParametersComma() short pResetSwitchId)
{
	initializeWithNewFromPool(m, EntityAnimation::MapKickResetSwitch)
	m->resetSwitchId = pResetSwitchId;
	return m;
}
pooledReferenceCounterDefineRelease(EntityAnimation::MapKickResetSwitch)
bool EntityAnimation::MapKickResetSwitch::handle(EntityState* entityState, int ticksTime) {
	entityState->mapKickResetSwitch(resetSwitchId, ticksTime);
	return true;
}

//////////////////////////////// EntityAnimation::SwitchToPlayerCamera ////////////////////////////////
EntityAnimation::SwitchToPlayerCamera::SwitchToPlayerCamera(objCounterParameters())
: Component(objCounterArguments()) {
}
EntityAnimation::SwitchToPlayerCamera::~SwitchToPlayerCamera() {}
EntityAnimation::SwitchToPlayerCamera* EntityAnimation::SwitchToPlayerCamera::produce(objCounterParameters()) {
	initializeWithNewFromPool(s, EntityAnimation::SwitchToPlayerCamera)
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SwitchToPlayerCamera)
bool EntityAnimation::SwitchToPlayerCamera::handle(EntityState* entityState, int ticksTime) {
	entityState->setShouldSwitchToPlayerCamera();
	return true;
}

//////////////////////////////// EntityAnimation ////////////////////////////////
EntityAnimation::EntityAnimation(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, lastUpdateTicksTime(0)
, components()
, nextComponentIndex(0) {
}
EntityAnimation::~EntityAnimation() {}
EntityAnimation* EntityAnimation::produce(
	objCounterParametersComma() int pStartTicksTime, vector<ReferenceCounterHolder<Component>>* pComponents)
{
	initializeWithNewFromPool(e, EntityAnimation)
	e->lastUpdateTicksTime = pStartTicksTime;
	e->components = *pComponents;
	e->nextComponentIndex = 0;
	return e;
}
pooledReferenceCounterDefineRelease(EntityAnimation)
void EntityAnimation::prepareReturnToPool() {
	components.clear();
}
bool EntityAnimation::update(EntityState* entityState, int ticksTime) {
	bool updated = false;
	while (nextComponentIndex < (int)components.size()) {
		Component* c = components[nextComponentIndex].get();
		if (c->handle(entityState, lastUpdateTicksTime))
			updated = true;
		//if the component needs to do more than just handle the entity state, check how long it delays the animation
		else {
			int endTicksTime = lastUpdateTicksTime + c->getDelayTicksDuration();
			//we still have time left, return that we need to do a future update
			if (ticksTime < endTicksTime)
				return true;
			lastUpdateTicksTime = endTicksTime;
		}
		nextComponentIndex++;
	}
	return updated;
}
int EntityAnimation::getComponentTotalTicksDuration(vector<ReferenceCounterHolder<Component>>& pComponents) {
	int totalTicksDuration = 0;
	for (ReferenceCounterHolder<Component>& component : pComponents)
		totalTicksDuration += component.get()->getDelayTicksDuration();
	return totalTicksDuration;
}
int EntityAnimation::getTotalTicksDuration() {
	return getComponentTotalTicksDuration(components);
}

//////////////////////////////// Holder_EntityAnimationComponentVector ////////////////////////////////
Holder_EntityAnimationComponentVector::Holder_EntityAnimationComponentVector(
	vector<ReferenceCounterHolder<EntityAnimation::Component>>* pVal)
: val(pVal) {
}
Holder_EntityAnimationComponentVector::~Holder_EntityAnimationComponentVector() {
	//don't delete the vector, it's owned by something else
}
