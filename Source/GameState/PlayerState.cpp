#include "PlayerState.h"
#include "GameState/PositionState.h"
#include "Sprites/Animation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

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
