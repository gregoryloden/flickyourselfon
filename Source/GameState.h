#include "General.h"

class Animation;
class PlayerState;
enum class PlayerSpriteDirection: int;

class GameState onlyInDebug(: public ObjCounter) {
private:
	int frame;
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
	float x;
	float y;
	char dX;
	char dY;
	int walkingAnimationFrame;
	PlayerSpriteDirection spriteDirection;

public:
	PlayerState(objCounterParameters());
	~PlayerState();

	float getX() { return x; }
	float getY() { return y; }
	void updateWithPreviousPlayerState(PlayerState* prev, int frame);
	void render();
};
