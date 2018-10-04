#include "flickyourselfon.h"
#include "CircularStateQueue.h"
#include "GameState.h"
#include "SpriteRegistry.h"
#include "SpriteSheet.h"
#include <thread>

int refreshRate = 60;
SDL_Window* window = nullptr;
SDL_GLContext glContext = nullptr;
int updatesPerSecond = 48;
int gameScreenWidth = 200;
int gameScreenHeight = 150;
float initialPixelXScale = 3.0f;
float initialPixelYScale = 3.0f;

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char *argv[]) {
	#ifdef DEBUG
		ObjCounter::start();
	#endif

	int initResult = SDL_Init(SDL_INIT_EVERYTHING);
	if (initResult < 0)
		return initResult;
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

	window = SDL_CreateWindow(
		"Flick Yourself On",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		(int)(gameScreenWidth * initialPixelXScale),
		(int)(gameScreenHeight * initialPixelYScale),
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &displayMode);
	if (displayMode.refresh_rate > 0)
		refreshRate = displayMode.refresh_rate;

	CircularStateQueue<GameState>* gameStateQueue =
		newTracked(CircularStateQueue<GameState>, (newTracked(GameState, ()), newTracked(GameState, ())));
	GameState* prevGameState = gameStateQueue->getNextReadableState();
	std::thread renderLoopThread (renderLoop, gameStateQueue);

	//wait for the render thread to catch up
	while (gameStateQueue->getNextReadableState() != nullptr)
		SDL_Delay(1);

	int updateDelay = -1;
	Uint32 startTime = 0;
	int updateNum = 0;
	while (true) {
		// If we missed an update or haven't begun the loop, reset the update number and time
		if (updateDelay <= 0) {
			updateNum = 0;
			startTime = SDL_GetTicks();
		} else
			SDL_Delay((Uint32)updateDelay);

		GameState* gameState = gameStateQueue->getNextWritableState();
		if (gameState == nullptr) {
			gameState = newTracked(GameState, ());
			gameStateQueue->addWritableState(gameState);
		}
		gameState->updateWithPreviousGameState(prevGameState);
		gameStateQueue->finishWritingToState();
		prevGameState = gameState;
		if (gameState->getShouldQuitGame())
			break;

		updateNum++;
		updateDelay = 1000 * updateNum / updatesPerSecond - (int)(SDL_GetTicks() - startTime);
	}

	//the render thread will quit once it reaches the game state that signalled that we should quit
	renderLoopThread.join();

	#ifdef DEBUG
		delete gameStateQueue;
		delete SpriteRegistry::player;
		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
		ObjCounter::end();
	#endif

	return 0;
}
void renderLoop(CircularStateQueue<GameState>* gameStateQueue) {
	int minMsPerFrame = 1000 / refreshRate;
	glContext = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glOrtho(0, (GLdouble)gameScreenWidth, (GLdouble)gameScreenHeight, 0, -1, 1);

	SpriteRegistry::player = newTracked(SpriteSheet, ("images/player.png", 11, 19));

	int lastWindowWidth = 0;
	int lastWindowHeight = 0;
	while (true) {
		Uint32 preRenderTicks = SDL_GetTicks();

		//wait until our game state is ready to render
		GameState* gameState;
		while ((gameState = gameStateQueue->advanceToLastReadableState()) == nullptr)
			SDL_Delay(1);

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
