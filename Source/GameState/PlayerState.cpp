#include "PlayerState.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/MapState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

const float PlayerState::boundingBoxLeftOffset = -5.5f;
const float PlayerState::boundingBoxRightOffset = 5.5f;
const float PlayerState::boundingBoxTopOffset = 4.5f;
const float PlayerState::boundingBoxBottomOffset = 9.5;
PlayerState::PlayerState(objCounterParameters())
: EntityState(objCounterArgumentsComma() 179.5f, 166.5f)
, xDirection(0)
, yDirection(0)
, animation(nullptr)
, animationStartTicksTime(-1)
, spriteDirection(PlayerSpriteDirection::Down)
, hasBoot(false)
, kickingAnimation(nullptr) {
}
PlayerState::~PlayerState() {
	if (kickingAnimation != nullptr)
		kickingAnimation->release();
}
//set the animation to the given animation at the given time
void PlayerState::setSpriteAnimation(SpriteAnimation* pAnimation, int pAnimationStartTicksTime) {
	animation = pAnimation;
	animationStartTicksTime = pAnimationStartTicksTime;
}
//update this player state by reading from the previous state
void PlayerState::updateWithPreviousPlayerState(PlayerState* prev, int ticksTime) {
	hasBoot = prev->hasBoot;
	if (kickingAnimation != nullptr)
		kickingAnimation->release();
	kickingAnimation = prev->kickingAnimation;

	//if we have a kicking animation, update with that instead
	if (kickingAnimation != nullptr) {
		copyEntityState(prev);
		xDirection = prev->xDirection;
		yDirection = prev->yDirection;
		animation = prev->animation;
		animationStartTicksTime = prev->animationStartTicksTime;
		spriteDirection = prev->spriteDirection;
		if (kickingAnimation->update(this, ticksTime))
			kickingAnimation->retain();
		else {
			kickingAnimation = nullptr;
			animation = nullptr;
		}
		return;
	}

	//update this player state normally by reading from the last state
	updatePositionWithPreviousPlayerState(prev, ticksTime);
	collideWithEnvironmentWithPreviousPlayerState(prev);
	updateSpriteWithPreviousPlayerState(prev, ticksTime);
}
//update the position of this player state by reading from the previous state
void PlayerState::updatePositionWithPreviousPlayerState(PlayerState* prev, int ticksTime) {
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	xDirection = (char)(keyboardState[Config::rightKey] - keyboardState[Config::leftKey]);
	yDirection = (char)(keyboardState[Config::downKey] - keyboardState[Config::upKey]);
	float speedPerTick = ((xDirection & yDirection) != 0 ? MapState::diagonalSpeed : MapState::speed) / 1000.0f;
	int ticksSinceLastUpdate = ticksTime - prev->lastUpdateTicksTime;
	x->release();
	x = callNewFromPool(CompositeLinearValue)->set(prev->x->getValue(ticksSinceLastUpdate), (float)xDirection * speedPerTick);
	y->release();
	y = callNewFromPool(CompositeLinearValue)->set(prev->y->getValue(ticksSinceLastUpdate), (float)yDirection * speedPerTick);
	lastUpdateTicksTime = ticksTime;
	renderInterpolatedX = true;
	renderInterpolatedY = true;
	z = prev->z;
}
//check if we moved onto map tiles of a different height
//use the previous move direction to figure out where to look
//if we did, move the player and don't render interpolated positions
void PlayerState::collideWithEnvironmentWithPreviousPlayerState(PlayerState* prev) {
	//find the row and column of tiles that we could collide with
	float xPosition = x->getValue(0);
	float yPosition = y->getValue(0);
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
		renderInterpolatedX = false;
		x->release();
		x = x->copyWithConstantValue(prev->xDirection < 0
			? wallX + MapState::smallDistance - boundingBoxLeftOffset
			: wallX - MapState::smallDistance - boundingBoxRightOffset);
	}
	if (hasYCollision) {
		renderInterpolatedY = false;
		y->release();
		y = y->copyWithConstantValue(prev->yDirection < 0
			? wallY + MapState::smallDistance - boundingBoxTopOffset
			: wallY - MapState::smallDistance - boundingBoxBottomOffset);
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
	if (!moving)
		animation = nullptr;
	else if (prev->animation != nullptr) {
		animation = prev->animation;
		animationStartTicksTime = prev->animationStartTicksTime;
	} else {
		animation = hasBoot ? SpriteRegistry::playerBootWalkingAnimation : SpriteRegistry::playerWalkingAnimation;
		animationStartTicksTime = ticksTime;
	}
}
//if we don't have a kicking animation, start one
void PlayerState::beginKickingAnimation(int ticksTime) {
	if (kickingAnimation != nullptr)
		return;

	SpriteAnimation* kickingSpriteAnimation = hasBoot
		? SpriteRegistry::playerKickingAnimation
		: SpriteRegistry::playerLegLiftAnimation;
	kickingAnimation = callNewFromPool(EntityAnimation)->set(
		ticksTime,
		{
			callNewFromPool(EntityAnimation::SetVelocity)->set(0.0f, 0.0f),
			callNewFromPool(EntityAnimation::SetSpriteAnimation)->set(kickingSpriteAnimation),
			callNewFromPool(EntityAnimation::Delay)->set(kickingSpriteAnimation->getTotalTicksDuration())
		});
}
//render this player state, which was deemed to be the last state to need rendering
void PlayerState::render(EntityState* camera, int ticksTime) {
	float renderCenterX =
		getRenderCenterX(ticksTime) - camera->getRenderCenterX(ticksTime) + (float)Config::gameScreenWidth * 0.5f;
	float renderCenterY =
		getRenderCenterY(ticksTime) - camera->getRenderCenterY(ticksTime) + (float)Config::gameScreenHeight * 0.5f;
	glEnable(GL_BLEND);
	if (animation != nullptr)
		animation->renderUsingCenter(
			renderCenterX, renderCenterY, ticksTime - animationStartTicksTime, 0, (int)spriteDirection);
	else
		SpriteRegistry::player->renderUsingCenter(renderCenterX, renderCenterY, hasBoot ? 4 : 0, (int)spriteDirection);
}
