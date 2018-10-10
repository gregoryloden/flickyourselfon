#include "Map.h"

unsigned char* Map::tiles = nullptr;
unsigned char* Map::heights = nullptr;
int Map::width = 1;
int Map::height = 1;
//load the map and extract all the map data from it
void Map::buildMap() {
	SDL_Surface* floor = IMG_Load("images/floor.png");
	width = floor->w;
	height = floor->h;
	int totalTiles = width * height;
	tiles = new unsigned char[totalTiles];
	heights = new unsigned char[totalTiles];

	int greenShift = floor->format->Gshift;
	int greenMask = floor->format->Gmask;
	int blueShift = floor->format->Bshift;
	int blueMask = floor->format->Bmask;
	int* pixels = static_cast<int*>(floor->pixels);

	for (int i = 0; i < totalTiles; i++) {
		tiles[i] = (unsigned char)(((pixels[i] & greenMask) >> greenShift) / tileDivisor);
		heights[i] = (unsigned char)(((pixels[i] & blueMask) >> blueShift) / heightDivisor);
	}

	SDL_FreeSurface(floor);
}
//delete the resources used to handle the map
void Map::deleteMap() {
	delete[] tiles;
	delete[] heights;
}
