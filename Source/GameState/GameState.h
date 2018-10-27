#include "General/General.h"

class PlayerState;

class GameState onlyInDebug(: public ObjCounter) {
private:
	PlayerState* playerState;
	bool shouldQuitGame;

public:
	GameState(objCounterParameters());
	~GameState();

	void updateWithPreviousGameState(GameState* prev);
	void render();
	bool getShouldQuitGame();
};
