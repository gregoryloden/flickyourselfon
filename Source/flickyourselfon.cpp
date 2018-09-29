#include "flickyourselfon.h"
#include <SDL.h>
#include <SDL_image.h>

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char *argv[]) {
	int initResult = SDL_Init(SDL_INIT_EVERYTHING);
	if (initResult < 0)
		return initResult;

	SDL_Window* window = SDL_CreateWindow(
		"Flick Yourself On", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 200, 150, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	SDL_Texture* player = loadImageTexture(renderer, "images/player.png");

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, player, nullptr, nullptr);
	SDL_RenderPresent(renderer);

	SDL_Event gameEvent;
	while (true) {
		if (SDL_PollEvent(&gameEvent)) {
			switch (gameEvent.type) {
				case SDL_QUIT:
					SDL_DestroyTexture(player);
					SDL_DestroyRenderer(renderer);
					SDL_DestroyWindow(window);
					SDL_Quit();
					return 0;
			}
		}
	}
}
SDL_Texture* loadImageTexture(SDL_Renderer* renderer, const char* path) {
	SDL_Surface* imageSurface = IMG_Load(path);
	SDL_Texture* image = SDL_CreateTextureFromSurface(renderer, imageSurface);
	SDL_FreeSurface(imageSurface);
	return image;
}
