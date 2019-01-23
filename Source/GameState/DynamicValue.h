#include "Util/PooledReferenceCounter.h"

#define newCompositeQuarticValue(\
		constantValue, linearValuePerTick, quadraticValuePerTick, cubicValuePerTick, quarticValuePerTick) \
	produceWithArgs(\
		CompositeQuarticValue, constantValue, linearValuePerTick, quadraticValuePerTick, cubicValuePerTick, quarticValuePerTick)

//each DynamicValue is only held in one place at a time
class DynamicValue: public PooledReferenceCounter {
public:
	DynamicValue(objCounterParameters());
	~DynamicValue();

	//copy this DynamicValue such that at 0 ticks elapsed, it equals the provided constant value
	virtual DynamicValue* copyWithConstantValue(float pConstantValue) = 0;
	//return the value after the given amount of time has elapsed
	virtual float getValue(int ticksElapsed) = 0;
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

	static CompositeQuarticValue* produce(
		objCounterParametersComma()
		float pConstantValue,
		float pLinearValuePerTick,
		float pQuadraticValuePerTick,
		float pCubicValuePerTick,
		float pQuarticValuePerTick);
	virtual void release();
	virtual DynamicValue* copyWithConstantValue(float pConstantValue);
	virtual float getValue(int ticksElapsed);
};
