#include "General/General.h"

class SpriteAnimation;
class SpriteSheet;

class SpriteRegistry {
public:
	static const int playerWalkingAnimationTicksPerFrame = 250;
	static const int playerKickingAnimationTicksPerFrame = 300;
	static const int playerFastKickingAnimationTicksPerFrame = 250;
	static const int radioWaveAnimationTicksPerFrame = 80;
	static const char* playerFileName;
	static const char* tilesFileName;
	static const char* radioTowerFileName;
	static const char* railsFileName;
	static const char* switchesFileName;
	static const char* radioWavesFileName;
	static const char* railWavesFileName;
	static const char* switchWavesFileName;
	static const char* resetSwitchFileName;
	static const char* kickIndicatorFileName;

	static SpriteSheet* player;
	static SpriteSheet* tiles;
	static SpriteSheet* radioTower;
	static SpriteSheet* rails;
	static SpriteSheet* switches;
	static SpriteSheet* radioWaves;
	static SpriteSheet* railWaves;
	static SpriteSheet* switchWaves;
	static SpriteSheet* resetSwitch;
	static SpriteSheet* kickIndicator;
	static SpriteAnimation* playerWalkingAnimation;
	static SpriteAnimation* playerLegLiftAnimation;
	static SpriteAnimation* playerBootWalkingAnimation;
	static SpriteAnimation* playerKickingAnimation;
	static SpriteAnimation* playerFastKickingAnimation;
	static SpriteAnimation* playerBootLiftAnimation;
	static SpriteAnimation* playerRidingRailAnimation;
	static SpriteAnimation* radioWavesAnimation;
	static SpriteAnimation* railWavesAnimation;
	static SpriteAnimation* switchWavesAnimation;

	//load all the sprite sheets
	//this should only be called after the gl context has been created
	static void loadAll();
	//delete all the sprite sheets
	static void unloadAll();
};
