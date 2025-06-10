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

//////////////////////////////// ExponentialValue ////////////////////////////////
ExponentialValue::ExponentialValue(objCounterParameters())
: DynamicValue(objCounterArguments())
, baseExponent(1.0f)
, baseDuration(1) {
}
ExponentialValue::~ExponentialValue() {}
ExponentialValue* ExponentialValue::produce(objCounterParametersComma() float pBaseExponent, int pBaseDuration) {
	initializeWithNewFromPool(e, ExponentialValue)
	e->baseExponent = pBaseExponent;
	e->baseDuration = pBaseDuration;
	return e;
}
pooledReferenceCounterDefineRelease(ExponentialValue)
DynamicValue* ExponentialValue::copyWithConstantValue(float pConstantValue) {
	//TODO not supported, needs a SumValue because the value at 0 is always 1
	return this;
}
float ExponentialValue::getValue(int ticksElapsed) {
	return powf(baseExponent, (float)ticksElapsed / baseDuration);
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

//////////////////////////////// PiecewiseValue::ValueAtTime ////////////////////////////////
PiecewiseValue::ValueAtTime::ValueAtTime(DynamicValue* pValue, int pAtTicksTime)
: value(pValue)
, atTicksTime(pAtTicksTime) {
}
PiecewiseValue::ValueAtTime::~ValueAtTime() {}

//////////////////////////////// PiecewiseValue ////////////////////////////////
PiecewiseValue::PiecewiseValue(objCounterParameters())
: DynamicValue(objCounterArguments())
, valuesAtTimes() {
}
PiecewiseValue::~PiecewiseValue() {}
PiecewiseValue* PiecewiseValue::produce(objCounterParametersComma() vector<ValueAtTime> valuesAtTimes) {
	initializeWithNewFromPool(p, PiecewiseValue)
	p->valuesAtTimes = valuesAtTimes;
	return p;
}
pooledReferenceCounterDefineRelease(PiecewiseValue)
DynamicValue* PiecewiseValue::copyWithConstantValue(float pConstantValue) {
	static vector<ValueAtTime> newValuesAtTimes;
	newValuesAtTimes.clear();
	float shift = pConstantValue - valuesAtTimes.front().getValue()->getValue(0);
	for (ValueAtTime valueAtTime : valuesAtTimes) {
		DynamicValue* value = valueAtTime.getValue();
		newValuesAtTimes.push_back(
			ValueAtTime(value->copyWithConstantValue(value->getValue(0) + shift), valueAtTime.getAtTicksTime()));
	}
	return newPiecewiseValue(newValuesAtTimes);
}
float PiecewiseValue::getValue(int ticksElapsed) {
	int activeValueIndex = 0;
	int futureValueIndex = (int)valuesAtTimes.size();
	while (activeValueIndex < futureValueIndex - 1) {
		int midIndex = (activeValueIndex + futureValueIndex) / 2;
		ValueAtTime* midValueAtTime = &valuesAtTimes[midIndex];
		if (midValueAtTime->getAtTicksTime() > ticksElapsed)
			futureValueIndex = midIndex;
		else
			activeValueIndex = midIndex;
	}
	ValueAtTime& activeValueAtTime = valuesAtTimes[activeValueIndex];
	return activeValueAtTime.getValue()->getValue(ticksElapsed - activeValueAtTime.getAtTicksTime());
}
