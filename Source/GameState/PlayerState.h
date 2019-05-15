#include "GameState/EntityState.h"

#define newPlayerState() produceWithoutArgs(PlayerState)

class PositionState;
class SpriteAnimation;
class EntityAnimation;
class MapState;

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
	static const string playerZFilePrefix;

	char xDirection;
	char yDirection;
	SpriteAnimation* spriteAnimation;
	int spriteAnimationStartTicksTime;
	SpriteDirection spriteDirection;
	bool hasBoot;
	float lastControlledX;
	float lastControlledY;
	char lastControlledZ;

public:
	PlayerState(objCounterParameters());
	~PlayerState();

	virtual void setSpriteDirection(SpriteDirection pSpriteDirection) { spriteDirection = pSpriteDirection; }
	void obtainBoot() { hasBoot = true; }
	static PlayerState* produce(objCounterParameters());
	void copyPlayerState(PlayerState* other);
	virtual void release();
	virtual void setNextCamera(GameState* nextGameState, int ticksTime);
	virtual void setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime);
	void updateWithPreviousPlayerState(PlayerState* prev, int ticksTime);
private:
	void updatePositionWithPreviousPlayerState(PlayerState* prev, int ticksTime);
	void collideWithEnvironmentWithPreviousPlayerState(PlayerState* prev);
	void updateSpriteWithPreviousPlayerState(PlayerState* prev, int ticksTime, bool usePreviousStateSpriteAnimation);
public:
	void beginKicking(MapState* mapState, int ticksTime);
private:
	void kickAir(int ticksTime);
	void kickClimb(float yMoveDistance, int ticksTime);
	void kickFall(float xMoveDistance, float yMoveDistance, char fallHeight, int ticksTime);
	bool kickRail(MapState* mapState, float xPosition, float yPosition, int ticksTime);
	bool kickSwitch(MapState* mapState, float xPosition, float yPosition, int ticksTime);
public:
	void render(EntityState* camera, int ticksTime);
	void saveState(ofstream& file);
	bool loadState(string& line);
};
