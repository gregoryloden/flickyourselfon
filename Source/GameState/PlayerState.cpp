#include "PlayerState.h"
#include "Audio/Audio.h"
#include "Editor/Editor.h"
#include "GameState/CollisionRect.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/GameState.h"
#include "GameState/HintState.h"
#include "GameState/KickAction.h"
#include "GameState/UndoState.h"
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

thread* PlayerState::hintSearchThread = nullptr;
ReferenceCounterHolder<HintState> PlayerState::hintSearchStorage (nullptr);
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
, lastStepSound(-1)
, hasBoot(false)
, ghostSpriteX(nullptr)
, ghostSpriteY(nullptr)
, ghostSpriteDirection(SpriteDirection::Down)
, ghostSpriteStartTicksTime(0)
, mapState(nullptr)
, availableKickAction(nullptr)
, autoKickStartTicksTime(-1)
, canImmediatelyAutoKick(false)
, lastControlledX(0.0f)
, lastControlledY(0.0f)
, worldGroundY(nullptr)
, worldGroundYStartTicksTime(0)
, worldGroundYOffset(0.0f)
, finishedMoveTutorial(false)
, finishedKickTutorial(false)
, finishedUndoRedoTutorial(false)
, lastGoalX(0)
, lastGoalY(0)
, undoState(nullptr)
, redoState(nullptr)
, hintState(nullptr)
, noClip(false) {
}
PlayerState::~PlayerState() {
	delete collisionRect;
	//only one PlayerState needs to do this, but this is the right place to do it
	waitForHintThreadToFinish();
}
PlayerState* PlayerState::produce(objCounterParametersComma() MapState* mapState) {
	initializeWithNewFromPool(p, PlayerState)
	p->z = 0;
	p->hasBoot = false;
	p->mapState.set(mapState);
	p->availableKickAction.set(nullptr);
	p->hintState.set(newHintState(&Hint::none));
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
	lastStepSound = other->lastStepSound;
	hasBoot = other->hasBoot;
	ghostSpriteX.set(other->ghostSpriteX.get());
	ghostSpriteY.set(other->ghostSpriteY.get());
	ghostSpriteDirection = other->ghostSpriteDirection;
	ghostSpriteStartTicksTime = other->ghostSpriteStartTicksTime;
	//don't copy map state, it was provided when this player state was produced
	availableKickAction.set(other->availableKickAction.get());
	autoKickStartTicksTime = other->autoKickStartTicksTime;
	canImmediatelyAutoKick = other->canImmediatelyAutoKick;
	lastControlledX = other->lastControlledX;
	lastControlledY = other->lastControlledY;
	worldGroundY.set(other->worldGroundY.get());
	worldGroundYStartTicksTime = other->worldGroundYStartTicksTime;
	worldGroundYOffset = other->worldGroundYOffset;
	finishedMoveTutorial = other->finishedMoveTutorial;
	finishedKickTutorial = other->finishedKickTutorial;
	finishedUndoRedoTutorial = other->finishedUndoRedoTutorial;
	lastGoalX = other->lastGoalX;
	lastGoalY = other->lastGoalY;
	setUndoState(undoState, other->undoState.get());
	setUndoState(redoState, other->redoState.get());
	hintState.set(other->hintState.get());
	noClip = other->noClip;
}
pooledReferenceCounterDefineRelease(PlayerState)
void PlayerState::prepareReturnToPool() {
	ghostSpriteX.set(nullptr);
	ghostSpriteY.set(nullptr);
	mapState.set(nullptr);
	availableKickAction.set(nullptr);
	worldGroundY.set(nullptr);
	undoState.set(nullptr);
	redoState.set(nullptr);
	hintState.set(nullptr);
}
float PlayerState::getWorldGroundY(int ticksTime) {
	return worldGroundY.get() != nullptr
		? worldGroundY.get()->getValue(ticksTime - worldGroundYStartTicksTime)
		: y.get()->getValue(ticksTime - lastUpdateTicksTime)
			+ z / 2 * MapState::tileSize
			+ boundingBoxCenterYOffset
			+ worldGroundYOffset;
}
float PlayerState::getDynamicZ(int ticksTime) {
	return worldGroundY.get() != nullptr
		? (worldGroundY.get()->getValue(ticksTime - worldGroundYStartTicksTime)
				- y.get()->getValue(ticksTime - lastUpdateTicksTime)
				- boundingBoxCenterYOffset
				- worldGroundYOffset)
			* 2
			/ MapState::tileSize
		: z;
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
void PlayerState::setGhostSprite(bool show, float pX, float pY, SpriteDirection direction, int ticksTime) {
	if (show) {
		ghostSpriteX.set(newConstantValue(pX));
		ghostSpriteY.set(newConstantValue(pY));
		ghostSpriteDirection = direction;
		ghostSpriteStartTicksTime = ticksTime;
	} else {
		ghostSpriteX.set(nullptr);
		ghostSpriteY.set(nullptr);
	}
}
void PlayerState::mapKickSwitch(short switchId, bool moveRailsForward, bool allowRadioTowerAnimation, int ticksTime) {
	mapState.get()->flipSwitch(switchId, moveRailsForward, allowRadioTowerAnimation, ticksTime);
}
void PlayerState::mapKickResetSwitch(short resetSwitchId, KickResetSwitchUndoState* kickResetSwitchUndoState, int ticksTime) {
	mapState.get()->flipResetSwitch(resetSwitchId, kickResetSwitchUndoState, ticksTime);
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
void PlayerState::generateHint(HintState* useHint, int ticksTime) {
	if (useHint != nullptr)
		hintState.set(useHint);
	else {
		int timeDiff = lastUpdateTicksTime - ticksTime;
		float hintX = x.get()->getValue(timeDiff);
		float hintY = y.get()->getValue(timeDiff) + boundingBoxCenterYOffset;
		ReferenceCounterHolder<MapState> mapStateCapture (mapState.get());
		//if another hint is being generated, force-wait for it to finish, even if it delays the update
		waitForHintThreadToFinish();
		hintSearchThread = new thread([hintX, hintY, mapStateCapture]() {
			Logger::setupLogQueue("H");
			hintSearchStorage.set(mapStateCapture.get()->generateHint(hintX, hintY));
			Logger::markLogQueueUnused();
		});
	}
}
void PlayerState::waitForHintThreadToFinish() {
	if (hintSearchThread == nullptr)
		return;
	hintSearchThread->join();
	delete hintSearchThread;
	hintSearchThread = nullptr;
	hintSearchStorage.set(nullptr);
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
	lastStepSound = prev->lastStepSound;
	ghostSpriteX.set(prev->ghostSpriteX.get());
	ghostSpriteY.set(prev->ghostSpriteY.get());
	worldGroundY.set(nullptr);
	worldGroundYOffset = 0.0f;
	finishedMoveTutorial = prev->finishedMoveTutorial;
	finishedKickTutorial = prev->finishedKickTutorial;
	finishedUndoRedoTutorial = prev->finishedUndoRedoTutorial;
	lastGoalX = prev->lastGoalX;
	lastGoalY = prev->lastGoalY;
	setUndoState(undoState, prev->undoState.get());
	setUndoState(redoState, prev->redoState.get());
	noClip = prev->noClip;

	//if we can control the player then that must mean the player has the boot
	hasBoot = true;

	//update this player state normally by reading from the last state
	const Uint8* keyboardState = hasKeyboardControl ? SDL_GetKeyboardState(nullptr) : nullptr;
	updatePositionWithPreviousPlayerState(prev, keyboardState, ticksTime);
	if (!Editor::isActive) {
		if (noClip) {
			char footHeight = MapState::getHeight(
				(int)x.get()->getValue(0) / MapState::tileSize,
				(int)(y.get()->getValue(0) + boundingBoxCenterYOffset) / MapState::tileSize);
			if (footHeight % 2 == 0)
				z = footHeight;
		} else
			collideWithEnvironmentWithPreviousPlayerState(prev);
	}
	updateSpriteWithPreviousPlayerState(prev, keyboardState, ticksTime, previousStateHadEntityAnimation);
	if (!Editor::isActive) {
		setKickAction();
		tryAutoKick(prev, ticksTime);
		trySpawnGoalSparks(ticksTime);
		tryCollectCompletedHint(prev);
	}

	//copy the position to the save values
	lastControlledX = x.get()->getValue(0);
	lastControlledY = y.get()->getValue(0);
}
void PlayerState::updatePositionWithPreviousPlayerState(PlayerState* prev, const Uint8* keyboardState, int ticksTime) {
	if (keyboardState != nullptr) {
		xDirection = (char)(keyboardState[Config::rightKeyBinding.value] - keyboardState[Config::leftKeyBinding.value]);
		yDirection = (char)(keyboardState[Config::downKeyBinding.value] - keyboardState[Config::upKeyBinding.value]);
	} else {
		xDirection = 0;
		yDirection = 0;
	}
	float speedPerTick = (xDirection & yDirection) != 0 ? diagonalSpeedPerTick : baseSpeedPerTick;
	if (keyboardState == nullptr)
		; //no adjustments to speedPerTick if we're not controlling the player
	else if (Editor::isActive)
		speedPerTick *= keyboardState[Config::sprintKeyBinding.value] == 0 ? 2.5f : 8.0f;
	else if (keyboardState[Config::sprintKeyBinding.value] != 0)
		speedPerTick *= sprintModifier;

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

	if ((xDirection | yDirection) != 0) {
		finishedMoveTutorial = true;
		setUndoState(redoState, nullptr);
		if (undoState.get() == nullptr || undoState.get()->getTypeIdentifier() != MoveUndoState::classTypeIdentifier)
			stackNewMoveUndoState(undoState, newX, newY);
	}
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
void PlayerState::updateSpriteWithPreviousPlayerState(
	PlayerState* prev, const Uint8* keyboardState, int ticksTime, bool restartSpriteAnimation)
{
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
	else {
		bool isSprinting = keyboardState != nullptr && keyboardState[Config::sprintKeyBinding.value] != 0;
		spriteAnimation = hasBoot
			? isSprinting
				? SpriteRegistry::playerBootSprintingAnimation
				: SpriteRegistry::playerBootWalkingAnimation
			: SpriteRegistry::playerWalkingAnimation;
		spriteAnimationStartTicksTime = (restartSpriteAnimation || prev->spriteAnimation == nullptr)
			? ticksTime
			: prev->spriteAnimationStartTicksTime;

		//play a sound if applicable
		int soundTicksPerFrame = isSprinting
			? SpriteRegistry::playerSprintingAnimationTicksPerFrame
			: SpriteRegistry::playerWalkingAnimationTicksPerFrame;
		int stepInterval = soundTicksPerFrame * 2;
		int soundNum = (ticksTime - spriteAnimationStartTicksTime + soundTicksPerFrame) / stepInterval;
		int prevSoundNum = (prev->lastUpdateTicksTime - spriteAnimationStartTicksTime + soundTicksPerFrame) / stepInterval;
		if (soundNum != prevSoundNum) {
			lastStepSound = rand() % (Audio::stepSoundsCount - 1);
			if (lastStepSound >= prev->lastStepSound)
				lastStepSound++;
			Audio::stepSounds[lastStepSound]->play(0);
		}
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
		int fallY;
		if (!MapState::tileFalls(centerMapX, oneTileDownMapY, z, &fallY, &fallHeight))
			return false;
		//we found a matching floor tile, move a distance such that the bottom of the player is slightly below the bottom of the
		//	cliff
		targetXPosition = xPosition;
		targetYPosition = (float)(fallY * MapState::tileSize) - boundingBoxTopOffset + smallDistance;
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
		int fallY;
		if (!MapState::tileFalls(sideTilesEdgeMapX, centerMapY, z, &fallY, &fallHeight))
			return false;
		targetYPosition = yPosition + (fallY - centerMapY) * MapState::tileSize;
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
			highMapX = lowMapX + (int)(boundingBoxWidth + smallDistance) / MapState::tileSize;
			*outActualXPosition = lowMapX * MapState::tileSize - boundingBoxLeftOffset + smallDistance;
		//the left edge is valid which means the right edge isn't valid, move it left
		} else {
			do {
				highMapX--;
			} while (MapState::verticalTilesHeight(highMapX, lowMapY, highMapY) != targetHeight);
			lowMapX = highMapX - (int)(boundingBoxWidth + smallDistance) / MapState::tileSize;
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
			highMapY = lowMapY + (int)(boundingBoxHeight + smallDistance) / MapState::tileSize;
			*outActualYPosition = lowMapY * MapState::tileSize - boundingBoxTopOffset + smallDistance;
		//the top edge is valid which means the bottom edge isn't valid, move it up
		} else {
			do {
				highMapY--;
			} while (MapState::horizontalTilesHeight(lowMapX, highMapX, highMapY) != targetHeight);
			lowMapY = highMapY - (int)(boundingBoxHeight + smallDistance) / MapState::tileSize;
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
	Audio::victorySound->play(0);
}
void PlayerState::tryCollectCompletedHint(PlayerState* other) {
	if (hintSearchThread == nullptr || hintSearchStorage.get() == nullptr) {
		hintState.set(other->hintState.get());
		return;
	}
	hintSearchThread->join();
	delete hintSearchThread;
	hintSearchThread = nullptr;
	hintState.set(hintSearchStorage.get());
	hintSearchStorage.set(nullptr);
}
bool PlayerState::shouldSuggestUndoReset() {
	return hintState.get()->hint->type == Hint::Type::UndoReset;
}
void PlayerState::beginKicking(int ticksTime) {
	if (entityAnimation.get() != nullptr)
		return;

	finishedKickTutorial = true;

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
			kickClimb(xPosition, yPosition, kickAction->getTargetPlayerX(), kickAction->getTargetPlayerY(), ticksTime);
			break;
		case KickActionType::Fall:
		case KickActionType::FallBig:
			kickFall(
				xPosition,
				yPosition,
				kickAction->getTargetPlayerX(),
				kickAction->getTargetPlayerY(),
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
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> kickAnimationComponents ({
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
	});
	if (hasBoot) {
		kickAnimationComponents.insert(
			kickAnimationComponents.end(),
			{
				newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerKickingAnimation),
				newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame),
				newEntityAnimationPlaySound(Audio::kickAirSound, 0),
				newEntityAnimationDelay(
					SpriteRegistry::playerKickingAnimation->getTotalTicksDuration()
						- SpriteRegistry::playerKickingAnimationTicksPerFrame),
			});
	} else {
		kickAnimationComponents.insert(
			kickAnimationComponents.end(),
			{
				newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerLegLiftAnimation),
				newEntityAnimationDelay(SpriteRegistry::playerLegLiftAnimation->getTotalTicksDuration()),
			});
	}
	beginEntityAnimation(&kickAnimationComponents, ticksTime);
}
void PlayerState::kickClimb(float currentX, float currentY, float targetX, float targetY, int ticksTime) {
	float xMoveDistance = targetX - currentX;
	float yMoveDistance = targetY - currentY;
	stringstream message;
	message << "  climb " << (int)currentX << " " << (int)currentY << "  " << (int)targetX << " " << (int)targetY;
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
		newEntityAnimationDelay(SpriteRegistry::playerFastKickingAnimationTicksPerFrame),
		//set the climb velocity
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
					0.0f, 2.0f * yMoveDistance / floatMoveDuration, -yMoveDistance / moveDurationSquared, 0.0f, 0.0f)),
		newEntityAnimationPlaySound(Audio::climbSound, 0),
		//then delay for the rest of the animation and stop the player
		newEntityAnimationDelay(moveDuration),
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
		newEntityAnimationGenerateHint(nullptr),
	});

	tryAddNoOpUndoState();
	stackNewClimbFallUndoState(undoState, currentX, currentY, z, hintState.get());
	beginEntityAnimation(&kickingAnimationComponents, ticksTime);
	float currentWorldGroundY = getWorldGroundY(ticksTime);
	//regardless of the movement direction, ground Y changes based on Y move distance
	float newWorldGroundY = currentWorldGroundY + yMoveDistance + MapState::tileSize;
	worldGroundY.set(
		newLinearInterpolatedValue({
			LinearInterpolatedValue::ValueAtTime(
				currentWorldGroundY, SpriteRegistry::playerFastKickingAnimationTicksPerFrame) COMMA
			LinearInterpolatedValue::ValueAtTime(newWorldGroundY, climbAnimationDuration) COMMA
		}));
	worldGroundYStartTicksTime = ticksTime;
	z += 2;
}
void PlayerState::kickFall(float currentX, float currentY, float targetX, float targetY, char fallHeight, int ticksTime) {
	float xMoveDistance = targetX - currentX;
	float yMoveDistance = targetY - currentY;
	stringstream message;
	message << "  fall " << (int)currentX << " " << (int)currentY << "  " << (int)targetX << " " << (int)targetY;
	Logger::gameplayLogger.logString(message.str());

	bool useFastKickingAnimation = fallHeight == z - 2;
	SpriteAnimation* fallAnimation =
		useFastKickingAnimation ? SpriteRegistry::playerFastKickingAnimation : SpriteRegistry::playerKickingAnimation;
	int fallAnimationFirstFrameTicks = useFastKickingAnimation
		? SpriteRegistry::playerFastKickingAnimationTicksPerFrame
		: SpriteRegistry::playerKickingAnimationTicksPerFrame;

	int fallAnimationDuration = fallAnimation->getTotalTicksDuration();
	int moveDuration = fallAnimationDuration - fallAnimationFirstFrameTicks;
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

	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> kickingAnimationComponents ({
		//start by stopping the player and delaying until the leg-sticking-out frame
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
		newEntityAnimationSetSpriteAnimation(fallAnimation),
		newEntityAnimationDelay(fallAnimationFirstFrameTicks),
		//set the fall velocity
		newEntityAnimationSetVelocity(xVelocity, yVelocity),
		newEntityAnimationPlaySound(Audio::jumpSound, 0),
		//then delay for the rest of the animation and stop the player
		newEntityAnimationDelay(moveDuration),
		newEntityAnimationPlaySound(Audio::landSound, 0),
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
		newEntityAnimationGenerateHint(nullptr),
	});

	tryAddNoOpUndoState();
	stackNewClimbFallUndoState(undoState, currentX, currentY, z, hintState.get());
	beginEntityAnimation(&kickingAnimationComponents, ticksTime);
	float currentWorldGroundY = getWorldGroundY(ticksTime);
	//regardless of the movement direction, ground Y changes based on Y move distance
	float newWorldGroundY = currentWorldGroundY + yMoveDistance + (fallHeight - z) / 2 * MapState::tileSize;
	worldGroundY.set(
		newLinearInterpolatedValue({
			LinearInterpolatedValue::ValueAtTime(currentWorldGroundY, fallAnimationFirstFrameTicks) COMMA
			LinearInterpolatedValue::ValueAtTime(newWorldGroundY, fallAnimationDuration) COMMA
		}));
	worldGroundYStartTicksTime = ticksTime;
	z = fallHeight;
}
void PlayerState::kickRail(short railId, float xPosition, float yPosition, int ticksTime) {
	MapState::logRailRide(railId, (int)xPosition, (int)yPosition);
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> ridingRailAnimationComponents;
	addRailRideComponents(
		railId,
		&ridingRailAnimationComponents,
		xPosition,
		yPosition,
		RideRailSpeed::Forward,
		nullptr,
		nullptr,
		nullptr,
		nullptr);
	tryAddNoOpUndoState();
	stackNewRideRailUndoState(undoState, railId, hintState.get());
	beginEntityAnimation(&ridingRailAnimationComponents, ticksTime);
	//the player is visually moved up to simulate a half-height raise on the rail, but the world ground y needs to stay the same
	worldGroundYOffset = MapState::halfTileSize + boundingBoxHeight / 2;
}
void PlayerState::addRailRideComponents(
	short railId,
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components,
	float xPosition,
	float yPosition,
	RideRailSpeed rideRailSpeed,
	HintState* useHint,
	float* outFinalXPosition,
	float* outFinalYPosition,
	SpriteDirection* outFinalSpriteDirection)
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
	int nextSegmentIndex, segmentIndexIncrement;
	if (startDist <= endDist) {
		nextSegmentIndex = 0;
		segmentIndexIncrement = 1;
	} else {
		nextSegmentIndex = endSegmentIndex;
		segmentIndexIncrement = -1;
	}

	int bootLiftDuration = rideRailSpeed == RideRailSpeed::Forward
		? SpriteRegistry::playerKickingAnimationTicksPerFrame
		: (int)(SpriteRegistry::playerKickingAnimationTicksPerFrame / 2.5f);
	int railToRailTicksDuration =
		rideRailSpeed == RideRailSpeed::Forward ? baseRailToRailTicksDuration : railToRailFastTicksDuration;
	float floatRailToRailTicksDuration = (float)railToRailTicksDuration;
	float railToRailTicksDurationSquared = floatRailToRailTicksDuration * floatRailToRailTicksDuration;
	float bootXShift = rideRailSpeed == RideRailSpeed::FastBackward ? -0.5f : 0.5f;

	components->push_back(newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerBootLiftAnimation));

	Rail::Segment* nextSegment = rail->getSegment(nextSegmentIndex);
	bool firstMove = true;
	float targetXPosition = xPosition;
	float targetYPosition = yPosition;
	SpriteDirection nextSpriteDirection = SpriteDirection::Down;
	int lastRideRailSound = -1;
	for (int segmentNum = 0; segmentNum < endSegmentIndex; segmentNum++) {
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
			targetXPosition -= bootXShift;
			nextSpriteDirection = SpriteDirection::Up;
		} else {
			targetYPosition += MapState::halfTileSize;
			//the boot is on the right foot, move right so the boot is centered over the rail when we go down
			targetXPosition += bootXShift;
			nextSpriteDirection = SpriteDirection::Down;
		}
		if (rideRailSpeed == RideRailSpeed::FastBackward)
			nextSpriteDirection = getOppositeDirection(nextSpriteDirection);

		//play a sound before moving if applicable
		if (rideRailSpeed == RideRailSpeed::Forward && segmentNum % Audio::rideRailOutSoundsCount == 1) {
			int segmentsRemaining = endSegmentIndex - segmentNum;
			if (segmentsRemaining > Audio::rideRailOutSoundsCount) {
				int rideRailSound = rand() % (Audio::rideRailSoundsCount - 1);
				if (rideRailSound >= lastRideRailSound)
					rideRailSound++;
				components->push_back(newEntityAnimationPlaySound(Audio::rideRailSounds[rideRailSound], 0));
				lastRideRailSound = rideRailSound;
			} else
				components->push_back(newEntityAnimationPlaySound(Audio::rideRailOutSounds[segmentsRemaining - 1], 0));
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
					newEntityAnimationDelay(bootLiftDuration),
					newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerRidingRailAnimation),
					newEntityAnimationSetDirection(nextSpriteDirection)
				});
			if (rideRailSpeed == RideRailSpeed::Forward)
				components->push_back(newEntityAnimationPlaySound(Audio::stepOnRailSound, 0));
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
		if (rideRailSpeed == RideRailSpeed::Forward)
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
		finalXPosition += bootXShift;
		finalYPosition += 3.0f;
	} else
		finalXPosition -= bootXShift;
	components->insert(
		components->end(),
		{
			newEntityAnimationSetVelocity(
				newCompositeQuarticValue(0.0f, (finalXPosition - targetXPosition) / bootLiftDuration, 0.0f, 0.0f, 0.0f),
				newCompositeQuarticValue(0.0f, (finalYPosition - targetYPosition) / bootLiftDuration, 0.0f, 0.0f, 0.0f)),
			newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerBootLiftAnimation),
			newEntityAnimationDelay(bootLiftDuration),
			newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
			newEntityAnimationGenerateHint(useHint),
		});
	if (rideRailSpeed == RideRailSpeed::Forward)
		components->push_back(newEntityAnimationPlaySound(Audio::stepOffRailSound, 0));

	if (outFinalXPosition != nullptr)
		*outFinalXPosition = finalXPosition;
	if (outFinalYPosition != nullptr)
		*outFinalYPosition = finalYPosition;
	if (outFinalSpriteDirection != nullptr)
		*outFinalSpriteDirection = nextSpriteDirection;
}
void PlayerState::kickSwitch(short switchId, int ticksTime) {
	MapState::logSwitchKick(switchId);
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> kickAnimationComponents;
	addKickSwitchComponents(switchId, &kickAnimationComponents, true, true, nullptr);
	beginEntityAnimation(&kickAnimationComponents, ticksTime);
	tryAddNoOpUndoState();
	stackNewKickSwitchUndoState(undoState, switchId, spriteDirection, hintState.get());
}
void PlayerState::addKickSwitchComponents(
	short switchId,
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components,
	bool moveRailsForward,
	bool allowRadioTowerAnimation,
	HintState* useHint)
{
	components->insert(
		components->end(),
		{
			newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
			newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerKickingAnimation),
			newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame),
			newEntityAnimationPlaySound(Audio::kickSound, 0),
			newEntityAnimationMapKickSwitch(switchId, moveRailsForward, allowRadioTowerAnimation),
			newEntityAnimationGenerateHint(useHint),
			newEntityAnimationDelay(
				SpriteRegistry::playerKickingAnimation->getTotalTicksDuration()
					- SpriteRegistry::playerKickingAnimationTicksPerFrame)
		});
}
void PlayerState::kickResetSwitch(short resetSwitchId, int ticksTime) {
	MapState::logResetSwitchKick(resetSwitchId);
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> kickAnimationComponents;
	addKickResetSwitchComponents(resetSwitchId, &kickAnimationComponents, nullptr, nullptr);
	beginEntityAnimation(&kickAnimationComponents, ticksTime);
	tryAddNoOpUndoState();
	mapState.get()->writeCurrentRailStates(
		resetSwitchId, stackNewKickResetSwitchUndoState(undoState, resetSwitchId, spriteDirection, hintState.get()));
}
void PlayerState::addKickResetSwitchComponents(
	short resetSwitchId,
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components,
	KickResetSwitchUndoState* kickResetSwitchUndoState,
	HintState* useHint)
{
	components->insert(
		components->end(),
		{
			newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
			newEntityAnimationSetSpriteAnimation(SpriteRegistry::playerKickingAnimation),
			newEntityAnimationDelay(SpriteRegistry::playerKickingAnimationTicksPerFrame),
			newEntityAnimationPlaySound(Audio::kickSound, 0),
			newEntityAnimationMapKickResetSwitch(resetSwitchId, kickResetSwitchUndoState),
			newEntityAnimationGenerateHint(useHint),
			newEntityAnimationDelay(
				SpriteRegistry::playerKickingAnimation->getTotalTicksDuration()
					- SpriteRegistry::playerKickingAnimationTicksPerFrame)
		});
}
void PlayerState::setUndoState(ReferenceCounterHolder<UndoState>& holder, UndoState* newUndoState) {
	if (newUndoState == nullptr) {
		//instead of just setting the state straight to nullptr, delete it state-by-state to avoid a stack overflow
		while (holder.get() != nullptr)
			holder.set(holder.get()->next.get());
	} else
		//we assume that we never replace an entire state stack with a new one, so this will not create a stack overflow
		holder.set(newUndoState);
}
void PlayerState::tryAddNoOpUndoState() {
	//include an extra no-op so that after undoing an action and moving, the player can undo straight to the action's location
	if (undoState.get() != nullptr && undoState.get()->getTypeIdentifier() != NoOpUndoState::classTypeIdentifier)
		stackNewNoOpUndoState(undoState);
}
void PlayerState::clearUndoRedoStates() {
	setUndoState(undoState, nullptr);
	setUndoState(redoState, nullptr);
}
void PlayerState::undo(int ticksTime) {
	if (hasAnimation() || undoState.get() == nullptr)
		return;
	Logger::gameplayLogger.log("  undo");
	availableKickAction.set(
		newKickAction(KickActionType::Undo, -1, -1, MapState::invalidHeight, MapState::absentRailSwitchId, -1));
	bool doneProcessing;
	do {
		doneProcessing = undoState.get()->handle(this, true, ticksTime);
		undoState.set(undoState.get()->next.get());
	} while (!doneProcessing && undoState.get() != nullptr);
}
void PlayerState::redo(int ticksTime) {
	if (hasAnimation() || redoState.get() == nullptr)
		return;
	finishedUndoRedoTutorial = true;
	Logger::gameplayLogger.log("  redo");
	availableKickAction.set(
		newKickAction(KickActionType::Redo, -1, -1, MapState::invalidHeight, MapState::absentRailSwitchId, -1));
	bool doneProcessing;
	do {
		doneProcessing = redoState.get()->handle(this, false, ticksTime);
		redoState.set(redoState.get()->next.get());
	} while (!doneProcessing && redoState.get() != nullptr);
}
void PlayerState::undoNoOp(bool isUndo) {
	//don't do anything, but maintain the state in the stacks
	stackNewNoOpUndoState(isUndo ? redoState : undoState);
}
bool PlayerState::undoMove(float fromX, float fromY, char fromHeight, HintState* fromHint, bool isUndo, int ticksTime) {
	float currentX = x.get()->getValue(0);
	float currentY = y.get()->getValue(0);
	SpriteAnimation* moveAnimation = SpriteRegistry::playerFastBootWalkingAnimation;
	if (fromHeight == MapState::invalidHeight) {
		stackNewMoveUndoState(isUndo ? redoState : undoState, currentX, currentY);
		if (currentX == fromX && currentY == fromY)
			return false;
	} else {
		stackNewClimbFallUndoState(isUndo ? redoState : undoState, currentX, currentY, z, hintState.get());
		moveAnimation = SpriteRegistry::playerBootLiftAnimation;
		z = fromHeight;
	}
	float xDist = fromX - currentX;
	float yDist = fromY - currentY;
	int totalTicksDuration =
		MathUtils::max(minUndoTicksDuration, (int)(sqrtf(xDist * xDist + yDist * yDist) / undoSpeedPerTick));
	SpriteDirection undoSpriteDirection = isUndo ? getSpriteDirection(-xDist, -yDist) : getSpriteDirection(xDist, yDist);
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> undoAnimationComponents ({
		newEntityAnimationSetGhostSprite(true, fromX, fromY, undoSpriteDirection),
		newEntityAnimationSetVelocity(
			newCompositeQuarticValue(0.0f, xDist / totalTicksDuration, 0.0f, 0.0f, 0.0f),
			newCompositeQuarticValue(0.0f, yDist / totalTicksDuration, 0.0f, 0.0f, 0.0f)),
		newEntityAnimationSetSpriteAnimation(moveAnimation),
		newEntityAnimationSetDirection(undoSpriteDirection),
		newEntityAnimationDelay(totalTicksDuration),
		newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f, SpriteDirection::Down),
		newEntityAnimationSetVelocity(newConstantValue(0.0f), newConstantValue(0.0f)),
	});
	if (fromHeight != MapState::invalidHeight)
		undoAnimationComponents.push_back(newEntityAnimationGenerateHint(fromHint));
	beginEntityAnimation(&undoAnimationComponents, ticksTime);
	return true;
}
void PlayerState::undoRideRail(short railId, HintState* fromHint, bool isUndo, int ticksTime) {
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> ridingRailAnimationComponents ({ nullptr });
	float finalX;
	float finalY;
	SpriteDirection undoGhostSpriteDirection;
	addRailRideComponents(
		railId,
		&ridingRailAnimationComponents,
		x.get()->getValue(0),
		y.get()->getValue(0),
		isUndo ? RideRailSpeed::FastBackward : RideRailSpeed::FastForward,
		fromHint,
		&finalX,
		&finalY,
		&undoGhostSpriteDirection);
	ridingRailAnimationComponents[0].set(newEntityAnimationSetGhostSprite(true, finalX, finalY, undoGhostSpriteDirection));
	ridingRailAnimationComponents.push_back(newEntityAnimationSetGhostSprite(false, 0.0f, 0.0f, SpriteDirection::Down));
	stackNewRideRailUndoState(isUndo ? redoState : undoState, railId, hintState.get());
	beginEntityAnimation(&ridingRailAnimationComponents, ticksTime);
	//the player is visually moved up to simulate a half-height raise on the rail, but the world ground y needs to stay the same
	worldGroundYOffset = MapState::halfTileSize + boundingBoxHeight / 2;
}
void PlayerState::undoKickSwitch(short switchId, SpriteDirection direction, HintState* fromHint, bool isUndo, int ticksTime) {
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> kickAnimationComponents ({
		newEntityAnimationSetDirection(direction),
	});
	addKickSwitchComponents(switchId, &kickAnimationComponents, !isUndo, false, fromHint);
	beginEntityAnimation(&kickAnimationComponents, ticksTime);
	stackNewKickSwitchUndoState(isUndo ? redoState : undoState, switchId, direction, hintState.get());
}
void PlayerState::undoKickResetSwitch(
	short resetSwitchId,
	SpriteDirection direction,
	KickResetSwitchUndoState* kickResetSwitchUndoState,
	HintState* fromHint,
	bool isUndo,
	int ticksTime)
{
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> kickAnimationComponents ({
		newEntityAnimationSetDirection(direction),
	});
	addKickResetSwitchComponents(resetSwitchId, &kickAnimationComponents, kickResetSwitchUndoState, fromHint);
	beginEntityAnimation(&kickAnimationComponents, ticksTime);
	mapState.get()->writeCurrentRailStates(
		resetSwitchId,
		stackNewKickResetSwitchUndoState(isUndo ? redoState : undoState, resetSwitchId, direction, hintState.get()));
}
void PlayerState::render(EntityState* camera, int ticksTime) {
	if (ghostSpriteX.get() != nullptr && ghostSpriteY.get() != nullptr) {
		float ghostRenderCenterX = getRenderCenterScreenXFromWorldX(
			ghostSpriteX.get()->getValue(ticksTime - ghostSpriteStartTicksTime), camera, ticksTime);
		float ghostRenderCenterY = getRenderCenterScreenYFromWorldY(
			ghostSpriteY.get()->getValue(ticksTime - ghostSpriteStartTicksTime), camera, ticksTime);

		glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
		SpriteRegistry::player->renderSpriteCenteredAtScreenPosition(
			hasBoot ? 4 : 0, (int)ghostSpriteDirection, ghostRenderCenterX, ghostRenderCenterY);
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
bool PlayerState::renderTutorials() {
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
	else if (!finishedUndoRedoTutorial && redoState.get() != nullptr)
		MapState::renderControlsTutorial(redoTutorialText, { Config::redoKeyBinding.value });
	else if (!finishedUndoRedoTutorial
			&& undoState.get() != nullptr
			&& (undoState.get()->getTypeIdentifier() == RideRailUndoState::classTypeIdentifier
				|| undoState.get()->getTypeIdentifier() == KickSwitchUndoState::classTypeIdentifier
				|| undoState.get()->getTypeIdentifier() == KickResetSwitchUndoState::classTypeIdentifier))
		MapState::renderControlsTutorial(undoRedoTutorialText, { Config::undoKeyBinding.value, Config::redoKeyBinding.value });
	else
		return false;
	return true;
}
void PlayerState::setHomeScreenState() {
	obtainBoot();
	x.set(newConstantValue(MapState::antennaCenterWorldX()));
	y.set(newConstantValue(MapState::radioTowerPlatformCenterWorldY() - boundingBoxCenterYOffset));
	//chosen as floor height 3 is in the middle of possible heights 0-7, and happens to be the height of platforms in level 1
	z = 6;
}
void PlayerState::saveState(ofstream& file) {
	file << playerXFilePrefix << lastControlledX << "\n";
	file << playerYFilePrefix << lastControlledY << "\n";
	file << playerDirectionFilePrefix << (int)spriteDirection << "\n";
	if (finishedMoveTutorial)
		file << finishedMoveTutorialFileValue << "\n";
	if (finishedKickTutorial)
		file << finishedKickTutorialFileValue << "\n";
	if (finishedUndoRedoTutorial)
		file << finishedUndoRedoTutorialFileValue << "\n";
	file << lastGoalFilePrefix << lastGoalX << " " << lastGoalY << "\n";
	if (noClip)
		file << noClipFileValue << "\n";
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
	else if (StringUtils::startsWith(line, finishedUndoRedoTutorialFileValue))
		finishedUndoRedoTutorial = true;
	else if (StringUtils::startsWith(line, lastGoalFilePrefix))
		StringUtils::parsePosition(line.c_str() + StringUtils::strlenConst(lastGoalFilePrefix), &lastGoalX, &lastGoalY);
	else if (StringUtils::startsWith(line, noClipFileValue))
		noClip = true;
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
	ghostSpriteX.set(nullptr);
	ghostSpriteY.set(nullptr);
	autoKickStartTicksTime = -1;
	canImmediatelyAutoKick = false;
	finishedMoveTutorial = false;
	finishedKickTutorial = false;
	finishedUndoRedoTutorial = false;
	lastGoalX = 0;
	lastGoalY = 0;
	clearUndoRedoStates();
	hintState.set(newHintState(&Hint::none));
}
void PlayerState::setHighestZ() {
	z = MapState::highestFloorHeight;
}
