#include "Util/PooledReferenceCounter.h"

class DynamicValue: public PooledReferenceCounter {
public:
	DynamicValue(objCounterParameters());
	~DynamicValue();

	//return the value after the given amount of time has elapsed
	virtual float getValue(int ticksElapsed) = 0;
	//copy this DynamicValue such that at 0 ticks elapsed, it equals the provided constant value
	virtual DynamicValue* copyWithConstantValue(float pConstantValue) = 0;
};
class CompositeQuarticValue: public DynamicValue {
private:
	float constantValue;
	float linearValuePerTick;
	float quadraticValuePerTick;
	float cubicValuePerTick;
	float quarticValuePerTick;

public:
	CompositeQuarticValue(objCounterParameters());
	~CompositeQuarticValue();

	CompositeQuarticValue* set(
		float pConstantValue,
		float pLinearValuePerTick,
		float pQuadraticValuePerTick,
		float pCubicValuePerTick,
		float pQuarticValuePerTick);
	virtual void release();
	virtual float getValue(int ticksElapsed);
	virtual DynamicValue* copyWithConstantValue(float pConstantValue);
};
