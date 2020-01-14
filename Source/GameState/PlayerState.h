#include "GameState/EntityState.h"

#define newPlayerState(mapState) produceWithArgs(PlayerState, mapState)

class CollisionRect;
class EntityAnimation;
class Holder_EntityAnimationComponentVector;
class MapState;
class PositionState;
class Rail;
class SpriteAnimation;

class PlayerState: public EntityState {
private:
	static const int railToRailTicksDuration = 80;
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
	float lastControlledX;
	float lastControlledY;
	ReferenceCounterHolder<MapState> mapState;

public:
	PlayerState(objCounterParameters());
	virtual ~PlayerState();

	virtual void setSpriteDirection(SpriteDirection pSpriteDirection) { spriteDirection = pSpriteDirection; }
	void obtainBoot() { hasBoot = true; }
	static PlayerState* produce(objCounterParametersComma() MapState* mapState);
	void copyPlayerState(PlayerState* other);
	virtual void release();
	virtual void setNextCamera(GameState* nextGameState, int ticksTime);
	virtual void setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime);
	virtual void setGhostSprite(bool show, float x, float y, int ticksTime);
	virtual void mapKickSwitch(short switchId, bool allowRadioTowerAnimation, int ticksTime);
	virtual void mapKickResetSwitch(short resetSwitchId, int ticksTime);
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
public:
	void beginKicking(int ticksTime);
private:
	void kickAir(int ticksTime);
	void kickClimb(float yMoveDistance, int ticksTime);
	void kickFall(float xMoveDistance, float yMoveDistance, char fallHeight, int ticksTime);
	bool kickRail(float xPosition, float yPosition, int ticksTime);
	static bool addRailRideComponents(
		Rail* rail,
		Holder_EntityAnimationComponentVector* componentsHolder,
		float xPosition,
		float yPosition,
		int railMapX,
		int railMapY,
		float* outFinalXPosition,
		float* outFinalYPosition);
public:
	#ifdef DEBUG
		static void addNearestRailRideComponents(
			int railIndex,
			Holder_EntityAnimationComponentVector* componentsHolder,
			float xPosition,
			float yPosition,
			float* outFinalXPosition,
			float* outFinalYPosition);
	#endif
private:
	bool kickSwitch(float xPosition, float yPosition, int ticksTime);
public:
	static void addKickSwitchComponents(
		short switchId, Holder_EntityAnimationComponentVector* componentsHolder, bool allowRadioTowerAnimation);
private:
	bool kickResetSwitch(float xPosition, float yPosition, int ticksTime);
public:
	static void addKickResetSwitchComponents(short resetSwitchId, Holder_EntityAnimationComponentVector* componentsHolder);
	void render(EntityState* camera, int ticksTime);
	void saveState(ofstream& file);
	bool loadState(string& line);
	void setInitialZ();
	#ifdef DEBUG
		void setHighestZ();
	#endif
};
