#include "DynamicValue.h"

//////////////////////////////// DynamicValue ////////////////////////////////
DynamicValue::DynamicValue(objCounterParameters())
: PooledReferenceCounter(objCounterArguments()) {
}
DynamicValue::~DynamicValue() {}

//////////////////////////////// CompositeQuarticValue ////////////////////////////////
CompositeQuarticValue::CompositeQuarticValue(objCounterParameters())
: DynamicValue(objCounterArguments())
, constantValue(0.0f)
, linearValuePerTick(0.0f)
, quadraticValuePerTick(0.0f)
, cubicValuePerTick(0.0f)
, quarticValuePerTick(0.0f) {
}
CompositeQuarticValue::~CompositeQuarticValue() {}
//initialize and return a CompositeQuarticValue
CompositeQuarticValue* CompositeQuarticValue::produce(
	objCounterParametersComma()
	float pConstantValue,
	float pLinearValuePerTick,
	float pQuadraticValuePerTick,
	float pCubicValuePerTick,
	float pQuarticValuePerTick)
{
	initializeWithNewFromPool(c, CompositeQuarticValue)
	c->constantValue = pConstantValue;
	c->linearValuePerTick = pLinearValuePerTick;
	c->quadraticValuePerTick = pQuadraticValuePerTick;
	c->cubicValuePerTick = pCubicValuePerTick;
	c->quarticValuePerTick = pQuarticValuePerTick;
	return c;
}
pooledReferenceCounterDefineRelease(CompositeQuarticValue)
//set the constant value to the provided value
DynamicValue* CompositeQuarticValue::copyWithConstantValue(float pConstantValue) {
	return newCompositeQuarticValue(
		pConstantValue, linearValuePerTick, quadraticValuePerTick, cubicValuePerTick, quarticValuePerTick);
}
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
