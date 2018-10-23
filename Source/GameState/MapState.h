#include "General/General.h"

class Map {
public:
	static const int tileCount = 64; // tile = green / 4
	static const int tileDivisor = 256 / tileCount;
	static const int heightCount = 16; // height = blue / 16
	static const int heightDivisor = 256 / heightCount;
	static const int tileSize = 6;
	static const float speed;
	static const float diagonalSpeed;

	static unsigned char* tiles;
	static unsigned char* heights;
	static int width;
	static int height;

	static void buildMap();
	static void deleteMap();
};
