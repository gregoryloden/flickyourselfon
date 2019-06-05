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
#include "Sprites/Text.h"
#include "Util/Config.h"
#include "Util/StringUtils.h"

const float GameState::squareSwitchesAnimationCenterWorldX =
	280.0f + (float)(MapState::firstLevelTileOffsetX * MapState::tileSize);
const float GameState::squareSwitchesAnimationCenterWorldY =
	80.0f + (float)(MapState::firstLevelTileOffsetY * MapState::tileSize);
const char* GameState::titleGameName = "Flick Yourself On";
const char* GameState::titleCreditsLine1 = "A game by";
const char* GameState::titleCreditsLine2 = "Gregory Loden";
const char* GameState::titlePostCreditsMessage = "Thanks for playing!";
const char* GameState::savedGameFileName = "fyo.sav";
const string GameState::sawIntroAnimationFilePrefix = "sawIntroAnimation ";
GameState::GameState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
sawIntroAnimation(false)
, titleAnimation(TitleAnimation::None)
, titleAnimationStartTicksTime(0)
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

	//copy values that don't usually change from state to state
	sawIntroAnimation = prev->sawIntroAnimation;
	titleAnimation = prev->titleAnimation;
	titleAnimationStartTicksTime = prev->titleAnimationStartTicksTime;

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
		camera = prev->camera;

		if (nextPauseState != nullptr) {
			int endPauseDecision = nextPauseState->getEndPauseDecision();
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::Save) != 0)
				saveState();
			//don't reset the game in editor mode
			#ifndef EDITOR
				if ((endPauseDecision & (int)PauseState::EndPauseDecision::Reset) != 0) {
					resetGame(ticksTime);
					//don't check for Exit, just return here
					return;
				}
			#endif
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::Exit) != 0)
				shouldQuitGame = true;
			else if (endPauseDecision != 0)
				pauseState.set(nullptr);
		}
		return;
	}

	//we're not paused, reset the pause state in case we had one from the last update, and find what time it really is
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

	//forget the previous camera if we're starting our radio tower animation
	if (mapState.get()->getShouldPlayRadioTowerAnimation()) {
		startRadioTowerAnimation(ticksTime);
		setDynamicCamera();
	//otherwise set our next camera anchor
	} else
		prev->camera->setNextCamera(this, gameTicksTime);

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
						playerState.get()->beginKicking(mapState.get(), gameTicksTime);
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
//begin a radio tower animation for the various states
void GameState::startRadioTowerAnimation(int ticksTime) {
	float playerX = playerState.get()->getRenderCenterWorldX(ticksTime);
	float playerY = playerState.get()->getRenderCenterWorldY(ticksTime);
	float radioTowerAntennaX = MapState::antennaCenterWorldX();
	float radioTowerAntennaY = MapState::antennaCenterWorldY();
	EntityAnimation::SetVelocity* stopMoving = newEntityAnimationSetVelocity(
		newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));

	//build the animation until right before the radio waves animation
	vector<ReferenceCounterHolder<EntityAnimation::Component>> dynamicCameraAnchorEntityAnimationComponents ({
		newEntityAnimationSetPosition(playerX, playerY) COMMA
		stopMoving COMMA
		newEntityAnimationDelay(radioTowerInitialPauseAnimationTicks) COMMA
		EntityAnimation::SetVelocity::cubicInterpolation(
			radioTowerAntennaX - playerX, radioTowerAntennaY - playerY, (float)playerToRadioTowerAnimationTicks) COMMA
		newEntityAnimationDelay(playerToRadioTowerAnimationTicks) COMMA
		stopMoving COMMA
		newEntityAnimationDelay(preRadioWavesAnimationTicks)
	});

	//add the radio waves animation
	int ticksUntilStartOfRadioWavesAnimation =
		EntityAnimation::getComponentTotalTicksDuration(dynamicCameraAnchorEntityAnimationComponents);
	mapState.get()->startRadioWavesAnimation(ticksUntilStartOfRadioWavesAnimation, ticksTime);
	int radioWavesAnimationTicksDuration =
		mapState.get()->getRadioWavesAnimationTicksDuration() - ticksUntilStartOfRadioWavesAnimation;

	//build the animation from after the radio waves animation until right before the switches animation
	dynamicCameraAnchorEntityAnimationComponents.insert(
		dynamicCameraAnchorEntityAnimationComponents.end(),
		{
			newEntityAnimationDelay(radioWavesAnimationTicksDuration + postRadioWavesAnimationTicks) COMMA
			EntityAnimation::SetVelocity::cubicInterpolation(
				squareSwitchesAnimationCenterWorldX - radioTowerAntennaX,
				squareSwitchesAnimationCenterWorldY - radioTowerAntennaY,
				(float)radioTowerToSwitchesAnimationTicks) COMMA
			newEntityAnimationDelay(radioTowerToSwitchesAnimationTicks) COMMA
			stopMoving COMMA
			newEntityAnimationDelay(preSwitchesFadeInAnimationTicks)
	});

	//add the switches animation
	int ticksUntilStartOfSwitchesAnimation =
		EntityAnimation::getComponentTotalTicksDuration(dynamicCameraAnchorEntityAnimationComponents);
	mapState.get()->startSwitchesFadeInAnimation(ticksTime + ticksUntilStartOfSwitchesAnimation);

	//finish the animation by returning to the player
	dynamicCameraAnchorEntityAnimationComponents.insert(
		dynamicCameraAnchorEntityAnimationComponents.end(),
		{
			newEntityAnimationDelay(MapState::switchesFadeInDuration) COMMA
			newEntityAnimationDelay(postSwitchesFadeInAnimationTicks) COMMA
			EntityAnimation::SetVelocity::cubicInterpolation(
				playerX - squareSwitchesAnimationCenterWorldX,
				playerY - squareSwitchesAnimationCenterWorldY,
				(float)switchesToPlayerAnimationTicks) COMMA
			newEntityAnimationDelay(switchesToPlayerAnimationTicks) COMMA
			stopMoving COMMA
			newEntityAnimationSwitchToPlayerCamera()
	});
	dynamicCameraAnchor.get()->beginEntityAnimation(
		newEntityAnimation(ticksTime, dynamicCameraAnchorEntityAnimationComponents), ticksTime);

	//delay the player for the duration of the animation
	int remainingKickingAimationTicksDuration =
		SpriteRegistry::playerKickingAnimation->getTotalTicksDuration()
			- SpriteRegistry::playerKickingAnimationTicksPerFrame;
	playerState.get()->beginEntityAnimation(
		newEntityAnimation(
			ticksTime,
			{
				newEntityAnimationDelay(remainingKickingAimationTicksDuration) COMMA
				newEntityAnimationSetSpriteAnimation(nullptr) COMMA
				newEntityAnimationDelay(
					EntityAnimation::getComponentTotalTicksDuration(dynamicCameraAnchorEntityAnimationComponents)
						- remainingKickingAimationTicksDuration)
			}),
		ticksTime);
}
//render this state, which was deemed to be the last state to need rendering
void GameState::render(int ticksTime) {
	int gameTicksTime = (pauseState.get() != nullptr ? pauseStartTicksTime : ticksTime) - gameTimeOffsetTicksDuration;
	int playerZ = playerState.get()->getZ();
	mapState.get()->render(camera, playerZ, gameTicksTime);
	playerState.get()->render(camera, gameTicksTime);
	mapState.get()->renderRailsAbovePlayer(camera, playerZ, gameTicksTime);

	if (camera == dynamicCameraAnchor.get())
		dynamicCameraAnchor.get()->render(gameTicksTime);
	renderTitleAnimation(gameTicksTime);

	//TODO: real win condition
	if (playerState.get()->getZ() == 6
		&& playerState.get()->getRenderCenterWorldX(gameTicksTime)
			<= (float)((31 + MapState::firstLevelTileOffsetX) * MapState::tileSize))
	{
		const char* win = "Win!";
		Text::Metrics winMetrics = Text::getMetrics(win, 2.0f);
		Text::render(win, 10.0f, 10.0f + winMetrics.aboveBaseline, 2.0f);
	}

	if (pauseState.get() != nullptr)
		pauseState.get()->render();
	#ifdef EDITOR
		Editor::render(camera, gameTicksTime);
	#endif
}
//render the title animation at the given time
void GameState::renderTitleAnimation(int gameTicksTime) {
	switch (titleAnimation) {
		case TitleAnimation::Intro: {
			SpriteSheet::renderFilledRectangle(
				0.0f, 0.0f, 0.0f, 1.0f, 0, 0, (GLint)Config::gameScreenWidth, (GLint)Config::gameScreenHeight);
			int titleAnimationTicksTime = gameTicksTime - titleAnimationStartTicksTime;
			GLfloat titleAlpha;
			if (titleAnimationTicksTime < introTitleFadeInTicksTime)
				titleAlpha = 0.0f;
			else if (titleAnimationTicksTime < introTitleDisplayTicksTime)
				titleAlpha =
					(GLfloat)(titleAnimationTicksTime - introTitleFadeInTicksTime) / (GLfloat)introTitleFadeInTicksDuration;
			else if (titleAnimationTicksTime < introTitleFadeOutTicksTime)
				titleAlpha = 1.0f;
			else if (titleAnimationTicksTime < introPostTitleTicksTime)
				titleAlpha =
					(GLfloat)(introPostTitleTicksTime - titleAnimationTicksTime) / (GLfloat)introTitleFadeOutTicksDuration;
			else if (titleAnimationTicksTime < introAnimationStartTicksTime)
				titleAlpha = 0.0f;
			else {
				titleAnimation = TitleAnimation::None;
				return;
			}

			Text::Metrics gameNameMetrics = Text::getMetrics(titleGameName, 2.0f);
			Text::Metrics spacerMetrics = Text::getMetrics(" ", 1.0f);
			Text::Metrics creditsLine1Metrics = Text::getMetrics(titleCreditsLine1, 1.0f);
			Text::Metrics creditsLine2Metrics = Text::getMetrics(titleCreditsLine2, 1.0f);
			float totalHeight =
				gameNameMetrics.getTotalHeight()
					+ spacerMetrics.getTotalHeight()
					+ creditsLine1Metrics.getTotalHeight()
					+ creditsLine2Metrics.getTotalHeight()
					- gameNameMetrics.topPadding
					- creditsLine2Metrics.bottomPadding;;

			glColor4f(1.0f, 1.0f, 1.0f, titleAlpha);
			float screenCenterX = (float)Config::gameScreenWidth * 0.5f;
			float gameNameBaseline = ((float)Config::gameScreenHeight - totalHeight) * 0.5f + gameNameMetrics.aboveBaseline;
			Text::render(titleGameName, screenCenterX - gameNameMetrics.charactersWidth * 0.5f, gameNameBaseline, 2.0f);
			float creditsLine1Baseline =
				gameNameBaseline
					+ spacerMetrics.getBaselineDistanceBelow(&gameNameMetrics)
					+ creditsLine1Metrics.getBaselineDistanceBelow(&spacerMetrics);
			Text::render(
				titleCreditsLine1, screenCenterX - creditsLine1Metrics.charactersWidth * 0.5f, creditsLine1Baseline, 1.0f);
			float creditsLine2Baseline =
				creditsLine1Baseline + creditsLine2Metrics.getBaselineDistanceBelow(&creditsLine1Metrics);
			Text::render(
				titleCreditsLine2, screenCenterX - creditsLine2Metrics.charactersWidth * 0.5f, creditsLine2Baseline, 1.0f);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			return;
		}
		case TitleAnimation::Outro:
			//TODO: outro animation
			return;
		case TitleAnimation::None:
		default:
			return;
	}
}
//save the state to a file
void GameState::saveState() {
	ofstream file;
	file.open(savedGameFileName);
	if (sawIntroAnimation)
		file << sawIntroAnimationFilePrefix << "true\n";
	playerState.get()->saveState(file);
	mapState.get()->saveState(file);
	file.close();
}
//initialize our state and load the state from the save file if there is one
void GameState::loadInitialState(int ticksTime) {
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
			playerState.get()->loadState(line) || mapState.get()->loadState(line);
	}
	file.close();

	#ifdef EDITOR
		//always skip the intro animation for the editor, this may stick the player in the top-left corner (0, 0)
		sawIntroAnimation = true;
	#endif

	//and finally, setup the initial state
	if (sawIntroAnimation) {
		playerState.get()->obtainBoot();
		camera = playerState.get();
	} else
		beginIntroAnimation(ticksTime);
}
//give the camera and player their intro animations
void GameState::beginIntroAnimation(int ticksTime) {
	MapState::setIntroAnimationBootTile(true);
	camera = dynamicCameraAnchor.get();

	titleAnimation = TitleAnimation::Intro;
	titleAnimationStartTicksTime = ticksTime;

	//player animation component helpers
	float speedPerTick = PlayerState::speedPerSecond / (float)Config::ticksPerSecond;
	EntityAnimation::SetVelocity* stopMoving = newEntityAnimationSetVelocity(
		newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
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
		ticksTime,
		{
			newEntityAnimationSetPosition(
				PlayerState::introAnimationPlayerCenterX, PlayerState::introAnimationPlayerCenterY) COMMA
			newEntityAnimationDelay(introAnimationStartTicksTime) COMMA
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
			//put on the boot, moving the arbitrary distance needed to get to the right position
			newEntityAnimationSetVelocity(
				newCompositeQuarticValue(0.0f, -17.0f / (float)Config::ticksPerSecond, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 4.0f / (float)Config::ticksPerSecond, 0.0f, 0.0f, 0.0f)) COMMA
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
			newCompositeQuarticValue(1.0f, 0.0f, 0.0f, 0.0f, 0.0f)) COMMA
		newEntityAnimationDelay(introAnimationStartTicksTime) COMMA
		newEntityAnimationSetScreenOverlayColor(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(1.0f, -0.875f / (float)Config::ticksPerSecond, 0.0f, 0.0f, 0.0f)) COMMA
		newEntityAnimationDelay(1000) COMMA
		newEntityAnimationSetScreenOverlayColor(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.125f, 0.0f, 0.0f, 0.0f, 0.0f))
	});

	//delay the camera animation until the end of the player animation minus the leg lift animation duration
	int legLiftAnimationTicksDuration = SpriteRegistry::playerLegLiftAnimation->getTotalTicksDuration();
	cameraAnimationComponents.insert(
		cameraAnimationComponents.end(),
		{
			newEntityAnimationDelay(
				playerEntityAnimation->getTotalTicksDuration()
					- EntityAnimation::getComponentTotalTicksDuration(cameraAnimationComponents)
					- legLiftAnimationTicksDuration) COMMA
			//fade in the screen and then switch to the player camera
			newEntityAnimationSetScreenOverlayColor(
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.125f, -0.125f / (float)legLiftAnimationTicksDuration, 0.0f, 0.0f, 0.0f)) COMMA
			newEntityAnimationDelay(legLiftAnimationTicksDuration) COMMA
			newEntityAnimationSetScreenOverlayColor(
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)) COMMA
			newEntityAnimationSwitchToPlayerCamera()
		});

	playerState.get()->beginEntityAnimation(playerEntityAnimation, ticksTime);
	dynamicCameraAnchor.get()->beginEntityAnimation(newEntityAnimation(ticksTime, cameraAnimationComponents), ticksTime);
}
//reset all state
void GameState::resetGame(int ticksTime) {
	sawIntroAnimation = false;
	gameTimeOffsetTicksDuration = 0;
	pauseState.set(nullptr);
	playerState.set(newPlayerState());
	mapState.set(newMapState());
	dynamicCameraAnchor.set(newDynamicCameraAnchor());

	mapState.get()->resetMap();
	beginIntroAnimation(ticksTime);
}
