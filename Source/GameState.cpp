#include "GameState.h"
#include "Map.h"
#include "SpriteRegistry.h"
#include "SpriteSheet.h"

GameState::GameState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
currentSpriteHorizontalIndex(0)
, currentSpriteVerticalIndex(0)
, currentAnimationFrame(0)
, playerX(179.5f)
, playerY(166.5f)
, shouldQuitGame(false) {
}
GameState::~GameState() {}
//update this game state by reading from the previous state
void GameState::updateWithPreviousGameState(GameState* prev) {
	//handle events
	int clicks = 0;
	SDL_Event gameEvent;
	while (SDL_PollEvent(&gameEvent) != 0) {
		switch (gameEvent.type) {
			case SDL_QUIT:
				shouldQuitGame = true;
				return;
			case SDL_MOUSEBUTTONDOWN:
				clicks++;
				break;
			default:
				break;
		}
	}

	//update state
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	playerX = prev->playerX - keyboardState[SDL_SCANCODE_LEFT] + keyboardState[SDL_SCANCODE_RIGHT];
	playerY = prev->playerY - keyboardState[SDL_SCANCODE_UP] + keyboardState[SDL_SCANCODE_DOWN];

	currentSpriteHorizontalIndex = (prev->currentSpriteHorizontalIndex + clicks) % 9;
	if (prev->currentAnimationFrame == 12) {
		currentSpriteVerticalIndex = (prev->currentSpriteVerticalIndex + 1) % 4;
		currentAnimationFrame = 1;
	} else {
		currentSpriteVerticalIndex = prev->currentSpriteVerticalIndex;
		currentAnimationFrame = prev->currentAnimationFrame + 1;
	}
}
//render this state, which was deemed to be the last state to need rendering
void GameState::render() {
	glDisable(GL_BLEND);
	//render the map
	//these values are just right so that every tile rendered is at least partially in the window and no tiles are left out
	int addX = Config::gameScreenWidth / 2 - (int)(floor(playerX));
	int addY = Config::gameScreenHeight / 2 - (int)(floor(playerY));
	int tileMinX = FYOMath::max(-addX / Map::tileSize, 0);
	int tileMinY = FYOMath::max(-addY / Map::tileSize, 0);
	int tileMaxX = FYOMath::min((Config::gameScreenWidth - addX - 1) / Map::tileSize + 1, Map::width);
	int tileMaxY = FYOMath::min((Config::gameScreenHeight - addY - 1) / Map::tileSize + 1, Map::height);
	for (int y = tileMinY; y < tileMaxY; y++) {
		for (int x = tileMinX; x < tileMaxX; x++) {
			//consider any tile at the max height to be filler
			int mapIndex = y * Map::width + x;
			if (Map::heights[mapIndex] != Map::heightCount - 1)
				SpriteRegistry::tiles->render(
					(int)(Map::tiles[mapIndex]), 0, x * Map::tileSize + addX, y * Map::tileSize + addY);
		}
	}

	glEnable(GL_BLEND);
	SpriteRegistry::player->renderUsingCenter(
		currentSpriteHorizontalIndex,
		currentSpriteVerticalIndex,
		(float)(Config::gameScreenWidth) / 2.0f,
		(float)(Config::gameScreenHeight) / 2.0f);
}
//return whether updates and renders should stop
bool GameState::getShouldQuitGame() {
	return shouldQuitGame;
}
