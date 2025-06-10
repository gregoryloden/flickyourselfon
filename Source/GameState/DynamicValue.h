#include "Util/PooledReferenceCounter.h"

#define newConstantValue(value) produceWithArgs(ConstantValue, value)
#define newCompositeQuarticValue(\
		constantValue, linearValuePerTick, quadraticValuePerTick, cubicValuePerTick, quarticValuePerTick) \
	produceWithArgs(\
		CompositeQuarticValue, constantValue, linearValuePerTick, quadraticValuePerTick, cubicValuePerTick, quarticValuePerTick)
#define newExponentialValue(baseExponent, baseDuration) produceWithArgs(ExponentialValue, baseExponent, baseDuration)
#define newLinearInterpolatedValue(valuesAtTimes) produceWithArgs(LinearInterpolatedValue, valuesAtTimes)
#define newPiecewiseValue(valuesAtTimes) produceWithArgs(PiecewiseValue, valuesAtTimes)

//each DynamicValue is only held in one place at a time
class DynamicValue: public PooledReferenceCounter {
public:
	DynamicValue(objCounterParameters());
	virtual ~DynamicValue();

	//copy this DynamicValue such that at 0 ticks elapsed, it equals the provided constant value
	virtual DynamicValue* copyWithConstantValue(float pConstantValue) = 0;
	//return the value after the given amount of time has elapsed
	virtual float getValue(float ticksElapsed) = 0;
};
class ConstantValue: public DynamicValue {
private:
	float value;

public:
	ConstantValue(objCounterParameters());
	virtual ~ConstantValue();

	//initialize and return a ConstantValue
	static ConstantValue* produce(objCounterParametersComma() float pValue);
	//release a reference to this ConstantValue and return it to the pool if applicable
	virtual void release();
	//return a new ConstantValue with the provided value
	virtual DynamicValue* copyWithConstantValue(float pConstantValue);
	//get the value at the given time
	virtual float getValue(float ticksElapsed);
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
	virtual ~CompositeQuarticValue();

	//initialize and return a CompositeQuarticValue
	static CompositeQuarticValue* produce(
		objCounterParametersComma()
		float pConstantValue,
		float pLinearValuePerTick,
		float pQuadraticValuePerTick,
		float pCubicValuePerTick,
		float pQuarticValuePerTick);
	//release a reference to this CompositeQuarticValue and return it to the pool if applicable
	virtual void release();
	//set the constant value to the provided value
	virtual DynamicValue* copyWithConstantValue(float pConstantValue);
	//get the value at the given time
	virtual float getValue(float ticksElapsed);
	//return a CompositeQuarticValue that follows a curve from (0, 0) to (1, 1) with 0 slope at (0, 0) and (1, 1)
	static CompositeQuarticValue* cubicInterpolation(float targetValue, float ticksDuration);
};
class ExponentialValue: public DynamicValue {
private:
	float baseExponent;
	float baseDuration;

public:
	ExponentialValue(objCounterParameters());
	virtual ~ExponentialValue();

	//initialize and return an ExponentialValue
	static ExponentialValue* produce(objCounterParametersComma() float pBaseExponent, float pBaseDuration);
	//release a reference to this ExponentialValue and return it to the pool if applicable
	virtual void release();
	//set the constant value to the provided value
	virtual DynamicValue* copyWithConstantValue(float pConstantValue);
	//get the value at the given time
	virtual float getValue(float ticksElapsed);
};
class LinearInterpolatedValue: public DynamicValue {
public:
	//Should only be allocated within an object, on the stack, or as a static object
	class ValueAtTime {
	private:
		float value;
		float atTicksTime;

	public:
		ValueAtTime(float pValue, float pAtTicksTime);
		virtual ~ValueAtTime();

		float getValue() { return value; }
		float getAtTicksTime() { return atTicksTime; }
	};

private:
	vector<ValueAtTime> valuesAtTimes;

public:
	LinearInterpolatedValue(objCounterParameters());
	virtual ~LinearInterpolatedValue();

	//initialize and return a LinearInterpolatedValue
	static LinearInterpolatedValue* produce(objCounterParametersComma() vector<ValueAtTime> valuesAtTimes);
	//release a reference to this LinearInterpolatedValue and return it to the pool if applicable
	virtual void release();
	//return a new value with all the values shifted so that it's the given value at time 0
	//assumes there is at least one value
	virtual DynamicValue* copyWithConstantValue(float pConstantValue);
	//get the value at the given time
	//assumes there is at least one value
	virtual float getValue(float ticksElapsed);
};
class PiecewiseValue: public DynamicValue {
public:
	//Should only be allocated within an object, on the stack, or as a static object
	class ValueAtTime {
	private:
		ReferenceCounterHolder<DynamicValue> value;
		float atTicksTime;

	public:
		ValueAtTime(DynamicValue* pValue, float pAtTicksTime);
		virtual ~ValueAtTime();

		DynamicValue* getValue() { return value.get(); }
		float getAtTicksTime() { return atTicksTime; }
	};

private:
	vector<ValueAtTime> valuesAtTimes;

public:
	PiecewiseValue(objCounterParameters());
	virtual ~PiecewiseValue();

	//initialize and return a PiecewiseValue
	static PiecewiseValue* produce(objCounterParametersComma() vector<ValueAtTime> valuesAtTimes);
	//release a reference to this PiecewiseValue and return it to the pool if applicable
	virtual void release();
protected:
	//release the inner values before this is returned to the pool
	virtual void prepareReturnToPool();
public:
	//return a new value with all the values shifted so that it's the given value at time 0
	//assumes there is at least one value
	virtual DynamicValue* copyWithConstantValue(float pConstantValue);
	//get the value at the given time
	//assumes there is at least one value
	virtual float getValue(float ticksElapsed);
};
