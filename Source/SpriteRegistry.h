#include "General.h"

class SpriteSheet;

class SpriteRegistry {
public:
	static SpriteSheet* player;

	static void loadAll();
	static void unloadAll();
};
