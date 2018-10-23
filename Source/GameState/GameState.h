#include "General/General.h"

class PlayerState;

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
