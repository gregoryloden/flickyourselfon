#include "PooledReferenceCounter.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/EntityState.h"
#include "GameState/MapState.h"
#include "GameState/PauseState.h"
#include "GameState/PlayerState.h"

#define instantiateObjectPool(className) \
	template class ObjectPool<className>; vector<className*> ObjectPool<className>::pool;
#define instantiateObjectPoolAndReferenceCounterHolder(className) \
	instantiateObjectPool(className) template class ReferenceCounterHolder<className>;

instantiateObjectPool(CompositeQuarticValue)
instantiateObjectPool(EntityAnimation::Delay)
instantiateObjectPool(EntityAnimation::SetVelocity)
instantiateObjectPool(EntityAnimation::SetSpriteAnimation)
instantiateObjectPoolAndReferenceCounterHolder(DynamicCameraAnchor)
instantiateObjectPoolAndReferenceCounterHolder(EntityAnimation)
instantiateObjectPoolAndReferenceCounterHolder(MapState)
instantiateObjectPoolAndReferenceCounterHolder(PauseState)
instantiateObjectPoolAndReferenceCounterHolder(PlayerState)
template class ReferenceCounterHolder<DynamicValue>;
template class ReferenceCounterHolder<EntityAnimation::Component>;
template class ReferenceCounterHolder<EntityState>;

//////////////////////////////// PooledReferenceCounter ////////////////////////////////
PooledReferenceCounter::PooledReferenceCounter(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
referenceCount(0) {
}
PooledReferenceCounter::~PooledReferenceCounter() {}
//increment the reference count of this object
void PooledReferenceCounter::retain() {
	referenceCount++;
}

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
//hold the new object and release/retain objects as appropriate
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

//////////////////////////////// ObjectPool ////////////////////////////////
//if we have objects in the pool then remove one and return it, otherwise make a new object
template <class PooledObject> PooledObject* ObjectPool<PooledObject>::newFromPool(objCounterParameters()) {
	if (pool.size() > 0) {
		PooledObject* p = pool.back();
		pool.pop_back();
		return p;
	}
	return new PooledObject(objCounterArguments());
}
//add this object back to the pool
template <class PooledObject> void ObjectPool<PooledObject>::returnToPool(PooledObject* p) {
	pool.push_back(p);
}
//clear the pool, deleting all the objects in it
template <class PooledObject> void ObjectPool<PooledObject>::clearPool() {
	for (PooledObject* p : pool) {
		delete p;
	}
	pool.clear();
}
