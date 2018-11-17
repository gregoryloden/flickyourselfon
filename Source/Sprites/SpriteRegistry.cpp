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
	player = newWithArgs(SpriteSheet, "images/player.png", 9, 4);
	playerWalkingAnimation = newWithArgs(SpriteAnimation,
		player,
		{
			newWithArgs(SpriteAnimation::Frame, 0, -1, playerWalkingAnimationTicksPerFrame),
			newWithArgs(SpriteAnimation::Frame, 1, -1, playerWalkingAnimationTicksPerFrame),
			newWithArgs(SpriteAnimation::Frame, 0, -1, playerWalkingAnimationTicksPerFrame),
			newWithArgs(SpriteAnimation::Frame, 2, -1, playerWalkingAnimationTicksPerFrame)
		});
	playerLegLiftAnimation = newWithArgs(SpriteAnimation,
		player,
		{
			newWithArgs(SpriteAnimation::Frame, 3, -1, playerKickingAnimationTicksPerFrame)
		});
	playerBootWalkingAnimation = newWithArgs(SpriteAnimation,
		player,
		{
			newWithArgs(SpriteAnimation::Frame, 4, -1, playerWalkingAnimationTicksPerFrame),
			newWithArgs(SpriteAnimation::Frame, 5, -1, playerWalkingAnimationTicksPerFrame),
			newWithArgs(SpriteAnimation::Frame, 4, -1, playerWalkingAnimationTicksPerFrame),
			newWithArgs(SpriteAnimation::Frame, 6, -1, playerWalkingAnimationTicksPerFrame)
		});
	playerKickingAnimation = newWithArgs(SpriteAnimation,
		player,
		{
			newWithArgs(SpriteAnimation::Frame, 7, -1, playerKickingAnimationTicksPerFrame),
			newWithArgs(SpriteAnimation::Frame, 8, -1, playerKickingAnimationTicksPerFrame),
			newWithArgs(SpriteAnimation::Frame, 7, -1, playerKickingAnimationTicksPerFrame)
		});
	tiles = newWithArgs(SpriteSheet, "images/tiles.png", MapState::tileCount, 1);
	tiles->clampSpriteRectForTilesSprite();
	radioTower = newWithArgs(SpriteSheet, "images/radiotower.png", 1, 1);
	SDL_Surface* fontSurface = IMG_Load("images/font.png");
	font = newWithArgs(SpriteSheet, fontSurface, 1, 1);
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
