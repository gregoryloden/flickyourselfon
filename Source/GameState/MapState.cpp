#include "MapState.h"
#include "GameState/EntityState.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

const float MapState::speedPerSecond = 40.0f;
const float MapState::diagonalSpeedPerSecond = MapState::speedPerSecond * sqrt(0.5f);
const float MapState::smallDistance = 1.0f / 256.0f;
char* MapState::tiles = nullptr;
char* MapState::heights = nullptr;
int MapState::width = 1;
int MapState::height = 1;
//load the map and extract all the map data from it
void MapState::buildMap() {
	SDL_Surface* floor = IMG_Load("images/floor.png");
	width = floor->w;
	height = floor->h;
	int totalTiles = width * height;
	tiles = new char[totalTiles];
	heights = new char[totalTiles];

	int greenShift = floor->format->Gshift;
	int greenMask = floor->format->Gmask;
	int blueShift = floor->format->Bshift;
	int blueMask = floor->format->Bmask;
	int* pixels = static_cast<int*>(floor->pixels);

	for (int i = 0; i < totalTiles; i++) {
		tiles[i] = (char)(((pixels[i] & greenMask) >> greenShift) / tileDivisor);
		heights[i] = (char)(((pixels[i] & blueMask) >> blueShift) / heightDivisor);
	}

	SDL_FreeSurface(floor);
}
//delete the resources used to handle the map
void MapState::deleteMap() {
	delete[] tiles;
	delete[] heights;
}
//check the height of all the tiles in the row, and return it if they're all the same or -1 if they differ
char MapState::horizontalTilesHeight(int lowMapX, int highMapX, int mapY) {
	char foundHeight = getHeight(lowMapX, mapY);
	for (int mapX = lowMapX + 1; mapX <= highMapX; mapX++) {
		if (getHeight(mapX, mapY) != foundHeight)
			return invalidHeight;
	}
	return foundHeight;
}
//draw the map
void MapState::render(EntityState* camera, int ticksTime) {
	glDisable(GL_BLEND);
	//render the map
	//these values are just right so that every tile rendered is at least partially in the window and no tiles are left out
	int centerX = (int)(camera->getRenderCenterX(ticksTime));
	int centerY = (int)(camera->getRenderCenterY(ticksTime));
	int addX = Config::gameScreenWidth / 2 - centerX;
	int addY = Config::gameScreenHeight / 2 - centerY;
	int tileMinX = FYOMath::max(-addX / MapState::tileSize, 0);
	int tileMinY = FYOMath::max(-addY / MapState::tileSize, 0);
	int tileMaxX = FYOMath::min((Config::gameScreenWidth - addX - 1) / MapState::tileSize + 1, MapState::width);
	int tileMaxY = FYOMath::min((Config::gameScreenHeight - addY - 1) / MapState::tileSize + 1, MapState::height);
	for (int y = tileMinY; y < tileMaxY; y++) {
		for (int x = tileMinX; x < tileMaxX; x++) {
			//consider any tile at the max height to be filler
			int mapIndex = y * MapState::width + x;
			if (MapState::heights[mapIndex] != MapState::heightCount - 1)
				SpriteRegistry::tiles->render(
					x * MapState::tileSize + addX, y * MapState::tileSize + addY, (int)(MapState::tiles[mapIndex]), 0);
		}
	}

	glEnable(GL_BLEND);
	SpriteRegistry::radioTower->render(423 - centerX, -32 - centerY, 0, 0);
}
