#ifndef POOLED_REFERENCE_COUNTER_H
#define POOLED_REFERENCE_COUNTER_H
#include "General/General.h"

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
	~PooledReferenceCounter();

	void retain();
	virtual void release() = 0;
	virtual void prepareReturnToPool() {}
};
//Should only be allocated within an object or on the stack
template <class ReferenceCountedObject> class ReferenceCounterHolder {
private:
	ReferenceCountedObject* object;

public:
	ReferenceCounterHolder(ReferenceCountedObject* pObject);
	ReferenceCounterHolder(const ReferenceCounterHolder<ReferenceCountedObject>& other);
	~ReferenceCounterHolder();

	ReferenceCountedObject* get() { return object; }
	void set(ReferenceCountedObject* pObject);

	ReferenceCounterHolder<ReferenceCountedObject>& operator =(const ReferenceCounterHolder<ReferenceCountedObject>& other);

	ReferenceCounterHolder(ReferenceCounterHolder<ReferenceCountedObject>&& other) = delete;
	ReferenceCounterHolder<ReferenceCountedObject>& operator =(ReferenceCounterHolder<ReferenceCountedObject>&& other) = delete;
};
template <class PooledObject> class ObjectPool {
private:
	static vector<PooledObject*> pool;

public:
	static PooledObject* newFromPool(objCounterParameters());
	static void returnToPool(PooledObject* p);
	static void clearPool();
};
#endif
