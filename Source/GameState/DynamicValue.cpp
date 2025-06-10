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
float ConstantValue::getValue(float ticksElapsed) {
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
float CompositeQuarticValue::getValue(float ticksElapsed) {
	float ticksElapsedSquared = ticksElapsed * ticksElapsed;
	float ticksElapsedCubed = ticksElapsedSquared * ticksElapsed;
	return constantValue
		+ linearValuePerTick * ticksElapsed
		+ quadraticValuePerTick * ticksElapsedSquared
		+ cubicValuePerTick * ticksElapsedCubed
		+ quarticValuePerTick * ticksElapsedCubed * ticksElapsed;
}
CompositeQuarticValue* CompositeQuarticValue::cubicInterpolation(float targetValue, float ticksDuration) {
	//vy = at(t-1) = at^2-at   (a < 0)
	//y = at^3/3-at^2/2
	//1 = a1^3/3-a1^2/2 = a(1/3 - 1/2) = a(-1/6)
	//a = 1/(-1/6) = -6
	//y = -2t^3+3t^2
	float ticksDurationSquared = ticksDuration * ticksDuration;
	float ticksDurationCubed = ticksDuration * ticksDurationSquared;
	return newCompositeQuarticValue(
		0.0f, 0.0f, 3.0f * targetValue / ticksDurationSquared, -2.0f * targetValue / ticksDurationCubed, 0.0f);
}

//////////////////////////////// ExponentialValue ////////////////////////////////
ExponentialValue::ExponentialValue(objCounterParameters())
: DynamicValue(objCounterArguments())
, baseExponent(1.0f)
, baseDuration(1) {
}
ExponentialValue::~ExponentialValue() {}
ExponentialValue* ExponentialValue::produce(objCounterParametersComma() float pBaseExponent, float pBaseDuration) {
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
float ExponentialValue::getValue(float ticksElapsed) {
	return powf(baseExponent, ticksElapsed / baseDuration);
}

//////////////////////////////// LinearInterpolatedValue::ValueAtTime ////////////////////////////////
LinearInterpolatedValue::ValueAtTime::ValueAtTime(float pValue, float pAtTicksTime)
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
float LinearInterpolatedValue::getValue(float ticksElapsed) {
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
PiecewiseValue::ValueAtTime::ValueAtTime(DynamicValue* pValue, float pAtTicksTime)
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
void PiecewiseValue::prepareReturnToPool() {
	valuesAtTimes.clear();
}
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
float PiecewiseValue::getValue(float ticksElapsed) {
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

//////////////////////////////// TimeFunctionValue ////////////////////////////////
TimeFunctionValue::TimeFunctionValue(objCounterParameters())
: DynamicValue(objCounterArguments())
, innerValue(nullptr)
, timeFunction(nullptr) {
}
TimeFunctionValue::~TimeFunctionValue() {}
TimeFunctionValue* TimeFunctionValue::produce(
	objCounterParametersComma() DynamicValue* pInnerValue, DynamicValue* pTimeFunction)
{
	initializeWithNewFromPool(t, TimeFunctionValue)
	t->innerValue.set(pInnerValue);
	t->timeFunction.set(pTimeFunction);
	return t;
}
pooledReferenceCounterDefineRelease(TimeFunctionValue)
void TimeFunctionValue::prepareReturnToPool() {
	innerValue.set(nullptr);
	timeFunction.set(nullptr);
}
DynamicValue* TimeFunctionValue::copyWithConstantValue(float pConstantValue) {
	//TODO not supported, needs a SumValue because the value at 0 is not controlled by this value
	return this;
}
float TimeFunctionValue::getValue(float ticksElapsed) {
	return innerValue.get()->getValue(timeFunction.get()->getValue(ticksElapsed));
}
