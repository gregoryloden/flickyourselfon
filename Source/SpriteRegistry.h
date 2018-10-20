#include "General.h"

class SpriteSheet;
class Animation;

enum class PlayerSpriteDirection: int {
	Right = 0,
	Up = 1,
	Left = 2,
	Down = 3
};
class SpriteRegistry {
public:
	static const int playerWalkingAnimationTicksPerFrame = 250;

	static SpriteSheet* player;
	static Animation* playerWalkingAnimation;
	static SpriteSheet* tiles;
	static SpriteSheet* radioTower;
	static SpriteSheet* font;

	static void loadAll();
	static void unloadAll();
};
