#include "flickyourselfon.h"
#include "CircularStateQueue.h"
#include "GameState.h"
#include "Logger.h"
#include "Map.h"
#include "SpriteRegistry.h"
#include "SpriteSheet.h"

SDL_Window* window = nullptr;
SDL_GLContext glContext = nullptr;

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char *argv[]) {
	#ifdef DEBUG
		ObjCounter::start();
	#endif
	Logger::beginLogging();
	Logger::beginMultiThreadedLogging();

	//initialize SDL
	int initResult = SDL_Init(SDL_INIT_EVERYTHING);
	if (initResult < 0)
		return initResult;
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

	//create a window
	window = SDL_CreateWindow(
		"Flick Yourself On",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		Config::gameScreenWidth * Config::defaultPixelWidth,
		Config::gameScreenHeight * Config::defaultPixelHeight,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

	//get the refresh rate
	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &displayMode);
	if (displayMode.refresh_rate > 0)
		Config::refreshRate = displayMode.refresh_rate;

	//create our state queue and start the render thread
	CircularStateQueue<GameState>* gameStateQueue =
		newWithArgs(CircularStateQueue<GameState>, newWithoutArgs(GameState), newWithoutArgs(GameState));
	GameState* prevGameState = gameStateQueue->getNextReadableState();
	thread renderLoopThread (renderLoop, gameStateQueue);

	//wait for the render thread to catch up
	while (gameStateQueue->getNextReadableState() != nullptr)
		SDL_Delay(1);

	//begin the update loop
	int updateDelay = -1;
	Uint32 startTime = 0;
	int updateNum = 0;
	while (true) {
		//if we missed an update or haven't begun the loop, reset the update number and time
		if (updateDelay <= 0) {
			updateNum = 0;
			startTime = SDL_GetTicks();
		} else
			SDL_Delay((Uint32)updateDelay);

		GameState* gameState = gameStateQueue->getNextWritableState();
		if (gameState == nullptr) {
			Logger::log("Adding additional GameState");
			gameState = newWithoutArgs(GameState);
			gameStateQueue->addWritableState(gameState);
		}
		gameState->updateWithPreviousGameState(prevGameState);
		gameStateQueue->finishWritingToState();
		prevGameState = gameState;
		if (gameState->getShouldQuitGame())
			break;

		updateNum++;
		updateDelay = 1000 * updateNum / Config::updatesPerSecond - (int)(SDL_GetTicks() - startTime);
	}

	//the render thread will quit once it reaches the game state that signalled that we should quit
	renderLoopThread.join();
	//stop the logging thread but keep the file open
	Logger::endMultiThreadedLogging();

	//cleanup
	#ifdef DEBUG
		delete gameStateQueue;
		SpriteRegistry::unloadAll();
		Map::deleteMap();
		ObjCounter::end();
	#endif
	Logger::endLogging();
	#ifdef DEBUG
		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
	#endif

	return 0;
}
void renderLoop(CircularStateQueue<GameState>* gameStateQueue) {
	//setup opengl
	Logger::setupLoggingForRenderThread();
	int minMsPerFrame = 1000 / Config::refreshRate;
	glContext = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glOrtho(0, (GLdouble)Config::gameScreenWidth, (GLdouble)Config::gameScreenHeight, 0, -1, 1);

	//load all the sprites now that our context has been created
	SpriteRegistry::loadAll();
	Map::buildMap();

	//begin the render loop
	int lastWindowWidth = 0;
	int lastWindowHeight = 0;
	while (true) {
		Uint32 preRenderTicks = SDL_GetTicks();

		//wait until our game state is ready to render
		GameState* gameState;
		while ((gameState = gameStateQueue->advanceToLastReadableState()) == nullptr)
			SDL_Delay(1);

		//adjust the viewport so that we scale the game window with the size of the screen
		int windowWidth = 0;
		int windowHeight = 0;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);
		if (windowWidth != lastWindowWidth || windowHeight != lastWindowHeight) {
			glViewport(0, 0, windowWidth, windowHeight);
			lastWindowWidth = windowWidth;
			lastWindowHeight = windowHeight;
		}

		glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		gameState->render();
		gameStateQueue->finishReadingFromState();
		glFlush();
		SDL_GL_SwapWindow(window);

		if (gameState->getShouldQuitGame())
			return;

		//sleep if we don't expect to render for at least 2 more milliseconds
		int remainingDelay = minMsPerFrame - (int)(SDL_GetTicks() - preRenderTicks);
		if (remainingDelay >= 2)
			SDL_Delay(remainingDelay);
	}
}
