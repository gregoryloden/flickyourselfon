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
	static const int playerKickingAnimationTicksPerFrame = 300;

	static SpriteSheet* player;
	static SpriteSheet* tiles;
	static SpriteSheet* radioTower;
	static SpriteAnimation* playerWalkingAnimation;
	static SpriteAnimation* playerLegLiftAnimation;
	static SpriteAnimation* playerBootWalkingAnimation;
	static SpriteAnimation* playerKickingAnimation;

	static void loadAll();
	static void unloadAll();
};
