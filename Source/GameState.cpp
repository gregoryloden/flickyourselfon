#include "GameState.h"
#include "Animation.h"
#include "Map.h"
#include "SpriteRegistry.h"
#include "SpriteSheet.h"

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
	int addX = Config::gameScreenWidth / 2 - (int)(floor(playerPosition->getX(currentTicksTime)));
	int addY = Config::gameScreenHeight / 2 - (int)(floor(playerPosition->getY(currentTicksTime)));
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
position(newWithArgs(PositionState, 179.5f, 166.5f))
, walkingAnimationStartTicksTime(-1)
, spriteDirection(PlayerSpriteDirection::Down) {
}
PlayerState::~PlayerState() {
	delete position;
}
//update this player state by reading from the previous state
void PlayerState::updateWithPreviousPlayerState(PlayerState* prev, int ticksTime) {
	position->updateWithPreviousPositionState(prev->position, ticksTime);
	bool moving = position->isMoving();
	char dX = position->getDX();
	char dY = position->getDY();

	//update the sprite direction
	//if the player did not change direction or is not moving, use the last direction
	if (position->updatedSameTimeAs(prev->position) || !moving)
		spriteDirection = prev->spriteDirection;
	//we are moving and we changed direction
	//use a horizontal sprite if we are not moving vertically
	else if (dY == 0
			//use a horizontal sprite if we are moving diagonally but we have no vertical direction change
			|| (dX != 0
				&& (dY == prev->position->getDY()
					//use a horizontal sprite if we changed both directions and we were facing horizontally before
					|| (dX != prev->position->getDX()
						&& (prev->spriteDirection == PlayerSpriteDirection::Left
							|| prev->spriteDirection == PlayerSpriteDirection::Right)))))
		spriteDirection = dX < 0 ? PlayerSpriteDirection::Left : PlayerSpriteDirection::Right;
	//use a vertical sprite
	else
		spriteDirection = dY < 0 ? PlayerSpriteDirection::Up : PlayerSpriteDirection::Down;

	//update the animation
	walkingAnimationStartTicksTime =
		!moving ? -1 :
		prev->walkingAnimationStartTicksTime >= 0 ? prev->walkingAnimationStartTicksTime :
		ticksTime;
}
//render this player state, which was deemed to be the last state to need rendering
void PlayerState::render() {
	glEnable(GL_BLEND);
	if (walkingAnimationStartTicksTime >= 0)
		SpriteRegistry::playerWalkingAnimation->renderUsingCenterWithVerticalIndex(
			SDL_GetTicks() - walkingAnimationStartTicksTime,
			(int)(spriteDirection),
			(float)(Config::gameScreenWidth) / 2.0f,
			(float)(Config::gameScreenHeight) / 2.0f);
	else
		SpriteRegistry::player->renderUsingCenter(
			0, (int)(spriteDirection), (float)(Config::gameScreenWidth) / 2.0f, (float)(Config::gameScreenHeight) / 2.0f);
}
PositionState::PositionState(objCounterParametersComma() float pX, float pY)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
x(pX)
, y(pY)
, dX(0)
, dY(0)
, speed(0.0f)
, lastUpdateTicksTime(0) {
}
PositionState::~PositionState() {}
//return whether this position is moving
bool PositionState::isMoving() {
	return (dX | dY) != 0;
}
//return whether these two states were last updated at the same time
bool PositionState::updatedSameTimeAs(PositionState* other) {
	return lastUpdateTicksTime == other->lastUpdateTicksTime;
}
//return the position's x coordinate at the given time
float PositionState::getX(int ticks) {
	return x + (float)dX * speed * (float)(ticks - lastUpdateTicksTime) / 1000.0f;
}
//return the position's x coordinate at the given time
float PositionState::getY(int ticks) {
	return y + (float)dY * speed * (float)(ticks - lastUpdateTicksTime) / 1000.0f;
}
//update this position state by reading from the previous state
void PositionState::updateWithPreviousPositionState(PositionState* prev, int ticksTime) {
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	dX = (char)(keyboardState[SDL_SCANCODE_RIGHT] - keyboardState[SDL_SCANCODE_LEFT]);
	dY = (char)(keyboardState[SDL_SCANCODE_DOWN] - keyboardState[SDL_SCANCODE_UP]);
	speed = (dX & dY) != 0 ? Map::diagonalSpeed : Map::speed;

	//if we didn't change direction, copy the position and we're done
	if (dX == prev->dX && dY == prev->dY) {
		x = prev->x;
		y = prev->y;
		lastUpdateTicksTime = prev->lastUpdateTicksTime;
	//we changed velocity in some way
	} else {
		//update the position to account for previous movement
		lastUpdateTicksTime = ticksTime;
		x = prev->getX(ticksTime);
		y = prev->getY(ticksTime);
	}
}
