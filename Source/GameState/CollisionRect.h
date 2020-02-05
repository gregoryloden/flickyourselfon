#include "Util/PooledReferenceCounter.h"

#define newCollisionRect(left, top, right, bottom) produceWithArgs(CollisionRect, left, top, right, bottom)

class CollisionRect: public PooledReferenceCounter {
public:
	float left;
	float top;
	float right;
	float bottom;

	CollisionRect(objCounterParameters());
	virtual ~CollisionRect();

	virtual void release();
	static CollisionRect* produce(objCounterParametersComma() float pLeft, float pTop, float pRight, float pBottom);
	bool intersects(CollisionRect* other);
};
