#include "General/General.h"

class EntityState;

class MapState {
public:
	static const int tileCount = 64; // tile = green / 4
	static const int tileDivisor = 256 / tileCount;
	static const int heightCount = 16; // height = blue / 16
	static const int heightDivisor = 256 / heightCount;
	static const int emptySpaceHeight = heightCount - 1;
	static const int tileSize = 6;
	static const char invalidHeight = -1;
	static const char* floorFileName;
	static const float speedPerSecond;
	static const float diagonalSpeedPerSecond;
	static const float smallDistance;
private:
	static char* tiles;
	static char* heights;
	static int width;
	static int height;

public:
	static char getTile(int x, int y) { return tiles[y * width + x]; }
	static char getHeight(int x, int y) { return heights[y * width + x]; }
	static int mapWidth() { return width; }
	static int mapHeight() { return height; }
	static void buildMap();
	static void deleteMap();
	static char horizontalTilesHeight(int lowMapX, int highMapX, int mapY);
	static void render(EntityState* camera, int ticksTime);
};
