#include "General/General.h"

class EntityState;
class PlayerState;

class GameState onlyInDebug(: public ObjCounter) {
private:
	PlayerState* playerState;
	EntityState* currentCamera;
	bool shouldQuitGame;

public:
	GameState(objCounterParameters());
	~GameState();

	void updateWithPreviousGameState(GameState* prev);
	void render(int ticksTime);
	bool getShouldQuitGame();
};
