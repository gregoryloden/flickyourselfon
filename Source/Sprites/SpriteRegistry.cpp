#include "SpriteRegistry.h"
#include "GameState/MapState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteSheet.h"

SpriteSheet* SpriteRegistry::player = nullptr;
SpriteAnimation* SpriteRegistry::playerWalkingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerLegLiftAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerBootWalkingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerKickingAnimation = nullptr;
SpriteSheet* SpriteRegistry::tiles = nullptr;
SpriteSheet* SpriteRegistry::radioTower = nullptr;
SpriteSheet* SpriteRegistry::font = nullptr;
//load all the sprite sheets
//this should only be called after the gl context has been created
void SpriteRegistry::loadAll() {
	player = newSpriteSheetWithImagePath("images/player.png", 9, 4);
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
	tiles = newSpriteSheetWithImagePath("images/tiles.png", MapState::tileCount, 1);
	tiles->clampSpriteRectForTilesSprite();
	radioTower = newSpriteSheetWithImagePath("images/radiotower.png", 1, 1);
	SDL_Surface* fontSurface = IMG_Load("images/font.png");
	font = newSpriteSheet(fontSurface, 1, 1);
	SDL_FreeSurface(fontSurface);
}
//delete all the sprite sheets
void SpriteRegistry::unloadAll() {
	delete player;
	delete playerWalkingAnimation;
	delete playerLegLiftAnimation;
	delete playerBootWalkingAnimation;
	delete playerKickingAnimation;
	delete tiles;
	delete radioTower;
	delete font;
}
