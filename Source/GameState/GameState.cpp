#include "GameState.h"
#include "Audio/Audio.h"
#include "Editor/Editor.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/HintState.h"
#include "GameState/KickAction.h"
#include "GameState/PauseState.h"
#include "GameState/PlayerState.h"
#include "GameState/MapState/MapState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Sprites/Text.h"
#ifdef STEAM
	#include "ThirdParty/Steam.h"
#endif
#include "Util/Config.h"
#include "Util/FileUtils.h"
#include "Util/Logger.h"
#include "Util/StringUtils.h"

#define entityAnimationVelocityAndAnimationForDelay(velocity, animation, delay) \
	velocity, \
	animation, \
	newEntityAnimationDelay(delay)
#define entityAnimationVelocityAndAnimationAndDirectionForDelay(velocity, animation, direction, delay) \
	velocity, \
	animation, \
	newEntityAnimationSetDirection(direction), \
	newEntityAnimationDelay(delay)
#define entityAnimationDirectionForDelay(direction, delay) \
	newEntityAnimationSetDirection(direction), \
	newEntityAnimationDelay(delay)

GameState::GameState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
levelsUnlocked(0)
, perpetualHints(false)
, textDisplayType(TextDisplayType::None)
, titleAnimationStartTicksTime(0)
, playerState(nullptr)
, mapState(nullptr)
, dynamicCameraAnchor(nullptr)
, camera(nullptr)
, tutorialFreezePlayerStartTicksTime(0)
, lastSaveTicksTime(0)
, savePerformed(false)
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
	mapState.clear();
	playerState.clear();
	dynamicCameraAnchor.clear();
	pauseState.clear();

	//copy values that don't usually change from state to state
	levelsUnlocked = prev->levelsUnlocked;
	perpetualHints = prev->perpetualHints;
	textDisplayType = prev->textDisplayType;
	titleAnimationStartTicksTime = prev->titleAnimationStartTicksTime;
	lastSaveTicksTime = prev->lastSaveTicksTime;
	savePerformed = prev->savePerformed;

	//don't update any state if we're paused
	PauseState* lastPauseState = prev->pauseState.get();
	if (lastPauseState != nullptr) {
		PauseState* nextPauseState = lastPauseState->getNextPauseState();
		gameTimeOffsetTicksDuration = prev->gameTimeOffsetTicksDuration + ticksTime - prev->pauseStartTicksTime;
		pauseStartTicksTime = ticksTime;
		int gameTicksTime = ticksTime - gameTimeOffsetTicksDuration;

		//use the direct pointers from the previous state since they didn't change
		mapState.set(prev->mapState.get());
		dynamicCameraAnchor.set(prev->dynamicCameraAnchor.get());

		//because of level select, the player can't be the same pointer and the camera might also not be the same pointer
		playerState.set(newPlayerState(mapState.get()));
		playerState.get()->copyPlayerState(prev->playerState.get());
		prev->camera->setNextCamera(this, gameTicksTime);
		int selectedLevelN = nextPauseState == nullptr ? 0 : nextPauseState->getSelectedLevelN();
		if (selectedLevelN != lastPauseState->getSelectedLevelN())
			playerState.get()->setLevelSelectState(selectedLevelN);

		if (nextPauseState != nullptr) {
			int endPauseDecision = nextPauseState->getEndPauseDecision();
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::Save) != 0) {
				Logger::gameplayLogger.log("  save state");
				saveState(gameTicksTime);
			}
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::Load) != 0) {
				Logger::gameplayLogger.log("  load state");
				startMusic();
			}
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::RequestHint) != 0 && !Editor::isActive)
				mapState.get()->setHint(playerState.get()->getHint(), gameTicksTime);
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::SelectLevel) != 0)
				playerState.get()->generateHint(&Hint::none, gameTicksTime);
			//don't reset the game in editor mode
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::Reset) != 0 && !Editor::isActive) {
				Logger::gameplayLogger.log("  reset game");
				resetGame(ticksTime);
				//don't check for Exit, just return here, after ensuring we release nextPauseState
				ReferenceCounterHolder<PauseState> nextPauseStateHolder (nextPauseState);
				return;
			}
			if ((endPauseDecision & (int)PauseState::EndPauseDecision::Exit) != 0)
				shouldQuitGame = true;
			else if (endPauseDecision != 0) {
				ReferenceCounterHolder<PauseState> nextPauseStateHolder (nextPauseState);
				nextPauseState = nullptr;
			}
		}
		if (nextPauseState == nullptr)
			Audio::resumeAll();
		pauseState.set(nextPauseState);
		return;
	}

	//we're not paused, find what time it really is
	gameTimeOffsetTicksDuration = prev->gameTimeOffsetTicksDuration;
	int gameTicksTime = ticksTime - gameTimeOffsetTicksDuration;

	//update states
	mapState.set(newMapState());
	mapState.get()->updateWithPreviousMapState(prev->mapState.get(), gameTicksTime);
	playerState.set(newPlayerState(mapState.get()));
	bool freezePlayer =
		prev->tutorialFreezePlayerStartTicksTime > 0 && gameTicksTime >= prev->tutorialFreezePlayerStartTicksTime;
	bool playerHasKeyboardControl = prev->camera == prev->playerState.get() && !freezePlayer;
	playerState.get()->updateWithPreviousPlayerState(prev->playerState.get(), playerHasKeyboardControl, gameTicksTime);
	dynamicCameraAnchor.set(newDynamicCameraAnchor());
	dynamicCameraAnchor.get()->updateWithPreviousDynamicCameraAnchor(
		prev->dynamicCameraAnchor.get(), prev->camera == prev->dynamicCameraAnchor.get(), gameTicksTime);

	//set the player to freeze if we want them to perform a tutorial
	if (playerState.get()->shouldFreezePlayerForTutorial() || mapState.get()->shouldFreezePlayerForTutorial()) {
		static constexpr int tutorialFreezePlayerGracePeriod = 5000;
		tutorialFreezePlayerStartTicksTime =
			//if the player was already frozen, keep the timer going
			freezePlayer ? prev->tutorialFreezePlayerStartTicksTime :
			//if the player was in an animation, clear the timer
			!playerHasKeyboardControl ? 0 :
			//if we already started the player-freeze timer, keep it going
			prev->tutorialFreezePlayerStartTicksTime > 0 ? prev->tutorialFreezePlayerStartTicksTime :
			//othersise, reset the time
			gameTicksTime + tutorialFreezePlayerGracePeriod;
	} else
		tutorialFreezePlayerStartTicksTime = 0;

	//forget the previous camera if we're starting our radio tower animation
	if (mapState.get()->getShouldPlayRadioTowerAnimation()) {
		startRadioTowerAnimation(gameTicksTime);
		setDynamicCamera();
	//otherwise set our next camera anchor
	} else
		prev->camera->setNextCamera(this, gameTicksTime);

	//unlock a new level
	int playerLevelN = playerState.get()->getLevelN();
	if (playerLevelN > levelsUnlocked) {
		//this is the end of the intro animation
		if (levelsUnlocked == 0) {
			MapState::setIntroAnimationBootTile(false);
			playerState.get()->setInitialZ();
		//the player beat a level, spawn goal sparks if it's not the last level
		} else if (playerLevelN < MapState::getLevelCount())
			mapState.get()->spawnGoalSparks(playerLevelN, gameTicksTime);
		levelsUnlocked = playerLevelN;
		//save the game if it's the last level, or autosaving at new levels is enabled
		if (playerLevelN == MapState::getLevelCount() || (playerLevelN > 1 && Config::autosaveEveryNewLevelEnabled.isOn()))
			saveState(gameTicksTime);
		#ifdef STEAM
			Steam::unlockLevelEndAchievement(playerLevelN - 1);
		#endif
	}

	//debug
	if (perpetualHints)
		mapState.get()->setHint(playerState.get()->getHint(), gameTicksTime);

	//handle events after states have been updated
	SDL_Event gameEvent;
	while (SDL_PollEvent(&gameEvent) != 0) {
		switch (gameEvent.type) {
			case SDL_QUIT:
				shouldQuitGame = true;
				break;
			case SDL_KEYDOWN:
				if (gameEvent.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					Audio::pauseAll();
					Audio::selectSound->play(0);
					pauseState.set(PauseState::produceBasePauseScreen(levelsUnlocked));
					pauseStartTicksTime = ticksTime;
					playerState.get()->savePauseState();
				} else if (gameEvent.key.keysym.scancode == Config::kickKeyBinding.value) {
					if (playerHasKeyboardControl && !Editor::isActive)
						playerState.get()->beginKicking(gameTicksTime);
				} else if (gameEvent.key.keysym.scancode == Config::showConnectionsKeyBinding.value) {
					if (!Config::showConnectionsMode.isHold() || !mapState.get()->getShowConnections() || Editor::isActive)
						mapState.get()->toggleShowConnections();
				} else if (gameEvent.key.keysym.scancode == Config::mapCameraKeyBinding.value) {
					mapState.get()->finishMapCameraTutorial();
					if (camera->hasAnimation())
						; //don't touch the camera during animations
					else if (camera == dynamicCameraAnchor.get())
						camera = playerState.get();
					else {
						ConstantValue* zero = newConstantValue(0.0f);
						ReferenceCounterHolder<DynamicValue> zeroHolder (zero); //needed to return it to the pool
						playerState.get()->setVelocity(zero, zero, gameTicksTime);
						camera = dynamicCameraAnchor.get();
						camera->copyEntityState(playerState.get());
					}
				} else if (gameEvent.key.keysym.scancode == Config::undoKeyBinding.value) {
					if (!Editor::isActive)
						playerState.get()->undo(gameTicksTime);
				} else if (gameEvent.key.keysym.scancode == Config::redoKeyBinding.value) {
					if (!Editor::isActive)
						playerState.get()->redo(gameTicksTime);
				}
				break;
			case SDL_KEYUP:
				if (gameEvent.key.keysym.scancode == Config::showConnectionsKeyBinding.value) {
					if (Config::showConnectionsMode.isHold() && !Editor::isActive)
						mapState.get()->toggleShowConnections();
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

	if (playerState.get()->getShouldEndGame()) {
		beginOutroAnimation(gameTicksTime);
		setDynamicCamera();
	}

	//if we saved the floor file, the editor requests that we save the game too, since rail/switch ids may have changed
	if (Editor::needsGameStateSave) {
		saveState(gameTicksTime);
		Editor::needsGameStateSave = false;
	}

	//autosave if applicable
	if (Config::autosaveAtIntervalsEnabled.isOn()
			&& gameTicksTime >= lastSaveTicksTime + Config::autosaveInterval.selectedValue * Config::ticksPerSecond)
		saveState(gameTicksTime);
}
void GameState::setPlayerCamera() {
	camera = playerState.get();
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
	float switchesAnimationCenterWorldX = 0;
	float switchesAnimationCenterWorldY = 0;
	char lastActivatedSwitchColor = mapState.get()->getLastActivatedSwitchColor();
	switch (lastActivatedSwitchColor) {
		case MapState::squareColor:
			switchesAnimationCenterWorldX = MapState::firstLevelTileOffsetX * MapState::tileSize + 280;
			switchesAnimationCenterWorldY = MapState::firstLevelTileOffsetY * MapState::tileSize + 80;
			break;
		case MapState::triangleColor:
			switchesAnimationCenterWorldX = MapState::firstLevelTileOffsetX * MapState::tileSize + 180;
			switchesAnimationCenterWorldY = MapState::firstLevelTileOffsetY * MapState::tileSize - 103;
			break;
		case MapState::sawColor:
			switchesAnimationCenterWorldX = MapState::firstLevelTileOffsetX * MapState::tileSize + 780;
			switchesAnimationCenterWorldY = MapState::firstLevelTileOffsetY * MapState::tileSize - 150;
			break;
		case MapState::sineColor:
			switchesAnimationCenterWorldX = MapState::firstLevelTileOffsetX * MapState::tileSize + 524;
			switchesAnimationCenterWorldY = MapState::firstLevelTileOffsetY * MapState::tileSize + 130;
			break;
		default:
			break;
	}

	//build the animation until right before the radio waves animation
	static constexpr int radioTowerInitialPauseAnimationTicks = 1500;
	static constexpr int playerToRadioTowerAnimationTicks = 2000;
	static constexpr int preRadioWavesAnimationTicks = 2000;
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> dynamicCameraAnchorAnimationComponents ({
		newEntityAnimationSetPosition(playerX, playerY),
		stopMoving,
		newEntityAnimationDelay(radioTowerInitialPauseAnimationTicks),
		EntityAnimation::SetVelocity::cubicInterpolation(
			radioTowerAntennaX - playerX, radioTowerAntennaY - playerY, (float)playerToRadioTowerAnimationTicks),
		newEntityAnimationDelay(playerToRadioTowerAnimationTicks),
		stopMoving,
		newEntityAnimationDelay(preRadioWavesAnimationTicks),
	});

	//add the radio waves animation
	int radioWavesAnimationTicksDuration = mapState.get()->startRadioWavesAnimation(
		EntityAnimation::getComponentTotalTicksDuration(dynamicCameraAnchorAnimationComponents), ticksTime);

	//build the animation from after the radio waves animation until right before the switches animation
	static constexpr int postRadioWavesAnimationTicks = 2000;
	static constexpr int radioTowerToSwitchesAnimationTicks = 2000;
	static constexpr int preSwitchesFadeInAnimationTicks = 1000;
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
			newEntityAnimationDelay(preSwitchesFadeInAnimationTicks),
			newEntityAnimationPlaySound(Audio::switchOnSound, 0),
	});

	//add the switches animation
	mapState.get()->startSwitchesFadeInAnimation(
		EntityAnimation::getComponentTotalTicksDuration(dynamicCameraAnchorAnimationComponents), ticksTime);

	//finish the animation by returning to the player
	static constexpr int postSwitchesFadeInAnimationTicks = 1000;
	static constexpr int switchesToPlayerAnimationTicks = 2000;
	dynamicCameraAnchorAnimationComponents.insert(
		dynamicCameraAnchorAnimationComponents.end(),
		{
			newEntityAnimationPlaySound(Audio::switchesFadeInSounds[lastActivatedSwitchColor], 0),
			newEntityAnimationDelay(MapState::switchesFadeInDuration + postSwitchesFadeInAnimationTicks),
			EntityAnimation::SetVelocity::cubicInterpolation(
				playerX - switchesAnimationCenterWorldX,
				playerY - switchesAnimationCenterWorldY,
				(float)switchesToPlayerAnimationTicks),
			newEntityAnimationDelay(switchesToPlayerAnimationTicks),
			stopMoving,
			newEntityAnimationSwitchToPlayerCamera(),
			newEntityAnimationPlaySound(Audio::musics[lastActivatedSwitchColor], -1),
	});
	dynamicCameraAnchor.get()->beginEntityAnimation(&dynamicCameraAnchorAnimationComponents, ticksTime);

	//delay the player for the duration of the animation
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> playerAnimationComponents ({
		newEntityAnimationDelay(
			SpriteRegistry::playerKickingAnimation->getTotalTicksDuration()
				- SpriteRegistry::playerKickingAnimationTicksPerFrame),
		newEntityAnimationSetSpriteAnimation(nullptr),
	});
	EntityAnimation::delayToEndOf(playerAnimationComponents, dynamicCameraAnchorAnimationComponents);
	playerState.get()->beginEntityAnimation(&playerAnimationComponents, ticksTime);
	playerState.get()->clearUndoRedoStates();

	Audio::fadeOutAll(3000);
}
void GameState::startMusic() {
	char lastActivatedSwitchColor = mapState.get()->getLastActivatedSwitchColor();
	if (lastActivatedSwitchColor >= 0 && lastActivatedSwitchColor < 4)
		Audio::musics[lastActivatedSwitchColor]->play(-1);
}
void GameState::renderLoading(int ticksTime) {
	static constexpr char* loadingText = "Loading...";
	static constexpr float loadingFontScale = 2.0f;
	static constexpr float loadingFlashPeriod = 2000.0f;
	static constexpr float loadingFlashMinAlpha = 0.5f;
	static constexpr float loadingFlashMedianAlpha = (1.0f + loadingFlashMinAlpha) / 2.0f;
	static constexpr float loadingFlashAlphaVariance = (1.0f - loadingFlashMinAlpha) / 2.0f;

	static Text::Metrics loadingMetrics = Text::getMetrics(loadingText, loadingFontScale);
	static float leftX = (Config::gameScreenWidth - loadingMetrics.charactersWidth) * 0.5f;
	static float baselineY = (Config::gameScreenHeight + loadingMetrics.aboveBaseline - loadingMetrics.belowBaseline) * 0.5f;

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	float alpha =
		loadingFlashMedianAlpha + sinf(MathUtils::twoPi * (ticksTime / loadingFlashPeriod)) * loadingFlashAlphaVariance;
	Text::setRenderColor(0.0f, 0.0f, 0.0f, (GLfloat)alpha);
	Text::render(loadingText, leftX, baselineY, loadingFontScale);
	Text::setRenderColor(1.0f, 1.0f, 1.0f, 1.0f);
}
void GameState::render(int ticksTime) {
	Editor::EditingMutexLocker editingMutexLocker;
	int gameTicksTime = (pauseState.get() != nullptr ? pauseStartTicksTime : ticksTime) - gameTimeOffsetTicksDuration;

	float zoomValue = camera->renderBeginZoom(gameTicksTime);
	Opengl::clearBackground();

	//map and player rendering
	char playerZ = (char)(floorf(playerState.get()->getDynamicZ(gameTicksTime) + 0.5f));
	mapState.get()->renderBelowPlayer(camera, playerState.get()->getWorldGroundY(gameTicksTime), playerZ, gameTicksTime);
	playerState.get()->render(camera, gameTicksTime);
	mapState.get()->renderAbovePlayer(camera, gameTicksTime);

	//kick-action-related rendering
	short contextualGroupsRailSwitchId;
	bool hasRailsToReset = false;
	if (playerState.get()->hasRailSwitchKickAction(KickActionType::ResetSwitch, &contextualGroupsRailSwitchId))
		hasRailsToReset = mapState.get()->renderGroupsForRailsToReset(camera, contextualGroupsRailSwitchId, gameTicksTime);
	else if (playerState.get()->hasRailSwitchKickAction(KickActionType::Switch, &contextualGroupsRailSwitchId))
		mapState.get()->renderGroupsForRailsFromSwitch(camera, contextualGroupsRailSwitchId, gameTicksTime);
	else if (playerState.get()->hasRailSwitchKickAction(KickActionType::Rail, &contextualGroupsRailSwitchId)
			|| playerState.get()->hasRailSwitchKickAction(KickActionType::NoRail, &contextualGroupsRailSwitchId))
		mapState.get()->renderGroupsForSwitchesFromRail(camera, contextualGroupsRailSwitchId, gameTicksTime);
	playerState.get()->renderKickAction(camera, hasRailsToReset, gameTicksTime);

	camera->renderEndZoom(zoomValue);

	//render UI
	if (levelsUnlocked > 0 && !camera->hasAnimation()) {
		int tutorialFlashTime = gameTicksTime - tutorialFreezePlayerStartTicksTime;
		bool flashTutorial = tutorialFreezePlayerStartTicksTime > 0 && tutorialFlashTime >= 0;
		if (flashTutorial) {
			static constexpr float tutorialFlashPeriod = 2000.0f;
			static constexpr float tutorialFlashMinAlpha = 0.5f;
			static constexpr float tutorialFlashMedianAlpha = (1.0f + tutorialFlashMinAlpha) / 2.0f;
			static constexpr float tutorialFlashAlphaVariance = (1.0f - tutorialFlashMinAlpha) / 2.0f;
			float alpha =
				tutorialFlashMedianAlpha
					+ cosf(MathUtils::twoPi * (tutorialFlashTime / tutorialFlashPeriod)) * tutorialFlashAlphaVariance;
			Text::setRenderColor(1.0f, 1.0f, 1.0f, (GLfloat)alpha);
		}
		playerState.get()->renderTutorials() || mapState.get()->renderTutorials();
		if (flashTutorial)
			Text::setRenderColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	if (camera == dynamicCameraAnchor.get())
		dynamicCameraAnchor.get()->render(gameTicksTime);
	if (textDisplayType != TextDisplayType::None)
		renderTextDisplay(gameTicksTime);
	if (gameTicksTime < lastSaveTicksTime + saveIconShowDuration && savePerformed)
		renderSaveIcon(gameTicksTime);

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
			textDisplayStrings = { "You are", "a boot.", " ", " ", " " };
			textFadeInStartTicksTime = introAnimationStartTicksTime + 5900;
			textFadeInTicksDuration = 500;
			textDisplayTicksDuration = 1000;
			textFadeOutTicksDuration = 500;
			nextTextDisplayType = TextDisplayType::RadioTowerExplanation;
			break;
		case TextDisplayType::RadioTowerExplanation:
			textDisplayStrings =
				{ "Your local radio tower", "lost connection", "with its", "master transmitter relay.", " ", " ", " ", " " };
			textFadeInStartTicksTime = introAnimationStartTicksTime + 11000;
			textFadeInTicksDuration = 500;
			textDisplayTicksDuration = 4500;
			textFadeOutTicksDuration = 500;
			nextTextDisplayType = TextDisplayType::GoalExplanation;
			break;
		case TextDisplayType::GoalExplanation:
			textDisplayStrings = { " ", " ", " ", " ", " ", " ", " ", " ", "Can you", "guide this person", "to turn it on?" };
			textFadeInStartTicksTime = introAnimationStartTicksTime + 18000;
			textFadeInTicksDuration = 500;
			textDisplayTicksDuration = 3000;
			textFadeOutTicksDuration = 500;
			break;
		case TextDisplayType::Outro: {
			static constexpr int outroTitleStartTimeMinusKick =
				outroPostKickPauseDuration
					+ outroPlayerToRadioTowerDuration
					+ outroPreRadioWavesPauseDuration
					+ (outroSingleRadioWavePauseDuration * 3 + outroSingleRadioWaveSoundDuration)
					+ outroInterRadioWavesPauseDuration
					+ (outroSingleRadioWavePauseDuration * 3 + outroSingleRadioWaveSoundDuration)
					+ outroPostRadioWavesPauseDuration
					+ outroRadioTowerToBootPanBeforeZoomDuration
					+ outroRadioTowerToBootPanAndZoomDuration
					+ outroRadioTowerToBootZoomAfterPanDuration
					+ outroPreTurnOnPauseDuration
					+ outroPostTurnOnPauseDuration
					+ outroFadeOutDuration
					+ outroPreTitlePauseDuration;
			textDisplayStrings = { titleGameName, " ", titleCreditsLine1, titleCreditsLine2, " ", "Thanks for playing!" };
			textDisplayMetrics = { Text::getMetrics(titleGameName, 2.0f) };
			textFadeInStartTicksTime =
				outroTitleStartTimeMinusKick + SpriteRegistry::playerKickingAnimation->getTotalTicksDuration();
			textFadeInTicksDuration = outroTextFadeInTicksDuration;
			textDisplayTicksDuration = outroForeverDuration;
			textFadeOutTicksDuration = 0;
			break;
		}
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
	Text::setRenderColor(1.0f, 1.0f, 1.0f, textDisplayAlpha);
	Text::renderLines(textDisplayStrings, textDisplayMetrics);
	Text::setRenderColor(1.0f, 1.0f, 1.0f, 1.0f);
}
void GameState::renderSaveIcon(int gameTicksTime) {
	static constexpr int saveIconEdgeSpacing = 10;
	static constexpr float saveIconMaxAlpha = 7.0f / 8.0f;
	float baseAlpha = 1.0f - MathUtils::fsqr((float)(gameTicksTime - lastSaveTicksTime) / saveIconShowDuration);
	GLint drawLeftX = Config::gameScreenWidth - SpriteRegistry::save->getSpriteWidth() - saveIconEdgeSpacing;
	GLint drawTopY = Config::gameScreenHeight - SpriteRegistry::save->getSpriteHeight() - saveIconEdgeSpacing;
	(SpriteRegistry::save->*SpriteSheet::setSpriteColor)(1.0f, 1.0f, 1.0f, saveIconMaxAlpha * baseAlpha);
	(SpriteRegistry::save->*SpriteSheet::renderSpriteAtScreenPosition)(0, 0, drawLeftX, drawTopY);
	(SpriteRegistry::save->*SpriteSheet::setSpriteColor)(1.0f, 1.0f, 1.0f, 1.0f);
}
void GameState::saveState(int gameTicksTime) {
	ofstream file;
	FileUtils::openFileForWrite(&file, savedGameFileName, ios::out | ios::trunc);
	file << versionFilePrefix << 1 << "\n";
	if (levelsUnlocked > 0)
		file << levelsUnlockedFilePrefix << levelsUnlocked << "\n";
	if (perpetualHints)
		file << perpetualHintsFileValue << "\n";
	playerState.get()->saveState(file);
	mapState.get()->saveState(file);
	file.close();
	lastSaveTicksTime = gameTicksTime;
	savePerformed = true;
}
void GameState::loadInitialState(int ticksTime) {
	//first things first, we need some state
	mapState.set(newMapState());
	playerState.set(newPlayerState(mapState.get()));
	dynamicCameraAnchor.set(newDynamicCameraAnchor());

	#ifdef DEBUG
		//before continuing setup, see if we have a replay, and stop if we do
		if (loadReplay())
			return;
	#endif

	//we have no replay, continue setup
	loadSaveFile();
	playerState.get()->obtainBoot();

	//special setup for the editor, skip the home screen and start in map mode
	if (Editor::isActive) {
		//put the player at the radio tower if we start the editor without a save file
		if (levelsUnlocked == 0)
			playerState.get()->setHomeScreenState();
		playerState.get()->setHighestZ();
		startMusic();
		//enable show connections if necessary
		if (!mapState.get()->getShowConnections()) {
			//toggle twice to skip the hide-non-tiles setting
			mapState.get()->toggleShowConnections();
			mapState.get()->toggleShowConnections();
		}
		//set the player as the camera
		camera = playerState.get();
		return;
	}

	//continue regular setup
	camera = playerState.get();
	pauseStartTicksTime = 0;
	pauseState.set(PauseState::produceHomeScreen(levelsUnlocked));
	if (levelsUnlocked > 0) {
		playerState.get()->setInitialZ();
		playerState.get()->generateHint(&Hint::none, ticksTime);
		playerState.get()->savePauseState();
	} else {
		playerState.get()->setHomeScreenState();
		PauseState::disableContinueOptions();
	}

	#ifdef STEAM
		Steam::tryFixLevelEndAchievements(MathUtils::min(7, levelsUnlocked - 1));
	#endif
}
void GameState::loadSaveFile() {
	ifstream file;
	FileUtils::openFileForRead(&file, savedGameFileName, FileUtils::FileReadLocation::ApplicationData);
	string line;
	while (getline(file, line)) {
		if (StringUtils::startsWith(line, versionFilePrefix))
			;
		else if (StringUtils::startsWith(line, levelsUnlockedFilePrefix))
			levelsUnlocked = atoi(line.c_str() + StringUtils::strlenConst(levelsUnlockedFilePrefix));
		else if (StringUtils::startsWith(line, perpetualHintsFileValue))
			perpetualHints = true;
		else
			playerState.get()->loadState(line) || mapState.get()->loadState(line);
	}
	file.close();
}
#ifdef DEBUG
	bool GameState::loadReplay() {
		ifstream file;
		FileUtils::openFileForRead(&file, "kyo_replay.log", FileUtils::FileReadLocation::ApplicationData);
		string line;
		bool beganGameplay = false;
		int lastTimestamp = 0;
		float lastX = 0.0f;
		float lastY = 0.0f;
		vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> replayComponents;
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
						newEntityAnimationDelay(timestamp),
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
					&replayComponents, lastX, lastY, (float)climbStartX, (float)climbStartY, timestamp - lastTimestamp);
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
				addMoveWithGhost(&replayComponents, (float)climbStartX, (float)climbStartY, lastX, lastY, climbDuration);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f, SpriteDirection::Down),
						newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
						newEntityAnimationSetSpriteAnimation(nullptr),
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
					&replayComponents, lastX, lastY, (float)fallStartX, (float)fallStartY, timestamp - lastTimestamp);
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
				addMoveWithGhost(&replayComponents, (float)fallStartX, (float)fallStartY, lastX, lastY, fallDuration);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f, SpriteDirection::Down),
						newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
						newEntityAnimationSetSpriteAnimation(nullptr),
					});
				lastTimestamp = timestamp + fallDuration;
			} else if (StringUtils::startsWith(logMessageString, "  rail ")) {
				logMessage = logMessage + 7;
				int railIndex;
				logMessage = StringUtils::parseNextInt(logMessage, &railIndex);
				int startX;
				int startY;
				StringUtils::parsePosition(logMessage, &startX, &startY);
				addMoveWithGhost(&replayComponents, lastX, lastY, (float)startX, (float)startY, timestamp - lastTimestamp);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f, SpriteDirection::Down),
						newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
					});
				PlayerState::addRailRideComponents(
					MapState::getIdFromRailIndex(railIndex),
					&replayComponents,
					(float)startX,
					(float)startY,
					PlayerState::RideRailSpeed::Forward,
					&Hint::none,
					&lastX,
					&lastY,
					nullptr);
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
				addMoveWithGhost(&replayComponents, lastX, lastY, moveX, moveY, timestamp - lastTimestamp);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f, SpriteDirection::Down),
						newEntityAnimationSetDirection(SpriteDirection::Down),
					});
				PlayerState::addKickSwitchComponents(
					MapState::getIdFromSwitchIndex(switchIndex), &replayComponents, true, false, &Hint::none);
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
				addMoveWithGhost(&replayComponents, lastX, lastY, moveX, moveY, timestamp - lastTimestamp);
				replayComponents.insert(
					replayComponents.end(),
					{
						newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f, SpriteDirection::Down),
						newEntityAnimationSetDirection(SpriteDirection::Down),
					});
				PlayerState::addKickResetSwitchComponents(
					MapState::getIdFromResetSwitchIndex(resetSwitchIndex), &replayComponents, nullptr, &Hint::none);
				replayComponents.push_back(newEntityAnimationSetSpriteAnimation(nullptr));
				lastX = moveX;
				lastY = moveY;
				lastTimestamp = EntityAnimation::getComponentTotalTicksDuration(replayComponents);
			}
			//TODO: perform undo/redo
		}
		file.close();

		if (replayComponents.empty())
			return false;

		playerState.get()->beginEntityAnimation(&replayComponents, 0);
		playerState.get()->setHighestZ();
		playerState.get()->obtainBoot();
		camera = playerState.get();
		levelsUnlocked = 100;
		return true;
	}
	void GameState::addMoveWithGhost(
		vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* replayComponents,
		float startX,
		float startY,
		float endX,
		float endY,
		int ticksDuration)
	{
		float moveX = (endX - startX) / (float)ticksDuration;
		float moveY = (endY - startY) / (float)ticksDuration;
		SpriteDirection spriteDirection = EntityState::getSpriteDirection(moveX, moveY);
		replayComponents->insert(
			replayComponents->end(),
			{
				newEntityAnimationSetGhostSprite(true, endX, endY, spriteDirection),
				newEntityAnimationSetVelocity(
					newCompositeQuarticValue(0.0f, moveX, 0.0f, 0.0f, 0.0f),
					newCompositeQuarticValue(0.0f, moveY, 0.0f, 0.0f, 0.0f)),
				entityAnimationDirectionForDelay(spriteDirection, ticksDuration),
			});
	}
