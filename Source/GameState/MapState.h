#include "General/General.h"

class CameraAnchor;

class MapState {
public:
	static const int tileCount = 64; // tile = green / 4
	static const int tileDivisor = 256 / tileCount;
	static const int heightCount = 16; // height = blue / 16
	static const int heightDivisor = 256 / heightCount;
	static const int tileSize = 6;
	static const float speed;
	static const float diagonalSpeed;
	static const float smallDistance;
private:
	static char* tiles;
	static char* heights;
	static int width;
	static int height;

public:
	static void buildMap();
	static void deleteMap();
	static char getTile(int x, int y);
	static char getHeight(int x, int y);
	static void render(CameraAnchor* camera);
};
