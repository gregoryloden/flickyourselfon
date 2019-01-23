#include "GameState.h"
#include "GameState/MapState.h"
#include "GameState/PauseState.h"
#include "GameState/PlayerState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

GameState::GameState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
playerState(newPlayerState())
, camera(nullptr)
, pauseState(nullptr)
, pauseStartTicksTime(-1)
, gameTimeOffsetTicksDuration(0)
, shouldQuitGame(false) {
	camera = playerState;
}
GameState::~GameState() {
	delete playerState;
}
//update this game state by reading from the previous state
void GameState::updateWithPreviousGameState(GameState* prev, int ticksTime) {
	if (prev->pauseState.get() != nullptr) {
		PauseState* nextPauseState = prev->pauseState.get()->getNextPauseState();
		pauseState.set(nextPauseState);
		gameTimeOffsetTicksDuration = prev->gameTimeOffsetTicksDuration + ticksTime - prev->pauseStartTicksTime;
		pauseStartTicksTime = ticksTime;
		playerState->copyPlayerState(prev->playerState);
		if (nextPauseState != nullptr && nextPauseState->getShouldQuitGame())
			shouldQuitGame = true;
		return;
	}

	pauseState.set(nullptr);
	gameTimeOffsetTicksDuration = prev->gameTimeOffsetTicksDuration;
	int gameTicksTime = ticksTime - gameTimeOffsetTicksDuration;

	playerState->updateWithPreviousPlayerState(prev->playerState, gameTicksTime);

	//handle events
	SDL_Event gameEvent;
	while (SDL_PollEvent(&gameEvent) != 0) {
		switch (gameEvent.type) {
			case SDL_QUIT:
				shouldQuitGame = true;
				return;
			case SDL_KEYDOWN:
				if (gameEvent.key.keysym.scancode == Config::kickKey)
					playerState->beginKicking(gameTicksTime);
				else if (gameEvent.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					pauseState.set(newBasePauseState());
					pauseStartTicksTime = ticksTime;
				}
				break;
			default:
				break;
		}
	}

	//get our next camera anchor
	camera = camera->getNextCameraAnchor(gameTicksTime);
}
//render this state, which was deemed to be the last state to need rendering
void GameState::render(int ticksTime) {
	int gameTicksTime = (pauseState.get() != nullptr ? pauseStartTicksTime : ticksTime) - gameTimeOffsetTicksDuration;
	MapState::render(camera, gameTicksTime);
	playerState->render(camera, gameTicksTime);
	if (pauseState.get() != nullptr)
		pauseState.get()->render();
}
