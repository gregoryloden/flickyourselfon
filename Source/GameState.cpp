#include "GameState.h"
#include "SpriteRegistry.h"
#include "SpriteSheet.h"

GameState::GameState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
currentSpriteHorizontalIndex(0)
, currentSpriteVerticalIndex(0)
, currentAnimationFrame(0)
, shouldQuitGame(false) {
}
GameState::~GameState() {}
//update this game state by reading from the previous state
void GameState::updateWithPreviousGameState(GameState* prev) {
	//handle events
	int clicks = 0;
	SDL_Event gameEvent;
	while (SDL_PollEvent(&gameEvent) != 0) {
		switch (gameEvent.type) {
			case SDL_QUIT:
				shouldQuitGame = true;
				return;
			case SDL_MOUSEBUTTONDOWN:
				clicks++;
				break;
			default:
				break;
		}
	}

	//update state
	currentSpriteHorizontalIndex = (prev->currentSpriteHorizontalIndex + clicks) % 9;
	if (prev->currentAnimationFrame == 12) {
		currentSpriteVerticalIndex = (prev->currentSpriteVerticalIndex + 1) % 4;
		currentAnimationFrame = 1;
	} else {
		currentSpriteVerticalIndex = prev->currentSpriteVerticalIndex;
		currentAnimationFrame = prev->currentAnimationFrame + 1;
	}
}
//render this state, which was deemed to be the last state to need rendering
void GameState::render() {
	SpriteRegistry::player->render(currentSpriteHorizontalIndex, currentSpriteVerticalIndex);
}
//return whether updates and renders should stop
bool GameState::getShouldQuitGame() {
	return shouldQuitGame;
}
