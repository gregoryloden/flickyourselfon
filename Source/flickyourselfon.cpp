#include "flickyourselfon.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <gl\GL.h>
#include <thread>

int refreshRate = 60;
int currentSpriteHorizontalIndex = 0;
int currentSpriteVerticalIndex = 0;
int currentAnimationFrame = 0;
SDL_Window* window = nullptr;
SDL_GLContext glContext = nullptr;
int updatesPerSecond = 48;
int gameScreenWidth = 200;
int gameScreenHeight = 150;
float initialPixelXScale = 3.0f;
float initialPixelYScale = 3.0f;

class SpriteSheet {
public:
	GLuint textureId;
	int spriteWidth;
	int spriteHeight;
	float spriteSheetWidth;
	float spriteSheetHeight;
	SpriteSheet(const char* imagePath, int pSpriteWidth, int pSpriteHeight)
	: textureId(0)
	, spriteWidth(pSpriteWidth)
	, spriteHeight(pSpriteHeight)
	, spriteSheetWidth(1.0f)
	, spriteSheetHeight(1.0f) {
		glGenTextures(1, &textureId);

		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		SDL_Surface* surface = IMG_Load(imagePath);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
		spriteSheetWidth = (float)(surface->w);
		spriteSheetHeight = (float)(surface->h);
		SDL_FreeSurface(surface);
	}
	~SpriteSheet() {}
	void render(int spriteHorizontalIndex, int spriteVerticalIndex) {
		float minX = (float)(spriteHorizontalIndex * spriteWidth);
		float minY = (float)(spriteVerticalIndex * spriteHeight);
		float maxX = minX + (float)(spriteWidth);
		float maxY = minY + (float)(spriteHeight);
		float texMinX = minX / spriteSheetWidth;
		float texMinY = minY / spriteSheetHeight;
		float texMaxX = maxX / spriteSheetWidth;
		float texMaxY = maxY / spriteSheetHeight;

		glBindTexture(GL_TEXTURE_2D, textureId);
        glBegin(GL_QUADS);
        glTexCoord2f(texMinX, texMinY);
        glVertex2f(minX, minY);
        glTexCoord2f(texMaxX, texMinY);
        glVertex2f(maxX, minY);
        glTexCoord2f(texMaxX, texMaxY);
        glVertex2f(maxX, maxY);
        glTexCoord2f(texMinX, texMaxY);
        glVertex2f(minX, maxY);
        glEnd();
	}
};
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
		(int)(gameScreenWidth * initialPixelXScale),
		(int)(gameScreenHeight * initialPixelYScale),
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &displayMode);
	if (displayMode.refresh_rate > 0)
		refreshRate = displayMode.refresh_rate;
	std::thread renderLoopThread (renderLoop);

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
		update();
		updateNum++;
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
	glOrtho(0, (GLdouble)gameScreenWidth, (GLdouble)gameScreenHeight, 0, -1, 1);

	SpriteSheet player ("images/player.png", 11, 19);

	int lastWindowWidth = 0;
	int lastWindowHeight = 0;
	while (true) {
		Uint32 preRenderTicks = SDL_GetTicks();

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
		player.render(currentSpriteHorizontalIndex, currentSpriteVerticalIndex);
        glFlush();
		SDL_GL_SwapWindow(window);

		//sleep if we don't expect to render for at least 2 more milliseconds
		int remainingDelay = minMsPerFrame - (int)(SDL_GetTicks() - preRenderTicks);
		if (remainingDelay >= 2)
			SDL_Delay(remainingDelay);
	}
}
void update() {
	SDL_Event gameEvent;
	while (SDL_PollEvent(&gameEvent) != 0) {
		switch (gameEvent.type) {
			case SDL_QUIT:
				SDL_GL_DeleteContext(glContext);
				SDL_DestroyWindow(window);
				SDL_Quit();
				std::exit(0);
			case SDL_MOUSEBUTTONDOWN:
				currentSpriteHorizontalIndex = (currentSpriteHorizontalIndex + 1) % 9;
				break;
			default:
				break;
		}
	}
	if (currentAnimationFrame == (updatesPerSecond / 4)) {
		currentSpriteVerticalIndex = (currentSpriteVerticalIndex + 1) % 4;
		currentAnimationFrame = 1;
	} else
		currentAnimationFrame++;
}
