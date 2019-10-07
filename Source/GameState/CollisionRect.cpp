#include "CollisionRect.h"

CollisionRect::CollisionRect(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, left(0)
, top(0)
, right(0)
, bottom(0) {
}
CollisionRect::~CollisionRect() {}
CollisionRect* CollisionRect::produce(objCounterParametersComma() float pLeft, float pTop, float pRight, float pBottom) {
	initializeWithNewFromPool(c, CollisionRect)
	c->left = pLeft;
	c->top = pTop;
	c->right = pRight;
	c->bottom = pBottom;
	return c;
}
pooledReferenceCounterDefineRelease(CollisionRect)
bool CollisionRect::intersects(CollisionRect* other) {
	return left < other->right && other->left < right && top < other->bottom && other->top < bottom;
}
