#include "GameState.h"
#include "GameState/MapState.h"
#include "GameState/PlayerState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

GameState::GameState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
playerState(newWithoutArgs(PlayerState))
, currentCamera(nullptr)
, shouldQuitGame(false) {
	currentCamera = playerState;
}
GameState::~GameState() {
	delete playerState;
}
//update this game state by reading from the previous state
void GameState::updateWithPreviousGameState(GameState* prev) {
	int ticksTime = (int)SDL_GetTicks();
	playerState->updateWithPreviousPlayerState(prev->playerState, ticksTime);

	//handle events
	SDL_Event gameEvent;
	while (SDL_PollEvent(&gameEvent) != 0) {
		switch (gameEvent.type) {
			case SDL_QUIT:
				shouldQuitGame = true;
				return;
			case SDL_KEYDOWN:
				if (gameEvent.key.keysym.scancode == Config::kickKey)
					playerState->beginKickingAnimation(ticksTime);
				break;
			default:
				break;
		}
	}
}
//render this state, which was deemed to be the last state to need rendering
void GameState::render(int ticksTime) {
	MapState::render(currentCamera, ticksTime);
	playerState->render(currentCamera, ticksTime);
}
//return whether updates and renders should stop
bool GameState::getShouldQuitGame() {
	return shouldQuitGame;
}
