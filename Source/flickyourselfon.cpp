#include "flickyourselfon.h"
#ifdef EDITOR
	#include "Editor/Editor.h"
#endif
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/EntityState.h"
#include "GameState/GameState.h"
#include "GameState/PauseState.h"
#include "GameState/MapState.h"
#include "Util/CircularStateQueue.h"
#include "Util/Config.h"
#include "Util/Logger.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Sprites/Text.h"

SDL_Window* window = nullptr;
SDL_GLContext glContext = nullptr;
bool renderThreadReadyForUpdates = false;

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char* argv[]) {
	//initialize SDL before we do anything else, we need it to log timestamps
	int initResult = SDL_Init(SDL_INIT_EVERYTHING);
	if (initResult < 0)
		return initResult;

	#ifdef DEBUG
		ObjCounter::start();
	#endif
	Logger::beginLogging();
	Logger::beginMultiThreadedLogging();

	Logger::log("SDL set up /// Setting up window...");

	//create a window
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
	window = SDL_CreateWindow(
		"Flick Yourself On",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		(int)((float)Config::windowScreenWidth * Config::defaultPixelWidth),
		(int)((float)Config::windowScreenHeight * Config::defaultPixelHeight),
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

	//get the refresh rate
	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &displayMode);
	if (displayMode.refresh_rate > 0)
		Config::refreshRate = displayMode.refresh_rate;
	Logger::log("Window set up /// Loading game state...");

	//load the map and settings which don't depend on the render thread
	MapState::buildMap();
	Config::loadSettings();

	//create our state queue
	CircularStateQueue<GameState>* gameStateQueue = newCircularStateQueue(GameState, newGameState(), newGameState());
	//the render thread will handle updating our initial state
	gameStateQueue->finishWritingToState();
	GameState* prevGameState = gameStateQueue->getNextReadableState();
	Logger::log("Game state loaded");

	//now that our initial state is ready, start the render thread
	thread renderLoopThread (renderLoop, gameStateQueue);

	//wait for the render thread to sync up
	Logger::log("Render thread created, waiting for it to be ready for updates...");
	while (!renderThreadReadyForUpdates)
		SDL_Delay(1);
	Logger::log("Beginning update loop");

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
		if (gameState == nullptr) {
			Logger::log("Adding additional GameState");
			gameState = newGameState();
			gameStateQueue->addWritableState(gameState);
		}
		gameState->updateWithPreviousGameState(prevGameState, (int)SDL_GetTicks());
		gameStateQueue->finishWritingToState();
		prevGameState = gameState;
		if (gameState->getShouldQuitGame())
			break;

		updateNum++;
		updateDelay = Config::ticksPerSecond * updateNum / Config::updatesPerSecond - ((int)SDL_GetTicks() - startTime);
	}

	//the render thread will quit once it reaches the game state that signalled that we should quit
	renderLoopThread.join();
	//stop the logging thread but keep the file open
	Logger::endMultiThreadedLogging();

	//cleanup
	#ifdef DEBUG
		#ifdef EDITOR
			Editor::unloadButtons();
		#endif
		PauseState::unloadMenu();
		SpriteRegistry::unloadAll();
		Text::unloadFont();
		delete gameStateQueue;
		MapState::deleteMap();
		//the order that these object pools are cleared matters since some earlier classes in this list may have retained
		//	objects from classes later in this list
		ObjectPool<PlayerState>::clearPool();
		ObjectPool<MapState>::clearPool();
		ObjectPool<PauseState>::clearPool();
		ObjectPool<DynamicCameraAnchor>::clearPool();
		ObjectPool<EntityAnimation>::clearPool();
		ObjectPool<EntityAnimation::SetScreenOverlayColor>::clearPool();
		ObjectPool<EntityAnimation::SetVelocity>::clearPool();
		ObjectPool<MapState::RadioWavesState>::clearPool();
		ObjectPool<CompositeQuarticValue>::clearPool();
		ObjectPool<EntityAnimation::Delay>::clearPool();
		ObjectPool<EntityAnimation::SetPosition>::clearPool();
		ObjectPool<EntityAnimation::SetSpriteAnimation>::clearPool();
		ObjectPool<EntityAnimation::SetSpriteDirection>::clearPool();
		ObjectPool<EntityAnimation::SwitchToPlayerCamera>::clearPool();
		ObjCounter::end();
	#endif
	Logger::log("Game exit");
	Logger::endLogging();
	//end SDL after we end logging since we use SDL_GetTicks for logging
	#ifdef DEBUG
		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
	#endif

	return 0;
}
void renderLoop(CircularStateQueue<GameState>* gameStateQueue) {
	Logger::setupLoggingForRenderThread();

	//setup opengl
	Logger::log("Render thread began /// Setting up OpenGL...");
	glContext = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glOrtho(0, (GLdouble)Config::windowScreenWidth, (GLdouble)Config::windowScreenHeight, 0, -1, 1);

	//load all the sprites now that our context has been created
	Logger::log("OpenGL set up /// Loading sprites and game state...");
	Text::loadFont();
	SpriteRegistry::loadAll();
	PauseState::loadMenu();
	#ifdef EDITOR
		Editor::loadButtons();
	#endif
	//load the initial state after loading all sprites
	gameStateQueue->getNextReadableState()->loadInitialState((int)SDL_GetTicks());
	Logger::log("Sprites and game state loaded /// Beginning render loop");

	//begin the render loop
	int lastWindowWidth = 0;
	int lastWindowHeight = 0;
	const int minMsPerFrame = Config::ticksPerSecond / Config::refreshRate;
	while (true) {
		int preRenderTicksTime = (int)SDL_GetTicks();

		//since we start this thread with a readable state and we always leave the most recent state as readable,
		//	this will never be nullptr and we'll still always be looking at the newest readable state
		//even if it's the same state as before, we just interpolate animations
		GameState* gameState = gameStateQueue->advanceToLastReadableState();

		//adjust the viewport so that we scale the game window with the size of the screen
		int windowWidth = 0;
		int windowHeight = 0;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);
		if (windowWidth != lastWindowWidth || windowHeight != lastWindowHeight) {
			glViewport(0, 0, windowWidth, windowHeight);
			lastWindowWidth = windowWidth;
			lastWindowHeight = windowHeight;
			Config::currentPixelWidth = (float)lastWindowWidth / (float)Config::windowScreenWidth;
			Config::currentPixelHeight = (float)lastWindowHeight / (float)Config::windowScreenHeight;
		}

		glClearColor(Config::backgroundColorRed, Config::backgroundColorGreen, Config::backgroundColorBlue, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		gameState->render(preRenderTicksTime);
		renderThreadReadyForUpdates = true;
		glFlush();
		SDL_GL_SwapWindow(window);

		if (gameState->getShouldQuitGame())
			break;

		//sleep if we don't expect to render for at least 2 more milliseconds
		int remainingDelay = minMsPerFrame - ((int)SDL_GetTicks() - preRenderTicksTime);
		if (remainingDelay >= 2)
			SDL_Delay(remainingDelay);
	}
	Logger::log("Render thread ended");
}
