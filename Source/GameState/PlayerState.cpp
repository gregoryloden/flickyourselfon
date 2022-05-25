#include "PlayerState.h"
#include "Editor/Editor.h"
#include "GameState/CollisionRect.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/GameState.h"
#include "GameState/KickAction.h"
#include "GameState/MapState/MapState.h"
#include "GameState/MapState/Rail.h"
#include "GameState/MapState/Switch.h"
#include "GameState/MapState/ResetSwitch.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Sprites/Text.h"
#include "Util/Config.h"
#include "Util/Logger.h"
#include "Util/StringUtils.h"

#ifdef DEBUG
	//**/#define SHOW_PLAYER_BOUNDING_BOX 1
#endif
#define newClimbFallKickAction(type, targetPlayerX, targetPlayerY, fallHeight) \
	newKickAction(type, targetPlayerX, targetPlayerY, fallHeight, MapState::absentRailSwitchId, -1)
#define newRailSwitchKickAction(type, railSwitchId, railSegmentIndex) \
	newKickAction(type, -1, -1, MapState::invalidHeight, railSwitchId, railSegmentIndex)

const float PlayerState::playerWidth = 11.0f;
const float PlayerState::playerHeight = 5.0f;
const float PlayerState::boundingBoxLeftOffset = PlayerState::playerWidth * -0.5f;
const float PlayerState::boundingBoxRightOffset = PlayerState::boundingBoxLeftOffset + PlayerState::playerWidth;
const float PlayerState::boundingBoxTopOffset = 4.5f;
const float PlayerState::boundingBoxBottomOffset = PlayerState::boundingBoxTopOffset + PlayerState::playerHeight;
const float PlayerState::boundingBoxCenterYOffset =
	(PlayerState::boundingBoxTopOffset + PlayerState::boundingBoxBottomOffset) * 0.5f;
