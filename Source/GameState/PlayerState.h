#include "GameState/EntityState.h"

#define newPlayerState(mapState) produceWithArgs(PlayerState, mapState)

class CollisionRect;
class EntityAnimation;
class Holder_EntityAnimationComponentVector;
class KickAction;
class MapState;
class PositionState;
class Rail;
class SpriteAnimation;

class PlayerState: public EntityState {
private:
	static const int railToRailTicksDuration = 80;
	static const int autoClimbFallTriggerDelay = 500;
	static const float playerWidth;
	static const float playerHeight;
	static const float boundingBoxLeftOffset;
	static const float boundingBoxRightOffset;
	static const float boundingBoxTopOffset;
	static const float boundingBoxBottomOffset;
	static const float boundingBoxCenterYOffset;
public:
	static const float introAnimationPlayerCenterX;
	static const float introAnimationPlayerCenterY;
	static const float speedPerSecond;
	static const float diagonalSpeedPerSecond;
private:
	//only kick something if you're less than this distance from it
	//visually, you have to be 1 pixel away or closer
	static const float kickingDistanceLimit;
	static const string playerXFilePrefix;
	static const string playerYFilePrefix;

	char z;
	char xDirection;
	char yDirection;
	float lastXMovedDelta;
	float lastYMovedDelta;
	CollisionRect* collisionRect;
	SpriteAnimation* spriteAnimation;
	int spriteAnimationStartTicksTime;
	SpriteDirection spriteDirection;
	bool hasBoot;
	ReferenceCounterHolder<DynamicValue> ghostSpriteX;
	ReferenceCounterHolder<DynamicValue> ghostSpriteY;
	int ghostSpriteStartTicksTime;
	ReferenceCounterHolder<MapState> mapState;
	ReferenceCounterHolder<KickAction> availableKickAction;
	int autoClimbFallStartTicksTime;
	bool canImmediatelyAutoClimbFall;
	float lastControlledX;
	float lastControlledY;

public:
	PlayerState(objCounterParameters());
	virtual ~PlayerState();

	virtual void setSpriteDirection(SpriteDirection pSpriteDirection) { spriteDirection = pSpriteDirection; }
	char getZ() { return z; }
	void obtainBoot() { hasBoot = true; }
	static PlayerState* produce(objCounterParametersComma() MapState* mapState);
	void copyPlayerState(PlayerState* other);
	virtual void release();
	virtual void setNextCamera(GameState* nextGameState, int ticksTime);
	virtual void setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime);
	virtual void setGhostSprite(bool show, float x, float y, int ticksTime);
	virtual void mapKickSwitch(short switchId, bool allowRadioTowerAnimation, int ticksTime);
	virtual void mapKickResetSwitch(short resetSwitchId, int ticksTime);
	bool showTutorialConnectionsForKickAction();
	short getKickActionResetSwitchId();
	void updateWithPreviousPlayerState(PlayerState* prev, int ticksTime);
private:
	void updatePositionWithPreviousPlayerState(PlayerState* prev, int ticksTime);
	void setXAndUpdateCollisionRect(DynamicValue* newX);
	void setYAndUpdateCollisionRect(DynamicValue* newY);
	void collideWithEnvironmentWithPreviousPlayerState(PlayerState* prev);
	void addMapCollisions(
		int mapX, int mapY, vector<ReferenceCounterHolder<CollisionRect>>& collidedRects, vector<short> seenSwitchIds);
	float netCollisionDuration(CollisionRect* other);
	float xCollisionDuration(CollisionRect* other);
	float yCollisionDuration(CollisionRect* other);
	void updateSpriteWithPreviousPlayerState(PlayerState* prev, int ticksTime, bool usePreviousStateSpriteAnimation);
	void setKickAction();
	bool setRailKickAction(float xPosition, float yPosition);
	bool setSwitchKickAction(float xPosition, float yPosition);
	bool setResetSwitchKickAction(float xPosition, float yPosition);
	bool setClimbKickAction(float xPosition, float yPosition);
	bool setFallKickAction(float xPosition, float yPosition);
	void tryAutoClimbFall(PlayerState* prev, int ticksTime);
public:
	void beginKicking(int ticksTime);
private:
	void kickAir(int ticksTime);
	void kickClimb(float yMoveDistance, int ticksTime);
	void kickFall(float xMoveDistance, float yMoveDistance, char fallHeight, int ticksTime);
	void kickRail(short railId, float xPosition, float yPosition, int ticksTime);
public:
	static void addRailRideComponents(
		short railId,
		Holder_EntityAnimationComponentVector* componentsHolder,
		float xPosition,
		float yPosition,
		float* outFinalXPosition,
		float* outFinalYPosition);
private:
	void kickSwitch(short switchId, int ticksTime);
public:
	static void addKickSwitchComponents(
		short switchId, Holder_EntityAnimationComponentVector* componentsHolder, bool allowRadioTowerAnimation);
private:
	void kickResetSwitch(short resetSwitchId, int ticksTime);
public:
	static void addKickResetSwitchComponents(short resetSwitchId, Holder_EntityAnimationComponentVector* componentsHolder);
	void render(EntityState* camera, int ticksTime);
	void renderKickAction(EntityState* camera, bool hasRailsToReset, int ticksTime);
	void saveState(ofstream& file);
	bool loadState(string& line);
	void setInitialZ();
	void reset();
	#ifdef DEBUG
		void setHighestZ();
	#endif
};
