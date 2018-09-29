#include "flickyourselfon.h"
#include <SDL.h>
#include <SDL_image.h>
#include <thread>

SDL_Rect drawRect {0, 0, 11, 19};
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* player = nullptr;
unsigned int updatesPerSecond = 48;
bool needsNewRenderer = false;

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char *argv[]) {
	int initResult = SDL_Init(SDL_INIT_EVERYTHING);
	if (initResult < 0)
		return initResult;

	window = SDL_CreateWindow(
		"Flick Yourself On", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 200, 150, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	player = loadImageTexture(renderer, "images/player.png");
	std::thread renderLoopThread (renderLoop);

	unsigned int startTime = SDL_GetTicks();
	unsigned int stateNum = 0;
	SDL_Event gameEvent;
	while (true) {
		if (SDL_PollEvent(&gameEvent)) {
			switch (gameEvent.type) {
				case SDL_QUIT:
					SDL_DestroyTexture(player);
					SDL_DestroyRenderer(renderer);
					SDL_DestroyWindow(window);
					SDL_Quit();
					std::exit(0);
				case SDL_MOUSEBUTTONDOWN:
					drawRect.x = (drawRect.x + drawRect.w) % 99;
					break;
			}
		}
		stateNum++;
		if (stateNum % (updatesPerSecond / 4) == 0)
			drawRect.y = (drawRect.y + drawRect.h) % 76;
		SDL_Delay(1000 * stateNum / updatesPerSecond - (SDL_GetTicks() - startTime));
	}
}
SDL_Texture* loadImageTexture(SDL_Renderer* renderer, const char* path) {
	SDL_Surface* imageSurface = IMG_Load(path);
	SDL_Texture* image = SDL_CreateTextureFromSurface(renderer, imageSurface);
	SDL_FreeSurface(imageSurface);
	return image;
}
void renderLoop() {
	while (true) {
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, player, &drawRect, &drawRect);
		SDL_RenderPresent(renderer);
	}
}
