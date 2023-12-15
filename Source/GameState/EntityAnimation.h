#include "Util/PooledReferenceCounter.h"

#define newEntityAnimation(startTicksTime, components) produceWithArgs(EntityAnimation, startTicksTime, components)
#define newEntityAnimationDelay(ticksDuration) produceWithArgs(EntityAnimation::Delay, ticksDuration)
#define newEntityAnimationSetPosition(x, y) produceWithArgs(EntityAnimation::SetPosition, x, y)
#define newEntityAnimationSetVelocity(vx, vy) produceWithArgs(EntityAnimation::SetVelocity, vx, vy)
#define newEntityAnimationSetSpriteAnimation(animation) produceWithArgs(EntityAnimation::SetSpriteAnimation, animation)
#define newEntityAnimationSetDirection(direction) produceWithArgs(EntityAnimation::SetDirection, direction)
#define newEntityAnimationSetGhostSprite(show, x, y, direction) \
	produceWithArgs(EntityAnimation::SetGhostSprite, show, x, y, direction)
#define newEntityAnimationSetScreenOverlayColor(r, g, b, a) produceWithArgs(EntityAnimation::SetScreenOverlayColor, r, g, b, a)
#define newEntityAnimationMapKickSwitch(switchId, moveRailsForward, allowRadioTowerAnimation) \
	produceWithArgs(EntityAnimation::MapKickSwitch, switchId, moveRailsForward, allowRadioTowerAnimation)
#define newEntityAnimationMapKickResetSwitch(resetSwitchId) produceWithArgs(EntityAnimation::MapKickResetSwitch, resetSwitchId)
#define newEntityAnimationSpawnParticle(x, y, animation, direction) \
	produceWithArgs(EntityAnimation::SpawnParticle, x, y, animation, direction)
#define newEntityAnimationSwitchToPlayerCamera() produceWithoutArgs(EntityAnimation::SwitchToPlayerCamera)

class DynamicValue;
class EntityState;
class SpriteAnimation;
enum class SpriteDirection: int;

namespace EntityAnimationTypes {
	//components are exclusively held by EntityAnimations
	class Component: public PooledReferenceCounter {
	public:
		Component(objCounterParameters());
		virtual ~Component();

		//return how long this component should last; only Delay overrides this
		virtual int getDelayTicksDuration() { return 0; }
		//return whether this component is done handling the state and we can move on to the next component
		virtual bool handle(EntityState* entityState, int ticksTime) = 0;
	};
}
//a single EntityAnimation instance is shared across multiple game states
//it gets mutated but does not mutate a game state unless it's in the middle of updating
class EntityAnimation: public PooledReferenceCounter {
public:
	class Delay: public EntityAnimationTypes::Component {
	private:
		int ticksDuration;

	public:
		Delay(objCounterParameters());
		virtual ~Delay();

