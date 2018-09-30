#include "flickyourselfon.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <gl\GL.h>
#include <thread>

int refreshRate = 60;
SDL_Rect drawRect {0, 0, 11, 19};
SDL_Window* window = nullptr;
SDL_GLContext glContext = nullptr;
int updatesPerSecond = 48;

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char *argv[]) {
	int initResult = SDL_Init(SDL_INIT_EVERYTHING);
	if (initResult < 0)
		return initResult;
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

	window = SDL_CreateWindow(
		"Flick Yourself On",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		200,
		150,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &displayMode);
	if (displayMode.refresh_rate > 0)
		refreshRate = displayMode.refresh_rate;
	std::thread renderLoopThread (renderLoop);

	SDL_Event gameEvent;
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
		if (SDL_PollEvent(&gameEvent) != 0) {
			switch (gameEvent.type) {
				case SDL_QUIT:
					SDL_GL_DeleteContext(glContext);
					SDL_DestroyWindow(window);
					SDL_Quit();
					std::exit(0);
				case SDL_MOUSEBUTTONDOWN:
					drawRect.x = (drawRect.x + drawRect.w) % 99;
					break;
				default:
					break;
			}
		}
		updateNum++;
		if (updateNum % (updatesPerSecond / 4) == 0)
			drawRect.y = (drawRect.y + drawRect.h) % 76;
		updateDelay = 1000 * updateNum / updatesPerSecond - (int)(SDL_GetTicks() - startTime);
	}
}
void renderLoop() {
	int minMsPerFrame = 1000 / refreshRate;
	glContext = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glOrtho(0, 200, 150, 0, -1, 1);

	SDL_Surface* playerSurface = IMG_Load("images/player.png");
	GLuint playerTexture;
	glGenTextures(1, &playerTexture);
	glBindTexture(GL_TEXTURE_2D, playerTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA, playerSurface->w, playerSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, playerSurface->pixels);
	SDL_FreeSurface(playerSurface);

	while (true) {
		int windowWidth = 0;
		int windowHeight = 0;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);
		Uint32 preRenderTicks = SDL_GetTicks();

		glViewport(0, 0, windowWidth, windowHeight);
		glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, playerTexture);
        glBegin(GL_QUADS);
		float minX = (float)(drawRect.x);
		float minY = (float)(drawRect.y);
		float maxX = (float)(drawRect.x + drawRect.w);
		float maxY = (float)(drawRect.y + drawRect.h);
		float texMinX = minX / 99.0f;
		float texMinY = minY / 76.0f;
		float texMaxX = maxX / 99.0f;
		float texMaxY = maxY / 76.0f;
        glTexCoord2f(texMinX, texMinY);
        glVertex2f(minX, minY);
        glTexCoord2f(texMaxX, texMinY);
        glVertex2f(maxX, minY);
        glTexCoord2f(texMaxX, texMaxY);
        glVertex2f(maxX, maxY);
        glTexCoord2f(texMinX, texMaxY);
        glVertex2f(minX, maxY);
        glEnd();
        glFlush();
		SDL_GL_SwapWindow(window);

		//sleep if we don't expect to render for at least 2 more milliseconds
		int remainingDelay = minMsPerFrame - (int)(SDL_GetTicks() - preRenderTicks);
		if (remainingDelay >= 2)
			SDL_Delay(remainingDelay);
	}
}
