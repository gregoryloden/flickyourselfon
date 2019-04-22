#include "GameState.h"
#ifdef EDITOR
	#include "Editor/Editor.h"
#endif
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
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
	sawIntroAnimation = prev->sawIntroAnimation;

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
					prev->camera->setNextCamera(this, gameTicksTime);
					Editor::handleClick(gameEvent.button, camera, gameTicksTime);
					break;
				case SDL_MOUSEMOTION:
					if ((SDL_GetMouseState(nullptr, nullptr) & (SDL_BUTTON_LMASK | SDL_BUTTON_MMASK | SDL_BUTTON_RMASK)) != 0) {
						prev->camera->setNextCamera(this, gameTicksTime);
						Editor::handleClick(gameEvent.button, camera, gameTicksTime);
					}
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

	//since we only set the player camera once we get the boot, reset the tile
	sawIntroAnimation = true;
	MapState::setIntroAnimationBootTile(false);
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
	if (camera == dynamicCameraAnchor.get())
		dynamicCameraAnchor.get()->render(gameTicksTime);
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
	if (sawIntroAnimation)
		file << sawIntroAnimationFilePrefix << "true\n";
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
		MapState::setIntroAnimationBootTile(true);
		camera = dynamicCameraAnchor.get();

		//player animation component helpers
		float speedPerTick = PlayerState::speedPerSecond / (float)Config::ticksPerSecond;
		EntityAnimation::SetVelocity* stopMoving = newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
		EntityAnimation::SetVelocity* walkRight = newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, speedPerTick, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
		EntityAnimation::SetVelocity* walkUp = newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, -speedPerTick, 0.0f, 0.0f, 0.0f));
		EntityAnimation::SetVelocity* walkLeft = newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, -speedPerTick, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
		EntityAnimation::SetVelocity* walkDown = newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, speedPerTick, 0.0f, 0.0f, 0.0f));
		EntityAnimation::SetSpriteAnimation* setWalkingAnimation =
			newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerWalkingAnimation);
		EntityAnimation::SetSpriteAnimation* clearSpriteAnimation = newEntityAnimationSetSpriteAnimation(nullptr);

		EntityAnimation* playerEntityAnimation = newEntityAnimation(
			0,
			{
				newEntityAnimationSetPosition(
					PlayerState::introAnimationPlayerCenterX, PlayerState::introAnimationPlayerCenterY) COMMA
				newEntityAnimationDelay(3000) COMMA
				//walk to the wall
				walkRight COMMA
				setWalkingAnimation COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Right) COMMA
				newEntityAnimationDelay(1700) COMMA
				//walk down, stop at the boot
				walkDown COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Down) COMMA
				newEntityAnimationDelay(1000) COMMA
				stopMoving COMMA
				clearSpriteAnimation COMMA
				newEntityAnimationDelay(1500) COMMA
				//look at the boot
				newEntityAnimationSetSpriteDirection(SpriteDirection::Right) COMMA
				newEntityAnimationDelay(800) COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Down) COMMA
				newEntityAnimationDelay(1000) COMMA
				//walk around the boot
				walkDown COMMA
				setWalkingAnimation COMMA
				newEntityAnimationDelay(900) COMMA
				stopMoving COMMA
				clearSpriteAnimation COMMA
				newEntityAnimationDelay(300) COMMA
				walkRight COMMA
				setWalkingAnimation COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Right) COMMA
				newEntityAnimationDelay(1800) COMMA
				//stop and look around
				stopMoving COMMA
				clearSpriteAnimation COMMA
				newEntityAnimationDelay(700) COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Up) COMMA
				newEntityAnimationDelay(700) COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Down) COMMA
				newEntityAnimationDelay(500) COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Left) COMMA
				newEntityAnimationDelay(350) COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Right) COMMA
				newEntityAnimationDelay(1000) COMMA
				//walk up
				newEntityAnimationSetSpriteDirection(SpriteDirection::Up) COMMA
				walkUp COMMA
				setWalkingAnimation COMMA
				newEntityAnimationDelay(1700) COMMA
				//stop and look around
				stopMoving COMMA
				clearSpriteAnimation COMMA
				newEntityAnimationDelay(1000) COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Right) COMMA
				newEntityAnimationDelay(600) COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Left) COMMA
				newEntityAnimationDelay(400) COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Up) COMMA
				newEntityAnimationDelay(1200) COMMA
				//walk back down to the boot
				walkDown COMMA
				setWalkingAnimation COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Down) COMMA
				newEntityAnimationDelay(900) COMMA
				walkLeft COMMA
				newEntityAnimationSetSpriteDirection(SpriteDirection::Left) COMMA
				newEntityAnimationDelay(400) COMMA
				stopMoving COMMA
				clearSpriteAnimation COMMA
				newEntityAnimationDelay(500) COMMA
				//approach more slowly
				newEntityAnimationSetVelocity(
					newCompositeQuarticValue(0.0f, -speedPerTick * 0.5f, 0.0f, 0.0f, 0.0f),
					newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)) COMMA
				setWalkingAnimation COMMA
				newEntityAnimationDelay(540) COMMA
				stopMoving COMMA
				clearSpriteAnimation COMMA
				newEntityAnimationDelay(800) COMMA
				//put on the boot
				newEntityAnimationSetVelocity(
					newCompositeQuarticValue(0.0f, -0.017f, 0.0f, 0.0f, 0.0f),
					newCompositeQuarticValue(0.0f, 0.004f, 0.0f, 0.0f, 0.0f)) COMMA
				newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerLegLiftAnimation) COMMA
				newEntityAnimationDelay(SpriteRegistry::playerLegLiftAnimation->getTotalTicksDuration()) COMMA
				stopMoving COMMA
				clearSpriteAnimation
			});
		vector<ReferenceCounterHolder<EntityAnimation::Component>> cameraAnimationComponents ({
			newEntityAnimationSetPosition(MapState::introAnimationCameraCenterX, MapState::introAnimationCameraCenterY) COMMA
			newEntityAnimationSetScreenOverlayColor(
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(1.0f, -0.000875f, 0.0f, 0.0f, 0.0f)) COMMA
			newEntityAnimationDelay(1000) COMMA
			newEntityAnimationSetScreenOverlayColor(
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.125f, 0.0f, 0.0f, 0.0f, 0.0f))
		});

		//delay the camera animation until the end of the player animation minus the leg lift animation duration
		int legLiftAnimationTicksDuration = SpriteRegistry::playerLegLiftAnimation->getTotalTicksDuration();
		int cameraAnimationTicksDuration = legLiftAnimationTicksDuration;
		for (ReferenceCounterHolder<EntityAnimation::Component>& component : cameraAnimationComponents)
			cameraAnimationTicksDuration += component.get()->getDelayTicksDuration();
		cameraAnimationComponents.push_back(
			newEntityAnimationDelay(
				playerEntityAnimation->getTotalTicksDuration() - cameraAnimationTicksDuration));
		//fade in the screen and then switch to the player camera
		cameraAnimationComponents.push_back(
			newEntityAnimationSetScreenOverlayColor(
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.125f, -0.125f / (float)legLiftAnimationTicksDuration, 0.0f, 0.0f, 0.0f)));
		cameraAnimationComponents.push_back(newEntityAnimationDelay(legLiftAnimationTicksDuration));
		cameraAnimationComponents.push_back(
			newEntityAnimationSetScreenOverlayColor(
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)));
		cameraAnimationComponents.push_back(newEntityAnimationSwitchToPlayerCamera());

		playerState.get()->beginEntityAnimation(playerEntityAnimation, 0);
		dynamicCameraAnchor.get()->beginEntityAnimation(newEntityAnimation(0, cameraAnimationComponents), 0);
	}
}
