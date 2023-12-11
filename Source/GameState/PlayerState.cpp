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
#include "Util/Logger.h"
#include "Util/StringUtils.h"

#ifdef DEBUG
	//**/#define SHOW_PLAYER_BOUNDING_BOX 1
#endif
#define newClimbFallKickAction(type, targetPlayerX, targetPlayerY, fallHeight) \
	newKickAction(type, targetPlayerX, targetPlayerY, fallHeight, MapState::absentRailSwitchId, -1)
#define newRailSwitchKickAction(type, railSwitchId, railSegmentIndex) \
	newKickAction(type, -1, -1, MapState::invalidHeight, railSwitchId, railSegmentIndex)

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
, lastControlledY(0.0f)
, worldGroundY(nullptr)
, worldGroundYOffset(0.0f)
, finishedMoveTutorial(false)
, finishedKickTutorial(false)
, lastGoalX(0)
, lastGoalY(0) {
}
PlayerState::~PlayerState() {
	delete collisionRect;
}
PlayerState* PlayerState::produce(objCounterParametersComma() MapState* mapState) {
	initializeWithNewFromPool(p, PlayerState)
	p->z = 0;
	p->hasBoot = false;
	p->mapState.set(mapState);
	p->availableKickAction.set(nullptr);
	return p;
}
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
	worldGroundY.set(other->worldGroundY.get());
	worldGroundYOffset = other->worldGroundYOffset;
	finishedMoveTutorial = other->finishedMoveTutorial;
	finishedKickTutorial = other->finishedKickTutorial;
	lastGoalX = other->lastGoalX;
	lastGoalY = other->lastGoalY;
}
pooledReferenceCounterDefineRelease(PlayerState)
float PlayerState::getWorldGroundY(int ticksTime) {
	int ticksSinceLastUpdate = ticksTime - lastUpdateTicksTime;
	return worldGroundY.get() != nullptr
		? worldGroundY.get()->getValue(ticksSinceLastUpdate)
		: y.get()->getValue(ticksSinceLastUpdate) + z / 2 * MapState::tileSize + boundingBoxCenterYOffset + worldGroundYOffset;
}
void PlayerState::setDirection(SpriteDirection pSpriteDirection) {
	spriteDirection = pSpriteDirection;
	xDirection = pSpriteDirection == SpriteDirection::Left ? -1 : pSpriteDirection == SpriteDirection::Right ? 1 : 0;
	yDirection = pSpriteDirection == SpriteDirection::Up ? -1 : pSpriteDirection == SpriteDirection::Down ? 1 : 0;
}
void PlayerState::setNextCamera(GameState* nextGameState, int ticksTime) {
	nextGameState->setPlayerCamera();
}
void PlayerState::setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime) {
	spriteAnimation = pSpriteAnimation;
	spriteAnimationStartTicksTime = pSpriteAnimationStartTicksTime;
}
void PlayerState::setGhostSprite(bool show, float x, float y, int ticksTime) {
	if (show) {
		ghostSpriteX.set(newConstantValue(x));
		ghostSpriteY.set(newConstantValue(y));
		ghostSpriteStartTicksTime = ticksTime;
	} else {
		ghostSpriteX.set(nullptr);
		ghostSpriteY.set(nullptr);
	}
}
void PlayerState::mapKickSwitch(short switchId, bool allowRadioTowerAnimation, int ticksTime) {
	mapState.get()->flipSwitch(switchId, allowRadioTowerAnimation, ticksTime);
}
void PlayerState::mapKickResetSwitch(short resetSwitchId, int ticksTime) {
	mapState.get()->flipResetSwitch(resetSwitchId, ticksTime);
}
void PlayerState::spawnParticle(
	float pX, float pY, SpriteAnimation* pAnimation, SpriteDirection pDirection, int particleStartTicksTime)
{
	mapState.get()->queueParticle(
		pX,
		pY,
		pDirection == SpriteDirection::Up,
		{
			newEntityAnimationSetSpriteAnimation(pAnimation),
			newEntityAnimationSetDirection(pDirection),
			newEntityAnimationDelay(pAnimation->getTotalTicksDuration()),
		},
		particleStartTicksTime);
}
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
bool PlayerState::hasRailSwitchKickAction(KickActionType kickActionType, short* outRailSwitchId) {
	if (availableKickAction.get() != nullptr && availableKickAction.get()->getType() == kickActionType) {
		*outRailSwitchId = availableKickAction.get()->getRailSwitchId();
		return true;
	}
	return false;
}
void PlayerState::updateWithPreviousPlayerState(PlayerState* prev, bool hasKeyboardControl, int ticksTime) {
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
	worldGroundY.set(nullptr);
	worldGroundYOffset = 0.0f;
	finishedMoveTutorial = prev->finishedMoveTutorial;
	finishedKickTutorial = prev->finishedKickTutorial;
	lastGoalX = prev->lastGoalX;
	lastGoalY = prev->lastGoalY;

	//if we can control the player then that must mean the player has the boot
	hasBoot = true;

	//update this player state normally by reading from the last state
	updatePositionWithPreviousPlayerState(prev, hasKeyboardControl, ticksTime);
	if (!Editor::isActive)
		collideWithEnvironmentWithPreviousPlayerState(prev);
	updateSpriteWithPreviousPlayerState(prev, ticksTime, !previousStateHadEntityAnimation);
	if (!Editor::isActive) {
		setKickAction();
		tryAutoKick(prev, ticksTime);
		trySpawnGoalSparks(ticksTime);
	}

	//copy the position to the save values
	lastControlledX = x.get()->getValue(0);
	lastControlledY = y.get()->getValue(0);
}
void PlayerState::updatePositionWithPreviousPlayerState(PlayerState* prev, bool hasKeyboardControl, int ticksTime) {
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	if (hasKeyboardControl) {
		xDirection = (char)(keyboardState[Config::rightKeyBinding.value] - keyboardState[Config::leftKeyBinding.value]);
		yDirection = (char)(keyboardState[Config::downKeyBinding.value] - keyboardState[Config::upKeyBinding.value]);
	} else {
		xDirection = 0;
		yDirection = 0;
	}
	float speedPerTick = (xDirection & yDirection) != 0 ? diagonalSpeedPerTick : baseSpeedPerTick;
	if (Editor::isActive)
		speedPerTick *= keyboardState[Config::kickKeyBinding.value] == 0 ? 2.5f : 8.0f;

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

	if (xDirection != 0 || yDirection != 0)
		finishedMoveTutorial = true;
}
void PlayerState::setXAndUpdateCollisionRect(DynamicValue* newX) {
	x.set(newX);
	float xPosition = x.get()->getValue(0);
	collisionRect->left = xPosition + boundingBoxLeftOffset;
	collisionRect->right = xPosition + boundingBoxRightOffset;
}
void PlayerState::setYAndUpdateCollisionRect(DynamicValue* newY) {
	y.set(newY);
	float yPosition = y.get()->getValue(0);
	collisionRect->top = yPosition + boundingBoxTopOffset;
	collisionRect->bottom = yPosition + boundingBoxBottomOffset;
}
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
					? collidedRect->right + smallDistance - boundingBoxLeftOffset
					: collidedRect->left - smallDistance - boundingBoxRightOffset));
		//we hit this rect from the top or bottom
		} else {
			renderInterpolatedY = false;
			setYAndUpdateCollisionRect(
				y.get()->copyWithConstantValue(prev->yDirection < 0
					? collidedRect->bottom + smallDistance - boundingBoxTopOffset
					: collidedRect->top - smallDistance - boundingBoxBottomOffset));
		}
		collidedRects.erase(collidedRects.begin() + mostCollidedRectIndex);
	}
}
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
float PlayerState::netCollisionDuration(CollisionRect* other) {
	return MathUtils::fmin(xCollisionDuration(other), yCollisionDuration(other));
}
float PlayerState::xCollisionDuration(CollisionRect* other) {
	return lastXMovedDelta < 0
		? (collisionRect->left - other->right) / lastXMovedDelta
		: (collisionRect->right - other->left) / lastXMovedDelta;
}
float PlayerState::yCollisionDuration(CollisionRect* other) {
	return lastYMovedDelta < 0
		? (collisionRect->top - other->bottom) / lastYMovedDelta
		: (collisionRect->bottom - other->top) / lastYMovedDelta;
}
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
		if (spriteDirection == SpriteDirection::Up || spriteDirection == SpriteDirection::Down)
			return MapState::tileHasRailEnd((int)(railCheckXPosition - MapState::halfTileSize) / MapState::tileSize, railMapY)
				|| MapState::tileHasRailEnd((int)(railCheckXPosition + MapState::halfTileSize) / MapState::tileSize, railMapY);
		else
			return MapState::tileHasRailEnd(railMapX, (int)(railCheckYPosition - MapState::halfTileSize) / MapState::tileSize)
				|| MapState::tileHasRailEnd(railMapX, (int)(railCheckYPosition + MapState::halfTileSize) / MapState::tileSize);
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
bool PlayerState::setSwitchKickAction(float xPosition, float yPosition) {
	short switchId = MapState::absentRailSwitchId;
	if (spriteDirection == SpriteDirection::Up || spriteDirection == SpriteDirection::Down) {
		int switchLeftMapX = (int)(xPosition + boundingBoxLeftOffset + MapState::switchSideInset) / MapState::tileSize;
		int switchRightMapX = (int)(xPosition + boundingBoxRightOffset - MapState::switchSideInset) / MapState::tileSize;
		int switchMapY = spriteDirection == SpriteDirection::Up
			? (int)(yPosition + boundingBoxTopOffset + MapState::switchBottomInset - kickingDistanceLimit) / MapState::tileSize
			: (int)(yPosition + boundingBoxBottomOffset - MapState::switchTopInset + downKickingDistanceLimit)
				/ MapState::tileSize;
		for (int switchMapX = switchLeftMapX; switchMapX <= switchRightMapX; switchMapX++) {
			if (MapState::tileHasSwitch(switchMapX, switchMapY)) {
				switchId = MapState::getRailSwitchId(switchMapX, switchMapY);
				break;
			}
		}
		//for the switches on the radio tower, accept a slightly further kick so that players who are too far can still trigger
		//	the switch
		if (switchId == MapState::absentRailSwitchId && spriteDirection == SpriteDirection::Up) {
			int switchCenterMapX = (int)xPosition / MapState::tileSize;
			switchMapY = (int)(yPosition + boundingBoxTopOffset - 0.5f) / MapState::tileSize;
			if (MapState::tileHasSwitch(switchCenterMapX, switchMapY)
					&& mapState.get()->getSwitchState(switchCenterMapX, switchMapY)->getSwitch()->getGroup() == 0)
				switchId = MapState::getRailSwitchId(switchCenterMapX, switchMapY);
		}
	} else {
		float kickingXOffset = spriteDirection == SpriteDirection::Left
			? boundingBoxLeftOffset + MapState::switchSideInset - kickingDistanceLimit
			: boundingBoxRightOffset - MapState::switchSideInset + kickingDistanceLimit;
		int switchMapX = (int)(xPosition + kickingXOffset) / MapState::tileSize;
		int switchTopMapY = (int)(yPosition + boundingBoxTopOffset + MapState::switchBottomInset) / MapState::tileSize;
		int switchBottomMapY = (int)(yPosition + boundingBoxBottomOffset - MapState::switchTopInset) / MapState::tileSize;
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
bool PlayerState::setClimbKickAction(float xPosition, float yPosition) {
	float targetXPosition;
	float targetYPosition;
	if (spriteDirection == SpriteDirection::Up || spriteDirection == SpriteDirection::Down) {
		targetXPosition = xPosition;
		if (spriteDirection == SpriteDirection::Up) {
			//set a distance such that the bottom of the player is slightly past the edge of the ledge
			//bottomMapY adds 1 to account for rounding, but then subtracts 1 because of the wall space that we skip
			int bottomMapY = (int)(yPosition + boundingBoxTopOffset - kickingDistanceLimit) / MapState::tileSize;
			targetYPosition = (float)(bottomMapY * MapState::tileSize) - boundingBoxBottomOffset - smallDistance;
		} else {
			//set a distance such that the top of the player is slightly past the edge of the ledge
			int topMapY = (int)(yPosition + boundingBoxBottomOffset + kickingDistanceLimit) / MapState::tileSize;
			targetYPosition = (float)(topMapY * MapState::tileSize) - boundingBoxTopOffset + smallDistance;
		}
	} else {
		targetYPosition = yPosition - (float)MapState::tileSize;
		if (spriteDirection == SpriteDirection::Left) {
			//set a distance such that the right of the player is slightly past the edge of the ledge
			int rightMapX = (int)(xPosition + boundingBoxLeftOffset - kickingDistanceLimit) / MapState::tileSize + 1;
			targetXPosition = (float)(rightMapX * MapState::tileSize) - boundingBoxRightOffset - smallDistance;
		} else {
			//set a distance such that the left of the player is slightly past the edge of the ledge
			int leftMapX = (int)(xPosition + boundingBoxRightOffset + kickingDistanceLimit) / MapState::tileSize;
			targetXPosition = (float)(leftMapX * MapState::tileSize) - boundingBoxLeftOffset + smallDistance;
		}
	}

	if (!checkCanMoveToPosition(targetXPosition, targetYPosition, z + 2, spriteDirection, &targetXPosition, &targetYPosition))
		return false;
	availableKickAction.set(newClimbFallKickAction(KickActionType::Climb, targetXPosition, targetYPosition, 0));
	return true;
}
bool PlayerState::setFallKickAction(float xPosition, float yPosition) {
	char fallHeight;
	float targetXPosition;
	float targetYPosition;
	if (spriteDirection == SpriteDirection::Up) {
		int centerMapX = (int)xPosition / MapState::tileSize;
		int oneTileUpMapY = (int)(yPosition + boundingBoxTopOffset - kickingDistanceLimit) / MapState::tileSize;
		fallHeight = MapState::getHeight(centerMapX, oneTileUpMapY);
		//can't fall unless the tile up is a lower floor height
		if (fallHeight == MapState::invalidHeight || fallHeight >= z || (fallHeight & 1) != 0)
			return false;

		//move a distance such that the bottom of the player is slightly above the top of the cliff
		targetXPosition = xPosition;
		targetYPosition = (float)((oneTileUpMapY + 1) * MapState::tileSize) - boundingBoxBottomOffset - smallDistance;
	} else if (spriteDirection == SpriteDirection::Down) {
		int centerMapX = (int)xPosition / MapState::tileSize;
		int oneTileDownMapY = (int)(yPosition + boundingBoxBottomOffset + kickingDistanceLimit) / MapState::tileSize;
		//start two tiles down and look for an eligible floor below our current height
		for (char tileOffset = 1; true; tileOffset++) {
			fallHeight = MapState::getHeight(centerMapX, oneTileDownMapY + tileOffset);
			char targetHeight = z - tileOffset * 2;
			//the tile is higher than us (or it's the empty tile), we can't fall here
			if (fallHeight > targetHeight)
				return false;
			//this is a cliff face, keep looking
			else if (fallHeight < targetHeight)
				continue;

			//we found a matching floor tile, move a distance such that the bottom of the player is slightly below the bottom of
			//	the cliff
			targetXPosition = xPosition;
			targetYPosition =
				(float)((oneTileDownMapY + tileOffset) * MapState::tileSize) - boundingBoxTopOffset + smallDistance;
			break;
		}
	} else {
		int centerMapY = (int)(yPosition + boundingBoxCenterYOffset) / MapState::tileSize;
		int sideTilesEdgeMapX;
		if (spriteDirection == SpriteDirection::Left) {
			sideTilesEdgeMapX = (int)(xPosition + boundingBoxLeftOffset - kickingDistanceLimit) / MapState::tileSize;
			targetXPosition =
				(float)((sideTilesEdgeMapX + 1) * MapState::tileSize) - boundingBoxRightOffset - smallDistance;
		} else {
			sideTilesEdgeMapX = (int)(xPosition + boundingBoxRightOffset + kickingDistanceLimit) / MapState::tileSize;
			targetXPosition = (float)(sideTilesEdgeMapX * MapState::tileSize) - boundingBoxLeftOffset + smallDistance;
		}
		//start one tile down and look for an eligible floor below our current height
		for (char tileOffset = 1; true; tileOffset++) {
			fallHeight = MapState::getHeight(sideTilesEdgeMapX, centerMapY + tileOffset);
			char targetHeight = z - tileOffset * 2;
			//an empty tile height is fine...
			if (fallHeight == MapState::emptySpaceHeight) {
				//...unless we reached the lowest height, in which case there is no longer a possible fall height
				if (targetHeight == 0)
					return false;
				continue;
			//the tile is higher than us, we can't fall here
			} else if (fallHeight > targetHeight)
				return false;
			//this is a cliff face or lower floor, keep looking
			else if (fallHeight < targetHeight)
				continue;

			//we found a matching floor tile
			targetYPosition = yPosition + tileOffset * MapState::tileSize;
			break;
		}
	}

	if (!checkCanMoveToPosition(
			targetXPosition, targetYPosition, fallHeight, spriteDirection, &targetXPosition, &targetYPosition))
		return false;
	KickActionType kickActionType = fallHeight < z - 2 ? KickActionType::FallBig : KickActionType::Fall;
	availableKickAction.set(newClimbFallKickAction(kickActionType, targetXPosition, targetYPosition, fallHeight));
	return true;
}
bool PlayerState::checkCanMoveToPosition(
	float targetXPosition,
	float targetYPosition,
	char targetHeight,
	SpriteDirection moveDirection,
	float* outActualXPosition,
	float* outActualYPosition)
{
	//check if every tile in our target area is the target height
	int lowMapX = (int)(targetXPosition + boundingBoxLeftOffset) / MapState::tileSize;
	int lowMapY = (int)(targetYPosition + boundingBoxTopOffset) / MapState::tileSize;
	int highMapX = (int)(targetXPosition + boundingBoxRightOffset) / MapState::tileSize;
	int highMapY = (int)(targetYPosition + boundingBoxBottomOffset) / MapState::tileSize;
	bool canMoveDirect = true;
	for (int mapY = lowMapY; mapY <= highMapY; mapY++) {
		if (MapState::horizontalTilesHeight(lowMapX, highMapX, mapY) != targetHeight) {
			canMoveDirect = false;
			break;
		}
	}
	//we can move without needing to shift the position at all
	if (canMoveDirect) {
		*outActualXPosition = targetXPosition;
		*outActualYPosition = targetYPosition;
		return true;
	}

	//for better usability, we'll also accept a shifted position as long as the target center is valid and there's room nearby
	//to validate a climbing target, check tiles by using a 1D line matching the horizontal/vertical direction of the move
	if (moveDirection == SpriteDirection::Up || moveDirection == SpriteDirection::Down) {
		*outActualYPosition = targetYPosition;
		//moving verically - check that the vertical line in the center of the target area is valid
		int centerMapX = (int)targetXPosition / MapState::tileSize;
		if (MapState::verticalTilesHeight(centerMapX, lowMapY, highMapY) != targetHeight)
			return false;

		//the left edge isn't valid, move it right
		if (MapState::verticalTilesHeight(lowMapX, lowMapY, highMapY) != targetHeight) {
			do {
				lowMapX++;
			} while (MapState::verticalTilesHeight(lowMapX, lowMapY, highMapY) != targetHeight);
			highMapX = lowMapX + (int)(playerWidth + smallDistance) / MapState::tileSize;
			*outActualXPosition = lowMapX * MapState::tileSize - boundingBoxLeftOffset + smallDistance;
		//the left edge is valid which means the right edge isn't valid, move it left
		} else {
			do {
				highMapX--;
			} while (MapState::verticalTilesHeight(highMapX, lowMapY, highMapY) != targetHeight);
			lowMapX = highMapX - (int)(playerWidth + smallDistance) / MapState::tileSize;
			*outActualXPosition = (highMapX + 1) * MapState::tileSize - boundingBoxRightOffset - smallDistance;
		}
	} else {
		*outActualXPosition = targetXPosition;
		//moving horizontally - check that the horizontal line in the center of the target area is valid
		int centerMapY = (int)(targetYPosition + boundingBoxCenterYOffset) / MapState::tileSize;
		if (MapState::horizontalTilesHeight(lowMapX, highMapX, centerMapY) != targetHeight)
			return false;

		//the top edge isn't valid, move it down
		if (MapState::horizontalTilesHeight(lowMapX, highMapX, lowMapY) != targetHeight) {
			do {
				lowMapY++;
			} while (MapState::horizontalTilesHeight(lowMapX, highMapX, lowMapY) != targetHeight);
			highMapY = lowMapY + (int)(playerHeight + smallDistance) / MapState::tileSize;
			*outActualYPosition = lowMapY * MapState::tileSize - boundingBoxTopOffset + smallDistance;
		//the top edge is valid which means the bottom edge isn't valid, move it up
		} else {
			do {
				highMapY--;
			} while (MapState::horizontalTilesHeight(lowMapX, highMapX, highMapY) != targetHeight);
			lowMapY = highMapY - (int)(playerHeight + smallDistance) / MapState::tileSize;
			*outActualYPosition = (highMapY + 1) * MapState::tileSize - boundingBoxBottomOffset - smallDistance;
		}
	}

	//now check that the shifted position is valid
	for (int mapY = lowMapY; mapY <= highMapY; mapY++) {
		if (MapState::horizontalTilesHeight(lowMapX, highMapX, mapY) != targetHeight)
			return false;
	}
	return true;
}
void PlayerState::tryAutoKick(PlayerState* prev, int ticksTime) {
	autoKickStartTicksTime = -1;
	canImmediatelyAutoKick = (xDirection != 0 || yDirection != 0) && prev->canImmediatelyAutoKick;
	//can't auto-kick if there's no kick action, and disallow auto-kick before the player finishes the tutorial
	if (availableKickAction.get() == nullptr || !finishedKickTutorial)
		return;

	//ensure we have an auto-compatible kick action and check any secondary requirements
	switch (availableKickAction.get()->getType()) {
		case KickActionType::Climb:
		case KickActionType::Fall:
			//no auto-climb or auto-fall-small if the player isn't moving straight
			if ((xDirection != 0) == (yDirection != 0))
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
void PlayerState::trySpawnGoalSparks(int ticksTime) {
	int tileX = (int)x.get()->getValue(0) / MapState::tileSize;
	int tileY = (int)(y.get()->getValue(0) + boundingBoxCenterYOffset) / MapState::tileSize;
	if (MapState::getTile(tileX, tileY) != MapState::tilePuzzleEnd || (lastGoalX == tileX && lastGoalY == tileY))
		return;
	lastGoalX = tileX;
	lastGoalY = tileY;
	SpriteDirection directions[] =
		{ SpriteDirection::Right, SpriteDirection::Up, SpriteDirection::Left, SpriteDirection::Down };
	for (SpriteDirection direction : directions) {
		SpriteAnimation* animations[] = { SpriteRegistry::sparksSlowAnimationA, SpriteRegistry::sparksSlowAnimationA };
		animations[rand() % 2] = SpriteRegistry::sparksSlowAnimationB;
		float sparkX = (float)(tileX * MapState::tileSize + MapState::halfTileSize);
		float sparkY = (float)(tileY * MapState::tileSize + MapState::halfTileSize);
		if (direction == SpriteDirection::Up)
			sparkY -= 1.0f;
		else if (direction == SpriteDirection::Down)
			sparkY += 1.0f;
		for (int i = 0; i < 2; i++) {
			SpriteAnimation* animation = animations[i];
			int initialDelay = rand() % SpriteRegistry::sparksSlowTicksPerFrame + i * SpriteRegistry::sparksSlowTicksPerFrame;
			mapState.get()->queueParticle(
				sparkX,
				sparkY,
				false,
				{
					newEntityAnimationDelay(initialDelay),
					newEntityAnimationSetSpriteAnimation(animation),
					newEntityAnimationSetDirection(direction),
					newEntityAnimationDelay(animation->getTotalTicksDuration()),
				},
				ticksTime);
		}
	}
}
void PlayerState::beginKicking(int ticksTime) {
	if (entityAnimation.get() != nullptr)
		return;

	finishedKickTutorial = true;
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
			kickClimb(kickAction->getTargetPlayerX() - xPosition, kickAction->getTargetPlayerY() - yPosition, ticksTime);
			break;
		case KickActionType::Fall:
		case KickActionType::FallBig:
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
void PlayerState::kickAir(int ticksTime) {
	SpriteAnimation* kickingSpriteAnimation = hasBoot
		? SpriteRegistry::playerKickingAnimation
		: SpriteRegistry::playerLegLiftAnimation;
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> kickAnimationComponents ({
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
		newEntityAnimationSetSpriteAnimation(kickingSpriteAnimation),
		newEntityAnimationDelay(kickingSpriteAnimation->getTotalTicksDuration())
	});
	beginEntityAnimation(&kickAnimationComponents, ticksTime);
}
void PlayerState::kickClimb(float xMoveDistance, float yMoveDistance, int ticksTime) {
	float currentX = x.get()->getValue(0);
	float currentY = y.get()->getValue(0);
	stringstream message;
	message << "  climb " << (int)currentX << " " << (int)currentY
		<< "  " << (int)(currentX + xMoveDistance) << " " << (int)(currentY + yMoveDistance);
	Logger::gameplayLogger.logString(message.str());

	int climbAnimationDuration = SpriteRegistry::playerFastKickingAnimation->getTotalTicksDuration();
	int moveDuration = climbAnimationDuration - SpriteRegistry::playerFastKickingAnimationTicksPerFrame;
	float floatMoveDuration = (float)moveDuration;
	float moveDurationSquared = floatMoveDuration * floatMoveDuration;
	float moveDurationCubed = moveDurationSquared * floatMoveDuration;

	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> kickingAnimationComponents ({
		//start by stopping the player and delaying until the leg-sticking-out frame
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
		newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerFastKickingAnimation),
		newEntityAnimationDelay(SpriteRegistry::playerFastKickingAnimationTicksPerFrame)
	});

	//set the climb velocity
	kickingAnimationComponents.push_back(
		xMoveDistance == 0
			? newEntityAnimationSetVelocity(
				newConstantValue(0.0f),
				newCompositeQuarticValue(
					0.0f,
					0.0f,
					0.0f,
					yMoveDistance / (moveDurationCubed * 2.0f),
					yMoveDistance / (moveDurationCubed * floatMoveDuration * 2.0f)))
			: newEntityAnimationSetVelocity(
				newCompositeQuarticValue(0.0f, 0.0f, xMoveDistance / moveDurationSquared, 0.0f, 0.0f),
				newCompositeQuarticValue(
					0.0f, 2.0f * yMoveDistance / floatMoveDuration, -yMoveDistance / moveDurationSquared, 0.0f, 0.0f)));

	kickingAnimationComponents.insert(
		kickingAnimationComponents.end(),
		{
			//then delay for the rest of the animation and stop the player
			newEntityAnimationDelay(moveDuration),
			newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f))
		});


	beginEntityAnimation(&kickingAnimationComponents, ticksTime);
	float currentWorldGroundY = getWorldGroundY(lastUpdateTicksTime);
	//regardless of the movement direction, ground Y changes based on Y move distance
	const float worldGroundYChange = (yMoveDistance + 1) * MapState::tileSize;
	worldGroundY.set(
		newCompositeQuarticValue(currentWorldGroundY, worldGroundYChange / climbAnimationDuration, 0.0f, 0.0f, 0.0f));
	z += 2;
}
void PlayerState::kickFall(float xMoveDistance, float yMoveDistance, char fallHeight, int ticksTime) {
	float currentX = x.get()->getValue(0);
	float currentY = y.get()->getValue(0);
	stringstream message;
	message << "  fall " << (int)currentX << " " << (int)currentY
		<< "  " << (int)(currentX + xMoveDistance) << " " << (int)(currentY + yMoveDistance);
	Logger::gameplayLogger.logString(message.str());

	bool useFastKickingAnimation = fallHeight == z - 2;
	SpriteAnimation* fallAnimation =
		useFastKickingAnimation ? SpriteRegistry::playerFastKickingAnimation : SpriteRegistry::playerKickingAnimation;
	int fallAnimationFirstFrameTicks = useFastKickingAnimation
		? SpriteRegistry::playerFastKickingAnimationTicksPerFrame
		: SpriteRegistry::playerKickingAnimationTicksPerFrame;

	//start by stopping the player and delaying until the leg-sticking-out frame
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> kickingAnimationComponents ({
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
		newEntityAnimationSetSpriteAnimation(fallAnimation),
		newEntityAnimationDelay(fallAnimationFirstFrameTicks)
	});

	int climbAnimationDuration = fallAnimation->getTotalTicksDuration();
	int moveDuration = climbAnimationDuration - fallAnimationFirstFrameTicks;
	float floatMoveDuration = (float)moveDuration;
	float moveDurationSquared = floatMoveDuration * floatMoveDuration;
	//regardless of the fall direction or movement distance, x is linear
	DynamicValue* xVelocity = newCompositeQuarticValue(0.0f, xMoveDistance / floatMoveDuration, 0.0f, 0.0f, 0.0f);
	DynamicValue* yVelocity;

	//up has a y jump trajectory that visually goes up far, then down
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
		constexpr float midpointX = 2.0f / 3.0f;
		constexpr float midpointY = 2.0f;
		constexpr float troughX =
			(2.0f * midpointY - 3.0f * midpointY * midpointX + midpointX * midpointX * midpointX)
				/ (3.0f * midpointX * midpointX + 3.0f * midpointY - 6.0f * midpointY * midpointX);
		constexpr float unitYMultiplier = 6.0f / (2.0f - 3.0f * troughX - 3.0f * midpointX + 6.0f * troughX * midpointX);
		float yMultiplier = unitYMultiplier * yMoveDistance;

		float yLinearValuePerDuration = yMultiplier * troughX * midpointX;
		float yQuadraticValuePerDuration = -yMultiplier * (troughX + midpointX) / 2.0f;
		float yCubicValuePerDuration = yMultiplier / 3.0f;

		yVelocity = newCompositeQuarticValue(
			0.0f,
			yLinearValuePerDuration / floatMoveDuration,
			yQuadraticValuePerDuration / moveDurationSquared,
			yCubicValuePerDuration / (moveDurationSquared * floatMoveDuration),
			0.0f);
	//left, right, and down all have the same quadratic y jump trajectory
	} else
		yVelocity = newCompositeQuarticValue(
			0.0f, -yMoveDistance / floatMoveDuration, 2.0f * yMoveDistance / moveDurationSquared, 0.0f, 0.0f);
	kickingAnimationComponents.push_back(newEntityAnimationSetVelocity(xVelocity, yVelocity));


	//delay for the rest of the animation and then stop the player
	kickingAnimationComponents.insert(
		kickingAnimationComponents.end(),
		{
			newEntityAnimationDelay(moveDuration),
			newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f))
		});

	beginEntityAnimation(&kickingAnimationComponents, ticksTime);
	float currentWorldGroundY = getWorldGroundY(lastUpdateTicksTime);
	//regardless of the movement direction, ground Y changes based on Y move distance
	const float worldGroundYChange = (yMoveDistance + (fallHeight - z) / 2) * MapState::tileSize;
	worldGroundY.set(
		newCompositeQuarticValue(currentWorldGroundY, worldGroundYChange / climbAnimationDuration, 0.0f, 0.0f, 0.0f));
	z = fallHeight;
}
void PlayerState::kickRail(short railId, float xPosition, float yPosition, int ticksTime) {
	MapState::logRailRide(railId, (int)xPosition, (int)yPosition);
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> ridingRailAnimationComponents;
	addRailRideComponents(railId, &ridingRailAnimationComponents, xPosition, yPosition, nullptr, nullptr);
	beginEntityAnimation(&ridingRailAnimationComponents, ticksTime);
	//the player is visually moved up to simulate a half-height raise on the rail, but the world ground y needs to stay the same
	worldGroundYOffset = MapState::halfTileSize + playerHeight / 2;
}
void PlayerState::addRailRideComponents(
	short railId,
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components,
	float xPosition,
	float yPosition,
	float* outFinalXPosition,
	float* outFinalYPosition)
{
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

	constexpr float bootLiftDuration = (float)SpriteRegistry::playerKickingAnimationTicksPerFrame;
	constexpr float floatRailToRailTicksDuration = (float)railToRailTicksDuration;
	constexpr float railToRailTicksDurationSquared = floatRailToRailTicksDuration * floatRailToRailTicksDuration;

	components->push_back(newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerBootLiftAnimation));

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

		float tileCenterX = currentSegment->tileCenterX();
		targetXPosition = tileCenterX;
		targetYPosition = currentSegment->tileCenterY() - boundingBoxBottomOffset;
		bool fromSide = lastYPosition == targetYPosition;
		bool toSide = false;
		SpriteDirection nextSpriteDirection;
		if (nextSegment->x < currentSegment->x) {
			targetXPosition -= MapState::halfTileSize;
			toSide = true;
			nextSpriteDirection = SpriteDirection::Left;
		} else if (nextSegment->x > currentSegment->x) {
			targetXPosition += MapState::halfTileSize;
			toSide = true;
			nextSpriteDirection = SpriteDirection::Right;
		} else if (nextSegment->y < currentSegment->y) {
			targetYPosition -= MapState::halfTileSize;
			//the boot is on the right foot, move left so the boot is centered over the rail when we go up
			targetXPosition -= 0.5f;
			nextSpriteDirection = SpriteDirection::Up;
		} else {
			targetYPosition += MapState::halfTileSize;
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
					newEntityAnimationSetDirection(nextSpriteDirection)
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
					newEntityAnimationSetDirection(nextSpriteDirection),
					newEntityAnimationDelay(railToRailTicksDuration - halfRailToRailTicksDuration)
				});
		}
		components->push_back(
			newEntityAnimationSpawnParticle(
				toSide ? targetXPosition : tileCenterX,
				targetYPosition + boundingBoxBottomOffset + (nextSpriteDirection == SpriteDirection::Up ? -1 : 0),
				rand() % 2 == 0 ? SpriteRegistry::sparksAnimationA : SpriteRegistry::sparksAnimationB,
				nextSpriteDirection));
	}

	//move to the last tile
	float finalXPosition = nextSegment->tileCenterX();
	float finalYPosition = nextSegment->tileCenterY() - boundingBoxBottomOffset;
	if (finalXPosition == targetXPosition + MapState::halfTileSize) {
		finalXPosition += -MapState::halfTileSize - boundingBoxLeftOffset + smallDistance;
		finalYPosition += 2.0f;
	} else if (finalXPosition == targetXPosition - MapState::halfTileSize) {
		finalXPosition += MapState::halfTileSize - boundingBoxRightOffset - smallDistance;
		finalYPosition += 2.0f;
	} else if (finalYPosition == targetYPosition + MapState::halfTileSize) {
		finalXPosition += 0.5f;
		finalYPosition += 3.0f;
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
void PlayerState::kickSwitch(short switchId, int ticksTime) {
	MapState::logSwitchKick(switchId);
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> kickAnimationComponents;
	addKickSwitchComponents(switchId, &kickAnimationComponents, true);
	beginEntityAnimation(&kickAnimationComponents, ticksTime);
}
void PlayerState::addKickSwitchComponents(
	short switchId, vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components, bool allowRadioTowerAnimation)
{
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
void PlayerState::kickResetSwitch(short resetSwitchId, int ticksTime) {
	MapState::logResetSwitchKick(resetSwitchId);
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> kickAnimationComponents;
	addKickResetSwitchComponents(resetSwitchId, &kickAnimationComponents);
	beginEntityAnimation(&kickAnimationComponents, ticksTime);
}
void PlayerState::addKickResetSwitchComponents(
	short resetSwitchId, vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components)
{
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
void PlayerState::renderKickAction(EntityState* camera, bool hasRailsToReset, int ticksTime) {
	if (!hasBoot || availableKickAction.get() == nullptr)
		return;
	float renderCenterX = getRenderCenterScreenX(camera,  ticksTime);
	float renderTopY = getRenderCenterScreenY(camera,  ticksTime) - (float)SpriteRegistry::player->getSpriteHeight() / 2.0f;
	availableKickAction.get()->render(renderCenterX, renderTopY, hasRailsToReset);
}
void PlayerState::renderTutorials() {
	if (!finishedMoveTutorial)
		MapState::renderControlsTutorial(
			moveTutorialText,
			{
				Config::leftKeyBinding.value,
				Config::upKeyBinding.value,
				Config::rightKeyBinding.value,
				Config::downKeyBinding.value,
			});
	else if (!finishedKickTutorial)
		MapState::renderControlsTutorial(kickTutorialText, { Config::kickKeyBinding.value });
}
void PlayerState::setHomeScreenState() {
	obtainBoot();
	x.set(newConstantValue(MapState::antennaCenterWorldX()));
	y.set(newConstantValue(MapState::radioTowerPlatformCenterWorldY() - boundingBoxCenterYOffset));
}
void PlayerState::saveState(ofstream& file) {
	file << playerXFilePrefix << lastControlledX << "\n";
	file << playerYFilePrefix << lastControlledY << "\n";
	file << playerDirectionFilePrefix << (int)spriteDirection << "\n";
	if (finishedMoveTutorial)
		file << finishedMoveTutorialFileValue << "\n";
	if (finishedKickTutorial)
		file << finishedKickTutorialFileValue << "\n";
	file << lastGoalFilePrefix << lastGoalX << " " << lastGoalY << "\n";
}
bool PlayerState::loadState(string& line) {
	if (StringUtils::startsWith(line, playerXFilePrefix)) {
		lastControlledX = (float)atof(line.c_str() + StringUtils::strlenConst(playerXFilePrefix));
		x.set(newCompositeQuarticValue(lastControlledX, 0.0f, 0.0f, 0.0f, 0.0f));
	} else if (StringUtils::startsWith(line, playerYFilePrefix)) {
		lastControlledY = (float)atof(line.c_str() + StringUtils::strlenConst(playerYFilePrefix));
		y.set(newCompositeQuarticValue(lastControlledY, 0.0f, 0.0f, 0.0f, 0.0f));
	} else if (StringUtils::startsWith(line, playerDirectionFilePrefix)) {
		int direction = atoi(line.c_str() + StringUtils::strlenConst(playerDirectionFilePrefix));
		if (direction >= 0 && direction < 4)
			spriteDirection = (SpriteDirection)direction;
	} else if (StringUtils::startsWith(line, finishedMoveTutorialFileValue))
		finishedMoveTutorial = true;
	else if (StringUtils::startsWith(line, finishedKickTutorialFileValue))
		finishedKickTutorial = true;
	else if (StringUtils::startsWith(line, lastGoalFilePrefix))
		StringUtils::parsePosition(line.c_str() + StringUtils::strlenConst(lastGoalFilePrefix), &lastGoalX, &lastGoalY);
	else
		return false;
	return true;
}
void PlayerState::setInitialZ() {
	z = MapState::getHeight(
		(int)x.get()->getValue(0) / MapState::tileSize,
		(int)(y.get()->getValue(0) + boundingBoxCenterYOffset) / MapState::tileSize);
}
void PlayerState::reset() {
	availableKickAction.set(nullptr);
	z = 0;
	hasBoot = false;
	autoKickStartTicksTime = -1;
	canImmediatelyAutoKick = false;
	finishedMoveTutorial = false;
	finishedKickTutorial = false;
	lastGoalX = 0;
	lastGoalY = 0;
}
void PlayerState::setHighestZ() {
	z = MapState::highestFloorHeight;
}
