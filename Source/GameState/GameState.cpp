#include "GameState.h"
#ifdef EDITOR
	#include "Editor/Editor.h"
#endif
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/PauseState.h"
#include "GameState/PlayerState.h"
#include "GameState/MapState/MapState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Sprites/Text.h"
#include "Util/Config.h"
#include "Util/FileUtils.h"
#include "Util/Logger.h"
#include "Util/StringUtils.h"

const float GameState::squareSwitchesAnimationCenterWorldX =
	280.0f + (float)(MapState::firstLevelTileOffsetX * MapState::tileSize);
const float GameState::squareSwitchesAnimationCenterWorldY =
	80.0f + (float)(MapState::firstLevelTileOffsetY * MapState::tileSize);
const float GameState::triangleSwitchesAnimationCenterWorldX = 100.0f; //todo: pick a position
const float GameState::triangleSwitchesAnimationCenterWorldY = 100.0f; //todo: pick a position
const float GameState::sawSwitchesAnimationCenterWorldX = 100.0f; //todo: pick a position
const float GameState::sawSwitchesAnimationCenterWorldY = 100.0f; //todo: pick a position
const float GameState::sineSwitchesAnimationCenterWorldX = 100.0f; //todo: pick a position
const float GameState::sineSwitchesAnimationCenterWorldY = 100.0f; //todo: pick a position
const char* GameState::titleGameName = "Flick Yourself On";
const char* GameState::titleCreditsLine1 = "A game by";
const char* GameState::titleCreditsLine2 = "Gregory Loden";
const char* GameState::titlePostCreditsMessage = "Thanks for playing!";
#ifdef DEBUG
	const char* GameState::replayFileName = "fyo_replay.log";
