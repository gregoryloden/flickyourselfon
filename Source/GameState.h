#include "General.h"

class Animation;
class PlayerState;
class PositionState;
enum class PlayerSpriteDirection: int;

class GameState onlyInDebug(: public ObjCounter) {
private:
	int ticksTime;
	PlayerState* playerState;
	bool shouldQuitGame;

public:
	GameState(objCounterParameters());
	~GameState();

	void updateWithPreviousGameState(GameState* prev);
	void render();
	bool getShouldQuitGame();
};
class PlayerState onlyInDebug(: public ObjCounter) {
private:
	PositionState* position;
	int walkingAnimationStartTicksTime;
	PlayerSpriteDirection spriteDirection;

public:
	PlayerState(objCounterParameters());
	~PlayerState();

	PositionState* getPosition() { return position; }
	void updateWithPreviousPlayerState(PlayerState* prev, int ticksTime);
	void render();
};
class PositionState onlyInDebug(: public ObjCounter) {
private:
	float x;
	float y;
	char dX;
	char dY;
	float speed;
	int lastUpdateTicksTime;

public:
	PositionState(objCounterParametersComma() float pX, float pY);
	~PositionState();

	char getDX() { return dX; }
	char getDY() { return dY; }
	float getX(int ticksTime);
	float getY(int ticksTime);
	bool isMoving();
	bool updatedSameTimeAs(PositionState* other);
	void updateWithPreviousPositionState(PositionState* prev, int ticksTime);
};
