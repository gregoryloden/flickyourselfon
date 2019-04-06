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
//initialize and return a Delay
EntityAnimation::Delay* EntityAnimation::Delay::produce(objCounterParametersComma() int pTicksDuration) {
	initializeWithNewFromPool(d, EntityAnimation::Delay)
	d->ticksDuration = pTicksDuration;
	return d;
}
pooledReferenceCounterDefineRelease(EntityAnimation::Delay)
//return that the animation should not continue updating without checking that the delay is finished
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
//initialize and return a SetPosition
EntityAnimation::SetPosition* EntityAnimation::SetPosition::produce(objCounterParametersComma() float pX, float pY) {
	initializeWithNewFromPool(s, EntityAnimation::SetPosition)
	s->x = pX;
	s->y = pY;
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetPosition)
//return that the animation should continue updating after setting the position on the entity state
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
//initialize and return a SetVelocity
EntityAnimation::SetVelocity* EntityAnimation::SetVelocity::produce(
	objCounterParametersComma() DynamicValue* pVx, DynamicValue* pVy)
{
	initializeWithNewFromPool(s, EntityAnimation::SetVelocity)
	s->vx.set(pVx);
	s->vy.set(pVy);
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetVelocity)
//release components before this is returned to the pool
void EntityAnimation::SetVelocity::prepareReturnToPool() {
	vx.set(nullptr);
	vy.set(nullptr);
}
//return that the animation should continue updating after setting the velocity on the entity state
bool EntityAnimation::SetVelocity::handle(EntityState* entityState, int ticksTime) {
	entityState->setVelocity(vx.get(), vy.get(), ticksTime);
	return true;
}

//////////////////////////////// EntityAnimation::SetSpriteAnimation ////////////////////////////////
EntityAnimation::SetSpriteAnimation::SetSpriteAnimation(objCounterParameters())
: Component(objCounterArguments())
, animation(nullptr) {
}
EntityAnimation::SetSpriteAnimation::~SetSpriteAnimation() {
	//don't delete the animation, something else owns it
}
//initialize and return a SetSpriteAnimation
EntityAnimation::SetSpriteAnimation* EntityAnimation::SetSpriteAnimation::produce(
	objCounterParametersComma() SpriteAnimation* pAnimation)
{
	initializeWithNewFromPool(s, EntityAnimation::SetSpriteAnimation)
	s->animation = pAnimation;
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetSpriteAnimation)
//return that the animation should continue updating after setting the sprite animation on the entity state
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
//initialize and return a SetSpriteDirection
EntityAnimation::SetSpriteDirection* EntityAnimation::SetSpriteDirection::produce(
	objCounterParametersComma() SpriteDirection pDirection)
{
	initializeWithNewFromPool(s, EntityAnimation::SetSpriteDirection)
	s->direction = pDirection;
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetSpriteDirection)
//return that the animation should continue updating after setting the sprite direction on the entity state
bool EntityAnimation::SetSpriteDirection::handle(EntityState* entityState, int ticksTime) {
	entityState->setSpriteDirection(direction);
	return true;
}

//////////////////////////////// EntityAnimation::SwitchToPlayerCamera ////////////////////////////////
EntityAnimation::SwitchToPlayerCamera::SwitchToPlayerCamera(objCounterParameters())
: Component(objCounterArguments()) {
}
EntityAnimation::SwitchToPlayerCamera::~SwitchToPlayerCamera() {}
//initialize and return a SwitchToPlayerCamera
EntityAnimation::SwitchToPlayerCamera* EntityAnimation::SwitchToPlayerCamera::produce(objCounterParameters()) {
	initializeWithNewFromPool(s, EntityAnimation::SwitchToPlayerCamera)
	return s;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SwitchToPlayerCamera)
//return that the animation should continue updating after setting that the entity state should switch to the player camera
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
//initialize and return an EntityAnimation
EntityAnimation* EntityAnimation::produce(
	objCounterParametersComma() int pStartTicksTime, vector<ReferenceCounterHolder<Component>> pComponents)
{
	initializeWithNewFromPool(e, EntityAnimation)
	e->lastUpdateTicksTime = pStartTicksTime;
	e->components = pComponents;
	e->nextComponentIndex = 0;
	return e;
}
pooledReferenceCounterDefineRelease(EntityAnimation)
//release components before this is returned to the pool
void EntityAnimation::prepareReturnToPool() {
	components.clear();
}
//update this animation- process any components that happened since the last time this was updated
//while this is state that is both shared across game states and mutated, the mutations only affect the state being updated
//return whether the given state was updated (false means that the default update logic for the state should be used)
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
//return the total ticks duration of all the components (really just the Delays)
int EntityAnimation::getTotalTicksDuration() {
	int totalTicksDuration = 0;
	for (ReferenceCounterHolder<Component>& component : components)
		totalTicksDuration += component.get()->getDelayTicksDuration();
	return totalTicksDuration;
}
