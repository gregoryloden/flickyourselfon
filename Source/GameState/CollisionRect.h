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

	//initialize and return a CollisionRect
	static CollisionRect* produce(objCounterParametersComma() float pLeft, float pTop, float pRight, float pBottom);
	//release a reference to this CollisionRect and return it to the pool if applicable
	virtual void release();
	//check if this CollisionRect has any overlap with the other CollisionRect
	bool intersects(CollisionRect* other);
};
