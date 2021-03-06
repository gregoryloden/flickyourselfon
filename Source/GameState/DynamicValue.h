#include "Util/PooledReferenceCounter.h"

#define newConstantValue(value) produceWithArgs(ConstantValue, value)
#define newCompositeQuarticValue(\
		constantValue, linearValuePerTick, quadraticValuePerTick, cubicValuePerTick, quarticValuePerTick) \
	produceWithArgs(\
		CompositeQuarticValue, constantValue, linearValuePerTick, quadraticValuePerTick, cubicValuePerTick, quarticValuePerTick)
#define newLinearInterpolatedValue(valuesAtTimes) produceWithArgs(LinearInterpolatedValue, valuesAtTimes)

//each DynamicValue is only held in one place at a time
class DynamicValue: public PooledReferenceCounter {
public:
	DynamicValue(objCounterParameters());
	virtual ~DynamicValue();

	//copy this DynamicValue such that at 0 ticks elapsed, it equals the provided constant value
	virtual DynamicValue* copyWithConstantValue(float pConstantValue) = 0;
	//return the value after the given amount of time has elapsed
	virtual float getValue(int ticksElapsed) = 0;
};
class ConstantValue: public DynamicValue {
private:
	float value;

public:
	ConstantValue(objCounterParameters());
	virtual ~ConstantValue();

	static ConstantValue* produce(objCounterParametersComma() float pValue);
	virtual void release();
	virtual DynamicValue* copyWithConstantValue(float pConstantValue);
	virtual float getValue(int ticksElapsed);
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
class LinearInterpolatedValue: public DynamicValue {
public:
	//Should only be allocated within an object, on the stack, or as a static object
	class ValueAtTime {
	private:
		float value;
		int atTicksTime;

	public:
		ValueAtTime(float pValue, int pAtTicksTime);
		virtual ~ValueAtTime();

		float getValue() { return value; }
		int getAtTicksTime() { return atTicksTime; }
	};

private:
	vector<ValueAtTime> valuesAtTimes;

public:
	LinearInterpolatedValue(objCounterParameters());
	virtual ~LinearInterpolatedValue();

	static LinearInterpolatedValue* produce(objCounterParametersComma() vector<ValueAtTime> valuesAtTimes);
	virtual void release();
	virtual DynamicValue* copyWithConstantValue(float pConstantValue);
	virtual float getValue(int ticksElapsed);
};
