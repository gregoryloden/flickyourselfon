#include "GameState.h"
#ifdef EDITOR
	#include "Editor/Editor.h"
#endif
#include "GameState/MapState.h"
#include "GameState/PauseState.h"
#include "GameState/PlayerState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

const char* GameState::savedGameFileName = "fyo.sav";
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

		if (nextPauseState != nullptr) {
			int endPauseDecision = nextPauseState->getEndPauseDecision();
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::Save) != 0)
				saveState();
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::Exit) != 0)
				shouldQuitGame = true;
			else if (endPauseDecision != 0)
				pauseState.set(nullptr);
		}
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
				#ifndef EDITOR
					if (gameEvent.key.keysym.scancode == Config::keyBindings.kickKey) {
						playerState->beginKicking(gameTicksTime);
						break;
					}
				#endif
				if (gameEvent.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					pauseState.set(newBasePauseState());
					pauseStartTicksTime = ticksTime;
				}
				break;
			#ifdef EDITOR
				case SDL_MOUSEBUTTONDOWN:
					Editor::handleClick(gameEvent.button);
					break;
			#endif
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
	#ifdef EDITOR
		Editor::render(camera, gameTicksTime);
	#endif
}
//save the state to a file
void GameState::saveState() {
	ofstream file;
	file.open(savedGameFileName);
	playerState->saveState(file);
	file.close();
}
//load the state from the save file if there is one
void GameState::loadSavedState() {
	ifstream file;
	file.open(savedGameFileName);
	string line;
	while (getline(file, line)) {
		playerState->loadState(line);
	}
	file.close();
}
