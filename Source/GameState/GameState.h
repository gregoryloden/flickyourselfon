#include "General/General.h"

class EntityState;
class PlayerState;

class GameState onlyInDebug(: public ObjCounter) {
private:
	PlayerState* playerState;
	EntityState* camera;
	bool shouldQuitGame;

public:
	GameState(objCounterParameters());
	~GameState();

	//return whether updates and renders should stop
	bool getShouldQuitGame() { return shouldQuitGame; }
	void updateWithPreviousGameState(GameState* prev, int ticksTime);
	void render(int ticksTime);
};
