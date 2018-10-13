#include "GameState.h"
#include "Animation.h"
#include "Map.h"
#include "SpriteRegistry.h"
#include "SpriteSheet.h"

GameState::GameState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
frame(0)
, playerState(newWithoutArgs(PlayerState))
, shouldQuitGame(false) {
}
GameState::~GameState() {
	delete playerState;
}
//update this game state by reading from the previous state
void GameState::updateWithPreviousGameState(GameState* prev) {
	frame = prev->frame + 1;

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

	playerState->updateWithPreviousPlayerState(prev->playerState, frame);
}
//render this state, which was deemed to be the last state to need rendering
void GameState::render() {
	glDisable(GL_BLEND);
	//render the map
	//these values are just right so that every tile rendered is at least partially in the window and no tiles are left out
	int addX = Config::gameScreenWidth / 2 - (int)(floor(playerState->getX()));
	int addY = Config::gameScreenHeight / 2 - (int)(floor(playerState->getY()));
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

	playerState->render();
}
//return whether updates and renders should stop
bool GameState::getShouldQuitGame() {
	return shouldQuitGame;
}
PlayerState::PlayerState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
x(179.5f)
, y(166.5f)
, dX(0)
, dY(0)
, walkingAnimationFrame(-1)
, spriteDirection(PlayerSpriteDirection::Down) {
}
PlayerState::~PlayerState() {}
//update this player state by reading from the previous state
void PlayerState::updateWithPreviousPlayerState(PlayerState* prev, int frame) {
	//update position
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	dX = (char)(keyboardState[SDL_SCANCODE_RIGHT] - keyboardState[SDL_SCANCODE_LEFT]);
	dY = (char)(keyboardState[SDL_SCANCODE_DOWN] - keyboardState[SDL_SCANCODE_UP]);
	float speed = (dX & dY) != 0 ? Map::diagonalSpeed : Map::speed;
	x = prev->x + speed * (float)dX;
	y = prev->y + speed * (float)dY;

	//update the sprite direction
	//if we didn't change direction or the player is not moving, use the last direction
	bool moving = (dX | dY) != 0;
	if ((dX == prev->dX && dY == prev->dY) || !moving)
		spriteDirection = prev->spriteDirection;
	//we are moving and we changed direction
	//use a horizontal sprite if we are not moving vertically
	else if (dY == 0
			//use a horizontal sprite if we are moving diagonally but we have no vertical direction change
			|| (dX != 0
				&& (dY == prev->dY
					//use a horizontal sprite if we changed both directions and we were facing horizontally before
					|| (dX != prev->dX
						&& (prev->spriteDirection == PlayerSpriteDirection::Left
							|| prev->spriteDirection == PlayerSpriteDirection::Right)))))
		spriteDirection = dX < 0 ? PlayerSpriteDirection::Left : PlayerSpriteDirection::Right;
	//use a vertical sprite
	else
		spriteDirection = dY < 0 ? PlayerSpriteDirection::Up : PlayerSpriteDirection::Down;

	//update the animation
	//since we reset the frame to -1 and start at 0, we don't have to check the previous state's frame
	walkingAnimationFrame = moving ? prev->walkingAnimationFrame + 1 : -1;
}
//render this player state, which was deemed to be the last state to need rendering
void PlayerState::render() {
	glEnable(GL_BLEND);
	if (walkingAnimationFrame >= 0)
		SpriteRegistry::playerWalkingAnimation->renderUsingCenterWithVerticalIndex(
			walkingAnimationFrame,
			(int)(spriteDirection),
			(float)(Config::gameScreenWidth) / 2.0f,
			(float)(Config::gameScreenHeight) / 2.0f);
	else
		SpriteRegistry::player->renderUsingCenter(
			0, (int)(spriteDirection), (float)(Config::gameScreenWidth) / 2.0f, (float)(Config::gameScreenHeight) / 2.0f);
}