#endif
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
	mapState.set(nullptr);
	playerState.set(nullptr);
	dynamicCameraAnchor.set(nullptr);
	pauseState.set(nullptr);

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
		mapState.set(prev->mapState.get());
		playerState.set(prev->playerState.get());
		dynamicCameraAnchor.set(prev->dynamicCameraAnchor.get());
		camera = prev->camera;

		if (nextPauseState != nullptr) {
			int endPauseDecision = nextPauseState->getEndPauseDecision();
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::Save) != 0) {
				Logger::gameplayLogger.log("  save state");
				saveState();
			}
			//don't reset the game in editor mode
			#ifndef EDITOR
				if ((endPauseDecision & (int)PauseState::EndPauseDecision::Reset) != 0) {
					Logger::gameplayLogger.log("  reset game");
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

	//we're not paused, find what time it really is
	gameTimeOffsetTicksDuration = prev->gameTimeOffsetTicksDuration;
	int gameTicksTime = ticksTime - gameTimeOffsetTicksDuration;

	//update states
	mapState.set(newMapState());
	mapState.get()->updateWithPreviousMapState(prev->mapState.get(), gameTicksTime);
	playerState.set(newPlayerState(mapState.get()));
	playerState.get()->updateWithPreviousPlayerState(prev->playerState.get(), gameTicksTime);
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
					Editor::handleClick(gameEvent.button, false, camera, gameTicksTime);
					break;
				case SDL_MOUSEMOTION:
					if ((SDL_GetMouseState(nullptr, nullptr) & (SDL_BUTTON_LMASK | SDL_BUTTON_MMASK | SDL_BUTTON_RMASK)) != 0)
						Editor::handleClick(gameEvent.button, true, camera, gameTicksTime);
					break;
			#endif
			default:
				break;
		}
	}

	#ifdef EDITOR
		//if we saved the floor file, the editor requests that we save the game too, since rail/switch ids may have changed
		if (Editor::needsGameStateSave) {
			saveState();
			Editor::needsGameStateSave = false;
		}
	#endif
}
//set our camera to our player
void GameState::setPlayerCamera() {
	camera = playerState.get();

	//the intro animation happens to call this when it ends, so we'll do cleanup here
	if (!sawIntroAnimation) {
		sawIntroAnimation = true;
		MapState::setIntroAnimationBootTile(false);
		playerState.get()->setInitialZ();
	}
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
	float switchesAnimationCenterWorldX = squareSwitchesAnimationCenterWorldX;
	float switchesAnimationCenterWorldY = squareSwitchesAnimationCenterWorldY;
	switch (mapState.get()->getLastActivatedSwitchColor()) {
		case MapState::triangleColor:
			switchesAnimationCenterWorldX = triangleSwitchesAnimationCenterWorldX;
			switchesAnimationCenterWorldY = triangleSwitchesAnimationCenterWorldY;
			break;
		case MapState::sawColor:
			switchesAnimationCenterWorldX = sawSwitchesAnimationCenterWorldX;
			switchesAnimationCenterWorldY = sawSwitchesAnimationCenterWorldY;
			break;
		case MapState::sineColor:
			switchesAnimationCenterWorldX = sineSwitchesAnimationCenterWorldX;
			switchesAnimationCenterWorldY = sineSwitchesAnimationCenterWorldY;
			break;
		default:
			break;
	}

	//build the animation until right before the radio waves animation
	vector<ReferenceCounterHolder<EntityAnimation::Component>> dynamicCameraAnchorAnimationComponents ({
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
		EntityAnimation::getComponentTotalTicksDuration(dynamicCameraAnchorAnimationComponents);
	mapState.get()->startRadioWavesAnimation(ticksUntilStartOfRadioWavesAnimation, ticksTime);
	int radioWavesAnimationTicksDuration =
		mapState.get()->getRadioWavesAnimationTicksDuration() - ticksUntilStartOfRadioWavesAnimation;

	//build the animation from after the radio waves animation until right before the switches animation
	dynamicCameraAnchorAnimationComponents.insert(
		dynamicCameraAnchorAnimationComponents.end(),
		{
			newEntityAnimationDelay(radioWavesAnimationTicksDuration + postRadioWavesAnimationTicks) COMMA
			EntityAnimation::SetVelocity::cubicInterpolation(
				switchesAnimationCenterWorldX - radioTowerAntennaX,
				switchesAnimationCenterWorldY - radioTowerAntennaY,
				(float)radioTowerToSwitchesAnimationTicks) COMMA
			newEntityAnimationDelay(radioTowerToSwitchesAnimationTicks) COMMA
			stopMoving COMMA
			newEntityAnimationDelay(preSwitchesFadeInAnimationTicks)
	});

	//add the switches animation
	int ticksUntilStartOfSwitchesAnimation =
		EntityAnimation::getComponentTotalTicksDuration(dynamicCameraAnchorAnimationComponents);
	mapState.get()->startSwitchesFadeInAnimation(ticksTime + ticksUntilStartOfSwitchesAnimation);

	//finish the animation by returning to the player
	dynamicCameraAnchorAnimationComponents.insert(
		dynamicCameraAnchorAnimationComponents.end(),
		{
			newEntityAnimationDelay(MapState::switchesFadeInDuration) COMMA
			newEntityAnimationDelay(postSwitchesFadeInAnimationTicks) COMMA
			EntityAnimation::SetVelocity::cubicInterpolation(
				playerX - switchesAnimationCenterWorldX,
				playerY - switchesAnimationCenterWorldY,
				(float)switchesToPlayerAnimationTicks) COMMA
			newEntityAnimationDelay(switchesToPlayerAnimationTicks) COMMA
			stopMoving COMMA
			newEntityAnimationSwitchToPlayerCamera()
	});
	Holder_EntityAnimationComponentVector dynamicCameraAnchorAnimationComponentsHolder (
		&dynamicCameraAnchorAnimationComponents);
	dynamicCameraAnchor.get()->beginEntityAnimation(&dynamicCameraAnchorAnimationComponentsHolder, ticksTime);

	//delay the player for the duration of the animation
	int remainingKickingAimationTicksDuration =
		SpriteRegistry::playerKickingAnimation->getTotalTicksDuration()
			- SpriteRegistry::playerKickingAnimationTicksPerFrame;
	vector<ReferenceCounterHolder<EntityAnimation::Component>> playerAnimationComponents ({
		newEntityAnimationDelay(remainingKickingAimationTicksDuration) COMMA
		newEntityAnimationSetSpriteAnimation(nullptr) COMMA
		newEntityAnimationDelay(
			EntityAnimation::getComponentTotalTicksDuration(dynamicCameraAnchorAnimationComponents)
				- remainingKickingAimationTicksDuration)
	});
	Holder_EntityAnimationComponentVector playerAnimationComponentsHolder (&playerAnimationComponents);
	playerState.get()->beginEntityAnimation(&playerAnimationComponentsHolder, ticksTime);
}
//render this state, which was deemed to be the last state to need rendering
void GameState::render(int ticksTime) {
	int gameTicksTime = (pauseState.get() != nullptr ? pauseStartTicksTime : ticksTime) - gameTimeOffsetTicksDuration;
	bool showConnections = SDL_GetKeyboardState(nullptr)[Config::keyBindings.showConnectionsKey] != 0;
	int playerZ = playerState.get()->getZ();
	mapState.get()->render(camera, playerZ, showConnections, gameTicksTime);
	playerState.get()->render(camera, gameTicksTime);
	mapState.get()->renderRailsAbovePlayer(camera, playerZ, showConnections, gameTicksTime);

	if (camera == dynamicCameraAnchor.get())
		dynamicCameraAnchor.get()->render(gameTicksTime);
	renderTitleAnimation(gameTicksTime);

	//TODO: real win condition
	float px = playerState.get()->getRenderCenterWorldX(gameTicksTime);
	float py = playerState.get()->getRenderCenterWorldY(gameTicksTime);
	if (playerState.get()->getZ() == 6 && px >= 260 && px <= 270 && py >= 240 && py <= 255) {
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
					- creditsLine2Metrics.bottomPadding;

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
	FileUtils::openFileForWrite(&file, savedGameFileName, ios::out | ios::trunc);
	if (sawIntroAnimation)
		file << sawIntroAnimationFilePrefix << "true\n";
	playerState.get()->saveState(file);
	mapState.get()->saveState(file);
	file.close();
}
//initialize our state and load the state from the save file if there is one
void GameState::loadInitialState(int ticksTime) {
	//first things first, we need some state
	mapState.set(newMapState());
	playerState.set(newPlayerState(mapState.get()));
	dynamicCameraAnchor.set(newDynamicCameraAnchor());

	#ifdef DEBUG
		//before loading the game, see if we have a replay, and stop if we do
		if (loadReplay())
			return;
	#endif

	//we have no replay, load state if we can
	ifstream file;
	FileUtils::openFileForRead(&file, savedGameFileName);
	string line;
	bool loadedState = false;
	while (getline(file, line)) {
		if (StringUtils::startsWith(line, sawIntroAnimationFilePrefix))
			sawIntroAnimation = strcmp(line.c_str() + sawIntroAnimationFilePrefix.size(), "true") == 0;
		else if (playerState.get()->loadState(line) || mapState.get()->loadState(line))
			;
		else
			continue;
		loadedState = true;
	}
	file.close();
	if (loadedState)
		Logger::gameplayLogger.log("  load state");

	//and finally, setup any remaining initial state
	#ifdef EDITOR
		playerState.get()->setHighestZ();
	#else
		playerState.get()->setInitialZ();
	#endif
	mapState.get()->sortInitialRails();

	#ifdef EDITOR
		//always skip the intro animation for the editor, jump straight into walking
		sawIntroAnimation = true;
	#endif
	if (sawIntroAnimation) {
		playerState.get()->obtainBoot();
		camera = playerState.get();
	} else
		beginIntroAnimation(ticksTime);
}
#ifdef DEBUG
	//load a replay and finish initialization if a replay is available, return whether it was
	bool GameState::loadReplay() {
		ifstream file;
		FileUtils::openFileForRead(&file, replayFileName);
		string line;
		bool beganGameplay = false;
		int lastTimestamp = 0;
		float lastX = 0.0f;
		float lastY = 0.0f;
		vector<ReferenceCounterHolder<EntityAnimation::Component>> replayComponents;
		Holder_EntityAnimationComponentVector replayComponentsHolder (&replayComponents);
		while (getline(file, line)) {
			const char* logMessage = line.c_str();
			int timestamp = StringUtils::parseLogFileTimestamp(logMessage);
			logMessage = logMessage + 13;
			string logMessageString (logMessage);
			if (!beganGameplay) {
				if (!StringUtils::startsWith(logMessageString, "---- begin gameplay ----"))
					continue;

				beganGameplay = true;
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetPosition(
							PlayerState::introAnimationPlayerCenterX, PlayerState::introAnimationPlayerCenterY) COMMA
						newEntityAnimationDelay(timestamp)
					});
				lastTimestamp = timestamp;
				lastX = PlayerState::introAnimationPlayerCenterX;
				lastY = PlayerState::introAnimationPlayerCenterY;
				continue;
			} else if (strcmp(logMessage, "----   end gameplay ----") == 0)
				break;

			if (StringUtils::startsWith(logMessageString, "  reset game"))
				break;
			else if (StringUtils::startsWith(logMessageString, "  climb ")) {
				logMessage = logMessage + 8;
				int climbStartX;
				int climbStartY;
				StringUtils::parsePosition(logMessage, &climbStartX, &climbStartY);
				addMoveWithGhost(
					&replayComponentsHolder, lastX, lastY, (float)climbStartX, (float)climbStartY, timestamp - lastTimestamp);
				lastX = (float)climbStartX;
				lastY = (float)climbStartY - 12.0f;
				int climbDuration = SpriteRegistry::playerKickingAnimation->getTotalTicksDuration();
				replayComponents.push_back(newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerKickingAnimation));
				addMoveWithGhost(&replayComponentsHolder, (float)climbStartX, (float)climbStartY, lastX, lastY, climbDuration);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f) COMMA
						newEntityAnimationSetVelocity(
							newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
							newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)) COMMA
						newEntityAnimationSetSpriteAnimation(nullptr)
					});
				lastTimestamp = timestamp + climbDuration;
			} else if (StringUtils::startsWith(logMessageString, "  fall ")) {
				logMessage = logMessage + 7;
				int fallStartX;
				int fallStartY;
				StringUtils::parsePosition(logMessage, &fallStartX, &fallStartY);
				addMoveWithGhost(
					&replayComponentsHolder, lastX, lastY, (float)fallStartX, (float)fallStartY, timestamp - lastTimestamp);
				lastX = (float)fallStartX;
				lastY = (float)fallStartY + 6.0f;
				int fallDuration = SpriteRegistry::playerKickingAnimation->getTotalTicksDuration();
				replayComponents.push_back(newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerKickingAnimation));
				addMoveWithGhost(&replayComponentsHolder, (float)fallStartX, (float)fallStartY, lastX, lastY, fallDuration);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f) COMMA
						newEntityAnimationSetVelocity(
							newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
							newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)) COMMA
						newEntityAnimationSetSpriteAnimation(nullptr)
					});
				lastTimestamp = timestamp + fallDuration;
			} else if (StringUtils::startsWith(logMessageString, "  rail ")) {
				logMessage = logMessage + 7;
				int railIndex;
				logMessage = logMessage + StringUtils::parseNextInt(logMessage, &railIndex);
				logMessage = logMessage + StringUtils::nonDigitPrefixLength(logMessage);
				int startX;
				int startY;
				StringUtils::parsePosition(logMessage, &startX, &startY);
				addMoveWithGhost(
					&replayComponentsHolder, lastX, lastY, (float)startX, (float)startY, timestamp - lastTimestamp);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f) COMMA
						newEntityAnimationSetVelocity(
							newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
							newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f))
					});
				PlayerState::addNearestRailRideComponents(
					railIndex, &replayComponentsHolder, (float)startX, (float)startY, &lastX, &lastY);
				replayComponents.push_back(newEntityAnimationSetSpriteAnimation(nullptr));
				lastTimestamp = EntityAnimation::getComponentTotalTicksDuration(replayComponents);
			} else if (StringUtils::startsWith(logMessageString, "  switch ")) {
				logMessage = logMessage + 9;
				int switchIndex;
				StringUtils::parseNextInt(logMessage, &switchIndex);
				int switchMapLeftX = 0;
				int switchMapTopY = 0;
				MapState::getSwitchMapTopLeft(switchIndex, &switchMapLeftX, &switchMapTopY);
				float moveX = (float)((switchMapLeftX + 1) * MapState::tileSize);
				float moveY = (float)((switchMapTopY + 1) * MapState::tileSize);
				addMoveWithGhost(&replayComponentsHolder, lastX, lastY, moveX, moveY, timestamp - lastTimestamp);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f) COMMA
						newEntityAnimationSetSpriteDirection(SpriteDirection::Down)
					});
				PlayerState::addKickSwitchComponents(
					MapState::getIdFromSwitchIndex(switchIndex), &replayComponentsHolder, false);
				replayComponents.push_back(newEntityAnimationSetSpriteAnimation(nullptr));
				lastX = moveX;
				lastY = moveY;
				lastTimestamp = EntityAnimation::getComponentTotalTicksDuration(replayComponents);
			} else if (StringUtils::startsWith(logMessageString, "  reset switch ")) {
				logMessage = logMessage + 15;
				int resetSwitchIndex;
				StringUtils::parseNextInt(logMessage, &resetSwitchIndex);
				int resetSwitchMapCenterX = 0;
				int resetSwitchMapTopY = 0;
				MapState::getResetSwitchMapTopCenter(resetSwitchIndex, &resetSwitchMapCenterX, &resetSwitchMapTopY);
				float moveX = ((float)resetSwitchMapCenterX + 0.5f) * (float)MapState::tileSize;
				float moveY = (float)(resetSwitchMapTopY * MapState::tileSize);
				addMoveWithGhost(&replayComponentsHolder, lastX, lastY, moveX, moveY, timestamp - lastTimestamp);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f) COMMA
						newEntityAnimationSetSpriteDirection(SpriteDirection::Down)
					});
				PlayerState::addKickResetSwitchComponents(
					MapState::getIdFromResetSwitchIndex(resetSwitchIndex), &replayComponentsHolder);
				replayComponents.push_back(newEntityAnimationSetSpriteAnimation(nullptr));
				lastX = moveX;
				lastY = moveY;
				lastTimestamp = EntityAnimation::getComponentTotalTicksDuration(replayComponents);
			}
		}
		file.close();

		if (replayComponents.empty())
			return false;

		playerState.get()->beginEntityAnimation(&replayComponentsHolder, 0);
		playerState.get()->setHighestZ();
		playerState.get()->obtainBoot();
		camera = playerState.get();
		sawIntroAnimation = true;
		mapState.get()->sortInitialRails();
		return true;
	}
	//move from one position to another, showing a ghost sprite at the end position
	void GameState::addMoveWithGhost(
		Holder_EntityAnimationComponentVector* replayComponentsHolder,
		float startX,
		float startY,
		float endX,
		float endY,
		int ticksDuration)
	{
		vector<ReferenceCounterHolder<EntityAnimation::Component>>* replayComponents = replayComponentsHolder->val;
		float moveX = (endX - startX) / (float)ticksDuration;
		float moveY = (endY - startY) / (float)ticksDuration;
		SpriteDirection spriteDirection = EntityState::getSpriteDirection(moveX, moveY);
		replayComponents->insert(
			replayComponents->end(),
			{
				newEntityAnimationSetGhostSprite(true, endX, endY) COMMA
				newEntityAnimationSetVelocity(
					newCompositeQuarticValue(0.0f, moveX, 0.0f, 0.0f, 0.0f),
					newCompositeQuarticValue(0.0f, moveY, 0.0f, 0.0f, 0.0f)) COMMA
				newEntityAnimationSetSpriteDirection(spriteDirection) COMMA
				newEntityAnimationDelay(ticksDuration)
			});
	}
#endif
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

	vector<ReferenceCounterHolder<EntityAnimation::Component>> playerAnimationComponents ({
		stopMoving COMMA
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
	Holder_EntityAnimationComponentVector playerAnimationComponentsHolder (&playerAnimationComponents);
	playerState.get()->beginEntityAnimation(&playerAnimationComponentsHolder, ticksTime);

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
				EntityAnimation::getComponentTotalTicksDuration(playerAnimationComponents)
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
	Holder_EntityAnimationComponentVector cameraAnimationComponentsHolder (&cameraAnimationComponents);
	dynamicCameraAnchor.get()->beginEntityAnimation(&cameraAnimationComponentsHolder, ticksTime);
}
//reset all state
void GameState::resetGame(int ticksTime) {
	sawIntroAnimation = false;
	gameTimeOffsetTicksDuration = 0;
	pauseState.set(nullptr);
	mapState.set(newMapState());
	mapState.get()->resetMap();
	PlayerState* resetPlayerState = newPlayerState(mapState.get());
	resetPlayerState->copyPlayerState(playerState.get());
	playerState.set(resetPlayerState);
	dynamicCameraAnchor.set(newDynamicCameraAnchor());

	beginIntroAnimation(ticksTime);
}
