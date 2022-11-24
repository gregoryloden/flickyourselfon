#include "DynamicValue.h"

//////////////////////////////// DynamicValue ////////////////////////////////
DynamicValue::DynamicValue(objCounterParameters())
: PooledReferenceCounter(objCounterArguments()) {
}
DynamicValue::~DynamicValue() {}

//////////////////////////////// ConstantValue ////////////////////////////////
ConstantValue::ConstantValue(objCounterParameters())
: DynamicValue(objCounterArguments())
, value(0.0f) {
}
ConstantValue::~ConstantValue() {}
ConstantValue* ConstantValue::produce(objCounterParametersComma() float pValue) {
	initializeWithNewFromPool(c, ConstantValue)
	c->value = pValue;
	return c;
}
pooledReferenceCounterDefineRelease(ConstantValue)
DynamicValue* ConstantValue::copyWithConstantValue(float pConstantValue) {
	return newConstantValue(pConstantValue);
}
float ConstantValue::getValue(int ticksElapsed) {
	return value;
}

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
DynamicValue* CompositeQuarticValue::copyWithConstantValue(float pConstantValue) {
	return newCompositeQuarticValue(
		pConstantValue, linearValuePerTick, quadraticValuePerTick, cubicValuePerTick, quarticValuePerTick);
}
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

//////////////////////////////// LinearInterpolatedValue::ValueAtTime ////////////////////////////////
LinearInterpolatedValue::ValueAtTime::ValueAtTime(float pValue, int pAtTicksTime)
: value(pValue)
, atTicksTime(pAtTicksTime) {
}
LinearInterpolatedValue::ValueAtTime::~ValueAtTime() {}

//////////////////////////////// LinearInterpolatedValue ////////////////////////////////
LinearInterpolatedValue::LinearInterpolatedValue(objCounterParameters())
: DynamicValue(objCounterArguments())
, valuesAtTimes() {
}
LinearInterpolatedValue::~LinearInterpolatedValue() {}
LinearInterpolatedValue* LinearInterpolatedValue::produce(objCounterParametersComma() vector<ValueAtTime> valuesAtTimes) {
	initializeWithNewFromPool(l, LinearInterpolatedValue)
	l->valuesAtTimes = valuesAtTimes;
	return l;
}
pooledReferenceCounterDefineRelease(LinearInterpolatedValue)
//TODO: find where 0 is and shift all the values properly
DynamicValue* LinearInterpolatedValue::copyWithConstantValue(float pConstantValue) {
	return this;
}
float LinearInterpolatedValue::getValue(int ticksElapsed) {
	//use the last value if we're after the last ticks time
	int highIndex = (int)valuesAtTimes.size() - 1;
	ValueAtTime* highValueAtTime = &valuesAtTimes[highIndex];
	if (ticksElapsed >= highValueAtTime->getAtTicksTime())
		return highValueAtTime->getValue();
	//use the first value if we're before the first ticks time
	int lowIndex = 0;
	ValueAtTime* lowValueAtTime = &valuesAtTimes[lowIndex];
	if (ticksElapsed <= lowValueAtTime->getAtTicksTime())
		return lowValueAtTime->getValue();
	//find which values we're between
	while (lowIndex < highIndex - 1) {
		int midIndex = (lowIndex + highIndex) / 2;
		ValueAtTime* midValueAtTime = &valuesAtTimes[midIndex];
		if (ticksElapsed >= midValueAtTime->getAtTicksTime()) {
			lowIndex = midIndex;
			lowValueAtTime = midValueAtTime;
		} else {
			highIndex = midIndex;
			highValueAtTime = midValueAtTime;
		}
	}
	float interValueDuration = (float)(highValueAtTime->getAtTicksTime() - lowValueAtTime->getAtTicksTime());
	float lowValuePart = lowValueAtTime->getValue() * (highValueAtTime->getAtTicksTime() - ticksElapsed) / interValueDuration;
	float highValuePart = highValueAtTime->getValue() * (ticksElapsed - lowValueAtTime->getAtTicksTime()) / interValueDuration;
	return highValuePart + lowValuePart;
}
