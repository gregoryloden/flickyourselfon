#include "flickyourselfon.h"
#include <time.h>
#include "Audio/Audio.h"
#include "Editor/Editor.h"
#include "GameState/CollisionRect.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/EntityState.h"
#include "GameState/GameState.h"
#include "GameState/HintState.h"
#include "GameState/KickAction.h"
#include "GameState/PauseState.h"
#include "GameState/UndoState.h"
#include "GameState/MapState/MapState.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Sprites/Text.h"
#ifdef STEAM
	#include "ThirdParty/Steam.h"
#endif
#include "Util/CircularStateQueue.h"
#include "Util/Config.h"
#include "Util/FileUtils.h"
#include "Util/Logger.h"
#include "Util/TimeUtils.h"

const int maxGameStates = 6;
SDL_Window* window = nullptr;
mutex renderThreadInitializingMutex;
bool renderThreadInitialized = false;
bool criticalError = false;

int gameMain(int argc, char* argv[]) {
	srand((unsigned int)time(nullptr));
	//initialize SDL before we do anything else, we need it to log timestamps
	int initResult = SDL_Init(SDL_INIT_EVERYTHING);
	if (initResult < 0) {
		messageBox("Error initializing SDL", MB_OK);
		return initResult;
	}
	#ifdef STEAM
		if (Steam::restartAppIfNecessary())
			return 1;
		if (!Steam::init()
				&& messageBox("Warning: unable to initialize with Steam.\nUnable to access achievements.", MB_OKCANCEL) != IDOK)
			return 1;
	#endif

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--editor") == 0 && !Editor::isActive) {
			Editor::isActive = true;
			Config::currentPixelWidth = Config::editorDefaultPixelWidth;
			Config::currentPixelHeight = Config::editorDefaultPixelHeight;
			Config::windowScreenWidth += Config::editorMarginRight;
			Config::windowScreenHeight += Config::editorMarginBottom;
			Logger::debugLogger.enableInEditor();
		}
		#ifdef STEAM
			else if (strcmp(argv[i], "--fix-achievements") == 0)
				Steam::setFixLevelEndAchievements();
		#endif
	}

	//some things need to happen before logging can begin
	#ifdef WIN32
		CreateDirectoryA(FileUtils::localAppDataDir.c_str(), nullptr);
	#endif

	#ifdef DEBUG
		ObjCounter::start();
	#endif
	Logger::debugLogger.beginLogging();
	Logger::gameplayLogger.beginLogging();
	Logger::beginMultiThreadedLogging();
	Logger::setupLogQueue("M");

	Logger::debugLogger.log("SDL set up /// Setting up window...");

	//create a window
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
	window = SDL_CreateWindow(
		GameState::titleGameName,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		(int)((float)Config::windowScreenWidth * Config::currentPixelWidth),
		(int)((float)Config::windowScreenHeight * Config::currentPixelHeight),
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

	//get the refresh rate
	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &displayMode);
	if (displayMode.refresh_rate > 0)
		Config::refreshRate = displayMode.refresh_rate;
	Logger::debugLogger.log("Window set up /// Waiting for render thread...");

	//create our state queue, but don't load the initial state yet
	//we need to wait for the the render thread to load because states store the SpriteSheets they use, which requires the
	//	sprites to have been loaded, which requires an OpenGL context, which has to be initialized on the render thread
	//once we have the state queue, we can start the render thread
	CircularStateQueue<GameState>* gameStateQueue = newCircularStateQueue(GameState, newGameState(), newGameState());
	GameState* prevGameState = gameStateQueue->getNextWritableState();

	//wait for it to load
	thread renderLoopThread(renderLoop, gameStateQueue);
	waitForOtherThread(renderThreadInitializingMutex, &renderThreadInitialized);
	if (criticalError) {
		renderLoopThread.join();
		return 1;
	}

	//now that the render thread is loaded, we'll load game world and state
	//load audio, settings, and the map first which we need before we can load game state
	Logger::debugLogger.log("Render thread loaded /// Loading game world...");
	Audio::setUp();
	Audio::loadSounds();
	Config::loadSettings();
	Audio::applyVolume();
	MapState::buildMap();
	PauseState::loadMenus();
	if (Editor::isActive)
		Editor::loadButtons();

	//load the initial game state
	Logger::debugLogger.log("Game world loaded /// Loading game state...");
	prevGameState->loadInitialState((int)SDL_GetTicks());
	gameStateQueue->finishWritingToState();
	stringstream beginGameplayMessage;
	beginGameplayMessage << "---- begin gameplay ---- ";
	TimeUtils::appendTimestamp(&beginGameplayMessage);
	Logger::gameplayLogger.logString(beginGameplayMessage.str());
	Logger::debugLogger.log("Game state loaded /// Beginning update loop");

	//begin the update loop
	int updateDelay = -1;
	int startTime = 0;
	int updateNum = 0;
	while (true) {
		//if we missed an update or haven't begun the loop, reset the update number and time
		if (updateDelay <= 0) {
			updateNum = 0;
			startTime = (int)SDL_GetTicks();
		} else
			SDL_Delay((Uint32)updateDelay);

		GameState* gameState = gameStateQueue->getNextWritableState();
		//add a new state if there isn't one we can write to and we haven't reached our limit
		if (gameState == nullptr && gameStateQueue->getStatesCount() < maxGameStates) {
			Logger::debugLogger.log("Adding additional GameState");
			gameState = newGameState();
			gameStateQueue->addWritableState(gameState);
		}

		//skip the update if we couldn't get a state to render to, leave prevGameState as it was
		if (gameState != nullptr) {
			gameState->updateWithPreviousGameState(prevGameState, (int)SDL_GetTicks());
			gameStateQueue->finishWritingToState();
			prevGameState = gameState;
			if (gameState->getShouldQuitGame())
				break;
		}

		updateNum++;
		updateDelay = Config::ticksPerSecond * updateNum / Config::updatesPerSecond - ((int)SDL_GetTicks() - startTime);
	}

	Audio::stopAudio();
	//the render thread will quit once it reaches the game state that signalled that we should quit
	renderLoopThread.join();
	Logger::gameplayLogger.log("----   end gameplay ----");

	//cleanup anything that might have run a separate thread
	delete gameStateQueue;
	ObjectPool<PlayerState>::clearPool();

	//stop the logging thread but keep the files open
	Logger::endMultiThreadedLogging();

	//cleanup
	Audio::unloadSounds();
	Audio::tearDown();
	#ifdef DEBUG
		MapState::deleteMap();
		//the order that these object pools are cleared matters since some earlier classes in this list may have retained
		//	objects from classes later in this list
		ObjectPool<MapState>::clearPool();
		ObjectPool<PauseState>::clearPool();
		ObjectPool<DynamicCameraAnchor>::clearPool();
		ObjectPool<Particle>::clearPool();
		ObjectPool<EntityAnimation>::clearPool();
		ObjectPool<EntityAnimation::SetScreenOverlayColor>::clearPool();
		ObjectPool<EntityAnimation::SetVelocity>::clearPool();
		ObjectPool<EntityAnimation::SetZoom>::clearPool();
		ObjectPool<PiecewiseValue>::clearPool();
		ObjectPool<TimeFunctionValue>::clearPool();
		ObjectPool<ConstantValue>::clearPool();
		ObjectPool<CompositeQuarticValue>::clearPool();
		ObjectPool<ExponentialValue>::clearPool();
		ObjectPool<LinearInterpolatedValue>::clearPool();
		ObjectPool<EntityAnimation::Delay>::clearPool();
		ObjectPool<EntityAnimation::SetPosition>::clearPool();
		ObjectPool<EntityAnimation::SetGhostSprite>::clearPool();
		ObjectPool<EntityAnimation::SetSpriteAnimation>::clearPool();
		ObjectPool<EntityAnimation::SetDirection>::clearPool();
		ObjectPool<EntityAnimation::SwitchToPlayerCamera>::clearPool();
		ObjectPool<EntityAnimation::MapKickSwitch>::clearPool();
		ObjectPool<EntityAnimation::MapKickResetSwitch>::clearPool();
		ObjectPool<EntityAnimation::SpawnParticle>::clearPool();
		ObjectPool<EntityAnimation::GenerateHint>::clearPool();
		ObjectPool<EntityAnimation::PlaySound>::clearPool();
		#ifdef STEAM
			ObjectPool<EntityAnimation::UnlockEndGameAchievement>::clearPool();
		#endif
		ObjectPool<CollisionRect>::clearPool();
		ObjectPool<KickAction>::clearPool();
		ObjectPool<NoOpUndoState>::clearPool();
		ObjectPool<MoveUndoState>::clearPool();
		ObjectPool<ClimbFallUndoState>::clearPool();
		ObjectPool<RideRailUndoState>::clearPool();
		ObjectPool<KickSwitchUndoState>::clearPool();
		ObjectPool<KickResetSwitchUndoState>::clearPool();
		ObjectPool<HintState>::clearPool();
		ObjectPool<HintState::PotentialLevelState>::clearPool();
		ObjCounter::end();
	#endif
	#ifdef STEAM
		Steam::shutdown();
	#endif
	Logger::gameplayLogger.endLogging();
	Logger::debugLogger.log("Game exit");
	Logger::debugLogger.endLogging();
	//end SDL after we end logging since we use SDL_GetTicks for logging
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
void renderLoop(CircularStateQueue<GameState>* gameStateQueue) {
	renderThreadInitializingMutex.lock();
	Logger::setupLogQueue("R");

	//setup opengl
	Logger::debugLogger.log("Render thread began /// Setting up OpenGL...");
	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	SDL_GL_LoadLibrary(nullptr);
	if (!Opengl::initExtensions()) {
		SDL_GL_UnloadLibrary();
		Logger::debugLogger.log("ERROR: Unable to initialize OpenGL extensions");
		messageBox("Error initializing OpenGL extensions", MB_OK);
		criticalError = true;
		renderThreadInitializingMutex.unlock();
		return;
	}
	EntityState::setupZoomFrameBuffers();
	SDL_GL_SetSwapInterval(1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	Opengl::orientRenderTarget(true);

	//load all the sprites now that our context has been created
	Logger::debugLogger.log("OpenGL set up /// Loading sprites...");
	Text::loadFont();
	SpriteRegistry::loadAll();
	renderThreadInitialized = true;
	renderThreadInitializingMutex.unlock();
	Logger::debugLogger.log("Sprites loaded /// Beginning render loop");

	//begin the render loop
	int lastWindowDisplayWidth = 0;
	int lastWindowDisplayHeight = 0;
	int minMsPerFrame = Config::ticksPerSecond / Config::refreshRate;
	int lagFrameMs = minMsPerFrame * 3 / 2;
	while (true) {
		int preRenderTicksTime = (int)SDL_GetTicks();

		//adjust the viewport so that we scale the game window with the size of the screen
		SDL_GetWindowSize(window, &Config::windowDisplayWidth, &Config::windowDisplayHeight);
		if (Config::windowDisplayWidth != lastWindowDisplayWidth || Config::windowDisplayHeight != lastWindowDisplayHeight) {
			glViewport(0, 0, (GLsizei)Config::windowDisplayWidth, (GLsizei)Config::windowDisplayHeight);
			Config::currentPixelWidth = (float)Config::windowDisplayWidth / (float)Config::windowScreenWidth;
			Config::currentPixelHeight = (float)Config::windowDisplayHeight / (float)Config::windowScreenHeight;
			lastWindowDisplayWidth = Config::windowDisplayWidth;
			lastWindowDisplayHeight = Config::windowDisplayHeight;
		}

		GameState* gameState = gameStateQueue->advanceToLastReadableState();
		if (gameState == nullptr)
			GameState::renderLoading(preRenderTicksTime);
		else
			gameState->render(preRenderTicksTime);
		glFlush();
		SDL_GL_SwapWindow(window);

		if (gameState != nullptr && gameState->getShouldQuitGame())
			break;

		int renderTime = (int)SDL_GetTicks() - preRenderTicksTime;
		if (renderTime > lagFrameMs)
			Logger::debugLogger.logString("lag frame took " + to_string(renderTime) + "ms");

		//sleep if we don't expect to render for at least 2 more milliseconds
		int remainingDelay = minMsPerFrame - renderTime;
		if (remainingDelay >= 2)
			SDL_Delay(remainingDelay);
	}
	//cleanup
	#ifdef DEBUG
		if (Editor::isActive)
			Editor::unloadButtons();
		PauseState::unloadMenus();
		SpriteRegistry::unloadAll();
		Text::unloadFont();
	#endif
	SDL_GL_UnloadLibrary();
	SDL_GL_DeleteContext(glContext);
	Logger::debugLogger.log("Render thread ended");
}
int messageBox(const char* message, UINT messageBoxType) {
	return MessageBoxA(nullptr, message, "Kick Yourself On: Initialization Error", messageBoxType);
}
void waitForOtherThread(mutex& waitMutex, bool* resumeCondition) {
	while (!*resumeCondition && !criticalError) {
		SDL_Delay(1);
		waitMutex.lock();
		waitMutex.unlock();
	}
}
