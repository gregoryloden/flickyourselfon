#include "GameState.h"
#include "GameState/MapState.h"
#include "GameState/PlayerState.h"
#include "GameState/PositionState.h"
#include "Sprites/Animation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

GameState::GameState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
ticksTime(0)
, playerState(newWithoutArgs(PlayerState))
, shouldQuitGame(false) {
}
GameState::~GameState() {
	delete playerState;
}
//update this game state by reading from the previous state
void GameState::updateWithPreviousGameState(GameState* prev) {
	ticksTime = SDL_GetTicks();

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

	playerState->updateWithPreviousPlayerState(prev->playerState, ticksTime);
}
//render this state, which was deemed to be the last state to need rendering
void GameState::render() {
	glDisable(GL_BLEND);
	//render the map
	//these values are just right so that every tile rendered is at least partially in the window and no tiles are left out
	int currentTicksTime = SDL_GetTicks();
	PositionState* playerPosition = playerState->getPosition();
	int playerX = (int)(floor(playerPosition->getX(currentTicksTime)));
	int playerY = (int)(floor(playerPosition->getY(currentTicksTime)));
	int addX = Config::gameScreenWidth / 2 - playerX;
	int addY = Config::gameScreenHeight / 2 - playerY;
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
	SpriteRegistry::radioTower->render(0, 0, 423 - playerX, -32 - playerY);

	playerState->render();
}
//return whether updates and renders should stop
bool GameState::getShouldQuitGame() {
	return shouldQuitGame;
}
