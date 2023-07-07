#include "SpriteRegistry.h"
#include "GameState/MapState/MapState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteSheet.h"
#include "Util/FileUtils.h"

const char* SpriteRegistry::playerFileName = "player.png";
const char* SpriteRegistry::tilesFileName = "tiles.png";
const char* SpriteRegistry::radioTowerFileName = "radiotower.png";
const char* SpriteRegistry::railsFileName = "rails.png";
const char* SpriteRegistry::switchesFileName = "switches.png";
const char* SpriteRegistry::radioWavesFileName = "radiowaves.png";
const char* SpriteRegistry::railWavesFileName = "railwaves.png";
const char* SpriteRegistry::switchWavesFileName = "switchwaves.png";
const char* SpriteRegistry::resetSwitchFileName = "resetswitch.png";
const char* SpriteRegistry::kickIndicatorFileName = "kickindicator.png";
SpriteSheet* SpriteRegistry::player = nullptr;
SpriteSheet* SpriteRegistry::tiles = nullptr;
SpriteSheet* SpriteRegistry::radioTower = nullptr;
SpriteSheet* SpriteRegistry::rails = nullptr;
SpriteSheet* SpriteRegistry::switches = nullptr;
SpriteSheet* SpriteRegistry::radioWaves = nullptr;
SpriteSheet* SpriteRegistry::railWaves = nullptr;
SpriteSheet* SpriteRegistry::switchWaves = nullptr;
SpriteSheet* SpriteRegistry::resetSwitch = nullptr;
SpriteSheet* SpriteRegistry::kickIndicator = nullptr;
SpriteAnimation* SpriteRegistry::playerWalkingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerLegLiftAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerBootWalkingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerKickingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerFastKickingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerBootLiftAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerRidingRailAnimation = nullptr;
SpriteAnimation* SpriteRegistry::radioWavesAnimation = nullptr;
SpriteAnimation* SpriteRegistry::railWavesAnimation = nullptr;
SpriteAnimation* SpriteRegistry::switchWavesAnimation = nullptr;
SpriteAnimation* SpriteRegistry::switchWavesShortAnimation = nullptr;
void SpriteRegistry::loadAll() {
	player = newSpriteSheetWithImagePath(playerFileName, 10, 4, false);
	tiles = newSpriteSheetWithImagePath(tilesFileName, MapState::tileCount, 1, true);
	radioTower = newSpriteSheetWithImagePath(radioTowerFileName, 1, 1, false);
	rails = newSpriteSheetWithImagePath(railsFileName, 22, 1, true);
	switches = newSpriteSheetWithImagePath(switchesFileName, 9, 1, true);
	radioWaves = newSpriteSheetWithImagePath(radioWavesFileName, 5, 1, false);
	railWaves = newSpriteSheetWithImagePath(railWavesFileName, 5, 1, true);
	switchWaves = newSpriteSheetWithImagePath(switchWavesFileName, 4, 1, true);
	resetSwitch = newSpriteSheetWithImagePath(resetSwitchFileName, 2, 1, true);
	kickIndicator = newSpriteSheetWithImagePath(kickIndicatorFileName, 10, 1, true);
	playerWalkingAnimation = newSpriteAnimation(
		player,
		{
			newSpriteAnimationFrame(0, SpriteAnimation::absentSpriteIndex, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(1, SpriteAnimation::absentSpriteIndex, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(0, SpriteAnimation::absentSpriteIndex, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(2, SpriteAnimation::absentSpriteIndex, playerWalkingAnimationTicksPerFrame)
		});
	playerLegLiftAnimation = newSpriteAnimation(
		player,
		{
			newSpriteAnimationFrame(3, SpriteAnimation::absentSpriteIndex, playerKickingAnimationTicksPerFrame)
		});
	playerBootWalkingAnimation = newSpriteAnimation(
		player,
		{
			newSpriteAnimationFrame(4, SpriteAnimation::absentSpriteIndex, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(5, SpriteAnimation::absentSpriteIndex, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(4, SpriteAnimation::absentSpriteIndex, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(6, SpriteAnimation::absentSpriteIndex, playerWalkingAnimationTicksPerFrame)
		});
	playerKickingAnimation = newSpriteAnimation(
		player,
		{
			newSpriteAnimationFrame(7, SpriteAnimation::absentSpriteIndex, playerKickingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(8, SpriteAnimation::absentSpriteIndex, playerKickingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(7, SpriteAnimation::absentSpriteIndex, playerKickingAnimationTicksPerFrame)
		});
	playerFastKickingAnimation = newSpriteAnimation(
		player,
		{
			newSpriteAnimationFrame(7, SpriteAnimation::absentSpriteIndex, playerFastKickingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(8, SpriteAnimation::absentSpriteIndex, playerFastKickingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(7, SpriteAnimation::absentSpriteIndex, playerFastKickingAnimationTicksPerFrame)
		});
	playerBootLiftAnimation = newSpriteAnimation(player, { newSpriteAnimationFrame(7, SpriteAnimation::absentSpriteIndex, 1) });
	playerRidingRailAnimation =
		newSpriteAnimation(player, { newSpriteAnimationFrame(9, SpriteAnimation::absentSpriteIndex, 1) });
	radioWavesAnimation = newSpriteAnimation(
		radioWaves,
		{
			newSpriteAnimationFrame(0, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(1, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(2, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(4, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(1, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(2, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(3, 0, radioWaveAnimationTicksPerFrame)
		});
	radioWavesAnimation->disableLooping();
	railWavesAnimation = newSpriteAnimation(
		railWaves,
		{
			newSpriteAnimationFrame(0, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(1, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(2, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(4, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(2, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(3, 0, radioWaveAnimationTicksPerFrame)
		});
	railWavesAnimation->disableLooping();
	switchWavesAnimation = newSpriteAnimation(
		switchWaves,
		{
			newSpriteAnimationFrame(0, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(1, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(3, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(1, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(2, 0, radioWaveAnimationTicksPerFrame)
		});
	switchWavesAnimation->disableLooping();
	switchWavesShortAnimation = newSpriteAnimation(
		switchWaves,
		{
			newSpriteAnimationFrame(0, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(1, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(2, 0, radioWaveAnimationTicksPerFrame) COMMA
		});
	switchWavesShortAnimation->disableLooping();
}
void SpriteRegistry::unloadAll() {
	delete player;
	delete tiles;
	delete radioTower;
	delete rails;
	delete switches;
	delete radioWaves;
	delete railWaves;
	delete switchWaves;
	delete resetSwitch;
	delete kickIndicator;
	delete playerWalkingAnimation;
	delete playerLegLiftAnimation;
	delete playerBootWalkingAnimation;
	delete playerKickingAnimation;
	delete playerFastKickingAnimation;
	delete playerBootLiftAnimation;
	delete playerRidingRailAnimation;
	delete radioWavesAnimation;
	delete railWavesAnimation;
	delete switchWavesAnimation;
	delete switchWavesShortAnimation;
}
