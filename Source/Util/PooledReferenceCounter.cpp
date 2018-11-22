#include "PooledReferenceCounter.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"

#define instantiatePooledReferenceCounter(className) \
	template class ObjectPool<className>; vector<className*> ObjectPool<className>::pool;

instantiatePooledReferenceCounter(CompositeQuarticValue);
instantiatePooledReferenceCounter(EntityAnimation);
instantiatePooledReferenceCounter(EntityAnimation::Delay);
instantiatePooledReferenceCounter(EntityAnimation::SetVelocity);
instantiatePooledReferenceCounter(EntityAnimation::SetSpriteAnimation);
template class ReferenceCounterHolder<DynamicValue>;
template class ReferenceCounterHolder<EntityAnimation>;
template class ReferenceCounterHolder<EntityAnimation::Component>;

PooledReferenceCounter::PooledReferenceCounter(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
referenceCount(0) {
}
PooledReferenceCounter::~PooledReferenceCounter() {}
//increment the reference count of this object
void PooledReferenceCounter::retain() {
	referenceCount++;
}
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
