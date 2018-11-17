#include "DynamicValue.h"

DynamicValue::DynamicValue(objCounterParameters())
: PooledReferenceCounter(objCounterArguments()) {
}
DynamicValue::~DynamicValue() {}
CompositeLinearValue::CompositeLinearValue(objCounterParameters())
: DynamicValue(objCounterArguments())
, constantValue(0.0f)
, linearValuePerTick(0.0f) {
}
CompositeLinearValue::~CompositeLinearValue() {}
//initialize this CompositeLinearValue
CompositeLinearValue* CompositeLinearValue::set(float pConstantValue, float pLinearValuePerTick) {
	constantValue = pConstantValue;
	linearValuePerTick = pLinearValuePerTick;
	return this;
}
pooledReferenceCounterDefineRelease(CompositeLinearValue)
//get the value at the given time
float CompositeLinearValue::getValue(int ticksElapsed) {
	return constantValue + linearValuePerTick * (float)ticksElapsed;
}
//copy this value and update its constant value
DynamicValue* CompositeLinearValue::copyWithConstantValue(float pConstantValue) {
	return callNewFromPool(CompositeLinearValue)->set(pConstantValue, linearValuePerTick);
}
