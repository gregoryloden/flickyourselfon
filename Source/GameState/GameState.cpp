#include "GameState.h"
#include "Editor/Editor.h"
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
const char* GameState::titleGameName = "Kick Yourself On";
const char* GameState::titleCreditsLine1 = "A game by";
const char* GameState::titleCreditsLine2 = "Gregory Loden";
const char* GameState::titlePostCreditsMessage = "Thanks for playing!";
const char* GameState::bootExplanationMessage1 = "You are";
const char* GameState::bootExplanationMessage2 = "a boot.";
const char* GameState::radioTowerExplanationMessageLine1 = "Your local radio tower";
const char* GameState::radioTowerExplanationMessageLine2 = "lost connection";
const char* GameState::radioTowerExplanationMessageLine3 = "with its";
const char* GameState::radioTowerExplanationMessageLine4 = "master transmitter relay.";
const char* GameState::goalExplanationMessageLine1 = "Can you";
const char* GameState::goalExplanationMessageLine2 = "guide this person";
const char* GameState::goalExplanationMessageLine3 = "to turn it on?";
#ifdef DEBUG
	const char* GameState::replayFileName = "kyo_replay.log";
#endif
const char* GameState::savedGameFileName = "kyo.sav";
const string GameState::sawIntroAnimationFilePrefix = "sawIntroAnimation ";
GameState::GameState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
sawIntroAnimation(false)
, textDisplayType(TextDisplayType::None)
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
void GameState::updateWithPreviousGameState(GameState* prev, int ticksTime) {
	//first things first: dump all our previous state so that we can start fresh
	mapState.set(nullptr);
	playerState.set(nullptr);
	dynamicCameraAnchor.set(nullptr);
	pauseState.set(nullptr);

	//copy values that don't usually change from state to state
	sawIntroAnimation = prev->sawIntroAnimation;
	textDisplayType = prev->textDisplayType;
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
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::Reset) != 0 && !Editor::isActive) {
				Logger::gameplayLogger.log("  reset game");
				resetGame(ticksTime);
				//don't check for Exit, just return here
				return;
			}
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
				if (gameEvent.key.keysym.scancode == Config::keyBindings.kickKey && !Editor::isActive) {
					playerState.get()->beginKicking(gameTicksTime);
					break;
				}
				if (gameEvent.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					pauseState.set(newBasePauseState());
					pauseStartTicksTime = ticksTime;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (Editor::isActive)
					Editor::handleClick(gameEvent.button, false, camera, gameTicksTime);
				break;
			case SDL_MOUSEMOTION:
				if (Editor::isActive
						&& (SDL_GetMouseState(nullptr, nullptr) & (SDL_BUTTON_LMASK | SDL_BUTTON_MMASK | SDL_BUTTON_RMASK))
							!= 0)
					Editor::handleClick(gameEvent.button, true, camera, gameTicksTime);
				break;
			default:
				break;
		}
	}

	//if we saved the floor file, the editor requests that we save the game too, since rail/switch ids may have changed
	if (Editor::needsGameStateSave) {
		saveState();
		Editor::needsGameStateSave = false;
	}
}
void GameState::setPlayerCamera() {
	camera = playerState.get();

	//the intro animation happens to call this when it ends, so we'll do cleanup here
	if (!sawIntroAnimation) {
		sawIntroAnimation = true;
		MapState::setIntroAnimationBootTile(false);
		playerState.get()->setInitialZ();
	}
}
void GameState::setDynamicCamera() {
	camera = dynamicCameraAnchor.get();
}
void GameState::startRadioTowerAnimation(int ticksTime) {
	float playerX = playerState.get()->getRenderCenterWorldX(ticksTime);
	float playerY = playerState.get()->getRenderCenterWorldY(ticksTime);
	float radioTowerAntennaX = MapState::antennaCenterWorldX();
	float radioTowerAntennaY = MapState::antennaCenterWorldY();
	EntityAnimation::SetVelocity* stopMoving = newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f));
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
		newEntityAnimationSetPosition(playerX, playerY),
		stopMoving,
		newEntityAnimationDelay(radioTowerInitialPauseAnimationTicks),
		EntityAnimation::SetVelocity::cubicInterpolation(
			radioTowerAntennaX - playerX, radioTowerAntennaY - playerY, (float)playerToRadioTowerAnimationTicks),
		newEntityAnimationDelay(playerToRadioTowerAnimationTicks),
		stopMoving,
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
			newEntityAnimationDelay(radioWavesAnimationTicksDuration + postRadioWavesAnimationTicks),
			EntityAnimation::SetVelocity::cubicInterpolation(
				switchesAnimationCenterWorldX - radioTowerAntennaX,
				switchesAnimationCenterWorldY - radioTowerAntennaY,
				(float)radioTowerToSwitchesAnimationTicks),
			newEntityAnimationDelay(radioTowerToSwitchesAnimationTicks),
			stopMoving,
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
			newEntityAnimationDelay(MapState::switchesFadeInDuration),
			newEntityAnimationDelay(postSwitchesFadeInAnimationTicks),
			EntityAnimation::SetVelocity::cubicInterpolation(
				playerX - switchesAnimationCenterWorldX,
				playerY - switchesAnimationCenterWorldY,
				(float)switchesToPlayerAnimationTicks),
			newEntityAnimationDelay(switchesToPlayerAnimationTicks),
			stopMoving,
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
		newEntityAnimationDelay(remainingKickingAimationTicksDuration),
		newEntityAnimationSetSpriteAnimation(nullptr),
		newEntityAnimationDelay(
			EntityAnimation::getComponentTotalTicksDuration(dynamicCameraAnchorAnimationComponents)
				- remainingKickingAimationTicksDuration)
	});
	Holder_EntityAnimationComponentVector playerAnimationComponentsHolder (&playerAnimationComponents);
	playerState.get()->beginEntityAnimation(&playerAnimationComponentsHolder, ticksTime);
}
void GameState::render(int ticksTime) {
	Editor::EditingMutexLocker editingMutexLocker;
	int gameTicksTime = (pauseState.get() != nullptr ? pauseStartTicksTime : ticksTime) - gameTimeOffsetTicksDuration;
	bool showConnections =
		SDL_GetKeyboardState(nullptr)[Config::keyBindings.showConnectionsKey] != 0
			|| (!mapState.get()->getFinishedConnectionsTutorial() && playerState.get()->showTutorialConnectionsForKickAction());
	int playerZ = playerState.get()->getZ();
	mapState.get()->render(camera, playerZ, showConnections, gameTicksTime);
	playerState.get()->render(camera, gameTicksTime);
	mapState.get()->renderAbovePlayer(camera, showConnections, gameTicksTime);

	short kickActionResetSwitchId = playerState.get()->getKickActionResetSwitchId();
	bool hasRailsToReset = false;
	if (kickActionResetSwitchId != MapState::absentRailSwitchId)
		hasRailsToReset = mapState.get()->renderGroupsForRailsToReset(camera, kickActionResetSwitchId, gameTicksTime);
	playerState.get()->renderKickAction(camera, hasRailsToReset, gameTicksTime);

	if (camera == dynamicCameraAnchor.get())
		dynamicCameraAnchor.get()->render(gameTicksTime);
	if (textDisplayType != TextDisplayType::None)
		renderTextDisplay(gameTicksTime);

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
	if (Editor::isActive)
		Editor::render(camera, gameTicksTime);
}
void GameState::renderTextDisplay(int gameTicksTime) {
	vector<string> textDisplayStrings;
	vector<Text::Metrics> textDisplayMetrics;
	int textFadeInStartTicksTime = 0;
	int textFadeInTicksDuration = 0;
	int textDisplayTicksDuration = 0;
	int textFadeOutTicksDuration = 0;
	TextDisplayType nextTextDisplayType = TextDisplayType::None;
	switch (textDisplayType) {
		case TextDisplayType::Intro:
			textDisplayStrings = { titleGameName, " ", titleCreditsLine1, titleCreditsLine2 };
			textDisplayMetrics = { Text::getMetrics(titleGameName, 2.0f) };
			textFadeInStartTicksTime = introTitleFadeInStartTicksTime;
			textFadeInTicksDuration = introTitleFadeInTicksDuration;
			textDisplayTicksDuration = introTitleDisplayTicksDuration;
			textFadeOutTicksDuration = introTitleFadeOutTicksDuration;
			nextTextDisplayType = TextDisplayType::BootExplanation;
			break;
		case TextDisplayType::BootExplanation:
			textDisplayStrings = { bootExplanationMessage1, bootExplanationMessage2, " ", " ", " " };
			textFadeInStartTicksTime = bootExplanationFadeInStartTicksTime;
			textFadeInTicksDuration = bootExplanationFadeInTicksDuration;
			textDisplayTicksDuration = bootExplanationDisplayTicksDuration;
			textFadeOutTicksDuration = bootExplanationFadeOutTicksDuration;
			nextTextDisplayType = TextDisplayType::RadioTowerExplanation;
			break;
		case TextDisplayType::RadioTowerExplanation:
			textDisplayStrings = {
				radioTowerExplanationMessageLine1,
				radioTowerExplanationMessageLine2,
				radioTowerExplanationMessageLine3,
				radioTowerExplanationMessageLine4,
				" ",
				" ",
				" ",
				" "
			};
			textFadeInStartTicksTime = radioTowerExplanationFadeInStartTicksTime;
			textFadeInTicksDuration = radioTowerExplanationFadeInTicksDuration;
			textDisplayTicksDuration = radioTowerExplanationDisplayTicksDuration;
			textFadeOutTicksDuration = radioTowerExplanationFadeOutTicksDuration;
			nextTextDisplayType = TextDisplayType::GoalExplanation;
			break;
		case TextDisplayType::GoalExplanation:
			textDisplayStrings = {
				" ",
				" ",
				" ",
				" ",
				" ",
				" ",
				" ",
				" ",
				goalExplanationMessageLine1,
				goalExplanationMessageLine2,
				goalExplanationMessageLine3
			};
			textFadeInStartTicksTime = goalExplanationFadeInStartTicksTime;
			textFadeInTicksDuration = goalExplanationFadeInTicksDuration;
			textDisplayTicksDuration = goalExplanationDisplayTicksDuration;
			textFadeOutTicksDuration = goalExplanationFadeOutTicksDuration;
			nextTextDisplayType = TextDisplayType::None;
			break;
		case TextDisplayType::Outro:
			//TODO: outro text
			return;
		case TextDisplayType::None:
		default:
			return;
	}

	while (textDisplayMetrics.size() < textDisplayStrings.size())
		textDisplayMetrics.push_back(Text::getMetrics(textDisplayStrings[textDisplayMetrics.size()].c_str(), 1.0f));

	int textFadeInEndTicksTime = textFadeInStartTicksTime + textFadeInTicksDuration;
	int textFadeOutStartTicksTime = textFadeInEndTicksTime + textDisplayTicksDuration;
	int textFadeOutEndTicksTime = textFadeOutStartTicksTime + textFadeOutTicksDuration;
	int textDisplayTicksTime = gameTicksTime - titleAnimationStartTicksTime;
	GLfloat textDisplayAlpha;
	if (textDisplayTicksTime < textFadeInStartTicksTime)
		textDisplayAlpha = 0.0f;
	else if (textDisplayTicksTime < textFadeInEndTicksTime)
		textDisplayAlpha = (GLfloat)(textDisplayTicksTime - textFadeInStartTicksTime) / (GLfloat)textFadeInTicksDuration;
	else if (textDisplayTicksTime < textFadeOutStartTicksTime)
		textDisplayAlpha = 1.0f;
	else if (textDisplayTicksTime < textFadeOutEndTicksTime)
		textDisplayAlpha = (GLfloat)(textFadeOutEndTicksTime - textDisplayTicksTime) / (GLfloat)textFadeOutTicksDuration;
	else {
		textDisplayType = nextTextDisplayType;
		renderTextDisplay(gameTicksTime);
		return;
	}
	glColor4f(1.0f, 1.0f, 1.0f, textDisplayAlpha);
	Text::renderLines(textDisplayStrings, textDisplayMetrics);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void GameState::saveState() {
	ofstream file;
	FileUtils::openFileForWrite(&file, savedGameFileName, ios::out | ios::trunc);
	if (sawIntroAnimation)
		file << sawIntroAnimationFilePrefix << "true\n";
	playerState.get()->saveState(file);
	mapState.get()->saveState(file);
	file.close();
}
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
	if (Editor::isActive) {
		playerState.get()->setHighestZ();
		//always skip the intro animation for the editor, jump straight into walking
		sawIntroAnimation = true;
	} else
		playerState.get()->setInitialZ();
	mapState.get()->sortInitialRails();

	if (sawIntroAnimation) {
		playerState.get()->obtainBoot();
		camera = playerState.get();
	} else
		beginIntroAnimation(ticksTime);
}
#ifdef DEBUG
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
			int timestamp;
			logMessage = StringUtils::parseLogFileTimestamp(logMessage, &timestamp) + 2;
			string logMessageString (logMessage);
			if (!beganGameplay) {
				if (!StringUtils::startsWith(logMessageString, "---- begin gameplay ----"))
					continue;

				beganGameplay = true;
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetPosition(
							PlayerState::introAnimationPlayerCenterX, PlayerState::introAnimationPlayerCenterY),
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
				logMessage = StringUtils::parsePosition(logMessage, &climbStartX, &climbStartY);
				int climbEndX;
				int climbEndY;
				StringUtils::parsePosition(logMessage, &climbEndX, &climbEndY);
				addMoveWithGhost(
					&replayComponentsHolder, lastX, lastY, (float)climbStartX, (float)climbStartY, timestamp - lastTimestamp);
				//for backwards compatibility, a climb without an end position is an Up climb
				if (climbEndX == 0 || climbEndY == 0) {
					lastX = (float)climbStartX;
					lastY = (float)climbStartY - 12.0f;
				} else {
					lastX = (float)climbEndX;
					lastY = (float)climbEndY;
				}
				int climbDuration = SpriteRegistry::playerKickingAnimation->getTotalTicksDuration();
				replayComponents.push_back(newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerKickingAnimation));
				addMoveWithGhost(&replayComponentsHolder, (float)climbStartX, (float)climbStartY, lastX, lastY, climbDuration);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f),
						newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
						newEntityAnimationSetSpriteAnimation(nullptr)
					});
				lastTimestamp = timestamp + climbDuration;
			} else if (StringUtils::startsWith(logMessageString, "  fall ")) {
				logMessage = logMessage + 7;
				int fallStartX;
				int fallStartY;
				logMessage = StringUtils::parsePosition(logMessage, &fallStartX, &fallStartY);
				int fallEndX;
				int fallEndY;
				StringUtils::parsePosition(logMessage, &fallEndX, &fallEndY);
				addMoveWithGhost(
					&replayComponentsHolder, lastX, lastY, (float)fallStartX, (float)fallStartY, timestamp - lastTimestamp);
				//a fall without an end position is treated as a 1-tile Down fall regardless of where it actually went
				if (fallEndX == 0 || fallEndY == 0) {
					lastX = (float)fallStartX;
					lastY = (float)fallStartY + 6.0f;
				} else {
					lastX = (float)fallEndX;
					lastY = (float)fallEndY;
				}
				int fallDuration = SpriteRegistry::playerFastKickingAnimation->getTotalTicksDuration();
				replayComponents.push_back(newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerFastKickingAnimation));
				addMoveWithGhost(&replayComponentsHolder, (float)fallStartX, (float)fallStartY, lastX, lastY, fallDuration);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f),
						newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
						newEntityAnimationSetSpriteAnimation(nullptr)
					});
				lastTimestamp = timestamp + fallDuration;
			} else if (StringUtils::startsWith(logMessageString, "  rail ")) {
				logMessage = logMessage + 7;
				int railIndex;
				logMessage = StringUtils::parseNextInt(logMessage, &railIndex);
				int startX;
				int startY;
				StringUtils::parsePosition(logMessage, &startX, &startY);
				addMoveWithGhost(
					&replayComponentsHolder, lastX, lastY, (float)startX, (float)startY, timestamp - lastTimestamp);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f),
						newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f))
					});
				PlayerState::addRailRideComponents(
					MapState::getIdFromRailIndex(railIndex),
					&replayComponentsHolder,
					(float)startX,
					(float)startY,
					&lastX,
					&lastY);
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
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f),
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
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f),
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
				newEntityAnimationSetGhostSprite(true, endX, endY),
				newEntityAnimationSetVelocity(
					newCompositeQuarticValue(0.0f, moveX, 0.0f, 0.0f, 0.0f),
					newCompositeQuarticValue(0.0f, moveY, 0.0f, 0.0f, 0.0f)),
				newEntityAnimationSetSpriteDirection(spriteDirection),
				newEntityAnimationDelay(ticksDuration)
			});
	}
