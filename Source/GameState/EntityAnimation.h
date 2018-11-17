#include "Util/PooledReferenceCounter.h"

class EntityState;
class SpriteAnimation;

class EntityAnimation: public PooledReferenceCounter {
public:
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

		Delay* set(int pTicksDuration);
		virtual void release();
		virtual bool handle(EntityState* entityState, int ticksTime);
		virtual int getDelayTicksDuration();
	};
	class SetPosition: public Component {
	private:
		float x;
		float y;
		char z;

	public:
		SetPosition(objCounterParameters());
		~SetPosition();

		SetPosition* set(float pX, float pY, char pZ);
		virtual void release();
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SetVelocity: public Component {
	private:
		float vx;
		float vy;

	public:
		SetVelocity(objCounterParameters());
		~SetVelocity();

		SetVelocity* set(float pVx, float pVy);
		virtual void release();
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SetSpriteAnimation: public Component {
	private:
		SpriteAnimation* animation;

	public:
		SetSpriteAnimation(objCounterParameters());
		~SetSpriteAnimation();

		SetSpriteAnimation* set(SpriteAnimation* pAnimation);
		virtual void release();
		virtual bool handle(EntityState* entityState, int ticksTime);
	};

	int startTicksTime;
	vector<Component*> components;
	int nextComponentIndex;

	EntityAnimation(objCounterParameters());
	~EntityAnimation();

	EntityAnimation* set(int pStartTicksTime, vector<Component*> pComponents);
	virtual void release();
	virtual void prepareReturnToPool();
	bool update(EntityState* entityState, int ticksTime);
};
