#include "SpriteRegistry.h"
#include "GameState/MapState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteSheet.h"

const char* SpriteRegistry::playerFileName = "images/player.png";
const char* SpriteRegistry::tilesFileName = "images/tiles.png";
const char* SpriteRegistry::radioTowerFileName = "images/radiotower.png";
SpriteSheet* SpriteRegistry::player = nullptr;
SpriteSheet* SpriteRegistry::tiles = nullptr;
SpriteSheet* SpriteRegistry::radioTower = nullptr;
SpriteAnimation* SpriteRegistry::playerWalkingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerLegLiftAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerBootWalkingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerKickingAnimation = nullptr;
//load all the sprite sheets
//this should only be called after the gl context has been created
void SpriteRegistry::loadAll() {
	player = newSpriteSheetWithImagePath(playerFileName, 9, 4);
	tiles = newSpriteSheetWithImagePath(tilesFileName, MapState::tileCount, 1);
	tiles->clampSpriteRectForTilesSprite();
	radioTower = newSpriteSheetWithImagePath(radioTowerFileName, 1, 1);
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
}
//delete all the sprite sheets
void SpriteRegistry::unloadAll() {
	delete player;
	delete tiles;
	delete radioTower;
	delete playerWalkingAnimation;
	delete playerLegLiftAnimation;
	delete playerBootWalkingAnimation;
	delete playerKickingAnimation;
}
