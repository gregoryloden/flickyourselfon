#include "PooledReferenceCounter.h"
#include "GameState/CollisionRect.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/EntityState.h"
#include "GameState/KickAction.h"
#include "GameState/PauseState.h"
#include "GameState/PlayerState.h"
#include "GameState/MapState/MapState.h"

#define instantiateObjectPoolAndReferenceCounterHolder(className) \
	template class ObjectPool<className>; template class ReferenceCounterHolder<className>;

//////////////////////////////// PooledReferenceCounter ////////////////////////////////
PooledReferenceCounter::PooledReferenceCounter(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
referenceCount(0) {
}
PooledReferenceCounter::~PooledReferenceCounter() {}

//////////////////////////////// ReferenceCounterHolder ////////////////////////////////
template <class ReferenceCountedObject> ReferenceCounterHolder<ReferenceCountedObject>::ReferenceCounterHolder(
	ReferenceCountedObject* pObject)
: object(pObject) {
	if (pObject != nullptr)
		pObject->retain();
}
template <class ReferenceCountedObject> ReferenceCounterHolder<ReferenceCountedObject>::ReferenceCounterHolder(
	const ReferenceCounterHolder<ReferenceCountedObject>& other)
: object(other.object) {
	if (object != nullptr)
		object->retain();
}
template <class ReferenceCountedObject> ReferenceCounterHolder<ReferenceCountedObject>::ReferenceCounterHolder(
	ReferenceCounterHolder<ReferenceCountedObject>&& other)
: object(other.object) {
	if (object != nullptr)
		object->retain();
}
template <class ReferenceCountedObject> ReferenceCounterHolder<ReferenceCountedObject>::~ReferenceCounterHolder() {
	if (object != nullptr)
		object->release();
}
template <class ReferenceCountedObject> void ReferenceCounterHolder<ReferenceCountedObject>::set(
	ReferenceCountedObject* pObject)
{
	if (object != nullptr)
		object->release();
	object = pObject;
	if (pObject != nullptr)
		pObject->retain();
}
template <class ReferenceCountedObject>
ReferenceCounterHolder<ReferenceCountedObject>& ReferenceCounterHolder<ReferenceCountedObject>::operator =(
	const ReferenceCounterHolder<ReferenceCountedObject>& other)
{
	set(other.object);
	return *this;
}
template <class ReferenceCountedObject>
ReferenceCounterHolder<ReferenceCountedObject>& ReferenceCounterHolder<ReferenceCountedObject>::operator =(
	ReferenceCounterHolder<ReferenceCountedObject>&& other)
{
	set(other.object);
	return *this;
}

//////////////////////////////// ObjectPool ////////////////////////////////
template <class PooledObject> vector<PooledObject*> ObjectPool<PooledObject>::pool;
template <class PooledObject> PooledObject* ObjectPool<PooledObject>::newFromPool(objCounterParameters()) {
	if (pool.size() > 0) {
		PooledObject* p = pool.back();
		pool.pop_back();
		return p;
	}
	return new PooledObject(objCounterArguments());
}
template <class PooledObject> void ObjectPool<PooledObject>::returnToPool(PooledObject* p) {
	pool.push_back(p);
}
template <class PooledObject> void ObjectPool<PooledObject>::clearPool() {
	for (PooledObject* p : pool)
		delete p;
	pool.clear();
}

//many subclasses only need their own pool, they are not held as the subclass itself
template class ObjectPool<CompositeQuarticValue>;
template class ObjectPool<ConstantValue>;
template class ObjectPool<EntityAnimation::Delay>;
template class ObjectPool<EntityAnimation::MapKickResetSwitch>;
template class ObjectPool<EntityAnimation::MapKickSwitch>;
template class ObjectPool<EntityAnimation::SetGhostSprite>;
template class ObjectPool<EntityAnimation::SetPosition>;
template class ObjectPool<EntityAnimation::SetScreenOverlayColor>;
template class ObjectPool<EntityAnimation::SetSpriteAnimation>;
template class ObjectPool<EntityAnimation::SetDirection>;
template class ObjectPool<EntityAnimation::SetVelocity>;
template class ObjectPool<EntityAnimation::SwitchToPlayerCamera>;
template class ObjectPool<LinearInterpolatedValue>;
//superclasses only need holders, the classes themselves are not allocated
template class ReferenceCounterHolder<DynamicValue>;
template class ReferenceCounterHolder<EntityAnimationTypes::Component>;
template class ReferenceCounterHolder<EntityState>;
//solo-concrete classes or held subclasses need both their own pools and holders
instantiateObjectPoolAndReferenceCounterHolder(CollisionRect)
instantiateObjectPoolAndReferenceCounterHolder(DynamicCameraAnchor)
instantiateObjectPoolAndReferenceCounterHolder(EntityAnimation)
instantiateObjectPoolAndReferenceCounterHolder(KickAction)
instantiateObjectPoolAndReferenceCounterHolder(MapState)
instantiateObjectPoolAndReferenceCounterHolder(PauseState)
instantiateObjectPoolAndReferenceCounterHolder(PlayerState)
instantiateObjectPoolAndReferenceCounterHolder(Particle)
