#include "MapState.h"
#include "Editor/Editor.h"
#include "GameState/EntityState.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

const char* MapState::floorFileName = "images/floor.png";
const float MapState::speedPerSecond = 40.0f;
const float MapState::diagonalSpeedPerSecond = MapState::speedPerSecond * sqrt(0.5f);
const float MapState::smallDistance = 1.0f / 256.0f;
char* MapState::tiles = nullptr;
char* MapState::heights = nullptr;
int MapState::width = 1;
int MapState::height = 1;
//load the map and extract all the map data from it
void MapState::buildMap() {
	SDL_Surface* floor = IMG_Load(floorFileName);
	width = floor->w;
	height = floor->h;
	int totalTiles = width * height;
	tiles = new char[totalTiles];
	heights = new char[totalTiles];

	int greenShift = (int)floor->format->Gshift;
	int greenMask = (int)floor->format->Gmask;
	int blueShift = (int)floor->format->Bshift;
	int blueMask = (int)floor->format->Bmask;
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
//get the world position of the left edge of the screen using the camera as the center of the screen
int MapState::getScreenLeftWorldX(EntityState* camera, int ticksTime) {
	//we convert the camera center to int first because with a position with 0.5 offsets, we render all pixels aligned (because
	//	the player width is odd); once we get to a .0 position, then we render one pixel over
	return (int)(camera->getRenderCenterWorldX(ticksTime)) - Config::gameScreenWidth / 2;
}
//get the world position of the top edge of the screen using the camera as the center of the screen
int MapState::getScreenTopWorldY(EntityState* camera, int ticksTime) {
	//(see getScreenLeftWorldX)
	return (int)(camera->getRenderCenterWorldY(ticksTime)) - Config::gameScreenHeight / 2;
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
	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	int tileMinX = MathUtils::max(screenLeftWorldX / tileSize, 0);
	int tileMinY = MathUtils::max(screenTopWorldY / tileSize, 0);
	int tileMaxX = MathUtils::min((Config::gameScreenWidth + screenLeftWorldX - 1) / tileSize + 1, width);
	int tileMaxY = MathUtils::min((Config::gameScreenHeight + screenTopWorldY - 1) / tileSize + 1, height);
	#ifdef EDITOR
		char editorSelectedHeight = Editor::getSelectedHeight();
	#endif
	for (int y = tileMinY; y < tileMaxY; y++) {
		for (int x = tileMinX; x < tileMaxX; x++) {
			//consider any tile at the max height to be filler
			int mapIndex = y * width + x;
			char mapHeight = heights[mapIndex];
			if (mapHeight == emptySpaceHeight)
				continue;

			GLint leftX = (GLint)(x * tileSize - screenLeftWorldX);
			GLint topY = (GLint)(y * tileSize - screenTopWorldY);
			SpriteRegistry::tiles->renderSpriteAtScreenPosition((int)(tiles[mapIndex]), 0, leftX, topY);
			#ifdef EDITOR
				if (editorSelectedHeight != -1 && editorSelectedHeight != mapHeight)
					SpriteSheet::renderFilledRectangle(
						0.0f, 0.0f, 0.0f, 0.5f, leftX, topY, leftX + (GLint)tileSize, topY + (GLint)tileSize);
			#endif
		}
	}

	//draw the radio tower after drawing the map
	glEnable(GL_BLEND);
	SpriteRegistry::radioTower->renderSpriteAtScreenPosition(
		0, 0, (GLint)(radioTowerLeftXOffset - screenLeftWorldX), (GLint)(radioTowerTopYOffset - screenTopWorldY));
}