#endif
void GameState::beginIntroAnimation(int ticksTime) {
	MapState::setIntroAnimationBootTile(true);
	camera = dynamicCameraAnchor.get();

	textDisplayType = TextDisplayType::Intro;
	titleAnimationStartTicksTime = ticksTime;

	//player animation component helpers
	float speedPerTick = PlayerState::speedPerSecond / (float)Config::ticksPerSecond;
	EntityAnimation::SetVelocity* stopMoving = newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f));
	EntityAnimation::SetVelocity* walkLeft =
		newEntityAnimationSetVelocity(newCompositeQuarticValue(0.0f, -speedPerTick, 0.0f, 0.0f, 0.0f), newConstantValue(0.0f));
	EntityAnimation::SetVelocity* walkRight =
		newEntityAnimationSetVelocity(newCompositeQuarticValue(0.0f, speedPerTick, 0.0f, 0.0f, 0.0f), newConstantValue(0.0f));
	EntityAnimation::SetVelocity* walkUp =
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newCompositeQuarticValue(0.0f, -speedPerTick, 0.0f, 0.0f, 0.0f));
	EntityAnimation::SetVelocity* walkDown =
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newCompositeQuarticValue(0.0f, speedPerTick, 0.0f, 0.0f, 0.0f));
	EntityAnimation::SetSpriteAnimation* setWalkingAnimation =
		newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerWalkingAnimation);
	EntityAnimation::SetSpriteAnimation* clearSpriteAnimation = newEntityAnimationSetSpriteAnimation(nullptr);

	vector<ReferenceCounterHolder<EntityAnimation::Component>> playerAnimationComponents ({
		stopMoving,
		newEntityAnimationSetPosition(
			PlayerState::introAnimationPlayerCenterX, PlayerState::introAnimationPlayerCenterY),
		newEntityAnimationDelay(introAnimationStartTicksTime),
		newEntityAnimationDelay(2500),
		//walk to the wall
		walkRight,
		setWalkingAnimation,
		newEntityAnimationSetSpriteDirection(SpriteDirection::Right),
		newEntityAnimationDelay(2200),
		//walk down, stop at the boot
		walkDown,
		newEntityAnimationSetSpriteDirection(SpriteDirection::Down),
		newEntityAnimationDelay(1000),
		stopMoving,
		clearSpriteAnimation,
		newEntityAnimationDelay(3000),
		//look at the boot
		newEntityAnimationSetSpriteDirection(SpriteDirection::Right),
		newEntityAnimationDelay(800),
		newEntityAnimationSetSpriteDirection(SpriteDirection::Down),
		newEntityAnimationDelay(1000),
		//walk around the boot
		walkDown,
		setWalkingAnimation,
		newEntityAnimationDelay(900),
		stopMoving,
		clearSpriteAnimation,
		newEntityAnimationDelay(700),
		walkRight,
		setWalkingAnimation,
		newEntityAnimationSetSpriteDirection(SpriteDirection::Right),
		newEntityAnimationDelay(1800),
		//stop and look around
		stopMoving,
		clearSpriteAnimation,
		newEntityAnimationDelay(700),
		newEntityAnimationSetSpriteDirection(SpriteDirection::Up),
		newEntityAnimationDelay(700),
		newEntityAnimationSetSpriteDirection(SpriteDirection::Down),
		newEntityAnimationDelay(500),
		newEntityAnimationSetSpriteDirection(SpriteDirection::Left),
		newEntityAnimationDelay(350),
		newEntityAnimationSetSpriteDirection(SpriteDirection::Right),
		newEntityAnimationDelay(1000),
		//walk up
		newEntityAnimationSetSpriteDirection(SpriteDirection::Up),
		walkUp,
		setWalkingAnimation,
		newEntityAnimationDelay(1700),
		//stop and look around
		stopMoving,
		clearSpriteAnimation,
		newEntityAnimationDelay(1000),
		newEntityAnimationSetSpriteDirection(SpriteDirection::Right),
		newEntityAnimationDelay(600),
		newEntityAnimationSetSpriteDirection(SpriteDirection::Left),
		newEntityAnimationDelay(400),
		newEntityAnimationSetSpriteDirection(SpriteDirection::Up),
		newEntityAnimationDelay(1200),
		//walk back down to the boot
		walkDown,
		setWalkingAnimation,
		newEntityAnimationSetSpriteDirection(SpriteDirection::Down),
		newEntityAnimationDelay(900),
		walkLeft,
		newEntityAnimationSetSpriteDirection(SpriteDirection::Left),
		newEntityAnimationDelay(400),
		stopMoving,
		clearSpriteAnimation,
		newEntityAnimationDelay(500),
		//approach more slowly
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, -speedPerTick * 0.5f, 0.0f, 0.0f, 0.0f), newConstantValue(0.0f)),
		setWalkingAnimation,
		newEntityAnimationDelay(540),
		stopMoving,
		clearSpriteAnimation,
		newEntityAnimationDelay(800),
		//put on the boot, moving the arbitrary distance needed to get to the right position
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, -17.0f / (float)Config::ticksPerSecond, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, 4.0f / (float)Config::ticksPerSecond, 0.0f, 0.0f, 0.0f)),
		newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerLegLiftAnimation),
		newEntityAnimationDelay(SpriteRegistry::playerLegLiftAnimation->getTotalTicksDuration()),
		stopMoving,
		clearSpriteAnimation
	});
	Holder_EntityAnimationComponentVector playerAnimationComponentsHolder (&playerAnimationComponents);
	playerState.get()->beginEntityAnimation(&playerAnimationComponentsHolder, ticksTime);

	int blackScreenFadeOutEndTime = introAnimationStartTicksTime + 1000;
	int animationEndTicksTime = EntityAnimation::getComponentTotalTicksDuration(playerAnimationComponents);
	int legLiftStartTime = animationEndTicksTime - SpriteRegistry::playerLegLiftAnimation->getTotalTicksDuration();
	vector<ReferenceCounterHolder<EntityAnimation::Component>> cameraAnimationComponents ({
		newEntityAnimationSetPosition(MapState::introAnimationCameraCenterX, MapState::introAnimationCameraCenterY),
		newEntityAnimationSetScreenOverlayColor(
			newConstantValue(0.0f),
			newConstantValue(0.0f),
			newConstantValue(0.0f),
			newLinearInterpolatedValue({
				LinearInterpolatedValue::ValueAtTime(1.0f, introAnimationStartTicksTime) COMMA
				LinearInterpolatedValue::ValueAtTime(0.125f, blackScreenFadeOutEndTime) COMMA
				LinearInterpolatedValue::ValueAtTime(0.125f, legLiftStartTime) COMMA
				LinearInterpolatedValue::ValueAtTime(0.0f, animationEndTicksTime)
			})),
		newEntityAnimationDelay(animationEndTicksTime),
		newEntityAnimationSetScreenOverlayColor(
			newConstantValue(0.0f), newConstantValue(0.0f), newConstantValue(0.0f), newConstantValue(0.0f)),
		newEntityAnimationSwitchToPlayerCamera()
	});
	Holder_EntityAnimationComponentVector cameraAnimationComponentsHolder (&cameraAnimationComponents);
	dynamicCameraAnchor.get()->beginEntityAnimation(&cameraAnimationComponentsHolder, ticksTime);
}
void GameState::resetGame(int ticksTime) {
	sawIntroAnimation = false;
	gameTimeOffsetTicksDuration = 0;
	pauseState.set(nullptr);
	mapState.get()->resetMap();
	playerState.get()->reset();
	dynamicCameraAnchor.set(newDynamicCameraAnchor());

	beginIntroAnimation(ticksTime);
}
