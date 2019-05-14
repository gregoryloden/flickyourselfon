#include "PlayerState.h"
#include "GameState/DynamicValue.h"
#include "GameState/GameState.h"
#include "GameState/EntityAnimation.h"
#include "GameState/MapState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"
#include "Util/Logger.h"
#include "Util/StringUtils.h"

#ifdef DEBUG
	//**/#define SHOW_PLAYER_BOUNDING_BOX 1
#endif

const float PlayerState::playerWidth = 11.0f;
const float PlayerState::playerHeight = 5.0f;
const float PlayerState::boundingBoxLeftOffset = PlayerState::playerWidth * -0.5f;
const float PlayerState::boundingBoxRightOffset = PlayerState::boundingBoxLeftOffset + PlayerState::playerWidth;
const float PlayerState::boundingBoxTopOffset = 4.5f;
const float PlayerState::boundingBoxBottomOffset = PlayerState::boundingBoxTopOffset + PlayerState::playerHeight;
const float PlayerState::boundingBoxCenterYOffset = PlayerState::boundingBoxTopOffset + PlayerState::playerHeight * 0.5f;
const float PlayerState::introAnimationPlayerCenterX = 70.5f;
const float PlayerState::introAnimationPlayerCenterY = 106.5f;
const float PlayerState::speedPerSecond = 40.0f;
const float PlayerState::diagonalSpeedPerSecond = speedPerSecond * sqrt(0.5f);
const float PlayerState::kickingDistanceLimit = 1.5f;
const string PlayerState::playerXFilePrefix = "playerX ";
const string PlayerState::playerYFilePrefix = "playerY ";
const string PlayerState::playerZFilePrefix = "playerZ ";
PlayerState::PlayerState(objCounterParameters())
: EntityState(objCounterArguments())
, xDirection(0)
, yDirection(0)
, spriteAnimation(nullptr)
, spriteAnimationStartTicksTime(-1)
, spriteDirection(SpriteDirection::Down)
, hasBoot(false)
, lastControlledX(0.0f)
, lastControlledY(0.0f)
, lastControlledZ(0) {
	lastControlledX = x.get()->getValue(0);
	lastControlledY = y.get()->getValue(0);
	lastControlledZ = z;
}
PlayerState::~PlayerState() {}
//initialize and return a PlayerState
PlayerState* PlayerState::produce(objCounterParameters()) {
	initializeWithNewFromPool(p, PlayerState)
	//TODO: what parameters does this function need to properly initialize a player state?
	return p;
}
//copy the other state
void PlayerState::copyPlayerState(PlayerState* other) {
	copyEntityState(other);
	xDirection = other->xDirection;
	yDirection = other->yDirection;
	spriteAnimation = other->spriteAnimation;
	spriteAnimationStartTicksTime = other->spriteAnimationStartTicksTime;
	spriteDirection = other->spriteDirection;
	hasBoot = other->hasBoot;
}
pooledReferenceCounterDefineRelease(PlayerState)
//use the player as the next camera anchor unless we're starting an animation with a new camera anchor
void PlayerState::setNextCamera(GameState* nextGameState, int ticksTime) {
	//TODO: set a different camera anchor at the end of our animation
	nextGameState->setPlayerCamera();
}
//set the animation to the given animation at the given time
void PlayerState::setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime) {
	spriteAnimation = pSpriteAnimation;
	spriteAnimationStartTicksTime = pSpriteAnimationStartTicksTime;
}
//update this player state by reading from the previous state
void PlayerState::updateWithPreviousPlayerState(PlayerState* prev, int ticksTime) {
	bool previousStateHadEntityAnimation = prev->entityAnimation.get() != nullptr;

	//if we have an entity animation, update with that instead
	if (previousStateHadEntityAnimation) {
		copyPlayerState(prev);
		if (entityAnimation.get()->update(this, ticksTime))
			return;
	}
	entityAnimation.set(nullptr);

	//if we can control the player then that must mean the player has the boot
	hasBoot = true;

	//update this player state normally by reading from the last state
	updatePositionWithPreviousPlayerState(prev, ticksTime);
	#ifndef EDITOR
		collideWithEnvironmentWithPreviousPlayerState(prev);
	#endif
	updateSpriteWithPreviousPlayerState(prev, ticksTime, !previousStateHadEntityAnimation);

	//copy the position to the save values
	lastControlledX = x.get()->getValue(0);
	lastControlledY = y.get()->getValue(0);
	lastControlledZ = z;
}
//update the position of this player state by reading from the previous state
void PlayerState::updatePositionWithPreviousPlayerState(PlayerState* prev, int ticksTime) {
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	xDirection = (char)(keyboardState[Config::keyBindings.rightKey] - keyboardState[Config::keyBindings.leftKey]);
	yDirection = (char)(keyboardState[Config::keyBindings.downKey] - keyboardState[Config::keyBindings.upKey]);
	float speedPerTick =
		((xDirection & yDirection) != 0 ? diagonalSpeedPerSecond : speedPerSecond) / (float)Config::ticksPerSecond;
	#ifdef EDITOR
		if (keyboardState[Config::keyBindings.kickKey] != 0)
			speedPerTick *= 8.0f;
	#endif

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
	int switchCollisionXOffset = 0;
	if (prev->xDirection != 0) {
		//don't check the corner
		int iterLowMapY = prev->yDirection < 0 ? lowMapY + 1 : lowMapY;
		int iterHighMapY = prev->yDirection > 0 ? highMapY - 1 : highMapY;
		for (int currentMapY = iterLowMapY; currentMapY <= iterHighMapY; currentMapY++) {
			if (MapState::getHeight(collisionMapX, currentMapY) != z) {
				hasXCollision = true;
				break;
			}
			if (MapState::tileHasSwitch(collisionMapX, currentMapY) &&
				(MapState::getRailSwitchId(collisionMapX, currentMapY)
						== MapState::getRailSwitchId(collisionMapX, currentMapY + 1)
					//looking at the top of the switch, only collide if our bottom edge is past the top of the switch
					? playerBottomEdge >= (float)(currentMapY * MapState::tileSize + 1)
					//looking at the bottom of the switch, only collide if our top edge is past the bottom of the switch
					: playerTopEdge <= (float)((currentMapY + 1) * MapState::tileSize - 2)))
			{
				//we went left, check if the player is far enough into the switch right side
				if (prev->xDirection < 0) {
					if (playerLeftEdge <= (float)((collisionMapX + 1) * MapState::tileSize - 2))
						switchCollisionXOffset = -2;
				//we went right, check if the player is far enough into the switch left side
				} else {
					if (playerRightEdge >= (float)(collisionMapX * MapState::tileSize + 2))
						switchCollisionXOffset = 2;
				}
			}
		}
		if (hasXCollision)
			switchCollisionXOffset = 0;
		else if (switchCollisionXOffset != 0)
			hasXCollision = true;
	}
	//check for vertical collisions, excluding the corner if we're moving diagonally
	int switchCollisionYOffset = 0;
	if (prev->yDirection != 0) {
		//don't check the corner
		int iterLowMapX = prev->xDirection < 0 ? lowMapX + 1 : lowMapX;
		int iterHighMapX = prev->xDirection > 0 ? highMapX - 1 : highMapX;
		for (int currentMapX = iterLowMapX; currentMapX <= iterHighMapX; currentMapX++) {
			if (MapState::getHeight(currentMapX, collisionMapY) != z) {
				hasYCollision = true;
				break;
			}
			if (MapState::tileHasSwitch(currentMapX, collisionMapY) &&
				(MapState::getRailSwitchId(currentMapX, collisionMapY)
						== MapState::getRailSwitchId(currentMapX + 1, collisionMapY)
					//looking at the left of the switch, only collide if our right edge is past the left of the switch
					? playerRightEdge >= (float)(currentMapX * MapState::tileSize + 2)
					//looking at the right of the switch, only collide if our left edge is past the right of the switch
					: playerLeftEdge <= (float)((currentMapX + 1) * MapState::tileSize - 2)))
			{
				//we went up, check if the player is far enough into the switch bottom
				if (prev->yDirection < 0) {
					if (playerTopEdge <= (float)((collisionMapY + 1) * MapState::tileSize - 2))
						switchCollisionYOffset = -2;
				//we went down, check if the player is far enough into the switch top
				} else {
					if (playerBottomEdge >= (float)(collisionMapY * MapState::tileSize + 1))
						switchCollisionYOffset = 1;
				}
			}
		}
		if (hasYCollision)
			switchCollisionYOffset = 0;
		else if (switchCollisionYOffset != 0)
			hasYCollision = true;
	}

	//if we haven't collided with any walls yet and we're moving diagonally, try to collide with the corner
	if ((!hasXCollision || switchCollisionXOffset != 0)
		&& (!hasYCollision || switchCollisionYOffset != 0)
		&& prev->xDirection != 0
		&& prev->yDirection != 0)
	{
		//if the heights differ then we have a collision, if not then check for a switch if we didn't already collide with one
		bool hasCornerCollision = MapState::getHeight(collisionMapX, collisionMapY) != z;
		if (!hasCornerCollision && !hasXCollision && !hasYCollision && MapState::tileHasSwitch(collisionMapX, collisionMapY)) {
			//the corner has a switch, this is only a collision if both the x and y are in it
			if (prev->xDirection < 0) {
				if (playerLeftEdge <= (float)((collisionMapX + 1) * MapState::tileSize - 2))
					switchCollisionXOffset = -2;
			} else {
				if (playerRightEdge >= (float)(collisionMapX * MapState::tileSize + 2))
					switchCollisionXOffset = 2;
			}
			if (prev->yDirection < 0) {
				if (playerTopEdge <= (float)((collisionMapY + 1) * MapState::tileSize - 2))
					switchCollisionYOffset = -2;
			} else {
				if (playerBottomEdge >= (float)(collisionMapY * MapState::tileSize + 1))
					switchCollisionYOffset = 1;
			}
			if (switchCollisionXOffset != 0 && switchCollisionYOffset != 0) {
				wallX += (float)switchCollisionXOffset;
				wallY += (float)switchCollisionYOffset;
				switchCollisionXOffset = 0;
				switchCollisionYOffset = 0;
				hasCornerCollision = true;
			}
		}
		if (hasCornerCollision) {
			//move in whichever direction makes us move less
			float xCollisionDistance = prev->xDirection < 0 ? wallX - playerLeftEdge : playerRightEdge - wallX;
			float yCollisionDistance = prev->yDirection < 0 ? wallY - playerTopEdge : playerBottomEdge - wallY;
			if (xCollisionDistance < yCollisionDistance)
				hasXCollision = true;
			else
				hasYCollision = true;
		}
	}

	//now add the switch offsets
	//in the event that we collided with a switch corner, wall x and y were already offset and the offsets were reset to 0
	wallX += (float)switchCollisionXOffset;
	wallY += (float)switchCollisionYOffset;

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
		spriteAnimation = nullptr;
	else if (prev->spriteAnimation != nullptr && usePreviousStateSpriteAnimation) {
		spriteAnimation = prev->spriteAnimation;
		spriteAnimationStartTicksTime = prev->spriteAnimationStartTicksTime;
	} else {
		spriteAnimation = hasBoot ? SpriteRegistry::playerBootWalkingAnimation : SpriteRegistry::playerWalkingAnimation;
		spriteAnimationStartTicksTime = ticksTime;
	}
}
//if we don't have a kicking animation, start one
//this should be called after the player has been updated
void PlayerState::beginKicking(MapState* mapState, int ticksTime) {
	if (entityAnimation.get() != nullptr)
		return;

	xDirection = 0;
	yDirection = 0;
	renderInterpolatedX = true;
	renderInterpolatedY = true;

	//kicking doesn't do anything if you don't have the boot
	if (!hasBoot) {
		kickAir(ticksTime);
		return;
	}

	//check if we're climbing, falling, or kicking a switch
	float xPosition = x.get()->getValue(0);
	float yPosition = y.get()->getValue(0);
	int lowMapX = (int)(xPosition + boundingBoxLeftOffset) / MapState::tileSize;
	int highMapX = (int)(xPosition + boundingBoxRightOffset) / MapState::tileSize;

	if (spriteDirection == SpriteDirection::Up) {
		int railMapX = (int)xPosition / MapState::tileSize;
		int railMapY = (int)(yPosition + boundingBoxTopOffset) / MapState::tileSize;
		if (kickRail(mapState, railMapX, railMapY, xPosition, yPosition, ticksTime))
			return;

		int oneTileUpMapY = (int)(yPosition + boundingBoxTopOffset - kickingDistanceLimit) / MapState::tileSize;
		char oneTileUpHeight = MapState::horizontalTilesHeight(lowMapX, highMapX, oneTileUpMapY);
		if (oneTileUpHeight != MapState::invalidHeight) {
			//we found a floor to fall to
			if (oneTileUpHeight < z && (oneTileUpHeight & 1) == 0) {
				//move a distance such that the bottom of the player is slightly above the top of the cliff
				float targetYPosition =
					(float)((oneTileUpMapY + 1) * MapState::tileSize) - boundingBoxBottomOffset - MapState::smallDistance;
				kickFall(0.0f, targetYPosition - yPosition, oneTileUpHeight, ticksTime);
				return;
			//we found a ledge to climb up
			} else if (oneTileUpHeight == z + 1
				&& MapState::horizontalTilesHeight(lowMapX, highMapX, oneTileUpMapY - 1) == z + 2)
			{
				//move a distance such that the bottom of the player is slightly past the edge of the ledge
				float targetYPosition =
					(float)(oneTileUpMapY * MapState::tileSize) - boundingBoxBottomOffset - MapState::smallDistance;
				kickClimb(targetYPosition - yPosition, ticksTime);
				return;
			}
		}
		//TODO: check if we're kicking a switch north
	} else if (spriteDirection == SpriteDirection::Down) {
		int railMapX = (int)xPosition / MapState::tileSize;
		int railMapY = (int)(yPosition + boundingBoxBottomOffset) / MapState::tileSize;
		if (kickRail(mapState, railMapX, railMapY, xPosition, yPosition, ticksTime))
			return;

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
			//move a distance such that the bottom of the player is slightly below the bottom of the cliff
			float targetYPosition =
				(float)((oneTileDownMapY + tileOffset) * MapState::tileSize) - boundingBoxTopOffset + MapState::smallDistance;
			kickFall(0.0f, targetYPosition - yPosition, fallHeight, ticksTime);
			return;
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

		int railMapX =
			(int)(xPosition + (spriteDirection == SpriteDirection::Left ? boundingBoxLeftOffset : boundingBoxRightOffset))
				/ MapState::tileSize;
		int railMapY = (int)(yPosition + boundingBoxCenterYOffset) / MapState::tileSize;
		if (kickRail(mapState, railMapX, railMapY, xPosition, yPosition, ticksTime))
			return;

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
			kickFall(targetXPosition - xPosition, (float)(tileOffset * MapState::tileSize), fallHeight, ticksTime);
			return;
		}
		//TODO: check if we're kicking a switch to the side
	}

	//we didn't do anything with the kick, just play the animation
	kickAir(ticksTime);
}
//begin a kicking animation without changing any state
void PlayerState::kickAir(int ticksTime) {
	SpriteAnimation* kickingSpriteAnimation = hasBoot
		? SpriteRegistry::playerKickingAnimation
		: SpriteRegistry::playerLegLiftAnimation;
	beginEntityAnimation(
		newEntityAnimation(
			ticksTime,
			{
				newEntityAnimationSetVelocity(
					newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
					newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)) COMMA
				newEntityAnimationSetSpriteAnimation(kickingSpriteAnimation) COMMA
				newEntityAnimationDelay(kickingSpriteAnimation->getTotalTicksDuration())
			}),
		ticksTime);
}
//begin a kicking animation and climb up to the next tile to the north
void PlayerState::kickClimb(float yMoveDistance, int ticksTime) {
	//start by stopping the player and delaying until the leg-sticking-out frame
	vector<ReferenceCounterHolder<EntityAnimation::Component>> kickingAnimationComponents ({
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)) COMMA
		newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerKickingAnimation) COMMA
		newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame)
	});

	int moveDuration =
		SpriteRegistry::playerKickingAnimation->getTotalTicksDuration() - SpriteRegistry::playerKickingAnimationTicksPerFrame;
	float floatMoveDuration = (float)moveDuration;
	float moveDurationCubed = floatMoveDuration * floatMoveDuration * floatMoveDuration;

	float yCubicValuePerDuration = yMoveDistance / 2.0f;
	float yQuarticValuePerDuration = yMoveDistance / 2.0f;

	kickingAnimationComponents.push_back(
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(
				0.0f,
				0.0f,
				0.0f,
				yCubicValuePerDuration / moveDurationCubed,
				yQuarticValuePerDuration / (moveDurationCubed * floatMoveDuration))));

	//delay for the rest of the animation and then stop the player
	kickingAnimationComponents.push_back(newEntityAnimationDelay(moveDuration));
	kickingAnimationComponents.push_back(
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)));

	beginEntityAnimation(newEntityAnimation(ticksTime, kickingAnimationComponents), ticksTime);
	z += 2;
}
//begin a kicking animation and fall in whatever direction we're facing
void PlayerState::kickFall(float xMoveDistance, float yMoveDistance, char fallHeight, int ticksTime) {
	//start by stopping the player and delaying until the leg-sticking-out frame
	vector<ReferenceCounterHolder<EntityAnimation::Component>> kickingAnimationComponents ({
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)) COMMA
		newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerKickingAnimation) COMMA
		newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame)
	});

	int moveDuration =
		SpriteRegistry::playerKickingAnimation->getTotalTicksDuration() - SpriteRegistry::playerKickingAnimationTicksPerFrame;
	float floatMoveDuration = (float)moveDuration;
	float moveDurationSquared = floatMoveDuration * floatMoveDuration;

	if (spriteDirection == SpriteDirection::Up) {
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
			6.0f / (2.0f - 3.0f * troughX - 3.0f * midpointX + 6.0f * troughX * midpointX) * yMoveDistance;

		float yLinearValuePerDuration = yMultiplier * troughX * midpointX;
		float yQuadraticValuePerDuration = -yMultiplier * (troughX + midpointX) / 2.0f;
		float yCubicValuePerDuration = yMultiplier / 3.0f;

		kickingAnimationComponents.push_back(
			newEntityAnimationSetVelocity(
				newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(
					0.0f,
					yLinearValuePerDuration / floatMoveDuration,
					yQuadraticValuePerDuration / moveDurationSquared,
					yCubicValuePerDuration / (moveDurationSquared * floatMoveDuration),
					0.0f)));
	//left, right, and down all have the same quadratic jump trajectory
	} else {
		const float yLinearValuePerDuration = -yMoveDistance;
		const float yQuadraticValuePerDuration = yMoveDistance * 2.0f;

		kickingAnimationComponents.push_back(
			newEntityAnimationSetVelocity(
				newCompositeQuarticValue(0.0f, xMoveDistance / floatMoveDuration, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(
					0.0f,
					yLinearValuePerDuration / floatMoveDuration,
					yQuadraticValuePerDuration / moveDurationSquared,
					0.0f,
					0.0f)));
	}

	//delay for the rest of the animation and then stop the player
	kickingAnimationComponents.push_back(newEntityAnimationDelay(moveDuration));
	kickingAnimationComponents.push_back(
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)));

	beginEntityAnimation(newEntityAnimation(ticksTime, kickingAnimationComponents), ticksTime);
	z = fallHeight;
}
//check if we're kicking the end of a rail, and if so, begin a kicking animation and ride it
//returns whether we handled a rail kick
bool PlayerState::kickRail(MapState* mapState, int railMapX, int railMapY, float xPosition, float yPosition, int ticksTime) {
	if (!MapState::tileHasRail(railMapX, railMapY))
		return false;

	MapState::RailState* railState = mapState->getRailState(railMapX, railMapY);
	MapState::Rail* rail = railState->getRail();
	int endSegmentIndex = rail->getSegmentCount() - 1;
	MapState::Rail::Segment* startSegment = rail->getSegment(0);
	MapState::Rail::Segment* endSegment = rail->getSegment(endSegmentIndex);
	int firstSegmentIndex = 0;
	int lastSegmentIndex = 0;
	int segmentIndexIncrement;
	if (startSegment->x == railMapX && startSegment->y == railMapY) {
		lastSegmentIndex = endSegmentIndex;
		segmentIndexIncrement = 1;
	} else if (endSegment->x == railMapX && endSegment->y == railMapY) {
		firstSegmentIndex = endSegmentIndex;
		segmentIndexIncrement = -1;
	} else
		return false;

	//if it's lowered, we can't ride it but don't try to fall
	if (!railState->canRide()) {
		kickAir(ticksTime);
		return true;
	}

	const float bootLiftDuration = (float)SpriteRegistry::playerKickingAnimationTicksPerFrame;
	const float floatRailToRailTicksDuration = (float)railToRailTicksDuration;
	const float railToRailTicksDurationSquared = floatRailToRailTicksDuration * floatRailToRailTicksDuration;
	const float halfTileSize = (float)MapState::tileSize * 0.5f;

	vector<ReferenceCounterHolder<EntityAnimation::Component>> ridingRailAnimationComponents ({
		newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerBootLiftAnimation)
	});

	MapState::Rail::Segment* nextSegment = rail->getSegment(firstSegmentIndex);
	int nextSegmentIndex = firstSegmentIndex;
	bool firstMove = true;
	float targetXPosition = xPosition;
	float targetYPosition = yPosition;
	while (nextSegmentIndex != lastSegmentIndex) {
		float lastXPosition = targetXPosition;
		float lastYPosition = targetYPosition;
		nextSegmentIndex += segmentIndexIncrement;
		MapState::Rail::Segment* currentSegment = nextSegment;
		nextSegment = rail->getSegment(nextSegmentIndex);

		targetXPosition = (float)(currentSegment->x * MapState::tileSize) + halfTileSize;
		targetYPosition = (float)(currentSegment->y * MapState::tileSize) + halfTileSize - boundingBoxBottomOffset;
		bool fromSide = lastYPosition == targetYPosition;
		bool toSide = false;
		SpriteDirection nextSpriteDirection;
		if (nextSegment->x < currentSegment->x) {
			targetXPosition -= halfTileSize;
			toSide = true;
			nextSpriteDirection = SpriteDirection::Left;
		} else if (nextSegment->x > currentSegment->x) {
			targetXPosition += halfTileSize;
			toSide = true;
			nextSpriteDirection = SpriteDirection::Right;
		} else if (nextSegment->y < currentSegment->y) {
			targetYPosition -= halfTileSize;
			//the boot is on the right foot, move left so the boot is centered over the rail when we go up
			targetXPosition -= 0.5f;
			nextSpriteDirection = SpriteDirection::Up;
		} else {
			targetYPosition += halfTileSize;
			//the boot is on the right foot, move right so the boot is centered over the rail when we go down
			targetXPosition += 0.5f;
			nextSpriteDirection = SpriteDirection::Down;
		}

		float xMoveDistance = targetXPosition - lastXPosition;
		float yMoveDistance = targetYPosition - lastYPosition;
		//we haven't done a move yet, get to the initial position
		if (firstMove) {
			ridingRailAnimationComponents.push_back(
				newEntityAnimationSetVelocity(
					newCompositeQuarticValue(0.0f, xMoveDistance / bootLiftDuration, 0.0f, 0.0f, 0.0f),
					newCompositeQuarticValue(0.0f, yMoveDistance / bootLiftDuration, 0.0f, 0.0f, 0.0f)));
			ridingRailAnimationComponents.push_back(
				newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame));
			ridingRailAnimationComponents.push_back(
				newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerRidingRailAnimation));
			ridingRailAnimationComponents.push_back(newEntityAnimationSetSpriteDirection(nextSpriteDirection));
			firstMove = false;
		//straight section
		} else if (fromSide == toSide) {
			ridingRailAnimationComponents.push_back(
				newEntityAnimationSetVelocity(
					newCompositeQuarticValue(0.0f, xMoveDistance / floatRailToRailTicksDuration, 0.0f, 0.0f, 0.0f),
					newCompositeQuarticValue(0.0f, yMoveDistance / floatRailToRailTicksDuration, 0.0f, 0.0f, 0.0f)));
			ridingRailAnimationComponents.push_back(newEntityAnimationDelay(railToRailTicksDuration));
		//curved section
		} else {
			//curved section going from side to top/bottom
			if (fromSide)
				ridingRailAnimationComponents.push_back(
					newEntityAnimationSetVelocity(
						newCompositeQuarticValue(
							0.0f,
							2.0f * xMoveDistance / floatRailToRailTicksDuration,
							-xMoveDistance / railToRailTicksDurationSquared,
							0.0f,
							0.0f),
						newCompositeQuarticValue(
							0.0f,
							0.0f,
							yMoveDistance / railToRailTicksDurationSquared,
							0.0f,
							0.0f)));
			//curved section going from top/bottom to side
			else
				ridingRailAnimationComponents.push_back(
					newEntityAnimationSetVelocity(
						newCompositeQuarticValue(
							0.0f,
							0.0f,
							xMoveDistance / railToRailTicksDurationSquared,
							0.0f,
							0.0f),
						newCompositeQuarticValue(
							0.0f,
							2.0f * yMoveDistance / floatRailToRailTicksDuration,
							-yMoveDistance / railToRailTicksDurationSquared,
							0.0f,
							0.0f)));
			const int halfRailToRailTicksDuration = railToRailTicksDuration / 2;
			ridingRailAnimationComponents.push_back(newEntityAnimationDelay(halfRailToRailTicksDuration));
			ridingRailAnimationComponents.push_back(newEntityAnimationSetSpriteDirection(nextSpriteDirection));
			ridingRailAnimationComponents.push_back(
				newEntityAnimationDelay(railToRailTicksDuration - halfRailToRailTicksDuration));
		}
	}

	//move to the last tile
	float finalXPosition = (float)(nextSegment->x * MapState::tileSize) + halfTileSize;
	float finalYPosition = (float)(nextSegment->y * MapState::tileSize) + halfTileSize - boundingBoxBottomOffset;
	if (finalXPosition == targetXPosition + halfTileSize) {
		finalXPosition += -halfTileSize - boundingBoxLeftOffset + MapState::smallDistance;
		finalYPosition += 2.0f;
	} else if (finalXPosition == targetXPosition - halfTileSize) {
		finalXPosition += halfTileSize - boundingBoxRightOffset - MapState::smallDistance;
		finalYPosition += 2.0f;
	} else if (finalYPosition == targetYPosition + halfTileSize) {
		finalXPosition += 0.5f;
		finalYPosition += 2.0f;
	} else
		finalXPosition -= 0.5f;
	ridingRailAnimationComponents.push_back(
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(
				0.0f,
				(finalXPosition - targetXPosition) / bootLiftDuration,
				0.0f,
				0.0f,
				0.0f),
			newCompositeQuarticValue(
				0.0f,
				(finalYPosition - targetYPosition) / bootLiftDuration,
				0.0f,
				0.0f,
				0.0f)));
	ridingRailAnimationComponents.push_back(newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerBootLiftAnimation));
	ridingRailAnimationComponents.push_back(newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame));
	ridingRailAnimationComponents.push_back(
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), newCompositeQuarticValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)));

	beginEntityAnimation(newEntityAnimation(ticksTime, ridingRailAnimationComponents), ticksTime);
	return true;
}
//begin a kicking animation and flip a switch
void PlayerState::kickSwitch(int ticksTime) {
	kickAir(ticksTime);
	//TODO: toggle the state of the switch
}
//render this player state, which was deemed to be the last state to need rendering
void PlayerState::render(EntityState* camera, int ticksTime) {
	//convert these to ints first to align with the map in case the camera is not the player
	int playerScreenXOffset = (int)getRenderCenterWorldX(ticksTime) - (int)camera->getRenderCenterWorldX(ticksTime);
	int playerScreenYOffset = (int)getRenderCenterWorldY(ticksTime) - (int)camera->getRenderCenterWorldY(ticksTime);
	float renderCenterX = (float)playerScreenXOffset + (float)Config::gameScreenWidth * 0.5f;
	float renderCenterY = (float)playerScreenYOffset + (float)Config::gameScreenHeight * 0.5f;
	glEnable(GL_BLEND);
	if (spriteAnimation != nullptr)
		spriteAnimation->renderUsingCenter(
			renderCenterX, renderCenterY, ticksTime - spriteAnimationStartTicksTime, 0, (int)spriteDirection);
	else
		SpriteRegistry::player->renderSpriteCenteredAtScreenPosition(
			hasBoot ? 4 : 0, (int)spriteDirection, renderCenterX, renderCenterY);

	#ifdef SHOW_PLAYER_BOUNDING_BOX
		SpriteSheet::renderFilledRectangle(
			1.0f,
			1.0f,
			1.0f,
			0.75f,
			(GLint)(renderCenterX + boundingBoxLeftOffset),
			(GLint)(renderCenterY + boundingBoxTopOffset),
			(GLint)(renderCenterX + boundingBoxRightOffset),
			(GLint)(renderCenterY + boundingBoxBottomOffset));
	#endif
}
//save this player state to the file
void PlayerState::saveState(ofstream& file) {
	file << playerXFilePrefix << lastControlledX << "\n";
	file << playerYFilePrefix << lastControlledY << "\n";
	file << playerZFilePrefix << (int)lastControlledZ << "\n";
}
//try to load state from the line of the file, return whether state was loaded
bool PlayerState::loadState(string& line) {
	if (StringUtils::startsWith(line, playerXFilePrefix)) {
		lastControlledX = (float)atof(line.c_str() + playerXFilePrefix.size());
		x.set(newCompositeQuarticValue(lastControlledX, 0.0f, 0.0f, 0.0f, 0.0f));
	} else if (StringUtils::startsWith(line, playerYFilePrefix)) {
		lastControlledY = (float)atof(line.c_str() + playerYFilePrefix.size());
		y.set(newCompositeQuarticValue(lastControlledY, 0.0f, 0.0f, 0.0f, 0.0f));
	} else if (StringUtils::startsWith(line, playerZFilePrefix)) {
		lastControlledZ = (char)atoi(line.c_str() + playerZFilePrefix.size());
		z = lastControlledZ;
	} else
		return false;
	return true;
}
