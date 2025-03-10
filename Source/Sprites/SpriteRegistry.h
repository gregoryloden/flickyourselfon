#include "General/General.h"

class SpriteAnimation;
class SpriteSheet;

class SpriteRegistry {
public:
	static const int playerWalkingAnimationTicksPerFrame = 250;
	static const int playerSprintingAnimationTicksPerFrame = playerWalkingAnimationTicksPerFrame / 2;
	static const int playerFastWalkingAnimationTicksPerFrame = playerWalkingAnimationTicksPerFrame / 4;
	static const int playerKickingAnimationTicksPerFrame = 300;
	static const int playerFastKickingAnimationTicksPerFrame = 250;
	static const int radioWaveAnimationTicksPerFrame = 80;
	static const int sparksTicksPerFrame = 50;
	static const int sparksSlowTicksPerFrame = 100;
	static constexpr char* playerFileName = "player.png";
	static constexpr char* tilesFileName = "tiles.png";
	static constexpr char* tileBordersFileName = "tileborders.png";
	static constexpr char* radioTowerFileName = "radiotower.png";
	static constexpr char* railsFileName = "rails.png";
	static constexpr char* switchesFileName = "switches.png";
	static constexpr char* radioWavesFileName = "radiowaves.png";
	static constexpr char* railWavesFileName = "railwaves.png";
	static constexpr char* switchWavesFileName = "switchwaves.png";
	static constexpr char* resetSwitchFileName = "resetswitch.png";
	static constexpr char* kickIndicatorFileName = "kickindicator.png";
	static constexpr char* sparksFileName = "sparks.png";
	static constexpr char* borderArrowsFileName = "borderarrows.png";
	static constexpr char* wavesActivatedFileName = "wavesactivated.png";
	static constexpr char* saveFileName = "save.png";

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
	static SpriteSheet* sparks;
	static SpriteSheet* borderArrows;
	static SpriteSheet* wavesActivated;
	static SpriteSheet* save;
	static SpriteAnimation* playerWalkingAnimation;
	static SpriteAnimation* playerLegLiftAnimation;
	static SpriteAnimation* playerBootWalkingAnimation;
	static SpriteAnimation* playerBootSprintingAnimation;
	static SpriteAnimation* playerFastBootWalkingAnimation;
	static SpriteAnimation* playerKickingAnimation;
	static SpriteAnimation* playerFastKickingAnimation;
	static SpriteAnimation* playerBootLiftAnimation;
	static SpriteAnimation* playerRidingRailAnimation;
	static SpriteAnimation* radioWavesAnimation;
	static SpriteAnimation* railWavesAnimation;
	static SpriteAnimation* switchWavesAnimation;
	static SpriteAnimation* switchWavesShortAnimation;
	static SpriteAnimation* sparksAnimationA;
	static SpriteAnimation* sparksAnimationB;
	static SpriteAnimation* sparksSlowAnimationA;
	static SpriteAnimation* sparksSlowAnimationB;

	//Prevent allocation
	SpriteRegistry() = delete;
	//load all the sprite sheets
	//this should only be called after the gl context has been created
	static void loadAll();
private:
	//load the tiles sprite sheet, overlaying the border image onto it
	static void loadTiles();
public:
	//delete all the sprite sheets
	static void unloadAll();
};
