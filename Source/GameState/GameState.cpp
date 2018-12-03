#include "GameState.h"
#include "GameState/MapState.h"
#include "GameState/PlayerState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

GameState::GameState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
playerState(newWithoutArgs(PlayerState))
, camera(nullptr)
, shouldQuitGame(false) {
	camera = playerState;
}
GameState::~GameState() {
	delete playerState;
}
//update this game state by reading from the previous state
void GameState::updateWithPreviousGameState(GameState* prev, int ticksTime) {
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
					playerState->beginKicking(ticksTime);
				break;
			default:
				break;
		}
	}

	//get our next camera anchor
	camera = camera->getNextCameraAnchor(ticksTime);
}
//render this state, which was deemed to be the last state to need rendering
void GameState::render(int ticksTime) {
	MapState::render(camera, ticksTime);
	playerState->render(camera, ticksTime);
}
