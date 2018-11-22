#include "DynamicValue.h"

DynamicValue::DynamicValue(objCounterParameters())
: PooledReferenceCounter(objCounterArguments()) {
}
DynamicValue::~DynamicValue() {}
CompositeQuarticValue::CompositeQuarticValue(objCounterParameters())
: DynamicValue(objCounterArguments())
, constantValue(0.0f)
, linearValuePerTick(0.0f) {
}
CompositeQuarticValue::~CompositeQuarticValue() {}
//initialize this CompositeQuarticValue
CompositeQuarticValue* CompositeQuarticValue::set(
	float pConstantValue,
	float pLinearValuePerTick,
	float pQuadraticValuePerTick,
	float pCubicValuePerTick,
	float pQuarticValuePerTick)
{
	constantValue = pConstantValue;
	linearValuePerTick = pLinearValuePerTick;
	quadraticValuePerTick = pQuadraticValuePerTick;
	cubicValuePerTick = pCubicValuePerTick;
	quarticValuePerTick = pQuarticValuePerTick;
	return this;
}
pooledReferenceCounterDefineRelease(CompositeQuarticValue)
//get the value at the given time
float CompositeQuarticValue::getValue(int ticksElapsed) {
	float floatTicksElapsed = (float)ticksElapsed;
	float ticksElapsedSquared = floatTicksElapsed * floatTicksElapsed;
	float ticksElapsedCubed = ticksElapsedSquared * floatTicksElapsed;
	return constantValue
		+ linearValuePerTick * floatTicksElapsed
		+ quadraticValuePerTick * ticksElapsedSquared
		+ cubicValuePerTick * ticksElapsedCubed
		+ quarticValuePerTick * ticksElapsedCubed * floatTicksElapsed;
}
//set the constant value to the provided value
DynamicValue* CompositeQuarticValue::copyWithConstantValue(float pConstantValue) {
	return callNewFromPool(CompositeQuarticValue)->set(
		pConstantValue, linearValuePerTick, quadraticValuePerTick, cubicValuePerTick, quarticValuePerTick);
}
