#ifndef POOLED_REFERENCE_COUNTER_H
#define POOLED_REFERENCE_COUNTER_H
#include "General/General.h"

#define pooledReferenceCounterDefineRelease(className) \
	void className::release() {\
		if (referenceCount > 1)\
			referenceCount--;\
		else {\
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
template <class PooledObject> class ObjectPool {
private:
	static vector<PooledObject*> pool;

public:
	static PooledObject* newFromPool(objCounterParameters());
	static void returnToPool(PooledObject* p);
	static void clearPool();
};
#endif
