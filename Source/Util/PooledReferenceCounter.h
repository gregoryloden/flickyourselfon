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
	~PooledReferenceCounter();

protected:
	virtual void prepareReturnToPool() {}
public:
	void retain();
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
	~ReferenceCounterHolder();

	ReferenceCountedObject* get() { return object; }
	void set(ReferenceCountedObject* pObject);

	ReferenceCounterHolder<ReferenceCountedObject>& operator =(const ReferenceCounterHolder<ReferenceCountedObject>& other);

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
