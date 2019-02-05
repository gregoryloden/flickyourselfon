#include "PlayerState.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/MapState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

const float PlayerState::playerStartingXPosition = 179.5f;
const float PlayerState::playerStartingYPosition = 166.5f;
const float PlayerState::playerWidth = 11.0f;
const float PlayerState::playerHeight = 5.0f;
const float PlayerState::boundingBoxLeftOffset = PlayerState::playerWidth * -0.5f;
const float PlayerState::boundingBoxRightOffset = PlayerState::boundingBoxLeftOffset + PlayerState::playerWidth;
const float PlayerState::boundingBoxTopOffset = 4.5f;
const float PlayerState::boundingBoxBottomOffset = PlayerState::boundingBoxTopOffset + PlayerState::playerHeight;
PlayerState::PlayerState(objCounterParameters())
: EntityState(objCounterArgumentsComma() playerStartingXPosition, playerStartingYPosition)
, xDirection(0)
, yDirection(0)
, animation(nullptr)
, animationStartTicksTime(-1)
, spriteDirection(SpriteDirection::Down)
, hasBoot(false)
, kickingAnimation(nullptr) {
}
PlayerState::~PlayerState() {}
//copy the other state
void PlayerState::copyPlayerState(PlayerState* other) {
	copyEntityState(other);
	xDirection = other->xDirection;
	yDirection = other->yDirection;
	animation = other->animation;
	animationStartTicksTime = other->animationStartTicksTime;
	spriteDirection = other->spriteDirection;
	hasBoot = other->hasBoot;
	kickingAnimation.set(other->kickingAnimation.get());
}
//use the player as the next camera anchor unless we're starting an animation with a new camera anchor
EntityState* PlayerState::getNextCameraAnchor(int ticksTime) {
	//TODO: return a different camera anchor for animations
	return this;
}
//set the animation to the given animation at the given time
void PlayerState::setSpriteAnimation(SpriteAnimation* pAnimation, int pAnimationStartTicksTime) {
	animation = pAnimation;
	animationStartTicksTime = pAnimationStartTicksTime;
}
//update this player state by reading from the previous state
void PlayerState::updateWithPreviousPlayerState(PlayerState* prev, int ticksTime) {
	bool previousStateHadKickingAnimation = prev->kickingAnimation.get() != nullptr;

	//if we have a kicking animation, update with that instead
	if (previousStateHadKickingAnimation) {
		copyPlayerState(prev);
		if (kickingAnimation.get()->update(this, ticksTime))
			return;
	} else
		hasBoot = prev->hasBoot;

//TODO: put on the boot in-game
hasBoot = true;
	kickingAnimation.set(nullptr);

	//update this player state normally by reading from the last state
	updatePositionWithPreviousPlayerState(prev, ticksTime);
	collideWithEnvironmentWithPreviousPlayerState(prev);
	updateSpriteWithPreviousPlayerState(prev, ticksTime, !previousStateHadKickingAnimation);
}
//update the position of this player state by reading from the previous state
void PlayerState::updatePositionWithPreviousPlayerState(PlayerState* prev, int ticksTime) {
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	xDirection = (char)(keyboardState[Config::keyBindings.rightKey] - keyboardState[Config::keyBindings.leftKey]);
	yDirection = (char)(keyboardState[Config::keyBindings.downKey] - keyboardState[Config::keyBindings.upKey]);
	float speedPerTick =
		((xDirection & yDirection) != 0 ? MapState::diagonalSpeedPerSecond : MapState::speedPerSecond)
			/ (float)Config::ticksPerSecond;
	int ticksSinceLastUpdate = ticksTime - prev->lastUpdateTicksTime;
	x.set(
		newCompositeQuarticValue(
			prev->x.get()->getValue(ticksSinceLastUpdate), (float)xDirection * speedPerTick, 0.0f, 0.0f, 0.0f));
	y.set(
		newCompositeQuarticValue(
			prev->y.get()->getValue(ticksSinceLastUpdate), (float)yDirection * speedPerTick, 0.0f, 0.0f, 0.0f));
	lastUpdateTicksTime = ticksTime;
	renderInterpolatedX = true;
	renderInterpolatedY = true;
	z = prev->z;
}
//check if we moved onto map tiles of a different height, using the previous move direction to figure out where to look
//if we did, move the player and don't render interpolated positions
void PlayerState::collideWithEnvironmentWithPreviousPlayerState(PlayerState* prev) {
	//find the row and column of tiles that we could collide with
	float xPosition = x.get()->getValue(0);
	float yPosition = y.get()->getValue(0);
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
		x.set(
			x.get()->copyWithConstantValue(prev->xDirection < 0
				? wallX + MapState::smallDistance - boundingBoxLeftOffset
				: wallX - MapState::smallDistance - boundingBoxRightOffset));
	}
	if (hasYCollision) {
		renderInterpolatedY = false;
		y.set(
			y.get()->copyWithConstantValue(prev->yDirection < 0
				? wallY + MapState::smallDistance - boundingBoxTopOffset
				: wallY - MapState::smallDistance - boundingBoxBottomOffset));
	}
}
//update the sprite (and possibly the animation) of this player state by reading from the previous state
void PlayerState::updateSpriteWithPreviousPlayerState(PlayerState* prev, int ticksTime, bool usePreviousStateSpriteAnimation) {
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
						&& (prev->spriteDirection == SpriteDirection::Left
							|| prev->spriteDirection == SpriteDirection::Right)))))
		spriteDirection = xDirection < 0 ? SpriteDirection::Left : SpriteDirection::Right;
	//use a vertical sprite if none of the above situations applied
	else
		spriteDirection = yDirection < 0 ? SpriteDirection::Up : SpriteDirection::Down;

	//update the animation
	if (!moving)
		animation = nullptr;
	else if (prev->animation != nullptr && usePreviousStateSpriteAnimation) {
		animation = prev->animation;
		animationStartTicksTime = prev->animationStartTicksTime;
	} else {
		animation = hasBoot ? SpriteRegistry::playerBootWalkingAnimation : SpriteRegistry::playerWalkingAnimation;
		animationStartTicksTime = ticksTime;
	}
}
//if we don't have a kicking animation, start one
//this should be called after the player has been updated
void PlayerState::beginKicking(int ticksTime) {
	if (kickingAnimation.get() != nullptr)
		return;

	xDirection = 0;
	yDirection = 0;
	renderInterpolatedX = true;
	renderInterpolatedY = true;

	SpriteAnimation* kickingSpriteAnimation = hasBoot
		? SpriteRegistry::playerKickingAnimation
		: SpriteRegistry::playerLegLiftAnimation;
	int kickingDuration = kickingSpriteAnimation->getTotalTicksDuration();
	vector<ReferenceCounterHolder<EntityAnimation::Component>> kickingAnimationComponents ({
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)),
		newEntityAnimationSetSpriteAnimation(kickingSpriteAnimation)
	});

	//kicking doesn't do anything if you don't have the boot
	//add the delay for the animation and then return
	if (!hasBoot) {
		kickingAnimationComponents.push_back(newEntityAnimationDelay(kickingDuration));
		kickingAnimation.set(newEntityAnimation(ticksTime, kickingAnimationComponents));
		kickingAnimation.get()->update(this, ticksTime);
		return;
	}

	//only kick something if you're less than this distance from it
	//visually, you have to be 1 pixel away or closer
	const float kickingDistanceLimit = 1.5f;

	//we are kicking, start by delaying until the leg-sticking-out frame
	kickingAnimationComponents.push_back(newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame));
	kickingDuration -= SpriteRegistry::playerKickingAnimationTicksPerFrame;

	//check if we're climbing, falling, or kicking a switch
	float xPosition = x.get()->getValue(0);
	float yPosition = y.get()->getValue(0);
	int lowMapX = (int)(xPosition + boundingBoxLeftOffset) / MapState::tileSize;
	int highMapX = (int)(xPosition + boundingBoxRightOffset) / MapState::tileSize;

	if (spriteDirection == SpriteDirection::Up) {
		int oneTileUpMapY = (int)(yPosition + boundingBoxTopOffset - kickingDistanceLimit) / MapState::tileSize;
		char oneTileUpHeight = MapState::horizontalTilesHeight(lowMapX, highMapX, oneTileUpMapY);
		if (oneTileUpHeight != MapState::invalidHeight) {
			//we found a floor to fall to
			if (oneTileUpHeight < z && (oneTileUpHeight & 1) == 0) {
				//move a distance such that the bottom of the player is slightly past the edge of the cliff
				float targetYPosition =
					(float)((oneTileUpMapY + 1) * MapState::tileSize) - boundingBoxBottomOffset - MapState::smallDistance;
				float moveDistance = targetYPosition - yPosition;
				//we want a cubic curve that goes through (0,0) and (1,1) and a chosen midpoint (i,j) where 0 < i < 1
				//it also goes through (c,#) such that dy/dt has roots at c (trough) and i (crest)
				//vy = d(t-c)(t-i) = d(t^2-(c+i)t+ci)   (d < 0)
				//y = d(t^3/3-(c+i)t^2/2+cit) = dt^3/3-d(c+i)t^2/2+dcit
				//1 = d(1^3/3-(c+i)1^2/2+ci1) = d(1/3-(c+i)/2+ci)
				//	d = 1/(1/3-(c+i)/2+ci) = 6/(2-3c-3i+6ci)
				//j = d(i^3/3-(c+i)i^2/2+cii) = d(i^3/3-ci^2/2-i^3/2+ci^2) = d(-i^3/6+ci^2/2)
				//	d = j/(-i^3/6+ci^2/2) = 6j/(-i^3+3ci^2)
				//solve for c:
				//6/(2-3c-3i+6ci) = 6j/(-i^3+3ci^2)
				//	6(-i^3+3ci^2) = 6j(2-3c-3i+6ci)
				//	-i^3+3ci^2 = j(2-3c-3i+6ci) = 2j-3jc-3ji+6jci
				//	2j-3ji+i^3 = 3ci^2+3jc-6jci = c(3i^2+3j-6ji)
				//	c = (2j-3ji+i^3)/(3i^2+3j-6ji)
				const float midpointX = 2.0f / 3.0f;
				const float midpointY = 2.0f;
				const float troughX =
					(2.0f * midpointY - 3.0f * midpointY * midpointX + midpointX * midpointX * midpointX)
						/ (3.0f * midpointX * midpointX + 3.0f * midpointY - 6.0f * midpointY * midpointX);
				float yMultiplier =
					6.0f / (2.0f - 3.0f * troughX - 3.0f * midpointX + 6.0f * troughX * midpointX) * moveDistance;

				float linearValuePerDuration = yMultiplier * troughX * midpointX;
				float quadraticValuePerDuration = -yMultiplier * (troughX + midpointX) / 2.0f;
				float cubicValuePerDuration = yMultiplier / 3.0f;

				float floatKickingDuration = (float)kickingDuration;
				float kickingDurationSquared = floatKickingDuration * floatKickingDuration;
				kickingAnimationComponents.push_back(
					newEntityAnimationSetVelocity(
						newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
						newCompositeQuarticValue(
							0.0f,
							linearValuePerDuration / floatKickingDuration,
							quadraticValuePerDuration / kickingDurationSquared,
							cubicValuePerDuration / (kickingDurationSquared * floatKickingDuration),
							0.0f)));
				z = oneTileUpHeight;
			//we found a ledge to climb up
			} else if (oneTileUpHeight == z + 1
				&& MapState::horizontalTilesHeight(lowMapX, highMapX, oneTileUpMapY - 1) == z + 2)
			{
				//move a distance such that the bottom of the player is slightly past the edge of the ledge
				float targetYPosition =
					(float)(oneTileUpMapY * MapState::tileSize) - boundingBoxBottomOffset - MapState::smallDistance;
				float moveDistance = targetYPosition - yPosition;

				float cubicValuePerDuration = moveDistance / 2.0f;
				float quarticValuePerDuration = moveDistance / 2.0f;

				float floatKickingDuration = (float)kickingDuration;
				float kickingDurationCubed = floatKickingDuration * floatKickingDuration * floatKickingDuration;
				kickingAnimationComponents.push_back(
					newEntityAnimationSetVelocity(
						newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
						newCompositeQuarticValue(
							0.0f,
							0.0f,
							0.0f,
							cubicValuePerDuration / kickingDurationCubed,
							quarticValuePerDuration / (kickingDurationCubed * floatKickingDuration))));
				z += 2;
			}
		}
		//TODO: check if we're kicking a switch north
	} else if (spriteDirection == SpriteDirection::Down) {
		int oneTileDownMapY = (int)(yPosition + boundingBoxBottomOffset + kickingDistanceLimit) / MapState::tileSize;
		char fallHeight = MapState::invalidHeight;
		int tileOffset = 0;
		for (; true; tileOffset++) {
			fallHeight = MapState::horizontalTilesHeight(lowMapX, highMapX, oneTileDownMapY + tileOffset);
			//stop looking if the heights differ
			if (fallHeight == MapState::invalidHeight)
				break;
			//cliff face, keep looking
			else if (fallHeight == z - 1 - tileOffset * 2)
				;
			//this is a tile we can fall to
			else if (fallHeight == z - tileOffset * 2 && tileOffset >= 1)
				break;
			//the row is higher than us so stop looking
			else {
				fallHeight = MapState::invalidHeight;
				break;
			}
		}
		//fall if we can
		if (fallHeight != MapState::invalidHeight) {
			//move a distance such that the bottom of the player is slightly past the edge of the cliff
			float targetYPosition =
				(float)((oneTileDownMapY + tileOffset) * MapState::tileSize) - boundingBoxTopOffset + MapState::smallDistance;
			float moveDistance = targetYPosition - yPosition;

			float linearValuePerDuration = -moveDistance;
			float quadraticValuePerDuration = moveDistance * 2.0f;

			float floatKickingDuration = (float)kickingDuration;
			kickingAnimationComponents.push_back(
				newEntityAnimationSetVelocity(
					newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
					newCompositeQuarticValue(
						0.0f,
						linearValuePerDuration / floatKickingDuration,
						quadraticValuePerDuration / (floatKickingDuration * floatKickingDuration),
						0.0f,
						0.0f)));
			z = fallHeight;
		}
	} else {
		int sideTilesLeftMapX;
		int sideTilesRightMapX;
		if (spriteDirection == SpriteDirection::Left) {
			float sideTilesRightXPosition = xPosition + boundingBoxLeftOffset - kickingDistanceLimit;
			sideTilesRightMapX = (int)sideTilesRightXPosition / MapState::tileSize;
			sideTilesLeftMapX = (int)(sideTilesRightXPosition - playerWidth) / MapState::tileSize;
		} else {
			float sideTilesLeftXPosition = xPosition + boundingBoxRightOffset + kickingDistanceLimit;
			sideTilesLeftMapX = (int)sideTilesLeftXPosition / MapState::tileSize;
			sideTilesRightMapX = (int)(sideTilesLeftXPosition + playerWidth) / MapState::tileSize;
		}
		int bottomMapY = (int)(yPosition + boundingBoxBottomOffset) / MapState::tileSize;
		int tileOffset = 1;
		char fallHeight = MapState::invalidHeight;
		for (; true; tileOffset++) {
			fallHeight = MapState::getHeight(sideTilesLeftMapX, bottomMapY + tileOffset);
			//this is the height we're trying to fall to
			if (fallHeight == z - tileOffset * 2) {
				//check that all tiles the player will land on are valid
				int offsetTopMapY = (int)(yPosition + boundingBoxTopOffset) / MapState::tileSize + tileOffset;
				for (int mapY = bottomMapY + tileOffset; mapY >= offsetTopMapY; mapY--) {
					//even if we found a tile we can land on, if the rest of them don't match then we can't fall
					if (MapState::horizontalTilesHeight(sideTilesLeftMapX, sideTilesRightMapX, mapY) != fallHeight) {
						fallHeight = MapState::invalidHeight;
						break;
					}
				}
				break;
			//this is part of a cliff or floor tile that's behind the player, keep going
			//the tile might be near us or far back, either case is fine
			} else if (fallHeight <= z - tileOffset * 2 + 1)
				;
			//anything else is invalid
			else {
				fallHeight = MapState::invalidHeight;
				break;
			}
		}
		//fall if we can
		if (fallHeight != MapState::invalidHeight) {
			float targetXPosition = spriteDirection == SpriteDirection::Left
				? (float)((sideTilesRightMapX + 1) * MapState::tileSize) - boundingBoxRightOffset - MapState::smallDistance
				: (float)(sideTilesLeftMapX * MapState::tileSize) - boundingBoxLeftOffset + MapState::smallDistance;
			float xMoveDistance = targetXPosition - xPosition;
			float yMoveDistance = (float)(tileOffset * MapState::tileSize);

			const float yLinearValuePerDuration = -yMoveDistance;
			const float yQuadraticValuePerDuration = yMoveDistance * 2.0f;

			float floatKickingDuration = (float)kickingDuration;
			kickingAnimationComponents.push_back(
				newEntityAnimationSetVelocity(
					newCompositeQuarticValue(0.0f, xMoveDistance / floatKickingDuration, 0.0f, 0.0f, 0.0f),
					newCompositeQuarticValue(
						0.0f,
						yLinearValuePerDuration / floatKickingDuration,
						yQuadraticValuePerDuration / (floatKickingDuration * floatKickingDuration),
						0.0f,
						0.0f)));
			z = fallHeight;
		}
		//TODO: check if we're kicking a switch to the side
	}

	//delay for the rest of the animation and then stop the player
	kickingAnimationComponents.push_back(newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame * 2));
	kickingAnimationComponents.push_back(
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)));
	kickingAnimation.set(newEntityAnimation(ticksTime, kickingAnimationComponents));
	//update it once to get it started
	kickingAnimation.get()->update(this, ticksTime);
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
		SpriteRegistry::player->renderSpriteCenteredAtScreenPosition(
			hasBoot ? 4 : 0, (int)spriteDirection, renderCenterX, renderCenterY);
}
