#include "Util/PooledReferenceCounter.h"

#define newEntityAnimation(startTicksTime, components) produceWithArgs(EntityAnimation, startTicksTime, components)
#define newEntityAnimationDelay(ticksDuration) produceWithArgs(EntityAnimation::Delay, ticksDuration)
#define newEntityAnimationSetPosition(x, y) produceWithArgs(EntityAnimation::SetPosition, x, y)
#define newEntityAnimationSetVelocity(vx, vy) produceWithArgs(EntityAnimation::SetVelocity, vx, vy)
#define newEntityAnimationSetSpriteAnimation(animation) produceWithArgs(EntityAnimation::SetSpriteAnimation, animation)
#define newEntityAnimationSetSpriteDirection(direction) produceWithArgs(EntityAnimation::SetSpriteDirection, direction)
#define newEntityAnimationSetScreenOverlayColor(r, g, b, a) produceWithArgs(EntityAnimation::SetScreenOverlayColor, r, g, b, a)
#define newEntityAnimationSwitchToPlayerCamera() produceWithoutArgs(EntityAnimation::SwitchToPlayerCamera)

class DynamicValue;
class EntityState;
class SpriteAnimation;
enum class SpriteDirection: int;

//a single EntityAnimation instance is shared across multiple game states
//it gets mutated but does not mutate a game state unless it's in the middle of updating
class EntityAnimation: public PooledReferenceCounter {
public:
	//components are exclusively held by EntityAnimations
	class Component: public PooledReferenceCounter {
	public:
		Component(objCounterParameters());
		~Component();

		//return how long this component should last; only Delay overrides this
		virtual int getDelayTicksDuration() { return 0; }
		//return whether this component is done handling the state and we can move on to the next component
		virtual bool handle(EntityState* entityState, int ticksTime) = 0;
	};
	class Delay: public Component {
	private:
		int ticksDuration;

	public:
		Delay(objCounterParameters());
		~Delay();

		virtual int getDelayTicksDuration() { return ticksDuration; }
		static Delay* produce(objCounterParametersComma() int pTicksDuration);
		virtual void release();
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SetPosition: public Component {
	private:
		float x;
		float y;

	public:
		SetPosition(objCounterParameters());
		~SetPosition();

		static SetPosition* produce(objCounterParametersComma() float pX, float pY);
		virtual void release();
		virtual bool handle(EntityState* entityState, int ticksTime);
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
	protected:
		virtual void prepareReturnToPool();
	public:
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
	class SetSpriteDirection: public Component {
	private:
		SpriteDirection direction;

	public:
		SetSpriteDirection(objCounterParameters());
		~SetSpriteDirection();

		static SetSpriteDirection* produce(objCounterParametersComma() SpriteDirection pDirection);
		virtual void release();
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SetScreenOverlayColor: public Component {
	private:
		ReferenceCounterHolder<DynamicValue> r;
		ReferenceCounterHolder<DynamicValue> g;
		ReferenceCounterHolder<DynamicValue> b;
		ReferenceCounterHolder<DynamicValue> a;

	public:
		SetScreenOverlayColor(objCounterParameters());
		~SetScreenOverlayColor();

		static SetScreenOverlayColor* produce(
			objCounterParametersComma() DynamicValue* pR, DynamicValue* pG, DynamicValue* pB, DynamicValue* pA);
		virtual void release();
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SwitchToPlayerCamera: public Component {
	public:
		SwitchToPlayerCamera(objCounterParameters());
		~SwitchToPlayerCamera();

		static SwitchToPlayerCamera* produce(objCounterParameters());
		virtual void release();
		virtual bool handle(EntityState* entityState, int ticksTime);
	};

private:
	int lastUpdateTicksTime;
	vector<ReferenceCounterHolder<Component>> components;
	int nextComponentIndex;

public:
	EntityAnimation(objCounterParameters());
	~EntityAnimation();

	int getLastUpdateTicksTime() { return lastUpdateTicksTime; }
	static EntityAnimation* produce(
		objCounterParametersComma() int pStartTicksTime, vector<ReferenceCounterHolder<Component>> pComponents);
	virtual void release();
protected:
	virtual void prepareReturnToPool();
public:
	bool update(EntityState* entityState, int ticksTime);
	int getTotalTicksDuration();
};
