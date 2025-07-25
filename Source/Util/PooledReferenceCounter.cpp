#include "PooledReferenceCounter.h"
#include "GameState/CollisionRect.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/EntityState.h"
#include "GameState/HintState.h"
#include "GameState/KickAction.h"
#include "GameState/PauseState.h"
#include "GameState/PlayerState.h"
#include "GameState/UndoState.h"
#include "GameState/MapState/MapState.h"

#define instantiateObjectPoolAndReferenceCounterHolder(className) \
	template class ObjectPool<className>; template class ReferenceCounterHolder<className>;
#define ENABLE_POOLING

//////////////////////////////// PooledReferenceCounter ////////////////////////////////
PooledReferenceCounter::PooledReferenceCounter(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
referenceCount(0) {
}
PooledReferenceCounter::~PooledReferenceCounter() {}

//////////////////////////////// ReferenceCounterHolder ////////////////////////////////
template <class ReferenceCountedObject>
ReferenceCounterHolder<ReferenceCountedObject>::ReferenceCounterHolder(ReferenceCountedObject* pObject)
: object(pObject) {
	if (pObject != nullptr)
		pObject->retain();
}
template <class ReferenceCountedObject>
ReferenceCounterHolder<ReferenceCountedObject>::ReferenceCounterHolder(
	const ReferenceCounterHolder<ReferenceCountedObject>& other)
: object(other.object) {
	if (object != nullptr)
		object->retain();
}
template <class ReferenceCountedObject>
ReferenceCounterHolder<ReferenceCountedObject>::ReferenceCounterHolder(ReferenceCounterHolder<ReferenceCountedObject>&& other)
: object(other.object) {
	if (object != nullptr)
		object->retain();
}
template <class ReferenceCountedObject> ReferenceCounterHolder<ReferenceCountedObject>::~ReferenceCounterHolder() {
	if (object != nullptr)
		object->release();
}
template <class ReferenceCountedObject>
void ReferenceCounterHolder<ReferenceCountedObject>::set(ReferenceCountedObject* pObject) {
	if (pObject != nullptr)
		pObject->retain();
	ReferenceCountedObject* oldObject = object;
	object = pObject;
	if (oldObject != nullptr)
		oldObject->release();
}
template <class ReferenceCountedObject> void ReferenceCounterHolder<ReferenceCountedObject>::clear() {
	if (object != nullptr) {
		object->release();
		object = nullptr;
	}
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
	if (!pool.empty()) {
		PooledObject* p = pool.back();
		pool.pop_back();
		return p;
	}
	return new PooledObject(objCounterArguments());
}
template <class PooledObject> void ObjectPool<PooledObject>::returnToPool(PooledObject* p) {
	#ifdef ENABLE_POOLING
		pool.push_back(p);
	#else
		delete p;
	#endif
}
template <class PooledObject> void ObjectPool<PooledObject>::clearPool() {
	for (PooledObject* p : pool)
		delete p;
	pool.clear();
}

//many subclasses only need their own pool, they are not held as the subclass itself
template class ObjectPool<ClimbFallUndoState>;
template class ObjectPool<CompositeQuarticValue>;
template class ObjectPool<ConstantValue>;
template class ObjectPool<EntityAnimation::Delay>;
template class ObjectPool<EntityAnimation::GenerateHint>;
template class ObjectPool<EntityAnimation::MapKickResetSwitch>;
template class ObjectPool<EntityAnimation::MapKickSwitch>;
template class ObjectPool<EntityAnimation::PlaySound>;
template class ObjectPool<EntityAnimation::SetGhostSprite>;
template class ObjectPool<EntityAnimation::SetPosition>;
template class ObjectPool<EntityAnimation::SetScreenOverlayColor>;
template class ObjectPool<EntityAnimation::SetSpriteAnimation>;
template class ObjectPool<EntityAnimation::SetDirection>;
template class ObjectPool<EntityAnimation::SetVelocity>;
template class ObjectPool<EntityAnimation::SetZoom>;
template class ObjectPool<EntityAnimation::SpawnParticle>;
template class ObjectPool<EntityAnimation::SwitchToPlayerCamera>;
#ifdef STEAM
	template class ObjectPool<EntityAnimation::UnlockEndGameAchievement>;
#endif
template class ObjectPool<ExponentialValue>;
template class ObjectPool<LinearInterpolatedValue>;
template class ObjectPool<KickSwitchUndoState>;
template class ObjectPool<MoveUndoState>;
template class ObjectPool<NoOpUndoState>;
template class ObjectPool<PiecewiseValue>;
template class ObjectPool<RideRailUndoState>;
template class ObjectPool<TimeFunctionValue>;
//superclasses only need holders, the classes themselves are not allocated
template class ReferenceCounterHolder<DynamicValue>;
template class ReferenceCounterHolder<EntityAnimationTypes::Component>;
template class ReferenceCounterHolder<EntityState>;
template class ReferenceCounterHolder<UndoState>;
//solo-concrete classes or held subclasses need both their own pools and holders
instantiateObjectPoolAndReferenceCounterHolder(CollisionRect)
instantiateObjectPoolAndReferenceCounterHolder(DynamicCameraAnchor)
instantiateObjectPoolAndReferenceCounterHolder(EntityAnimation)
instantiateObjectPoolAndReferenceCounterHolder(HintState)
instantiateObjectPoolAndReferenceCounterHolder(KickAction)
instantiateObjectPoolAndReferenceCounterHolder(KickResetSwitchUndoState)
instantiateObjectPoolAndReferenceCounterHolder(MapState)
instantiateObjectPoolAndReferenceCounterHolder(PauseState)
instantiateObjectPoolAndReferenceCounterHolder(PlayerState)
instantiateObjectPoolAndReferenceCounterHolder(Particle)
instantiateObjectPoolAndReferenceCounterHolder(HintState::PotentialLevelState)
