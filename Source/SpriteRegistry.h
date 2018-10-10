#include "General.h"

class SpriteSheet;

class SpriteRegistry {
public:
	static SpriteSheet* player;
	static SpriteSheet* tiles;
	static SpriteSheet* radioTower;
	static SpriteSheet* font;

	static void loadAll();
	static void unloadAll();
};
