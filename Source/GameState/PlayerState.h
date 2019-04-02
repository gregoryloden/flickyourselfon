#include "GameState/EntityState.h"

#define newPlayerState() produceWithoutArgs(PlayerState)

class PositionState;
class SpriteAnimation;
class EntityAnimation;

class PlayerState: public EntityState {
private:
	enum class SpriteDirection: int {
		Right = 0,
		Up = 1,
		Left = 2,
		Down = 3
	};

	static const float playerStartingXPosition;
	static const float playerStartingYPosition;
	static const float playerWidth;
	static const float playerHeight;
	static const float boundingBoxLeftOffset;
	static const float boundingBoxRightOffset;
	static const float boundingBoxTopOffset;
	static const float boundingBoxBottomOffset;
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
	void beginKicking(int ticksTime);
	void render(EntityState* camera, int ticksTime);
	void saveState(ofstream& file);
	bool loadState(string& line);
};
