#include "SpriteRegistry.h"
#include "SpriteSheet.h"

SpriteSheet* SpriteRegistry::player = nullptr;
//load all the sprite sheets
//this should only be called after the gl context has been created
void SpriteRegistry::loadAll() {
	player = newWithArgs(SpriteSheet, "images/player.png", 9, 4);
}
//delete all the sprite sheets
void SpriteRegistry::unloadAll() {
	delete player;
}