const float PlayerState::introAnimationPlayerCenterX = 50.5f + (float)(MapState::firstLevelTileOffsetX * MapState::tileSize);
const float PlayerState::introAnimationPlayerCenterY = 106.5f + (float)(MapState::firstLevelTileOffsetY * MapState::tileSize);
const float PlayerState::speedPerSecond = 40.0f;
const float PlayerState::diagonalSpeedPerSecond = speedPerSecond * sqrt(0.5f);
const float PlayerState::kickingDistanceLimit = 1.5f;
const string PlayerState::playerXFilePrefix = "playerX ";
const string PlayerState::playerYFilePrefix = "playerY ";
PlayerState::PlayerState(objCounterParameters())
: EntityState(objCounterArguments())
, z(0)
, xDirection(0)
, yDirection(0)
, lastXMovedDelta(0)
, lastYMovedDelta(0)
, collisionRect(newCollisionRect(boundingBoxLeftOffset, boundingBoxTopOffset, boundingBoxRightOffset, boundingBoxBottomOffset))
, spriteAnimation(nullptr)
, spriteAnimationStartTicksTime(-1)
, spriteDirection(SpriteDirection::Down)
, hasBoot(false)
, ghostSpriteX(nullptr)
, ghostSpriteY(nullptr)
, ghostSpriteStartTicksTime(0)
, mapState(nullptr)
, availableKickAction(nullptr)
, autoKickStartTicksTime(-1)
, canImmediatelyAutoKick(false)
, lastControlledX(0.0f)
, lastControlledY(0.0f) {
	lastControlledX = x.get()->getValue(0);
	lastControlledY = y.get()->getValue(0);
}
PlayerState::~PlayerState() {
	delete collisionRect;
}
//initialize and return a PlayerState
PlayerState* PlayerState::produce(objCounterParametersComma() MapState* mapState) {
	initializeWithNewFromPool(p, PlayerState)
	p->z = 0;
	p->hasBoot = false;
	p->mapState.set(mapState);
	p->availableKickAction.set(nullptr);
	return p;
}
//copy the other state
void PlayerState::copyPlayerState(PlayerState* other) {
	copyEntityState(other);
	z = other->z;
	xDirection = other->xDirection;
	yDirection = other->yDirection;
	lastXMovedDelta = other->lastXMovedDelta;
	lastYMovedDelta = other->lastYMovedDelta;
	//don't copy collisionRect, it will get updated when needed
	spriteAnimation = other->spriteAnimation;
	spriteAnimationStartTicksTime = other->spriteAnimationStartTicksTime;
	spriteDirection = other->spriteDirection;
	hasBoot = other->hasBoot;
	ghostSpriteX.set(other->ghostSpriteX.get());
	ghostSpriteY.set(other->ghostSpriteY.get());
	ghostSpriteStartTicksTime = other->ghostSpriteStartTicksTime;
	//don't copy map state, it was provided when this player state was produced
	availableKickAction.set(other->availableKickAction.get());
	autoKickStartTicksTime = other->autoKickStartTicksTime;
	canImmediatelyAutoKick = other->canImmediatelyAutoKick;
	lastControlledX = other->lastControlledX;
	lastControlledY = other->lastControlledY;
}
pooledReferenceCounterDefineRelease(PlayerState)
//use the player as the next camera anchor, we will handle switching to a different camera somewhere else
void PlayerState::setNextCamera(GameState* nextGameState, int ticksTime) {
	nextGameState->setPlayerCamera();
}
//set the animation to the given animation at the given time
void PlayerState::setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime) {
	spriteAnimation = pSpriteAnimation;
	spriteAnimationStartTicksTime = pSpriteAnimationStartTicksTime;
}
//set the position of the ghost sprite, or clear it
void PlayerState::setGhostSprite(bool show, float x, float y, int ticksTime) {
	if (show) {
		ghostSpriteX.set(newCompositeQuarticValue(x, 0.0f, 0.0f, 0.0f, 0.0f));
		ghostSpriteY.set(newCompositeQuarticValue(y, 0.0f, 0.0f, 0.0f, 0.0f));
		ghostSpriteStartTicksTime = ticksTime;
	} else {
		ghostSpriteX.set(nullptr);
		ghostSpriteY.set(nullptr);
	}
}
//tell the map to kick a switch
void PlayerState::mapKickSwitch(short switchId, bool allowRadioTowerAnimation, int ticksTime) {
	mapState.get()->flipSwitch(switchId, allowRadioTowerAnimation, ticksTime);
}
//tell the map to kick a reset switch
void PlayerState::mapKickResetSwitch(short resetSwitchId, int ticksTime) {
	mapState.get()->flipResetSwitch(resetSwitchId, ticksTime);
}
//return whether we have a kick action where we can show connections
bool PlayerState::showTutorialConnectionsForKickAction() {
	KickAction* kickAction = availableKickAction.get();
	if (kickAction == nullptr)
		return false;
	switch (kickAction->getType()) {
		//show connections for rails, reset switches and puzzle switches
		case KickActionType::Rail:
		case KickActionType::ResetSwitch:
		case KickActionType::Switch:
			return true;
		default:
			return false;
	}
}
//if we have a reset switch kick action, return its id
short PlayerState::getKickActionResetSwitchId() {
	return availableKickAction.get() != nullptr && availableKickAction.get()->getType() == KickActionType::ResetSwitch
		? availableKickAction.get()->getRailSwitchId()
		: MapState::absentRailSwitchId;
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
	//if the previous play state had an animation, we copied it already so clear it
	//if not, we might have a leftover entity animation from a previous state
	entityAnimation.set(nullptr);

	//if we can control the player then that must mean the player has the boot
	hasBoot = true;

	//update this player state normally by reading from the last state
	updatePositionWithPreviousPlayerState(prev, ticksTime);
	if (!Editor::isActive)
		collideWithEnvironmentWithPreviousPlayerState(prev);
	updateSpriteWithPreviousPlayerState(prev, ticksTime, !previousStateHadEntityAnimation);
	if (!Editor::isActive) {
		setKickAction();
		tryAutoKick(prev, ticksTime);
	}

	//copy the position to the save values
	lastControlledX = x.get()->getValue(0);
	lastControlledY = y.get()->getValue(0);
}
//update the position of this player state by reading from the previous state
void PlayerState::updatePositionWithPreviousPlayerState(PlayerState* prev, int ticksTime) {
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	xDirection = (char)(keyboardState[Config::keyBindings.rightKey] - keyboardState[Config::keyBindings.leftKey]);
	yDirection = (char)(keyboardState[Config::keyBindings.downKey] - keyboardState[Config::keyBindings.upKey]);
	float speedPerTick =
		((xDirection & yDirection) != 0 ? diagonalSpeedPerSecond : speedPerSecond) / (float)Config::ticksPerSecond;
	if (Editor::isActive && keyboardState[Config::keyBindings.kickKey] != 0)
		speedPerTick *= 8.0f;

	int ticksSinceLastUpdate = ticksTime - prev->lastUpdateTicksTime;
	DynamicValue* prevX = prev->x.get();
	DynamicValue* prevY = prev->y.get();
	float newX = prevX->getValue(ticksSinceLastUpdate);
	float newY = prevY->getValue(ticksSinceLastUpdate);
	setXAndUpdateCollisionRect(newCompositeQuarticValue(newX, (float)xDirection * speedPerTick, 0.0f, 0.0f, 0.0f));
	setYAndUpdateCollisionRect(newCompositeQuarticValue(newY, (float)yDirection * speedPerTick, 0.0f, 0.0f, 0.0f));
	lastXMovedDelta = newX - prevX->getValue(0);
	lastYMovedDelta = newY - prevY->getValue(0);
	lastUpdateTicksTime = ticksTime;
	renderInterpolatedX = true;
	renderInterpolatedY = true;
	z = prev->z;
}
//set the new value and update the left and right
void PlayerState::setXAndUpdateCollisionRect(DynamicValue* newX) {
	x.set(newX);
	float xPosition = x.get()->getValue(0);
	collisionRect->left = xPosition + boundingBoxLeftOffset;
	collisionRect->right = xPosition + boundingBoxRightOffset;
}
//set the new value and update the top and bottom
void PlayerState::setYAndUpdateCollisionRect(DynamicValue* newY) {
	y.set(newY);
	float yPosition = y.get()->getValue(0);
	collisionRect->top = yPosition + boundingBoxTopOffset;
	collisionRect->bottom = yPosition + boundingBoxBottomOffset;
}
//check if we moved onto map tiles of a different height, using the previous move direction to figure out where to look
//if we did, move the player and don't render interpolated positions
void PlayerState::collideWithEnvironmentWithPreviousPlayerState(PlayerState* prev) {
	//find the row and column of tiles that we could collide with
	int lowMapX = (int)collisionRect->left / MapState::tileSize;
	int highMapX = (int)collisionRect->right / MapState::tileSize;
	int lowMapY = (int)collisionRect->top / MapState::tileSize;
	int highMapY = (int)collisionRect->bottom / MapState::tileSize;
	int collisionMapX = prev->xDirection < 0 ? lowMapX : highMapX;
	int collisionMapY = prev->yDirection < 0 ? lowMapY : highMapY;

	vector<ReferenceCounterHolder<CollisionRect>> collidedRects;
	vector<short> seenSwitchIds;
	//check for horizontal collisions, including the corner if we're moving diagonally
	if (prev->xDirection != 0) {
		for (int currentMapY = lowMapY; currentMapY <= highMapY; currentMapY++)
			addMapCollisions(collisionMapX, currentMapY, collidedRects, seenSwitchIds);
	}
	//check for vertical collisions, excluding the corner if we're moving diagonally
	if (prev->yDirection != 0) {
		int iterLowMapX = prev->xDirection < 0 ? lowMapX + 1 : lowMapX;
		int iterHighMapX = prev->xDirection > 0 ? highMapX - 1 : highMapX;
		for (int currentMapX = iterLowMapX; currentMapX <= iterHighMapX; currentMapX++)
			addMapCollisions(currentMapX, collisionMapY, collidedRects, seenSwitchIds);
	}

	//now go through all the rects and collide with them in chronological order from when the player collided with them
	while (!collidedRects.empty()) {
		int mostCollidedRectIndex = -1;
		float mostCollidedRectDuration = 0;
		for (int i = 0; i < (int)collidedRects.size(); i++) {
			CollisionRect* collidedRect = collidedRects[i].get();
			if (!collisionRect->intersects(collidedRect))
				continue;

			float collisionDuration = netCollisionDuration(collidedRect);
			if (mostCollidedRectIndex == -1 || collisionDuration > mostCollidedRectDuration) {
				mostCollidedRectIndex = i;
				mostCollidedRectDuration = collisionDuration;
			}
		}

		//no collision
		if (mostCollidedRectIndex == -1)
			break;

		//we got a collision, find out where we collided and move away from there
		CollisionRect* collidedRect = collidedRects[mostCollidedRectIndex].get();
		//we hit this rect from the side
		if (xCollisionDuration(collidedRect) < yCollisionDuration(collidedRect)) {
			renderInterpolatedX = false;
			setXAndUpdateCollisionRect(
				x.get()->copyWithConstantValue(prev->xDirection < 0
					? collidedRect->right + MapState::smallDistance - boundingBoxLeftOffset
					: collidedRect->left - MapState::smallDistance - boundingBoxRightOffset));
		//we hit this rect from the top or bottom
		} else {
			renderInterpolatedY = false;
			setYAndUpdateCollisionRect(
				y.get()->copyWithConstantValue(prev->yDirection < 0
					? collidedRect->bottom + MapState::smallDistance - boundingBoxTopOffset
					: collidedRect->top - MapState::smallDistance - boundingBoxBottomOffset));
		}
		collidedRects.erase(collidedRects.begin() + mostCollidedRectIndex);
	}
}
//check for collisions at the map tile, either because of a different height or because there's a new switch there
void PlayerState::addMapCollisions(
	int mapX, int mapY, vector<ReferenceCounterHolder<CollisionRect>>& collidedRects, vector<short> seenSwitchIds)
{
	if (MapState::getHeight(mapX, mapY) != z)
		collidedRects.push_back(
			newCollisionRect(
				(float)(mapX * MapState::tileSize),
				(float)(mapY * MapState::tileSize),
				(float)((mapX + 1) * MapState::tileSize),
				(float)((mapY + 1) * MapState::tileSize)));
	else if (MapState::tileHasSwitch(mapX, mapY)) {
		//make sure we haven't already seen this switch yet
		short switchId = MapState::getRailSwitchId(mapX, mapY);
		for (short seenSwitchId : seenSwitchIds) {
			if (seenSwitchId == switchId)
				return;
		}
		//get the top-left corner of the switch
		if (MapState::getRailSwitchId(mapX - 1, mapY) == switchId)
			mapX--;
		if (MapState::getRailSwitchId(mapX, mapY - 1) == switchId)
			mapY--;
		//now add the switch rect and id
		collidedRects.push_back(
			newCollisionRect(
				(float)(mapX * MapState::tileSize + MapState::switchSideInset),
				(float)(mapY * MapState::tileSize + MapState::switchTopInset),
				(float)((mapX + 2) * MapState::tileSize - MapState::switchSideInset),
				(float)((mapY + 2) * MapState::tileSize - MapState::switchBottomInset)));
		seenSwitchIds.push_back(switchId);
	} else if (MapState::tileHasResetSwitchBody(mapX, mapY))
		collidedRects.push_back(
			newCollisionRect(
				(float)(mapX * MapState::tileSize),
				(float)(mapY * MapState::tileSize),
				(float)((mapX + 1) * MapState::tileSize),
				(float)((mapY + 1) * MapState::tileSize)));
}
//returns the fraction of the update spent within the bounds of the given rect; since a collision only happens once the player
//	is within the bounds in both directions, this is the min of the two collision durations
float PlayerState::netCollisionDuration(CollisionRect* other) {
	return MathUtils::fmin(xCollisionDuration(other), yCollisionDuration(other));
}
//returns the fraction of the update spent within the x bounds of the given rect, based on the velocity this frame
float PlayerState::xCollisionDuration(CollisionRect* other) {
	return lastXMovedDelta < 0
		? (collisionRect->left - other->right) / lastXMovedDelta
		: (collisionRect->right - other->left) / lastXMovedDelta;
}
//returns the fraction of the update spent within the y bounds of the given rect, based on the velocity this frame
float PlayerState::yCollisionDuration(CollisionRect* other) {
	return lastYMovedDelta < 0
		? (collisionRect->top - other->bottom) / lastYMovedDelta
		: (collisionRect->bottom - other->top) / lastYMovedDelta;
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
//set the kick action if one is available
void PlayerState::setKickAction() {
	float xPosition = x.get()->getValue(0);
	float yPosition = y.get()->getValue(0);
	availableKickAction.set(nullptr);
	setRailKickAction(xPosition, yPosition)
		|| setSwitchKickAction(xPosition, yPosition)
		|| setResetSwitchKickAction(xPosition, yPosition)
		|| setFallKickAction(xPosition, yPosition)
		|| setClimbKickAction(xPosition, yPosition);
}
//set a rail kick action if there is a raised rail we can ride across
//returns whether it was set
bool PlayerState::setRailKickAction(float xPosition, float yPosition) {
	float railCheckXPosition;
	float railCheckYPosition;
	if (spriteDirection == SpriteDirection::Up || spriteDirection == SpriteDirection::Down) {
		railCheckXPosition = xPosition;
		railCheckYPosition =
			yPosition + (spriteDirection == SpriteDirection::Up ? boundingBoxTopOffset : boundingBoxBottomOffset);
	} else {
		railCheckXPosition =
			xPosition + (spriteDirection == SpriteDirection::Left ? boundingBoxLeftOffset : boundingBoxRightOffset);
		railCheckYPosition = yPosition + boundingBoxCenterYOffset;
	}
	int railMapX = (int)railCheckXPosition / MapState::tileSize;
	int railMapY = (int)railCheckYPosition / MapState::tileSize;
	//the player isn't in front of a rail, but if the player is within a half tile of a rail, don't let them fall
	if (!MapState::tileHasRail(railMapX, railMapY)) {
		const float halfTileSize = (float)MapState::tileSize * 0.5f;
		if (spriteDirection == SpriteDirection::Up || spriteDirection == SpriteDirection::Down)
			return MapState::tileHasRail((int)(railCheckXPosition - halfTileSize) / MapState::tileSize, railMapY)
				|| MapState::tileHasRail((int)(railCheckXPosition + halfTileSize) / MapState::tileSize, railMapY);
		else
			return MapState::tileHasRail(railMapX, (int)(railCheckYPosition - halfTileSize) / MapState::tileSize)
				|| MapState::tileHasRail(railMapX, (int)(railCheckYPosition + halfTileSize) / MapState::tileSize);
	}

	RailState* railState = mapState.get()->getRailState(railMapX, railMapY);
	Rail* rail = railState->getRail();
	//if it's the wrong height, just ignore it completely
	if (rail->getBaseHeight() != z)
		return false;
	//if it's lowered, we can't ride it but don't allow falling
	if (!railState->canRide()) {
		availableKickAction.set(newRailSwitchKickAction(KickActionType::NoRail, MapState::absentRailSwitchId, -1));
		return true;
	}

	//ensure it's the start or end of the rail
	int railSegmentIndex = rail->getSegmentCount() - 1;
	Rail::Segment* startSegment = rail->getSegment(0);
	Rail::Segment* endSegment = rail->getSegment(railSegmentIndex);
	if (startSegment->x == railMapX && startSegment->y == railMapY)
		railSegmentIndex = 0;
	else if (endSegment->x == railMapX && endSegment->y == railMapY)
		;
	else
		return false;

	availableKickAction.set(
		newRailSwitchKickAction(KickActionType::Rail, MapState::getRailSwitchId(railMapX, railMapY), railSegmentIndex));
	return true;
}
//set a switch kick action if there is a switch in front of the player
//returns whether it was set
bool PlayerState::setSwitchKickAction(float xPosition, float yPosition) {
	//no switch kicking when facing down
	if (spriteDirection == SpriteDirection::Down)
		return false;

	short switchId = MapState::absentRailSwitchId;
	if (spriteDirection == SpriteDirection::Up) {
		int switchLeftMapX = (int)(xPosition + boundingBoxLeftOffset + 2.0f) / MapState::tileSize;
		int switchCenterMapX = (int)xPosition / MapState::tileSize;
		int switchRightMapX = (int)(xPosition + boundingBoxRightOffset - 2.0f) / MapState::tileSize;
		int switchTopMapY = (int)(yPosition + boundingBoxTopOffset + 2.0f - kickingDistanceLimit) / MapState::tileSize;
		if (MapState::tileHasSwitch(switchLeftMapX, switchTopMapY))
			switchId = MapState::getRailSwitchId(switchLeftMapX, switchTopMapY);
		else if (MapState::tileHasSwitch(switchCenterMapX, switchTopMapY))
			switchId = MapState::getRailSwitchId(switchCenterMapX, switchTopMapY);
		else if (MapState::tileHasSwitch(switchRightMapX, switchTopMapY))
			switchId = MapState::getRailSwitchId(switchRightMapX, switchTopMapY);
		//for the switches on the radio tower, accept a slightly further kick so that players who are too far can still trigger
		//	the switch
		else {
			switchTopMapY = (int)(yPosition + boundingBoxTopOffset - 0.5f) / MapState::tileSize;
			if (MapState::tileHasSwitch(switchCenterMapX, switchTopMapY)) {
				SwitchState* switchState = mapState.get()->getSwitchState(switchCenterMapX, switchTopMapY);
				if (switchState->getSwitch()->getGroup() == 0)
					switchId = MapState::getRailSwitchId(switchCenterMapX, switchTopMapY);
			}
		}
	} else {
		float kickingXOffset = spriteDirection == SpriteDirection::Left
			? boundingBoxLeftOffset + 2.0f - kickingDistanceLimit
			: boundingBoxRightOffset - 2.0f + kickingDistanceLimit;
		int switchMapX = (int)(xPosition + kickingXOffset) / MapState::tileSize;
		int switchTopMapY = (int)(yPosition + boundingBoxTopOffset + 2.0f) / MapState::tileSize;
		int switchBottomMapY = (int)(yPosition + boundingBoxBottomOffset - 1.0f) / MapState::tileSize;
		if (MapState::tileHasSwitch(switchMapX, switchTopMapY))
			switchId = MapState::getRailSwitchId(switchMapX, switchTopMapY);
		else if (MapState::tileHasSwitch(switchMapX, switchBottomMapY))
			switchId = MapState::getRailSwitchId(switchMapX, switchBottomMapY);
	}
	if (switchId == MapState::absentRailSwitchId)
		return false;
	KickActionType switchKickActionType = mapState.get()->getSwitchKickActionType(switchId);
	if (switchKickActionType == KickActionType::None)
		return false;

	availableKickAction.set(newRailSwitchKickAction(switchKickActionType, switchId, -1));
	return true;
}
//set a reset switch kick action if we can reset switch
//returns whether it was set
bool PlayerState::setResetSwitchKickAction(float xPosition, float yPosition) {
	int resetSwitchMapX = -1;
	int resetSwitchMapY = -1;
	if (spriteDirection == SpriteDirection::Up || spriteDirection == SpriteDirection::Down) {
		float kickingYOffset = spriteDirection == SpriteDirection::Up
			? boundingBoxTopOffset - kickingDistanceLimit
			: boundingBoxBottomOffset + kickingDistanceLimit;
		int resetSwitchLeftMapX = (int)(xPosition + boundingBoxLeftOffset) / MapState::tileSize;
		int resetSwitchCenterMapX = (int)xPosition / MapState::tileSize;
		int resetSwitchRightMapX = (int)(xPosition + boundingBoxRightOffset) / MapState::tileSize;
		resetSwitchMapY = (int)(yPosition + kickingYOffset) / MapState::tileSize;
		if (MapState::tileHasResetSwitchBody(resetSwitchLeftMapX, resetSwitchMapY))
			resetSwitchMapX = resetSwitchLeftMapX;
		else if (MapState::tileHasResetSwitchBody(resetSwitchCenterMapX, resetSwitchMapY))
			resetSwitchMapX = resetSwitchCenterMapX;
		else if (MapState::tileHasResetSwitchBody(resetSwitchRightMapX, resetSwitchMapY))
			resetSwitchMapX = resetSwitchRightMapX;
	} else {
		float kickingXOffset = spriteDirection == SpriteDirection::Left
			? boundingBoxLeftOffset - kickingDistanceLimit
			: boundingBoxRightOffset + kickingDistanceLimit;
		resetSwitchMapX = (int)(xPosition + kickingXOffset) / MapState::tileSize;
		int resetSwitchTopMapY = (int)(yPosition + boundingBoxTopOffset) / MapState::tileSize;
		int resetSwitchBottomMapY = (int)(yPosition + boundingBoxBottomOffset) / MapState::tileSize;
		if (MapState::tileHasResetSwitchBody(resetSwitchMapX, resetSwitchTopMapY))
			resetSwitchMapY = resetSwitchTopMapY;
		else if (MapState::tileHasResetSwitchBody(resetSwitchMapX, resetSwitchBottomMapY))
			resetSwitchMapY = resetSwitchBottomMapY;
	}
	//because reset switches can be kicked from outside their tile, we also need to make sure the player is on the same height
	if (resetSwitchMapX == -1 || resetSwitchMapY == -1 || MapState::getHeight(resetSwitchMapX, resetSwitchMapY) != z)
		return false;

	short resetSwitchId = MapState::getRailSwitchId(resetSwitchMapX, resetSwitchMapY);
	availableKickAction.set(newRailSwitchKickAction(KickActionType::ResetSwitch, resetSwitchId, -1));
	return true;
}
//set a climb kick action if we can climb
//returns whether it was set
bool PlayerState::setClimbKickAction(float xPosition, float yPosition) {
	//we can only climb when facing up
	if (spriteDirection != SpriteDirection::Up)
		return false;

	int lowMapX = (int)(xPosition + boundingBoxLeftOffset) / MapState::tileSize;
	int highMapX = (int)(xPosition + boundingBoxRightOffset) / MapState::tileSize;
	int oneTileUpMapY = (int)(yPosition + boundingBoxTopOffset - kickingDistanceLimit) / MapState::tileSize;
	char oneTileUpHeight = MapState::horizontalTilesHeight(lowMapX, highMapX, oneTileUpMapY);
	//make sure tiles are all at the same height
	if (oneTileUpHeight == MapState::invalidHeight)
		return false;

	//make sure the next tiles are 1 height up and the row after that is 2 height up, the next available floor
	if (oneTileUpHeight != z + 1 || MapState::horizontalTilesHeight(lowMapX, highMapX, oneTileUpMapY - 1) != z + 2)
		return false;

	//set a distance such that the bottom of the player is slightly past the edge of the ledge
	float targetYPosition =
		(float)(oneTileUpMapY * MapState::tileSize) - boundingBoxBottomOffset - MapState::smallDistance;
	availableKickAction.set(newClimbFallKickAction(KickActionType::Climb, 0, targetYPosition, 0));
	return true;
}
//set a fall kick action if we can fall
//returns whether it was set
bool PlayerState::setFallKickAction(float xPosition, float yPosition) {
	int lowMapX = (int)(xPosition + boundingBoxLeftOffset) / MapState::tileSize;
	int highMapX = (int)(xPosition + boundingBoxRightOffset) / MapState::tileSize;
	char fallHeight;
	float targetXPosition;
	float targetYPosition;
	if (spriteDirection == SpriteDirection::Up) {
		int oneTileUpMapY = (int)(yPosition + boundingBoxTopOffset - kickingDistanceLimit) / MapState::tileSize;
		char oneTileUpHeight = MapState::horizontalTilesHeight(lowMapX, highMapX, oneTileUpMapY);
		//can't fall unless we found tiles that are all the same lower floor height
		if (oneTileUpHeight == MapState::invalidHeight || oneTileUpHeight >= z || (oneTileUpHeight & 1) != 0)
			return false;

		//move a distance such that the bottom of the player is slightly above the top of the cliff
		targetXPosition = xPosition;
		targetYPosition = (float)((oneTileUpMapY + 1) * MapState::tileSize) - boundingBoxBottomOffset - MapState::smallDistance;
		fallHeight = oneTileUpHeight;
	} else if (spriteDirection == SpriteDirection::Down) {
		int oneTileDownMapY = (int)(yPosition + boundingBoxBottomOffset + kickingDistanceLimit) / MapState::tileSize;
		fallHeight = MapState::invalidHeight;
		int tileOffset = 0;
		for (; true; tileOffset++) {
			fallHeight = MapState::horizontalTilesHeight(lowMapX, highMapX, oneTileDownMapY + tileOffset);
			//stop looking if the heights differ
			if (fallHeight == MapState::invalidHeight)
				return false;
			//cliff face, keep looking
			else if (fallHeight == z - 1 - tileOffset * 2)
				;
			//this is a tile we can fall to
			else if (fallHeight == z - tileOffset * 2 && tileOffset >= 1)
				break;
			//the row is higher than us (or the empty tile) so stop looking
			else
				return false;
		}

		//move a distance such that the bottom of the player is slightly below the bottom of the cliff
		targetXPosition = xPosition;
		targetYPosition =
			(float)((oneTileDownMapY + tileOffset) * MapState::tileSize) - boundingBoxTopOffset + MapState::smallDistance;
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

		int topMapY = (int)(yPosition + boundingBoxTopOffset) / MapState::tileSize;
		for (int tileOffset = 1; true; tileOffset++) {
			int checkMapY = topMapY + tileOffset;
			if (checkMapY >= MapState::mapHeight())
				return false;

			char tilesHeight = MapState::horizontalTilesHeight(sideTilesLeftMapX, sideTilesRightMapX, checkMapY);
			//keep looking if we see an empty space or two different heights
			if (tilesHeight == MapState::emptySpaceHeight || tilesHeight == MapState::invalidHeight)
				continue;

			fallHeight = z - (char)tileOffset * 2;
			//we found tiles that would be in front of the player after a fall, stop looking
			if (tilesHeight > fallHeight)
				return false;
			//we found tiles that would be behind the player after a fall, keep looking
			else if (tilesHeight < fallHeight)
				continue;

			//ensure that all tiles the player will land on are the same height
			int bottomCheckMapY = (int)(yPosition + boundingBoxTopOffset) / MapState::tileSize + tileOffset;
			for (checkMapY++; checkMapY <= bottomCheckMapY; checkMapY++) {
				if (MapState::horizontalTilesHeight(sideTilesLeftMapX, sideTilesRightMapX, checkMapY) != tilesHeight)
					return false;
			}

			//we found a spot we can fall to
			targetXPosition = spriteDirection == SpriteDirection::Left
				? (float)((sideTilesRightMapX + 1) * MapState::tileSize) - boundingBoxRightOffset - MapState::smallDistance
				: (float)(sideTilesLeftMapX * MapState::tileSize) - boundingBoxLeftOffset + MapState::smallDistance;
			targetYPosition = yPosition + (float)(tileOffset * MapState::tileSize);
			break;
		}
	}

	availableKickAction.set(newClimbFallKickAction(KickActionType::Fall, targetXPosition, targetYPosition, fallHeight));
	return true;
}
//auto-climb, auto-fall, or auto-ride-rail if we can
void PlayerState::tryAutoKick(PlayerState* prev, int ticksTime) {
	autoKickStartTicksTime = -1;
	canImmediatelyAutoKick = (xDirection != 0 || yDirection != 0) && prev->canImmediatelyAutoKick;
	if (availableKickAction.get() == nullptr)
		return;

	//ensure we have an auto-compatible kick action and check any secondary requirements
	switch (availableKickAction.get()->getType()) {
		case KickActionType::Climb:
			//no auto-climb if the player isn't moving directly up
			if (xDirection != 0 || yDirection != -1)
				return;
			break;
		case KickActionType::Fall:
			//no auto-fall if the player isn't moving directly down towards a floor they can climb back up from
			if (xDirection != 0 || yDirection != 1 || availableKickAction.get()->getFallHeight() != z - 2)
				return;
			break;
		case KickActionType::Rail: {
			//no auto-ride-rail if the player isn't moving straight
			if ((xDirection != 0) == (yDirection != 0))
				return;
			Rail* rail = mapState.get()->getRailFromId(availableKickAction.get()->getRailSwitchId());
			Rail::Segment* startEndSegment = rail->getSegment(availableKickAction.get()->getRailSegmentIndex());
			//ensure the rail matches our movement direction
			if (Rail::endSegmentSpriteHorizontalIndex(xDirection, yDirection) != startEndSegment->spriteHorizontalIndex)
				return;
			break;
		}
		default:
			return;
	}

	autoKickStartTicksTime = ticksTime + autoKickTriggerDelay;
	//we had a kick action before and it's the same climb/fall as we have now, copy its start time
	if (prev->availableKickAction.get() != nullptr
			&& prev->availableKickAction.get()->getType() == availableKickAction.get()->getType()
			&& prev->autoKickStartTicksTime != -1)
		autoKickStartTicksTime = prev->autoKickStartTicksTime;

	//if we've reached the threshold, start kicking now
	if (ticksTime >= autoKickStartTicksTime || canImmediatelyAutoKick) {
		beginKicking(ticksTime);
		autoKickStartTicksTime = -1;
		canImmediatelyAutoKick = true;
	}
}
//if we don't have a kicking animation, start one
//this should be called after the player has been updated
void PlayerState::beginKicking(int ticksTime) {
	if (entityAnimation.get() != nullptr)
		return;

	renderInterpolatedX = true;
	renderInterpolatedY = true;

	//kicking doesn't do anything if you don't have the boot or if there is no available kick action
	KickAction* kickAction = availableKickAction.get();
	if (!hasBoot || kickAction == nullptr) {
		kickAir(ticksTime);
		return;
	}

	float xPosition = x.get()->getValue(0);
	float yPosition = y.get()->getValue(0);
	switch (kickAction->getType()) {
		case KickActionType::Climb:
			kickClimb(kickAction->getTargetPlayerY() - yPosition, ticksTime);
			break;
		case KickActionType::Fall:
			kickFall(
				kickAction->getTargetPlayerX() - xPosition,
				kickAction->getTargetPlayerY() - yPosition,
				kickAction->getFallHeight(),
				ticksTime);
			break;
		case KickActionType::Rail:
			kickRail(kickAction->getRailSwitchId(), xPosition, yPosition, ticksTime);
			break;
		case KickActionType::Switch:
		case KickActionType::Square:
		case KickActionType::Triangle:
		case KickActionType::Saw:
		case KickActionType::Sine:
			kickSwitch(kickAction->getRailSwitchId(), ticksTime);
			break;
		case KickActionType::ResetSwitch:
			kickResetSwitch(kickAction->getRailSwitchId(), ticksTime);
			break;
		default:
			kickAir(ticksTime);
			break;
	}
	availableKickAction.set(nullptr);
}
//begin a kicking animation without changing any state
void PlayerState::kickAir(int ticksTime) {
	SpriteAnimation* kickingSpriteAnimation = hasBoot
		? SpriteRegistry::playerKickingAnimation
		: SpriteRegistry::playerLegLiftAnimation;
	vector<ReferenceCounterHolder<EntityAnimation::Component>> kickAnimationComponents ({
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
		newEntityAnimationSetSpriteAnimation(kickingSpriteAnimation),
		newEntityAnimationDelay(kickingSpriteAnimation->getTotalTicksDuration())
	});
	Holder_EntityAnimationComponentVector kickAnimationComponentsHolder (&kickAnimationComponents);
	beginEntityAnimation(&kickAnimationComponentsHolder, ticksTime);
}
//begin a kicking animation and climb up to the next tile to the north
void PlayerState::kickClimb(float yMoveDistance, int ticksTime) {
	stringstream message;
	message << "  climb " << (int)(x.get()->getValue(0)) << " " << (int)(y.get()->getValue(0));
	Logger::gameplayLogger.logString(message.str());

	int moveDuration =
		SpriteRegistry::playerFastKickingAnimation->getTotalTicksDuration()
			- SpriteRegistry::playerFastKickingAnimationTicksPerFrame;
	float floatMoveDuration = (float)moveDuration;
	float moveDurationCubed = floatMoveDuration * floatMoveDuration * floatMoveDuration;

	float yCubicValuePerDuration = yMoveDistance / 2.0f;
	float yQuarticValuePerDuration = yMoveDistance / 2.0f;

	vector<ReferenceCounterHolder<EntityAnimation::Component>> kickingAnimationComponents ({
		//start by stopping the player and delaying until the leg-sticking-out frame
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
		newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerFastKickingAnimation),
		newEntityAnimationDelay(SpriteRegistry::playerFastKickingAnimationTicksPerFrame),
		//then set the climb velocity, delay for the rest of the animation, and then stop the player
		newEntityAnimationSetVelocity(
			newConstantValue(0.0f),
			newCompositeQuarticValue(
				0.0f,
				0.0f,
				0.0f,
				yCubicValuePerDuration / moveDurationCubed,
				yQuarticValuePerDuration / (moveDurationCubed * floatMoveDuration))),
		newEntityAnimationDelay(moveDuration),
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f))
	});

	Holder_EntityAnimationComponentVector kickingAnimationComponentsHolder (&kickingAnimationComponents);
	beginEntityAnimation(&kickingAnimationComponentsHolder, ticksTime);
	z += 2;
}
//begin a kicking animation and fall in whatever direction we're facing
void PlayerState::kickFall(float xMoveDistance, float yMoveDistance, char fallHeight, int ticksTime) {
	stringstream message;
	message << "  fall " << (int)(x.get()->getValue(0)) << " " << (int)(y.get()->getValue(0));
	Logger::gameplayLogger.logString(message.str());

	bool useFastKickingAnimation = fallHeight == z - 2 && spriteDirection == SpriteDirection::Down;
	SpriteAnimation* fallAnimation =
		useFastKickingAnimation ? SpriteRegistry::playerFastKickingAnimation : SpriteRegistry::playerKickingAnimation;
	int fallAnimationTicksPerFrame = useFastKickingAnimation
		? SpriteRegistry::playerFastKickingAnimationTicksPerFrame
		: SpriteRegistry::playerKickingAnimationTicksPerFrame;

	//start by stopping the player and delaying until the leg-sticking-out frame
	vector<ReferenceCounterHolder<EntityAnimation::Component>> kickingAnimationComponents ({
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
		newEntityAnimationSetSpriteAnimation(fallAnimation),
		newEntityAnimationDelay(fallAnimationTicksPerFrame)
	});

	int moveDuration = fallAnimation->getTotalTicksDuration() - fallAnimationTicksPerFrame;
	float floatMoveDuration = (float)moveDuration;
	float moveDurationSquared = floatMoveDuration * floatMoveDuration;

	//up has a jump trajectory that visually goes up far, then down
	if (spriteDirection == SpriteDirection::Up) {
		//we want a cubic curve that goes through (0,0) and (1,1) and a chosen midpoint (i,j) where 0 < i < 1
		//it also goes through (c,#) such that dy/dt has roots at c (trough) and i (crest) (and arbitrary multiplier d)
		//vy = d(t-c)(t-i) = d(t^2-(c+i)t+ci)   (d < 0)
		//y = d(t^3/3-(c+i)t^2/2+cit) = dt^3/3-d(c+i)t^2/2+dcit
		//plug in 1,1 and i,j, solve for d
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
				newConstantValue(0.0f),
				newCompositeQuarticValue(
					0.0f,
					yLinearValuePerDuration / floatMoveDuration,
					yQuadraticValuePerDuration / moveDurationSquared,
					yCubicValuePerDuration / (moveDurationSquared * floatMoveDuration),
					0.0f)));
	//left, right, and down all have the same quadratic jump trajectory (for y, and x is linear or 0)
	} else {
		kickingAnimationComponents.push_back(
			newEntityAnimationSetVelocity(
				newCompositeQuarticValue(0.0f, xMoveDistance / floatMoveDuration, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(
					0.0f,
					-yMoveDistance / floatMoveDuration,
					2.0f * yMoveDistance / moveDurationSquared,
					0.0f,
					0.0f)));
	}

	//delay for the rest of the animation and then stop the player
	kickingAnimationComponents.insert(
		kickingAnimationComponents.end(),
		{
			newEntityAnimationDelay(moveDuration),
			newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f))
		});

	Holder_EntityAnimationComponentVector kickingAnimationComponentsHolder (&kickingAnimationComponents);
	beginEntityAnimation(&kickingAnimationComponentsHolder, ticksTime);
	z = fallHeight;
}
//begin a rail-riding animation and follow it to its other end
void PlayerState::kickRail(short railId, float xPosition, float yPosition, int ticksTime) {
	MapState::logRailRide(railId, (int)xPosition, (int)yPosition);
	vector<ReferenceCounterHolder<EntityAnimation::Component>> ridingRailAnimationComponents;
	Holder_EntityAnimationComponentVector ridingRailAnimationComponentsHolder (&ridingRailAnimationComponents);
	addRailRideComponents(railId, &ridingRailAnimationComponentsHolder, xPosition, yPosition, nullptr, nullptr);
	beginEntityAnimation(&ridingRailAnimationComponentsHolder, ticksTime);
}
//add the components for a rail-riding animation
void PlayerState::addRailRideComponents(
	short railId,
	Holder_EntityAnimationComponentVector* componentsHolder,
	float xPosition,
	float yPosition,
	float* outFinalXPosition,
	float* outFinalYPosition)
{
	vector<ReferenceCounterHolder<EntityAnimation::Component>>* components = componentsHolder->val;
	Rail* rail = MapState::getRailFromId(railId);
	Rail::Segment* startSegment = rail->getSegment(0);
	Rail::Segment* endSegment = rail->getSegment(rail->getSegmentCount() - 1);

	//define the distance to the segment as the manhattan distance of the center of the player bounding box to the center of
	//	the tile that the segment is on
	float feetYPosition = yPosition + boundingBoxCenterYOffset;
	float startDist = abs(xPosition - startSegment->tileCenterX()) + abs(feetYPosition - startSegment->tileCenterY());
	float endDist = abs(xPosition - endSegment->tileCenterX()) + abs(feetYPosition - endSegment->tileCenterY());

	int endSegmentIndex = rail->getSegmentCount() - 1;
	int firstSegmentIndex = 0;
	int lastSegmentIndex = 0;
	int segmentIndexIncrement;
	if (startDist <= endDist) {
		lastSegmentIndex = endSegmentIndex;
		segmentIndexIncrement = 1;
	} else {
		firstSegmentIndex = endSegmentIndex;
		segmentIndexIncrement = -1;
	}

	const float bootLiftDuration = (float)SpriteRegistry::playerKickingAnimationTicksPerFrame;
	const float floatRailToRailTicksDuration = (float)railToRailTicksDuration;
	const float railToRailTicksDurationSquared = floatRailToRailTicksDuration * floatRailToRailTicksDuration;

	components->push_back(newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerBootLiftAnimation));

	const float halfTileSize = (float)MapState::tileSize * 0.5f;
	Rail::Segment* nextSegment = rail->getSegment(firstSegmentIndex);
	int nextSegmentIndex = firstSegmentIndex;
	bool firstMove = true;
	float targetXPosition = xPosition;
	float targetYPosition = yPosition;
	while (nextSegmentIndex != lastSegmentIndex) {
		float lastXPosition = targetXPosition;
		float lastYPosition = targetYPosition;
		nextSegmentIndex += segmentIndexIncrement;
		Rail::Segment* currentSegment = nextSegment;
		nextSegment = rail->getSegment(nextSegmentIndex);

		targetXPosition = currentSegment->tileCenterX();
		targetYPosition = currentSegment->tileCenterY() - boundingBoxBottomOffset;
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
			components->insert(
				components->end(),
				{
					newEntityAnimationSetVelocity(
						newCompositeQuarticValue(0.0f, xMoveDistance / bootLiftDuration, 0.0f, 0.0f, 0.0f),
						newCompositeQuarticValue(0.0f, yMoveDistance / bootLiftDuration, 0.0f, 0.0f, 0.0f)),
					newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame),
					newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerRidingRailAnimation),
					newEntityAnimationSetSpriteDirection(nextSpriteDirection)
				});
			firstMove = false;
		//straight section
		} else if (fromSide == toSide) {
			components->insert(
				components->end(),
				{
					newEntityAnimationSetVelocity(
						newCompositeQuarticValue(0.0f, xMoveDistance / floatRailToRailTicksDuration, 0.0f, 0.0f, 0.0f),
						newCompositeQuarticValue(0.0f, yMoveDistance / floatRailToRailTicksDuration, 0.0f, 0.0f, 0.0f)),
					newEntityAnimationDelay(railToRailTicksDuration)
				});
		//curved section
		} else {
			//curved section going from side to top/bottom
			if (fromSide)
				components->push_back(
					newEntityAnimationSetVelocity(
						newCompositeQuarticValue(
							0.0f,
							2.0f * xMoveDistance / floatRailToRailTicksDuration,
							-xMoveDistance / railToRailTicksDurationSquared,
							0.0f,
							0.0f),
						newCompositeQuarticValue(0.0f, 0.0f, yMoveDistance / railToRailTicksDurationSquared, 0.0f, 0.0f)));
			//curved section going from top/bottom to side
			else
				components->push_back(
					newEntityAnimationSetVelocity(
						newCompositeQuarticValue(0.0f, 0.0f, xMoveDistance / railToRailTicksDurationSquared, 0.0f, 0.0f),
						newCompositeQuarticValue(
							0.0f,
							2.0f * yMoveDistance / floatRailToRailTicksDuration,
							-yMoveDistance / railToRailTicksDurationSquared,
							0.0f,
							0.0f)));
			const int halfRailToRailTicksDuration = railToRailTicksDuration / 2;
			components->insert(
				components->end(),
				{
					newEntityAnimationDelay(halfRailToRailTicksDuration),
					newEntityAnimationSetSpriteDirection(nextSpriteDirection),
					newEntityAnimationDelay(railToRailTicksDuration - halfRailToRailTicksDuration)
				});
		}
	}

	//move to the last tile
	float finalXPosition = nextSegment->tileCenterX();
	float finalYPosition = nextSegment->tileCenterY() - boundingBoxBottomOffset;
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
	components->insert(
		components->end(),
		{
			newEntityAnimationSetVelocity(
				newCompositeQuarticValue(0.0f, (finalXPosition - targetXPosition) / bootLiftDuration, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, (finalYPosition - targetYPosition) / bootLiftDuration, 0.0f, 0.0f, 0.0f)),
			newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerBootLiftAnimation),
			newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame),
			newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f))
		});

	if (outFinalXPosition != nullptr)
		*outFinalXPosition = finalXPosition;
	if (outFinalYPosition != nullptr)
		*outFinalYPosition = finalYPosition;
}
//begin a kicking animation and set the switch to flip
void PlayerState::kickSwitch(short switchId, int ticksTime) {
	MapState::logSwitchKick(switchId);
	vector<ReferenceCounterHolder<EntityAnimation::Component>> kickAnimationComponents;
	Holder_EntityAnimationComponentVector kickAnimationComponentsHolder (&kickAnimationComponents);
	addKickSwitchComponents(switchId, &kickAnimationComponentsHolder, true);
	beginEntityAnimation(&kickAnimationComponentsHolder, ticksTime);
}
//add the animation components for a switch kicking animation
void PlayerState::addKickSwitchComponents(
	short switchId, Holder_EntityAnimationComponentVector* componentsHolder, bool allowRadioTowerAnimation)
{
	vector<ReferenceCounterHolder<EntityAnimation::Component>>* components = componentsHolder->val;
	components->insert(
		components->end(),
		{
			newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
			newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerKickingAnimation),
			newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame),
			newEntityAnimationMapKickSwitch(switchId, allowRadioTowerAnimation),
			newEntityAnimationDelay(
				SpriteRegistry::playerKickingAnimation->getTotalTicksDuration()
					- SpriteRegistry::playerKickingAnimationTicksPerFrame)
		});
}
//begin a kicking animation and set the reset switch to flip
void PlayerState::kickResetSwitch(short resetSwitchId, int ticksTime) {
	MapState::logResetSwitchKick(resetSwitchId);
	vector<ReferenceCounterHolder<EntityAnimation::Component>> kickAnimationComponents;
	Holder_EntityAnimationComponentVector kickAnimationComponentsHolder (&kickAnimationComponents);
	addKickResetSwitchComponents(resetSwitchId, &kickAnimationComponentsHolder);
	beginEntityAnimation(&kickAnimationComponentsHolder, ticksTime);
}
//add the animation components for a reset switch kicking animation
void PlayerState::addKickResetSwitchComponents(short resetSwitchId, Holder_EntityAnimationComponentVector* componentsHolder) {
	vector<ReferenceCounterHolder<EntityAnimation::Component>>* components = componentsHolder->val;
	components->insert(
		components->end(),
		{
			newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
			newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerKickingAnimation),
			newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame),
			newEntityAnimationMapKickResetSwitch(resetSwitchId),
			newEntityAnimationDelay(
				SpriteRegistry::playerKickingAnimation->getTotalTicksDuration()
					- SpriteRegistry::playerKickingAnimationTicksPerFrame)
		});
}
//render this player state, which was deemed to be the last state to need rendering
void PlayerState::render(EntityState* camera, int ticksTime) {
	if (ghostSpriteX.get() != nullptr && ghostSpriteY.get() != nullptr) {
		float ghostRenderCenterX = getRenderCenterScreenXFromWorldX(
			ghostSpriteX.get()->getValue(ticksTime - ghostSpriteStartTicksTime), camera, ticksTime);
		float ghostRenderCenterY = getRenderCenterScreenYFromWorldY(
			ghostSpriteY.get()->getValue(ticksTime - ghostSpriteStartTicksTime), camera, ticksTime);

		glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
		SpriteRegistry::player->renderSpriteCenteredAtScreenPosition(
			hasBoot ? 4 : 0, (int)(SpriteDirection::Down), ghostRenderCenterX, ghostRenderCenterY);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	float renderCenterX = getRenderCenterScreenX(camera,  ticksTime);
	float renderCenterY = getRenderCenterScreenY(camera,  ticksTime);
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
//render the kick action for this player state if one is available
void PlayerState::renderKickAction(EntityState* camera, bool hasRailsToReset, int ticksTime) {
	if (!hasBoot || availableKickAction.get() == nullptr)
		return;
	float renderCenterX = getRenderCenterScreenX(camera,  ticksTime);
	float renderTopY = getRenderCenterScreenY(camera,  ticksTime) - (float)SpriteRegistry::player->getSpriteHeight() / 2.0f;
	availableKickAction.get()->render(renderCenterX, renderTopY, hasRailsToReset);
}
//save this player state to the file
void PlayerState::saveState(ofstream& file) {
	file << playerXFilePrefix << lastControlledX << "\n";
	file << playerYFilePrefix << lastControlledY << "\n";
}
//try to load state from the line of the file, return whether state was loaded
bool PlayerState::loadState(string& line) {
	if (StringUtils::startsWith(line, playerXFilePrefix)) {
		lastControlledX = (float)atof(line.c_str() + playerXFilePrefix.size());
		x.set(newCompositeQuarticValue(lastControlledX, 0.0f, 0.0f, 0.0f, 0.0f));
	} else if (StringUtils::startsWith(line, playerYFilePrefix)) {
		lastControlledY = (float)atof(line.c_str() + playerYFilePrefix.size());
		y.set(newCompositeQuarticValue(lastControlledY, 0.0f, 0.0f, 0.0f, 0.0f));
	} else
		return false;
	return true;
}
//set the initial z for the player after loading the position
//this isn't technically loading anything from the file but it is part of setting the initial state
void PlayerState::setInitialZ() {
	z = MapState::getHeight(
		(int)x.get()->getValue(0) / MapState::tileSize,
		(int)(y.get()->getValue(0) + boundingBoxCenterYOffset) / MapState::tileSize);
}
//reset any state on the player that shouldn't exist for the intro animation
void PlayerState::reset() {
	availableKickAction.set(nullptr);
	z = 0;
	hasBoot = false;
	autoKickStartTicksTime = -1;
	canImmediatelyAutoKick = false;
}
#ifdef DEBUG
	//move the player as high as possible so that all rails render under
	void PlayerState::setHighestZ() {
		z = MapState::highestFloorHeight;
	}
#endif
