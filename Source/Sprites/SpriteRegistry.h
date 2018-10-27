#include "General/General.h"

class SpriteSheet;
class SpriteAnimation;

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
	static SpriteAnimation* playerWalkingAnimation;
	static SpriteAnimation* playerBootWalkingAnimation;
	static SpriteSheet* tiles;
	static SpriteSheet* radioTower;
	static SpriteSheet* font;

	static void loadAll();
	static void unloadAll();
};
