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
#include "Util/StringUtils.h"

const char* GameState::savedGameFileName = "fyo.sav";
const string GameState::sawIntroAnimationFilePrefix = "sawIntroAnimation ";
GameState::GameState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
sawIntroAnimation(false)
, playerState(nullptr)
, mapState(nullptr)
, dynamicCameraAnchor(nullptr)
, camera(nullptr)
, pauseState(nullptr)
, pauseStartTicksTime(-1)
, gameTimeOffsetTicksDuration(0)
, shouldQuitGame(false) {
}
GameState::~GameState() {
	//don't delete the camera, it's one of our other states
}
//update this game state by reading from the previous state
void GameState::updateWithPreviousGameState(GameState* prev, int ticksTime) {
	//first things first: dump all our previous state so that we can start fresh
	playerState.set(nullptr);
	mapState.set(nullptr);
	dynamicCameraAnchor.set(nullptr);

	//don't update any state if we're paused
	if (prev->pauseState.get() != nullptr) {
		PauseState* nextPauseState = prev->pauseState.get()->getNextPauseState();
		pauseState.set(nextPauseState);
		gameTimeOffsetTicksDuration = prev->gameTimeOffsetTicksDuration + ticksTime - prev->pauseStartTicksTime;
		pauseStartTicksTime = ticksTime;

		//use the direct pointers from the previous state since they didn't change
		playerState.set(prev->playerState.get());
		mapState.set(prev->mapState.get());
		dynamicCameraAnchor.set(prev->dynamicCameraAnchor.get());

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

	//we're not paused, reset the pause state just to be safe, and find what time it really is
	pauseState.set(nullptr);
	gameTimeOffsetTicksDuration = prev->gameTimeOffsetTicksDuration;
	int gameTicksTime = ticksTime - gameTimeOffsetTicksDuration;

	//update states
	playerState.set(newPlayerState());
	playerState.get()->updateWithPreviousPlayerState(prev->playerState.get(), gameTicksTime);
	mapState.set(newMapState());
	mapState.get()->updateWithPreviousMapState(prev->mapState.get(), gameTicksTime);
	dynamicCameraAnchor.set(newDynamicCameraAnchor());
	dynamicCameraAnchor.get()->updateWithPreviousDynamicCameraAnchor(prev->dynamicCameraAnchor.get(), gameTicksTime);

	//handle events after states have been updated
	SDL_Event gameEvent;
	while (SDL_PollEvent(&gameEvent) != 0) {
		switch (gameEvent.type) {
			case SDL_QUIT:
				shouldQuitGame = true;
				break;
			case SDL_KEYDOWN:
				#ifndef EDITOR
					if (gameEvent.key.keysym.scancode == Config::keyBindings.kickKey) {
						playerState.get()->beginKicking(gameTicksTime);
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
					Editor::handleClick(gameEvent.button, camera.get(), gameTicksTime);
					break;
				case SDL_MOUSEMOTION:
					if ((SDL_GetMouseState(nullptr, nullptr) & (SDL_BUTTON_LMASK | SDL_BUTTON_MMASK | SDL_BUTTON_RMASK)) != 0)
						Editor::handleClick(gameEvent.button, camera.get(), gameTicksTime);
					break;
			#endif
			default:
				break;
		}
	}

	//set our next camera anchor
	prev->camera->setNextCamera(this, gameTicksTime);
}
//set our camera to our player
void GameState::setPlayerCamera() {
	camera = playerState.get();
}
//set our camera to the dynamic camera anchor
void GameState::setDynamicCamera() {
	camera = dynamicCameraAnchor.get();
}
//render this state, which was deemed to be the last state to need rendering
void GameState::render(int ticksTime) {
	int gameTicksTime = (pauseState.get() != nullptr ? pauseStartTicksTime : ticksTime) - gameTimeOffsetTicksDuration;
	mapState.get()->render(camera, gameTicksTime);
	playerState.get()->render(camera, gameTicksTime);
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
	playerState.get()->saveState(file);
	file.close();
}
//initialize our state and load the state from the save file if there is one
void GameState::loadInitialState() {
	//first things first, we need some state
	playerState.set(newPlayerState());
	mapState.set(newMapState());
	dynamicCameraAnchor.set(newDynamicCameraAnchor());

	//next, load state if we can
	ifstream file;
	file.open(savedGameFileName);
	string line;
	while (getline(file, line)) {
		if (StringUtils::startsWith(line, sawIntroAnimationFilePrefix))
			sawIntroAnimation = strcmp(line.c_str() + sawIntroAnimationFilePrefix.size(), "true") == 0;
		else
			playerState.get()->loadState(line);
	}
	file.close();

//TODO: intro animation
//sawIntroAnimation = true;
	//and finally, setup the initial state
	if (sawIntroAnimation) {
		playerState.get()->obtainBoot();
		camera = playerState.get();
	} else {
		MapState::setIntroAnimationBootTile();
		dynamicCameraAnchor.get()->setPosition(MapState::introAnimationMapCenterX, MapState::introAnimationMapCenterY, 0);
		camera = dynamicCameraAnchor.get();
	}
}
