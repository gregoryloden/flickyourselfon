#ifndef POOLED_REFERENCE_COUNTER_H
#define POOLED_REFERENCE_COUNTER_H
#include "General/General.h"

#define initializeWithNewFromPool(var, className) className* var = ObjectPool<className>::newFromPool(objCounterArguments());
#define pooledReferenceCounterDefineRelease(className) \
	void className::release() {\
		referenceCount--;\
		if (referenceCount == 0) {\
			prepareReturnToPool();\
			ObjectPool<className>::returnToPool(this);\
		}\
	}

class PooledReferenceCounter onlyInDebug(: public ObjCounter) {
protected:
	int referenceCount;

public:
	PooledReferenceCounter(objCounterParameters());
	virtual ~PooledReferenceCounter();

protected:
	virtual void prepareReturnToPool() {}
public:
	//increment the reference count of this object
	void retain();
	//release a reference to this PooledReferenceCounter and return it to the pool if applicable
	virtual void release() = 0;
};
//Should only be allocated within an object, or on the stack if its internal object will not be returned
template <class ReferenceCountedObject> class ReferenceCounterHolder {
private:
	ReferenceCountedObject* object;

public:
	ReferenceCounterHolder(ReferenceCountedObject* pObject);
	ReferenceCounterHolder(const ReferenceCounterHolder<ReferenceCountedObject>& other);
	ReferenceCounterHolder(ReferenceCounterHolder<ReferenceCountedObject>&& other);
	virtual ~ReferenceCounterHolder();

	ReferenceCountedObject* get() { return object; }
	//hold the new object and release/retain objects as appropriate
	void set(ReferenceCountedObject* pObject);

	ReferenceCounterHolder<ReferenceCountedObject>& operator =(const ReferenceCounterHolder<ReferenceCountedObject>& other);
	ReferenceCounterHolder<ReferenceCountedObject>& operator =(ReferenceCounterHolder<ReferenceCountedObject>&& other);
};
template <class PooledObject> class ObjectPool {
private:
	static vector<PooledObject*> pool;

public:
	//if we have objects in the pool then remove one and return it, otherwise make a new object
	static PooledObject* newFromPool(objCounterParameters());
	//add this object back to the pool
	static void returnToPool(PooledObject* p);
	//clear the pool, deleting all the objects in it
	static void clearPool();
};
#endif
