#include "General/General.h"

class SpriteAnimation;
class SpriteSheet;

class SpriteRegistry {
public:
	static constexpr int playerWalkingAnimationTicksPerFrame = 250;
	static constexpr int playerSprintingAnimationTicksPerFrame = playerWalkingAnimationTicksPerFrame / 2;
	static constexpr int playerFastWalkingAnimationTicksPerFrame = playerWalkingAnimationTicksPerFrame / 4;
	static constexpr int playerKickingAnimationTicksPerFrame = 300;
	static constexpr int playerFastKickingAnimationTicksPerFrame = 250;
	static constexpr int radioWaveAnimationTicksPerFrame = 80;
	static constexpr int sparksTicksPerFrame = 50;
	static constexpr int sparksSlowTicksPerFrame = 100;

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
	static SpriteSheet* loadTiles();
public:
	//delete all the sprite sheets
	static void unloadAll();
};
