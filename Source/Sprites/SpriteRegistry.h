#include "General/General.h"

class SpriteAnimation;
class SpriteSheet;

class SpriteRegistry {
public:
	static const int playerWalkingAnimationTicksPerFrame = 250;
	static const int playerKickingAnimationTicksPerFrame = 300;
	static const int playerFastKickingAnimationTicksPerFrame = 250;
	static const int radioWaveAnimationTicksPerFrame = 80;
	static constexpr char* playerFileName = "player.png";
	static constexpr char* tilesFileName = "tiles.png";
	static constexpr char* radioTowerFileName = "radiotower.png";
	static constexpr char* railsFileName = "rails.png";
	static constexpr char* switchesFileName = "switches.png";
	static constexpr char* radioWavesFileName = "radiowaves.png";
	static constexpr char* railWavesFileName = "railwaves.png";
	static constexpr char* switchWavesFileName = "switchwaves.png";
	static constexpr char* resetSwitchFileName = "resetswitch.png";
	static constexpr char* kickIndicatorFileName = "kickindicator.png";

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
	static SpriteAnimation* switchWavesShortAnimation;

	//load all the sprite sheets
	//this should only be called after the gl context has been created
	static void loadAll();
	//delete all the sprite sheets
	static void unloadAll();
};
