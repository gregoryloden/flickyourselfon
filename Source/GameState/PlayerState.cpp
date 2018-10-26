#include "PlayerState.h"
#include "GameState/MapState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

PlayerState::PlayerState(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
boundingBoxLeftOffset(-5.5f)
, boundingBoxRightOffset(5.5f)
, boundingBoxTopOffset(4.5f)
, boundingBoxBottomOffset(9.5)
, xPosition(179.5f)
, xDirection(0)
, xVelocityPerTick(0.0f)
, renderInterpolatedXPosition(true)
, yPosition(166.5f)
, yDirection(0)
, yVelocityPerTick(0.0f)
, renderInterpolatedYPosition(true)
, z(0)
, lastUpdateTicksTime(0)
, walkingAnimationStartTicksTime(-1)
, spriteDirection(PlayerSpriteDirection::Down) {
}
PlayerState::~PlayerState() {}
//return the player's x coordinate at the given time that we should use for rendering things like the map
float PlayerState::getRenderableX(int ticksTime) {
	return renderInterpolatedXPosition ? getInterpolatedX(ticksTime) : xPosition;
}
//return the player's y coordinate at the given time that we should use for rendering things like the map
float PlayerState::getRenderableY(int ticksTime) {
	return renderInterpolatedYPosition ? getInterpolatedY(ticksTime) : yPosition;
}
//return the player's x coordinate at the given time after accounting for velocity
float PlayerState::getInterpolatedX(int ticksTime) {
	return xPosition + (float)xVelocityPerTick * (float)(ticksTime - lastUpdateTicksTime);
}
//return the player's y coordinate at the given time after accounting for velocity
float PlayerState::getInterpolatedY(int ticksTime) {
	return yPosition + (float)yVelocityPerTick * (float)(ticksTime - lastUpdateTicksTime);
}
//update this player state by reading from the previous state
void PlayerState::updateWithPreviousPlayerState(PlayerState* prev, int ticksTime) {
	updatePositionWithPreviousPlayerState(prev, ticksTime);
	collideWithEnvironmentWithPreviousPlayerState(prev);
	updateSpriteWithPreviousPlayerState(prev, ticksTime);
}
//update the position of this player state by reading from the previous state
void PlayerState::updatePositionWithPreviousPlayerState(PlayerState* prev, int ticksTime) {
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	xPosition = prev->getInterpolatedX(ticksTime);
	yPosition = prev->getInterpolatedY(ticksTime);
	xDirection = (char)(keyboardState[SDL_SCANCODE_RIGHT] - keyboardState[SDL_SCANCODE_LEFT]);
	yDirection = (char)(keyboardState[SDL_SCANCODE_DOWN] - keyboardState[SDL_SCANCODE_UP]);
	float speedPerTick = ((xDirection & yDirection) != 0 ? MapState::diagonalSpeed : MapState::speed) / 1000.0f;
	xVelocityPerTick = (float)xDirection * speedPerTick;
	yVelocityPerTick = (float)yDirection * speedPerTick;
	lastUpdateTicksTime = ticksTime;
	renderInterpolatedXPosition = true;
	renderInterpolatedYPosition = true;
}
//check if we moved onto map tiles of a different height
//use the previous move direction to figure out where to look
//if we did, move the player and don't render interpolated positions
void PlayerState::collideWithEnvironmentWithPreviousPlayerState(PlayerState* prev) {
	//find the row and column of tiles that we could collide with
	float playerLeftEdge = xPosition + boundingBoxLeftOffset;
	float playerRightEdge = xPosition + boundingBoxRightOffset;
	float playerTopEdge = yPosition + boundingBoxTopOffset;
	float playerBottomEdge = yPosition + boundingBoxBottomOffset;
	int lowMapX = (int)playerLeftEdge / MapState::tileSize;
	int highMapX = (int)playerRightEdge / MapState::tileSize;
	int lowMapY = (int)playerTopEdge / MapState::tileSize;
	int highMapY = (int)playerBottomEdge / MapState::tileSize;
	int collisionMapX = prev->xDirection < 0 ? lowMapX : highMapX;
	int collisionMapY = prev->yDirection < 0 ? lowMapY : highMapY;
	float wallX = (float)((prev->xDirection < 0 ? collisionMapX + 1 : collisionMapX) * MapState::tileSize);
	float wallY = (float)((prev->yDirection < 0 ? collisionMapY + 1 : collisionMapY) * MapState::tileSize);
	bool hasXCollision = false;
	bool hasYCollision = false;

	//check for horizontal collisions, excluding the corner if we're moving diagonally
	if (prev->xDirection != 0) {
		if (prev->yDirection < 0)
			lowMapY++;
		else if (prev->yDirection > 0)
			highMapY--;
		for (int currentMapY = lowMapY; currentMapY <= highMapY; currentMapY++) {
			if (MapState::getHeight(collisionMapX, currentMapY) != z) {
				hasXCollision = true;
				break;
			}
		}
	}
	//check for vertical collisions, excluding the corner if we're moving diagonally
	if (prev->yDirection != 0) {
		if (prev->xDirection < 0)
			lowMapX++;
		else if (prev->xDirection > 0)
			highMapX--;
		for (int currentMapX = lowMapX; currentMapX <= highMapX; currentMapX++) {
			if (MapState::getHeight(currentMapX, collisionMapY) != z) {
				hasYCollision = true;
				break;
			}
		}
	}

	//if we're moving diagonally and haven't collided with anything yet, collide with the corner
	if (prev->xDirection != 0
		&& prev->yDirection != 0
		&& !hasXCollision
		&& !hasYCollision
		&& MapState::getHeight(collisionMapX, collisionMapY) != z)
	{
		//move in whichever direction makes us move less
		float xCollisionDistance = prev->xDirection < 0 ? wallX - playerLeftEdge : playerRightEdge - wallX;
		float yCollisionDistance = prev->yDirection < 0 ? wallY - playerTopEdge : playerBottomEdge - wallY;
		if (xCollisionDistance < yCollisionDistance)
			hasXCollision = true;
		else
			hasYCollision = true;
	}

	//adjust our position if we collided
	if (hasXCollision) {
		renderInterpolatedXPosition = false;
		xPosition = prev->xDirection < 0
			? wallX + MapState::smallDistance - boundingBoxLeftOffset
			: wallX - MapState::smallDistance - boundingBoxRightOffset;
	}
	if (hasYCollision) {
		renderInterpolatedYPosition = false;
		yPosition = prev->yDirection < 0
			? wallY + MapState::smallDistance - boundingBoxTopOffset
			: wallY - MapState::smallDistance - boundingBoxBottomOffset;
	}
}
//update the sprite (and possibly the animation) of this player state by reading from the previous state
void PlayerState::updateSpriteWithPreviousPlayerState(PlayerState* prev, int ticksTime) {
	bool moving = (xDirection | yDirection) != 0;
	//update the sprite direction
	//if the player did not change direction or is not moving, use the last direction
	if ((xDirection == prev->xDirection && yDirection == prev->yDirection) || !moving)
		spriteDirection = prev->spriteDirection;
	//we are moving and we changed direction
	//use a horizontal sprite if we are not moving vertically
	else if (yDirection == 0
			//use a horizontal sprite if we are moving diagonally but we have no vertical direction change
			|| (xDirection != 0
				&& (yDirection == prev->yDirection
					//use a horizontal sprite if we changed both directions and we were facing horizontally before
					|| (xDirection != prev->xDirection
						&& (prev->spriteDirection == PlayerSpriteDirection::Left
							|| prev->spriteDirection == PlayerSpriteDirection::Right)))))
		spriteDirection = xDirection < 0 ? PlayerSpriteDirection::Left : PlayerSpriteDirection::Right;
	//use a vertical sprite
	else
		spriteDirection = yDirection < 0 ? PlayerSpriteDirection::Up : PlayerSpriteDirection::Down;

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
