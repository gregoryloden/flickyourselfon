#include "SpriteRegistry.h"
#include "GameState/MapState/MapState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteSheet.h"
#include "Util/FileUtils.h"

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
SpriteSheet* SpriteRegistry::sparks = nullptr;
SpriteSheet* SpriteRegistry::borderArrows = nullptr;
SpriteSheet* SpriteRegistry::wavesActivated = nullptr;
SpriteSheet* SpriteRegistry::save = nullptr;
SpriteAnimation* SpriteRegistry::playerWalkingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerLegLiftAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerBootWalkingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerBootSprintingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerFastBootWalkingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerKickingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerFastKickingAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerBootLiftAnimation = nullptr;
SpriteAnimation* SpriteRegistry::playerRidingRailAnimation = nullptr;
SpriteAnimation* SpriteRegistry::radioWavesAnimation = nullptr;
SpriteAnimation* SpriteRegistry::railWavesAnimation = nullptr;
SpriteAnimation* SpriteRegistry::switchWavesAnimation = nullptr;
SpriteAnimation* SpriteRegistry::switchWavesShortAnimation = nullptr;
SpriteAnimation* SpriteRegistry::sparksAnimationA = nullptr;
SpriteAnimation* SpriteRegistry::sparksAnimationB = nullptr;
SpriteAnimation* SpriteRegistry::sparksSlowAnimationA = nullptr;
SpriteAnimation* SpriteRegistry::sparksSlowAnimationB = nullptr;
void SpriteRegistry::loadAll() {
	player = newSpriteSheetWithImagePath("player.png", 10, 4, false);
	tiles = loadTiles();
	radioTower = newSpriteSheetWithImagePath("radiotower.png", 1, 1, false);
	rails = newSpriteSheetWithImagePath("rails.png", 22, 1, true);
	switches = newSpriteSheetWithImagePath("switches.png", 9, 1, true);
	radioWaves = newSpriteSheetWithImagePath("radiowaves.png", 5, 1, false);
	railWaves = newSpriteSheetWithImagePath("railwaves.png", 4, 1, true);
	railWaves->setBottomAnchorY();
	switchWaves = newSpriteSheetWithImagePath("switchwaves.png", 4, 1, true);
	resetSwitch = newSpriteSheetWithImagePath("resetswitch.png", 2, 1, true);
	kickIndicator = newSpriteSheetWithImagePath("kickindicator.png", 13, 1, true);
	sparks = newSpriteSheetWithImagePath("sparks.png", 6, 4, true);
	borderArrows = newSpriteSheetWithImagePath("borderarrows.png", 3, 3, false);
	wavesActivated = newSpriteSheetWithImagePath("wavesactivated.png", 1, 4, false);
	save = newSpriteSheetWithImagePath("save.png", 1, 1, false);
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
	playerBootSprintingAnimation = newSpriteAnimation(
		player,
		{
			newSpriteAnimationFrame(4, SpriteAnimation::absentSpriteIndex, playerSprintingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(5, SpriteAnimation::absentSpriteIndex, playerSprintingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(4, SpriteAnimation::absentSpriteIndex, playerSprintingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(6, SpriteAnimation::absentSpriteIndex, playerSprintingAnimationTicksPerFrame) COMMA
		});
	playerFastBootWalkingAnimation = newSpriteAnimation(
		player,
		{
			newSpriteAnimationFrame(4, SpriteAnimation::absentSpriteIndex, playerFastWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(5, SpriteAnimation::absentSpriteIndex, playerFastWalkingAnimationTicksPerFrame + 1) COMMA
			newSpriteAnimationFrame(4, SpriteAnimation::absentSpriteIndex, playerFastWalkingAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(6, SpriteAnimation::absentSpriteIndex, playerFastWalkingAnimationTicksPerFrame + 1)
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
			newSpriteAnimationFrame(3, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(1, 0, radioWaveAnimationTicksPerFrame) COMMA
			newSpriteAnimationFrame(2, 0, radioWaveAnimationTicksPerFrame)
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
	sparksAnimationA = newSpriteAnimation(
		sparks,
		{
			newSpriteAnimationFrame(0, SpriteAnimation::absentSpriteIndex, sparksTicksPerFrame) COMMA
			newSpriteAnimationFrame(1, SpriteAnimation::absentSpriteIndex, sparksTicksPerFrame) COMMA
			newSpriteAnimationFrame(2, SpriteAnimation::absentSpriteIndex, sparksTicksPerFrame) COMMA
		});
	sparksAnimationA->disableLooping();
	sparksAnimationB = newSpriteAnimation(
		sparks,
		{
			newSpriteAnimationFrame(3, SpriteAnimation::absentSpriteIndex, sparksTicksPerFrame) COMMA
			newSpriteAnimationFrame(4, SpriteAnimation::absentSpriteIndex, sparksTicksPerFrame) COMMA
			newSpriteAnimationFrame(5, SpriteAnimation::absentSpriteIndex, sparksTicksPerFrame) COMMA
		});
	sparksAnimationB->disableLooping();
	sparksSlowAnimationA = newSpriteAnimation(
		sparks,
		{
			newSpriteAnimationFrame(0, SpriteAnimation::absentSpriteIndex, sparksSlowTicksPerFrame) COMMA
			newSpriteAnimationFrame(1, SpriteAnimation::absentSpriteIndex, sparksSlowTicksPerFrame) COMMA
			newSpriteAnimationFrame(2, SpriteAnimation::absentSpriteIndex, sparksSlowTicksPerFrame) COMMA
		});
	sparksSlowAnimationA->disableLooping();
	sparksSlowAnimationB = newSpriteAnimation(
		sparks,
		{
			newSpriteAnimationFrame(3, SpriteAnimation::absentSpriteIndex, sparksSlowTicksPerFrame) COMMA
			newSpriteAnimationFrame(4, SpriteAnimation::absentSpriteIndex, sparksSlowTicksPerFrame) COMMA
			newSpriteAnimationFrame(5, SpriteAnimation::absentSpriteIndex, sparksSlowTicksPerFrame) COMMA
		});
	sparksSlowAnimationB->disableLooping();
}
SpriteSheet* SpriteRegistry::loadTiles() {
	SDL_Surface* tilesSurface = FileUtils::loadImage("tiles.png");
	SDL_Surface* tileBordersSurface = FileUtils::loadImage("tileborders.png");
	SDL_Surface* combinedSurface = SDL_CreateRGBSurface(
		tilesSurface->flags,
		tilesSurface->w - 1,
		(tilesSurface->h - 1) * 16,
		tilesSurface->format->BitsPerPixel,
		tilesSurface->format->Rmask,
		tilesSurface->format->Gmask,
		tilesSurface->format->Bmask,
		tilesSurface->format->Amask);
	SDL_Rect tilesRect = { 0, 0, tilesSurface->w - 1, tilesSurface->h - 1 };
	SDL_Rect tilesDestRect = tilesRect;
	SDL_Rect tileBordersRect = { 0, 0, MapState::tileSize, MapState::tileSize };
	SDL_Rect tileBordersDestRect = tileBordersRect;
	for (int i = 0; i < 16; i++) {
		tilesDestRect.y = (tileBordersDestRect.y = MapState::tileSize * i);
		SDL_BlitSurface(tilesSurface, &tilesRect, combinedSurface, &tilesDestRect);
		for (int k = 0; k < 4; k++) {
			if ((i & (1 << k)) == 0)
				continue;
			tileBordersRect.x = k * MapState::tileSize;
			for (int j = 0; j < MapState::tileCount; j++) {
				tileBordersDestRect.x = j * MapState::tileSize;
				SDL_BlitSurface(tileBordersSurface, &tileBordersRect, combinedSurface, &tileBordersDestRect);
			}
		}
	}
	SDL_FreeSurface(tilesSurface);
	SDL_FreeSurface(tileBordersSurface);
	return newSpriteSheet(combinedSurface, MapState::tileCount, 16, false);
}
void SpriteRegistry::unloadAll() {
	delete playerWalkingAnimation;
	delete playerLegLiftAnimation;
	delete playerBootWalkingAnimation;
	delete playerBootSprintingAnimation;
	delete playerFastBootWalkingAnimation;
	delete playerKickingAnimation;
	delete playerFastKickingAnimation;
	delete playerBootLiftAnimation;
	delete playerRidingRailAnimation;
	delete radioWavesAnimation;
	delete railWavesAnimation;
	delete switchWavesAnimation;
	delete switchWavesShortAnimation;
	delete sparksAnimationA;
	delete sparksAnimationB;
	delete sparksSlowAnimationA;
	delete sparksSlowAnimationB;
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
	delete sparks;
	delete borderArrows;
	delete wavesActivated;
	delete save;
}
