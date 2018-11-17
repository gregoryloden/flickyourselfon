#include "EntityAnimation.h"
#include "GameState/EntityState.h"
#include "Sprites/SpriteAnimation.h"

EntityAnimation::Component::Component(objCounterParameters())
: PooledReferenceCounter(objCounterArguments()) {
}
EntityAnimation::Component::~Component() {}
//return how long this component should last; only Delay overrides this
int EntityAnimation::Component::getDelayTicksDuration() {
	return 0;
}
EntityAnimation::Delay::Delay(objCounterParameters())
: Component(objCounterArguments())
, ticksDuration(0) {
}
EntityAnimation::Delay::~Delay() {}
//initialize this Delay
EntityAnimation::Delay* EntityAnimation::Delay::set(int pTicksDuration) {
	ticksDuration = pTicksDuration;
	return this;
}
pooledReferenceCounterDefineRelease(EntityAnimation::Delay)
//return that the animation should not proceed without checking that the delay is finished
bool EntityAnimation::Delay::handle(EntityState* entityState, int ticksTime) {
	return false;
}
//return the ticks duration for this delay
int EntityAnimation::Delay::getDelayTicksDuration() {
	return ticksDuration;
}
EntityAnimation::SetVelocity::SetVelocity(objCounterParameters())
: Component(objCounterArguments())
, vx(0.0f)
, vy(0.0f) {
}
EntityAnimation::SetVelocity::~SetVelocity() {}
//initialize this SetVelocity
EntityAnimation::SetVelocity* EntityAnimation::SetVelocity::set(float pVx, float pVy) {
	vx = pVx;
	vy = pVy;
	return this;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetVelocity)
//return that the animation should proceed after setting the velocity on the entity state
bool EntityAnimation::SetVelocity::handle(EntityState* entityState, int ticksTime) {
	entityState->setVelocity(vx, vy, ticksTime);
	return true;
}
EntityAnimation::SetSpriteAnimation::SetSpriteAnimation(objCounterParameters())
: Component(objCounterArguments())
, animation(nullptr) {
}
EntityAnimation::SetSpriteAnimation::~SetSpriteAnimation() {
	//don't delete the animation, something else owns it
}
//initialize this SetSpriteAnimation
EntityAnimation::SetSpriteAnimation* EntityAnimation::SetSpriteAnimation::set(SpriteAnimation* pAnimation) {
	animation = pAnimation;
	return this;
}
pooledReferenceCounterDefineRelease(EntityAnimation::SetSpriteAnimation)
//return that the animation should proceed after setting the sprite animation on the entity state
bool EntityAnimation::SetSpriteAnimation::handle(EntityState* entityState, int ticksTime) {
	entityState->setSpriteAnimation(animation, ticksTime);
	return true;
}
EntityAnimation::EntityAnimation(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, startTicksTime(0)
, components()
, nextComponentIndex(0) {
}
EntityAnimation::~EntityAnimation() {
	for (Component* c : components) {
		c->release();
	}
}
//initialize this EntityAnimation
EntityAnimation* EntityAnimation::set(int pStartTicksTime, vector<Component*> pComponents) {
	for (Component* c : components) {
		c->release();
	}
	startTicksTime = pStartTicksTime;
	components = pComponents;
	nextComponentIndex = 0;
	return this;
}
pooledReferenceCounterDefineRelease(EntityAnimation)
//release components before this is returned to the pool
void EntityAnimation::prepareReturnToPool() {
	for (Component* c : components) {
		c->release();
	}
	components.clear();
}
//update this animation, and process any components that happened since the last time this was updated
//return whether there are more components to process in a future update
bool EntityAnimation::update(EntityState* entityState, int ticksTime) {
	while (nextComponentIndex < (int)components.size()) {
		Component* c = components[nextComponentIndex];
		//if the component needs to do more than just handle the entity state, check how long it delays the animation
		if (!c->handle(entityState, ticksTime)) {
			int endTicksTime = startTicksTime + c->getDelayTicksDuration();
			if (ticksTime < endTicksTime)
				return true;
			startTicksTime = endTicksTime;
		}
		nextComponentIndex++;
	}
	return false;
}
