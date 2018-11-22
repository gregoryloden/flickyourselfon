#include "flickyourselfon.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/GameState.h"
#include "GameState/MapState.h"
#include "Util/CircularStateQueue.h"
#include "Util/Logger.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

SDL_Window* window = nullptr;
SDL_GLContext glContext = nullptr;
bool renderThreadBegan = false;

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
	//our initial state is renderable
	gameStateQueue->finishWritingToState();
	GameState* prevGameState = gameStateQueue->getNextReadableState();
	thread renderLoopThread (renderLoop, gameStateQueue);

	//wait for the render thread to sync up
	while (!renderThreadBegan)
		SDL_Delay(1);

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
			gameState = newWithoutArgs(GameState);
			gameStateQueue->addWritableState(gameState);
		}
		gameState->updateWithPreviousGameState(prevGameState);
		gameStateQueue->finishWritingToState();
		prevGameState = gameState;
		if (gameState->getShouldQuitGame())
			break;

		updateNum++;
		updateDelay = 1000 * updateNum / Config::updatesPerSecond - ((int)SDL_GetTicks() - startTime);
	}

	//the render thread will quit once it reaches the game state that signalled that we should quit
	renderLoopThread.join();
	//stop the logging thread but keep the file open
	Logger::endMultiThreadedLogging();

	//cleanup
	#ifdef DEBUG
		delete gameStateQueue;
		SpriteRegistry::unloadAll();
		MapState::deleteMap();
		ObjectPool<CompositeQuarticValue>::clearPool();
		ObjectPool<EntityAnimation>::clearPool();
		ObjectPool<EntityAnimation::Delay>::clearPool();
		ObjectPool<EntityAnimation::SetVelocity>::clearPool();
		ObjectPool<EntityAnimation::SetSpriteAnimation>::clearPool();
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
	MapState::buildMap();

	//begin the render loop
	int lastWindowWidth = 0;
	int lastWindowHeight = 0;
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
		}

//TODO: settle on final background color
glClearColor(0.1875f, 0.0f, 0.1875f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		gameState->render(preRenderTicksTime);
		renderThreadBegan = true;
		glFlush();
		SDL_GL_SwapWindow(window);

		if (gameState->getShouldQuitGame())
			return;

		//sleep if we don't expect to render for at least 2 more milliseconds
		int remainingDelay = minMsPerFrame - ((int)SDL_GetTicks() - preRenderTicksTime);
		if (remainingDelay >= 2)
			SDL_Delay(remainingDelay);
	}
}