#endif
void GameState::beginIntroAnimation(int ticksTime) {
	MapState::setIntroAnimationBootTile(true);
	camera = dynamicCameraAnchor.get();

	textDisplayType = TextDisplayType::Intro;
	titleAnimationStartTicksTime = ticksTime;

	//player animation component helpers
	EntityAnimation::SetVelocity* stopMoving = newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f));
	EntityAnimation::SetSpriteAnimation* clearSpriteAnimation = newEntityAnimationSetSpriteAnimation(nullptr);

	//wait the player offscreen
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> playerAnimationComponents ({
		stopMoving,
		newEntityAnimationSetPosition(
			PlayerState::introAnimationPlayerCenterX, PlayerState::introAnimationPlayerCenterY),
		newEntityAnimationDelay(introAnimationStartTicksTime + 2500),
	});
	//walk to the wall and down
	int walkingDuration = introAnimationWalk(playerAnimationComponents, SpriteDirection::Right, 2200, 0);
	introAnimationWalk(playerAnimationComponents, SpriteDirection::Down, 1000, walkingDuration);
	//stop and look at the boot
	playerAnimationComponents.insert(
		playerAnimationComponents.end(),
		{
			entityAnimationVelocityAndAnimationForDelay(stopMoving, clearSpriteAnimation, 3000),
			entityAnimationDirectionForDelay(SpriteDirection::Right, 800),
			entityAnimationDirectionForDelay(SpriteDirection::Down, 1000),
		});
	//walk around the boot
	introAnimationWalk(playerAnimationComponents, SpriteDirection::Down, 900, 0),
	playerAnimationComponents.insert(
		playerAnimationComponents.end(),
		{
			entityAnimationVelocityAndAnimationForDelay(stopMoving, clearSpriteAnimation, 700),
		});
	introAnimationWalk(playerAnimationComponents, SpriteDirection::Right, 1800, 0),
	//stop and look around
	playerAnimationComponents.insert(
		playerAnimationComponents.end(),
		{
			entityAnimationVelocityAndAnimationForDelay(stopMoving, clearSpriteAnimation, 700),
			entityAnimationDirectionForDelay(SpriteDirection::Up, 700),
			entityAnimationDirectionForDelay(SpriteDirection::Down, 500),
			entityAnimationDirectionForDelay(SpriteDirection::Left, 350),
			entityAnimationDirectionForDelay(SpriteDirection::Right, 1000),
		});
	//walk up
	introAnimationWalk(playerAnimationComponents, SpriteDirection::Up, 1700, 0);
	//stop and look around
	playerAnimationComponents.insert(
		playerAnimationComponents.end(),
		{
			entityAnimationVelocityAndAnimationForDelay(stopMoving, clearSpriteAnimation, 1000),
			entityAnimationDirectionForDelay(SpriteDirection::Right, 600),
			entityAnimationDirectionForDelay(SpriteDirection::Left, 400),
			entityAnimationDirectionForDelay(SpriteDirection::Up, 1200),
		});
	//walk back down and left to the boot
	walkingDuration = introAnimationWalk(playerAnimationComponents, SpriteDirection::Down, 900, 0);
	introAnimationWalk(playerAnimationComponents, SpriteDirection::Left, 400, walkingDuration);
	//finish the animation near the boot
	playerAnimationComponents.insert(
		playerAnimationComponents.end(),
		{
			//stop near the boot
			entityAnimationVelocityAndAnimationForDelay(stopMoving, clearSpriteAnimation, 500),
			//approach more slowly
			newEntityAnimationSetVelocity(
				newCompositeQuarticValue(0.0f, -PlayerState::baseSpeedPerTick * 0.5f, 0.0f, 0.0f, 0.0f),
				newConstantValue(0.0f)),
			newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerWalkingAnimation),
			newEntityAnimationDelay(250),
			newEntityAnimationPlaySound(Audio::stepSounds[rand() % Audio::stepSoundsCount], 0),
			newEntityAnimationDelay(290),
			entityAnimationVelocityAndAnimationForDelay(stopMoving, clearSpriteAnimation, 800),
			//put on the boot, moving the arbitrary distance needed to get to the right position
			newEntityAnimationSetVelocity(
				newCompositeQuarticValue(0.0f, -52.0f / 3.0f / (float)Config::ticksPerSecond, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, 10.0f / 3.0f / (float)Config::ticksPerSecond, 0.0f, 0.0f, 0.0f)),
			newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerLegLiftAnimation),
			newEntityAnimationDelay(SpriteRegistry::playerLegLiftAnimation->getTotalTicksDuration()),
			stopMoving,
			clearSpriteAnimation,
			newEntityAnimationGenerateHint(&Hint::none),
			newEntityAnimationPlaySound(Audio::stepOffRailSound, 0),
		});
	playerState.get()->beginEntityAnimation(&playerAnimationComponents, ticksTime);
	//fix the initial z because the player teleported to the start of the intro animation
	playerState.get()->setInitialZ();

	int blackScreenFadeOutEndTime = introAnimationStartTicksTime + 1000;
	int animationEndTicksTime = EntityAnimation::getComponentTotalTicksDuration(playerAnimationComponents);
	int legLiftStartTime = animationEndTicksTime - SpriteRegistry::playerLegLiftAnimation->getTotalTicksDuration();
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> cameraAnimationComponents ({
		newEntityAnimationSetPosition(MapState::introAnimationCameraCenterX, MapState::introAnimationCameraCenterY),
		newEntityAnimationSetScreenOverlayColor(
			newConstantValue(0.0f),
			newConstantValue(0.0f),
			newConstantValue(0.0f),
			newLinearInterpolatedValue({
				LinearInterpolatedValue::ValueAtTime(1.0f, (float)introAnimationStartTicksTime) COMMA
				LinearInterpolatedValue::ValueAtTime(0.125f, (float)blackScreenFadeOutEndTime) COMMA
				LinearInterpolatedValue::ValueAtTime(0.125f, (float)legLiftStartTime) COMMA
				LinearInterpolatedValue::ValueAtTime(0.0f, (float)animationEndTicksTime) COMMA
			})),
		newEntityAnimationDelay(animationEndTicksTime),
		newEntityAnimationSetScreenOverlayColor(
			newConstantValue(0.0f), newConstantValue(0.0f), newConstantValue(0.0f), newConstantValue(0.0f)),
		newEntityAnimationSwitchToPlayerCamera(),
	});
	dynamicCameraAnchor.get()->beginEntityAnimation(&cameraAnimationComponents, ticksTime);
}
int GameState::introAnimationWalk(
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>& playerAnimationComponents,
	SpriteDirection spriteDirection,
	int duration,
	int previousDuration)
{
	ConstantValue* zero = newConstantValue(0.0f);
	DynamicValue* xVelocity = zero;
	DynamicValue* yVelocity = zero;
	switch (spriteDirection) {
		case SpriteDirection::Left:
			xVelocity = newCompositeQuarticValue(0.0f, -PlayerState::baseSpeedPerTick, 0.0f, 0.0f, 0.0f);
			break;
		case SpriteDirection::Right:
			xVelocity = newCompositeQuarticValue(0.0f, PlayerState::baseSpeedPerTick, 0.0f, 0.0f, 0.0f);
			break;
		case SpriteDirection::Up:
			yVelocity = newCompositeQuarticValue(0.0f, -PlayerState::baseSpeedPerTick, 0.0f, 0.0f, 0.0f);
			break;
		default:
			yVelocity = newCompositeQuarticValue(0.0f, PlayerState::baseSpeedPerTick, 0.0f, 0.0f, 0.0f);
			break;
	}
	//begin the walking animation
	playerAnimationComponents.push_back(newEntityAnimationSetVelocity(xVelocity, yVelocity));
	if (previousDuration == 0)
		playerAnimationComponents.push_back(newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerWalkingAnimation));
	playerAnimationComponents.push_back(newEntityAnimationSetDirection(spriteDirection));
	//check to see if we'll reach the part where we start playing sounds
	static constexpr int stepInterval = SpriteRegistry::playerWalkingAnimationTicksPerFrame * 2;
	static constexpr int stepOffset = SpriteRegistry::playerWalkingAnimationTicksPerFrame;
	int delayBeforeNextSound = stepInterval - (previousDuration + stepOffset - 1) % stepInterval - 1;
	if (duration <= delayBeforeNextSound) {
		playerAnimationComponents.push_back(newEntityAnimationDelay(duration));
		return duration + previousDuration;
	}
	//play sounds
	playerAnimationComponents.push_back(newEntityAnimationDelay(delayBeforeNextSound));
	int durationRemaining = duration - delayBeforeNextSound;
	int lastStepSound = -1;
	while (true) {
		int stepSound = rand() % (Audio::stepSoundsCount - 1);
		if (stepSound >= lastStepSound)
			stepSound++;
		playerAnimationComponents.push_back(newEntityAnimationPlaySound(Audio::stepSounds[stepSound], 0));
		lastStepSound = stepSound;
		if (durationRemaining <= stepInterval) {
			playerAnimationComponents.push_back(newEntityAnimationDelay(durationRemaining));
			return duration + previousDuration;
		}
		playerAnimationComponents.push_back(newEntityAnimationDelay(stepInterval));
		durationRemaining -= stepInterval;
	}
}
void GameState::beginOutroAnimation(int ticksTime) {
	textDisplayType = TextDisplayType::Outro;
	titleAnimationStartTicksTime = ticksTime;

	EntityAnimation::SetVelocity* stopMoving = newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f));

	//first, have the player kick the end-game reset switch
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> playerAnimationComponents ({
		stopMoving,
		newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerKickingAnimation),
		newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame),
		newEntityAnimationPlaySound(Audio::kickSound, 0),
		newEntityAnimationDelay(
			SpriteRegistry::playerKickingAnimation->getTotalTicksDuration()
				- SpriteRegistry::playerKickingAnimationTicksPerFrame),
		newEntityAnimationSetSpriteAnimation(nullptr),
	});
	float playerX = playerState.get()->getRenderCenterWorldX(ticksTime);
	float playerY = playerState.get()->getRenderCenterWorldY(ticksTime);
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> dynamicCameraAnchorAnimationComponents ({
		newEntityAnimationSetPosition(playerX, playerY),
		stopMoving,
		newEntityAnimationDelay(EntityAnimation::getComponentTotalTicksDuration(playerAnimationComponents)),
	});

	//pan to the radio tower antenna
	float radioTowerAntennaX = MapState::antennaCenterWorldX();
	float radioTowerAntennaY = MapState::antennaCenterWorldY();
	dynamicCameraAnchorAnimationComponents.insert(
		dynamicCameraAnchorAnimationComponents.end(),
		{
			newEntityAnimationDelay(outroPostKickPauseDuration),
			EntityAnimation::SetVelocity::cubicInterpolation(
				radioTowerAntennaX - playerX, radioTowerAntennaY - playerY, (float)outroPlayerToRadioTowerDuration),
			newEntityAnimationDelay(outroPlayerToRadioTowerDuration),
			stopMoving,
		});

	//show the end-game radio waves
	static constexpr int singleRadioWavesDuration = outroSingleRadioWavePauseDuration * 3 + outroSingleRadioWaveSoundDuration;
	static constexpr int interSingleRadioWavesPauseDuration = singleRadioWavesDuration + outroInterRadioWavesPauseDuration;
	static constexpr int singleRadioWavesAndPostPauseDuration = singleRadioWavesDuration + outroPostRadioWavesPauseDuration;
	dynamicCameraAnchorAnimationComponents.push_back(newEntityAnimationDelay(outroPreRadioWavesPauseDuration));
	int radioWavesStartTime = EntityAnimation::getComponentTotalTicksDuration(dynamicCameraAnchorAnimationComponents);
	for (int color = 0; color < MapState::colorCount; color++)
		mapState.get()->startEndGameWavesAnimation(
			radioWavesStartTime + outroSingleRadioWavePauseDuration * color,
			(char)color,
			interSingleRadioWavesPauseDuration,
			ticksTime);
	dynamicCameraAnchorAnimationComponents.insert(
		dynamicCameraAnchorAnimationComponents.end(),
		{
			newEntityAnimationPlaySound(Audio::endGameWavesSound, 0),
			newEntityAnimationDelay(interSingleRadioWavesPauseDuration),
			newEntityAnimationPlaySound(Audio::endGameWavesSound, 0),
			newEntityAnimationDelay(singleRadioWavesAndPostPauseDuration),
		});

	//pan to the boot and zoom in
	static constexpr float maxZoom = 13.0f;
	static constexpr int radioTowerToBootPanDuration =
		outroRadioTowerToBootPanBeforeZoomDuration + outroRadioTowerToBootPanAndZoomDuration;
	static constexpr int radioTowerToBootZoomDuration =
		outroRadioTowerToBootPanAndZoomDuration + outroRadioTowerToBootZoomAfterPanDuration;
	static constexpr int radioTowerToBootPanAndZoomFullDuration =
		radioTowerToBootPanDuration + outroRadioTowerToBootZoomAfterPanDuration;
	float bootX = playerState.get()->getEndGameBootX(ticksTime);
	float bootY = playerState.get()->getEndGameBootY(ticksTime);
	EntityAnimation::delayToEndOf(playerAnimationComponents, dynamicCameraAnchorAnimationComponents);
	playerAnimationComponents.push_back(newEntityAnimationSetDirection(SpriteDirection::Right));
	dynamicCameraAnchorAnimationComponents.insert(
		dynamicCameraAnchorAnimationComponents.end(),
		{
			EntityAnimation::SetVelocity::cubicInterpolation(
				bootX - radioTowerAntennaX, bootY - radioTowerAntennaY, (float)radioTowerToBootPanDuration),
			newEntityAnimationSetZoom(
				newPiecewiseValue({
					PiecewiseValue::ValueAtTime(newConstantValue(1.0f), 0.0f) COMMA
					PiecewiseValue::ValueAtTime(
						newTimeFunctionValue(
							newExponentialValue(maxZoom, (float)radioTowerToBootZoomDuration),
							newCompositeQuarticValue(
								0.0f,
								0.0f,
								0.5f / radioTowerToBootZoomDuration,
								0.5f / radioTowerToBootZoomDuration / radioTowerToBootZoomDuration,
								0.0f)),
						outroRadioTowerToBootPanBeforeZoomDuration) COMMA
					PiecewiseValue::ValueAtTime(newConstantValue(maxZoom), (float)radioTowerToBootPanAndZoomFullDuration) COMMA
				})),
			newEntityAnimationDelay(radioTowerToBootPanAndZoomFullDuration),
			stopMoving,
			newEntityAnimationSetZoom(newConstantValue(maxZoom)),
		});

	//big reveal - turn the boot on
	dynamicCameraAnchorAnimationComponents.push_back(newEntityAnimationDelay(outroPreTurnOnPauseDuration));
	int bootTurnOnTime = EntityAnimation::getComponentTotalTicksDuration(dynamicCameraAnchorAnimationComponents);
	dynamicCameraAnchorAnimationComponents.insert(
		dynamicCameraAnchorAnimationComponents.end(),
		{
			newEntityAnimationPlaySound(Audio::switchOnSound, 0),
			newEntityAnimationPlaySound(Audio::endGameVictorySound, 0),
			newEntityAnimationDelay(outroPostTurnOnPauseDuration),
			#ifdef STEAM
				newEntityAnimationUnlockEndGameAchievement(),
			#endif
		});
	playerAnimationComponents.insert(
		playerAnimationComponents.end(),
		{
			newEntityAnimationDelay(
				bootTurnOnTime - EntityAnimation::getComponentTotalTicksDuration(playerAnimationComponents)),
			newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerBootOnAnimation),
		});
	for (int color = 0; color < MapState::colorCount; color++)
		mapState.get()->spawnBootTurnOnWaves(
			bootTurnOnTime + (Config::ticksPerSecond * color / 6), bootX, bootY, (char)color, ticksTime);

	//finally, fade out
	dynamicCameraAnchorAnimationComponents.insert(
		dynamicCameraAnchorAnimationComponents.end(),
		{
			newEntityAnimationSetScreenOverlayColor(
				newConstantValue(0.0f),
				newConstantValue(0.0f),
				newConstantValue(0.0f),
				newLinearInterpolatedValue({
					LinearInterpolatedValue::ValueAtTime(0.0f, 0.0f) COMMA
					LinearInterpolatedValue::ValueAtTime(1.0f, outroFadeOutDuration) COMMA
				})),
			newEntityAnimationDelay(outroFadeOutDuration),
			newEntityAnimationSetScreenOverlayColor(
				newConstantValue(0.0f), newConstantValue(0.0f), newConstantValue(0.0f), newConstantValue(1.0f)),
		});

	//now that we've assembled the animations, begin them
	static constexpr int finalPauseDuration = outroPreTitlePauseDuration + outroTextFadeInTicksDuration + outroForeverDuration;
	dynamicCameraAnchorAnimationComponents.insert(
		dynamicCameraAnchorAnimationComponents.end(),
		{
			newEntityAnimationDelay(finalPauseDuration),
			//the player probably alreaty quit before this, but if not, return them to where they just were
			newEntityAnimationSetZoom(newConstantValue(1.0f)),
			newEntityAnimationSetPosition(playerX, playerY),
			newEntityAnimationSetScreenOverlayColor(
				newConstantValue(0.0f), newConstantValue(0.0f), newConstantValue(0.0f), newConstantValue(0.0f)),
			newEntityAnimationSwitchToPlayerCamera(),
		});
	dynamicCameraAnchor.get()->beginEntityAnimation(&dynamicCameraAnchorAnimationComponents, ticksTime);

	EntityAnimation::delayToEndOf(playerAnimationComponents, dynamicCameraAnchorAnimationComponents);
	playerAnimationComponents.insert(
		playerAnimationComponents.end(),
		{
			//the player probably alreaty quit before this, but if not, return them to where they just were
			newEntityAnimationSetDirection(SpriteDirection::Up),
			newEntityAnimationSetSpriteAnimation(nullptr),
		});
	playerState.get()->beginEntityAnimation(&playerAnimationComponents, ticksTime);

	Audio::fadeOutAll(3000);
}
void GameState::resetGame(int ticksTime) {
	Audio::stopAll();
	levelsUnlocked = 0;
	lastSaveTicksTime = ticksTime;
	savePerformed = false;
	gameTimeOffsetTicksDuration = 0;
	pauseState.clear();
	//discard old states before assigning new states and resetting them, that way we reuse states only if they aren't in use by
	//a previous paused state
	mapState.clear();
	mapState.set(newMapState());
	mapState.get()->resetMap();
	playerState.clear();
	playerState.set(newPlayerState(mapState.get()));
	playerState.get()->reset();
	dynamicCameraAnchor.clear();
	dynamicCameraAnchor.set(newDynamicCameraAnchor());
	tutorialFreezePlayerStartTicksTime = 0;

	beginIntroAnimation(ticksTime);
}
