#include "SpriteRegistry.h"
#include "GameState/MapState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteSheet.h"
#include "Util/FileUtils.h"

const char* SpriteRegistry::playerFileName = "player.png";
const char* SpriteRegistry::tilesFileName = "tiles.png";
const char* SpriteRegistry::radioTowerFileName = "radiotower.png";
const char* SpriteRegistry::railsFileName = "rails.png";
const char* SpriteRegistry::switchesFileName = "switches.png";
const char* SpriteRegistry::radioWavesFileName = "radiowaves.png";
SpriteSheet* SpriteRegistry::player = nullptr;
SpriteSheet* SpriteRegistry::tiles = nullptr;
SpriteSheet* SpriteRegistry::radioTower = nullptr;
SpriteSheet* SpriteRegistry::rails = nullptr;
SpriteSheet* SpriteRegistry::switches = nullptr;
SpriteSheet* SpriteRegistry::radioWaves = nullptr;
SpriteAnimation* SpriteRegistry::playerWalkingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerLegLiftAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerBootWalkingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerKickingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerBootLiftAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerRidingRailAnimation = nullptr;
SpriteAnimation* SpriteRegistry::radioWavesAnimation = nullptr;
//load all the sprite sheets
//this should only be called after the gl context has been created
void SpriteRegistry::loadAll() {
	player = newSpriteSheetWithImagePath(playerFileName, 10, 4);
	tiles = newSpriteSheetWithImagePath(tilesFileName, MapState::tileCount, 1);
	tiles->removeBottomRightPixelBorder();
	radioTower = newSpriteSheetWithImagePath(radioTowerFileName, 1, 1);
	rails = newSpriteSheetWithImagePath(railsFileName, 16, 1);
	rails->removeBottomRightPixelBorder();
	switches = newSpriteSheetWithImagePath(switchesFileName, 9, 1);
	switches->removeBottomRightPixelBorder();
	radioWaves = newSpriteSheetWithImagePath(radioWavesFileName, 5, 1);
	playerWalkingAnimation = newSpriteAnimation(
		player,
		{
			newSpriteAnimationFrame(0, -1, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(1, -1, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(0, -1, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(2, -1, playerWalkingAnimationTicksPerFrame)
		});
	playerLegLiftAnimation = newSpriteAnimation(
		player,
		{
			newSpriteAnimationFrame(3, -1, playerKickingAnimationTicksPerFrame)
		});
	playerBootWalkingAnimation = newSpriteAnimation(
		player,
		{
			newSpriteAnimationFrame(4, -1, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(5, -1, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(4, -1, playerWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(6, -1, playerWalkingAnimationTicksPerFrame)
		});
	playerKickingAnimation = newSpriteAnimation(
		player,
		{
			newSpriteAnimationFrame(7, -1, playerKickingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(8, -1, playerKickingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(7, -1, playerKickingAnimationTicksPerFrame)
		});
	playerBootLiftAnimation = newSpriteAnimation(player, { newSpriteAnimationFrame(7, -1, 1) });
	playerRidingRailAnimation = newSpriteAnimation(player, { newSpriteAnimationFrame(9, -1, 1) });
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
}
//delete all the sprite sheets
void SpriteRegistry::unloadAll() {
	delete player;
	delete tiles;
	delete radioTower;
	delete rails;
	delete switches;
	delete radioWaves;
	delete playerWalkingAnimation;
	delete playerLegLiftAnimation;
	delete playerBootWalkingAnimation;
	delete playerKickingAnimation;
	delete playerBootLiftAnimation;
	delete playerRidingRailAnimation;
	delete radioWavesAnimation;
}
