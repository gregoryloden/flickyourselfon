#include "General.h"

class GameState onlyInDebug(: public ObjCounter) {
private:
	int currentSpriteHorizontalIndex;
	int currentSpriteVerticalIndex;
	int currentAnimationFrame;
	bool shouldQuitGame;

public:
	GameState();
	~GameState();

	void updateWithPreviousGameState(GameState* prev);
	void render();
	bool getShouldQuitGame();
};
