#include "General/General.h"

class SpriteSheet;
class SpriteAnimation;

class SpriteRegistry {
public:
	static const int playerWalkingAnimationTicksPerFrame = 250;
	static const int playerKickingAnimationTicksPerFrame = 300;
	static const char* playerFileName;
	static const char* tilesFileName;
	static const char* radioTowerFileName;
	static const char* railsFileName;
	static const char* switchesFileName;

	static SpriteSheet* player;
	static SpriteSheet* tiles;
	static SpriteSheet* radioTower;
	static SpriteSheet* rails;
	static SpriteSheet* switches;
	static SpriteAnimation* playerWalkingAnimation;
	static SpriteAnimation* playerLegLiftAnimation;
	static SpriteAnimation* playerBootWalkingAnimation;
	static SpriteAnimation* playerKickingAnimation;

	static void loadAll();
	static void unloadAll();
};
