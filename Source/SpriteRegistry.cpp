#include "SpriteRegistry.h"
#include "Animation.h"
#include "Map.h"
#include "SpriteSheet.h"

SpriteSheet* SpriteRegistry::player = nullptr;
Animation* SpriteRegistry::playerWalkingAnimation = nullptr;
SpriteSheet* SpriteRegistry::tiles = nullptr;
SpriteSheet* SpriteRegistry::radioTower = nullptr;
SpriteSheet* SpriteRegistry::font = nullptr;
//load all the sprite sheets
//this should only be called after the gl context has been created
void SpriteRegistry::loadAll() {
	player = newWithArgs(SpriteSheet, "images/player.png", 9, 4);
	playerWalkingAnimation = newWithArgs(Animation,
		player,
		{
			newWithArgs(Animation::Frame, 0, 0, playerWalkingAnimationUpdatesPerFrame),
			newWithArgs(Animation::Frame, 1, 0, playerWalkingAnimationUpdatesPerFrame),
			newWithArgs(Animation::Frame, 0, 0, playerWalkingAnimationUpdatesPerFrame),
			newWithArgs(Animation::Frame, 2, 0, playerWalkingAnimationUpdatesPerFrame)
		});
	tiles = newWithArgs(SpriteSheet, "images/tiles.png", Map::tileCount, 1);
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
	delete tiles;
	delete radioTower;
	delete font;
}
