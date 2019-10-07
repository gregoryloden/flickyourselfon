#include "General/General.h"

class SpriteAnimation;
class SpriteSheet;

class SpriteRegistry {
public:
	static const int playerWalkingAnimationTicksPerFrame = 250;
	static const int playerKickingAnimationTicksPerFrame = 300;
	static const int radioWaveAnimationTicksPerFrame = 80;
	static const char* playerFileName;
	static const char* tilesFileName;
	static const char* radioTowerFileName;
	static const char* railsFileName;
	static const char* switchesFileName;
	static const char* radioWavesFileName;

	static SpriteSheet* player;
	static SpriteSheet* tiles;
	static SpriteSheet* radioTower;
	static SpriteSheet* rails;
	static SpriteSheet* switches;
	static SpriteSheet* radioWaves;
	static SpriteAnimation* playerWalkingAnimation;
	static SpriteAnimation* playerLegLiftAnimation;
	static SpriteAnimation* playerBootWalkingAnimation;
	static SpriteAnimation* playerKickingAnimation;
	static SpriteAnimation* playerBootLiftAnimation;
	static SpriteAnimation* playerRidingRailAnimation;
	static SpriteAnimation* radioWavesAnimation;

	static void loadAll();
	static void unloadAll();
};
