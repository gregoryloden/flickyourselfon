#include "Util/PooledReferenceCounter.h"

#define newEntityAnimation(startTicksTime, components) produceWithArgs(EntityAnimation, startTicksTime, components)
#define newEntityAnimationDelay(ticksDuration) produceWithArgs(EntityAnimation::Delay, ticksDuration)
#define newEntityAnimationSetVelocity(vx, vy) produceWithArgs(EntityAnimation::SetVelocity, vx, vy)
#define newEntityAnimationSetSpriteAnimation(animation) produceWithArgs(EntityAnimation::SetSpriteAnimation, animation)

class DynamicValue;
class EntityState;
class SpriteAnimation;

//a single EntityAnimation instance is shared across multiple game states
//it gets mutated but does not mutate a game state unless it's in the middle of updating
class EntityAnimation: public PooledReferenceCounter {
public:
	//components are exclusively held by EntityAnimations
	class Component: public PooledReferenceCounter {
	public:
		Component(objCounterParameters());
		~Component();

		//return whether this component is done handling the state and we can move on to the next component
		virtual bool handle(EntityState* entityState, int ticksTime) = 0;
		virtual int getDelayTicksDuration();
	};
	class Delay: public Component {
	private:
		int ticksDuration;

	public:
		Delay(objCounterParameters());
		~Delay();

		static Delay* produce(objCounterParametersComma() int pTicksDuration);
		virtual void release();
		virtual bool handle(EntityState* entityState, int ticksTime);
		virtual int getDelayTicksDuration();
	};
	class SetVelocity: public Component {
	private:
		ReferenceCounterHolder<DynamicValue> vx;
		ReferenceCounterHolder<DynamicValue> vy;

	public:
		SetVelocity(objCounterParameters());
		~SetVelocity();

		static SetVelocity* produce(objCounterParametersComma() DynamicValue* pVx, DynamicValue* pVy);
		virtual void release();
		virtual void prepareReturnToPool();
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SetSpriteAnimation: public Component {
	private:
		SpriteAnimation* animation;

	public:
		SetSpriteAnimation(objCounterParameters());
		~SetSpriteAnimation();

		static SetSpriteAnimation* produce(objCounterParametersComma() SpriteAnimation* pAnimation);
		virtual void release();
		virtual bool handle(EntityState* entityState, int ticksTime);
	};

	int lastUpdateTicksTime;
	vector<ReferenceCounterHolder<Component>> components;
	int nextComponentIndex;

	EntityAnimation(objCounterParameters());
	~EntityAnimation();

	static EntityAnimation* produce(
		objCounterParametersComma() int pStartTicksTime, vector<ReferenceCounterHolder<Component>> pComponents);
	virtual void release();
	virtual void prepareReturnToPool();
	bool update(EntityState* entityState, int ticksTime);
};