		virtual int getDelayTicksDuration() { return ticksDuration; }
		//initialize and return a Delay
		static Delay* produce(objCounterParametersComma() int pTicksDuration);
		//release a reference to this Delay and return it to the pool if applicable
		virtual void release();
		//return that the animation should not continue updating without checking that the delay is finished
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SetPosition: public EntityAnimationTypes::Component {
	private:
		float x;
		float y;

	public:
		SetPosition(objCounterParameters());
		virtual ~SetPosition();

		//initialize and return a SetPosition
		static SetPosition* produce(objCounterParametersComma() float pX, float pY);
		//release a reference to this SetPosition and return it to the pool if applicable
		virtual void release();
		//return that the animation should continue updating after setting the position on the entity state
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SetVelocity: public EntityAnimationTypes::Component {
	private:
		ReferenceCounterHolder<DynamicValue> vx;
		ReferenceCounterHolder<DynamicValue> vy;

	public:
		SetVelocity(objCounterParameters());
		virtual ~SetVelocity();

		//initialize and return a SetVelocity
		static SetVelocity* produce(objCounterParametersComma() DynamicValue* pVx, DynamicValue* pVy);
		//release a reference to this SetVelocity and return it to the pool if applicable
		virtual void release();
	protected:
		//release components before this is returned to the pool
		virtual void prepareReturnToPool();
	public:
		//return that the animation should continue updating after setting the velocity on the entity state
		virtual bool handle(EntityState* entityState, int ticksTime);
		//return a SetVelocity that follows a curve from (0, 0) to (1, 1) with 0 slope at (0, 0) and (1, 1)
		static SetVelocity* cubicInterpolation(float xMoveDistance, float yMoveDistance, float ticksDuration);
	};
	class SetSpriteAnimation: public EntityAnimationTypes::Component {
	private:
		SpriteAnimation* animation;

	public:
		SetSpriteAnimation(objCounterParameters());
		virtual ~SetSpriteAnimation();

		//initialize and return a SetSpriteAnimation
		static SetSpriteAnimation* produce(objCounterParametersComma() SpriteAnimation* pAnimation);
		//release a reference to this SetSpriteAnimation and return it to the pool if applicable
		virtual void release();
		//return that the animation should continue updating after setting the sprite animation on the entity state
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SetDirection: public EntityAnimationTypes::Component {
	private:
		SpriteDirection direction;

	public:
		SetDirection(objCounterParameters());
		virtual ~SetDirection();

		//initialize and return a SetDirection
		static SetDirection* produce(objCounterParametersComma() SpriteDirection pDirection);
		//release a reference to this SetDirection and return it to the pool if applicable
		virtual void release();
		//return that the animation should continue updating after setting the sprite direction on the entity state
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SetGhostSprite: public EntityAnimationTypes::Component {
	private:
		bool show;
		float x;
		float y;
		SpriteDirection direction;

	public:
		SetGhostSprite(objCounterParameters());
		virtual ~SetGhostSprite();

		//initialize and return a SetGhostSprite
		static SetGhostSprite* produce(objCounterParametersComma() bool pShow, float pX, float pY, SpriteDirection pDirection);
		//release a reference to this SetGhostSprite and return it to the pool if applicable
		virtual void release();
		//return that the animation should continue updating after setting the ghost sprite on the entity state
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SetScreenOverlayColor: public EntityAnimationTypes::Component {
	private:
		ReferenceCounterHolder<DynamicValue> r;
		ReferenceCounterHolder<DynamicValue> g;
		ReferenceCounterHolder<DynamicValue> b;
		ReferenceCounterHolder<DynamicValue> a;

	public:
		SetScreenOverlayColor(objCounterParameters());
		virtual ~SetScreenOverlayColor();

		//initialize and return a SetScreenOverlayColor
		static SetScreenOverlayColor* produce(
			objCounterParametersComma() DynamicValue* pR, DynamicValue* pG, DynamicValue* pB, DynamicValue* pA);
		//release a reference to this SetScreenOverlayColor and return it to the pool if applicable
		virtual void release();
		//return that the animation should continue updating after setting the screen overlay color on the entity state
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class MapKickSwitch: public EntityAnimationTypes::Component {
	private:
		short switchId;
		bool moveRailsForward;
		bool allowRadioTowerAnimation;

	public:
		MapKickSwitch(objCounterParameters());
		virtual ~MapKickSwitch();

		//initialize and return a MapKickSwitch
		static MapKickSwitch* produce(
			objCounterParametersComma() short pSwitchId, bool pMoveRailsForward, bool pAllowRadioTowerAnimation);
		//release a reference to this MapKickSwitch and return it to the pool if applicable
		virtual void release();
		//return that the animation should continue updating after telling the map to kick the switch
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class MapKickResetSwitch: public EntityAnimationTypes::Component {
	private:
		short resetSwitchId;

	public:
		MapKickResetSwitch(objCounterParameters());
		virtual ~MapKickResetSwitch();

		//initialize and return a MapKickResetSwitch
		static MapKickResetSwitch* produce(objCounterParametersComma() short pResetSwitchId);
		//release a reference to this MapKickResetSwitch and return it to the pool if applicable
		virtual void release();
		//return that the animation should continue updating after telling the map to kick the reset switch
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SpawnParticle: public EntityAnimationTypes::Component {
	private:
		float x;
		float y;
		SpriteAnimation* animation;
		SpriteDirection direction;

	public:
		SpawnParticle(objCounterParameters());
		virtual ~SpawnParticle();

		//initialize and return a SpawnParticle
		static SpawnParticle* produce(
			objCounterParametersComma() float pX, float pY, SpriteAnimation* pAnimation, SpriteDirection pDirection);
		//release a reference to this SpawnParticle and return it to the pool if applicable
		virtual void release();
		//return that the animation should continue updating after spawning the particle
		virtual bool handle(EntityState* entityState, int ticksTime);
	};
	class SwitchToPlayerCamera: public EntityAnimationTypes::Component {
	public:
		SwitchToPlayerCamera(objCounterParameters());
		virtual ~SwitchToPlayerCamera();

		//initialize and return a SwitchToPlayerCamera
		static SwitchToPlayerCamera* produce(objCounterParameters());
		//release a reference to this SwitchToPlayerCamera and return it to the pool if applicable
		virtual void release();
		//return that the animation should continue updating after setting that the entity state should switch to the player
		//	camera
		virtual bool handle(EntityState* entityState, int ticksTime);
	};

private:
	int lastUpdateTicksTime;
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> components;
	int nextComponentIndex;

public:
	EntityAnimation(objCounterParameters());
	virtual ~EntityAnimation();

	//initialize and return an EntityAnimation
	static EntityAnimation* produce(
		objCounterParametersComma()
		int pStartTicksTime,
		vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* pComponents);
	//release a reference to this EntityAnimation and return it to the pool if applicable
	virtual void release();
protected:
	//release components before this is returned to the pool
	virtual void prepareReturnToPool();
public:
	//update this animation- process any components that happened since the last time this was updated
	//while this is state that is both shared across game states and mutated, the mutations only affect the state being updated
	//return whether the given state was updated (false means that the default update logic for the state should be used)
	bool update(EntityState* entityState, int ticksTime);
	//return the total ticks duration of all the components (really just the Delays)
	static int getComponentTotalTicksDuration(vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>& pComponents);
	//get the total ticks duration of this animation's components
	int getTotalTicksDuration();
};
