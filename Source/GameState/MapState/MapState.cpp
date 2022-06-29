#include "MapState.h"
#include "Editor/Editor.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/EntityState.h"
#include "GameState/KickAction.h"
#include "GameState/MapState/Rail.h"
#include "GameState/MapState/ResetSwitch.h"
#include "GameState/MapState/Switch.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Sprites/Text.h"
#include "Util/Config.h"
#include "Util/FileUtils.h"
#include "Util/Logger.h"
#include "Util/StringUtils.h"

#define newRadioWavesState() produceWithoutArgs(MapState::RadioWavesState)
#define spriteAnimationAfterDelay(animation, delay) \
	newEntityAnimationDelay(delay), \
	newEntityAnimationSetSpriteAnimation(animation)

//////////////////////////////// MapState::RadioWavesState ////////////////////////////////
MapState::RadioWavesState::RadioWavesState(objCounterParameters())
: EntityState(objCounterArguments())
, spriteAnimation(nullptr)
, spriteAnimationStartTicksTime(0)
{
	x.set(newConstantValue(antennaCenterWorldX()));
	y.set(newConstantValue(antennaCenterWorldY()));
}
MapState::RadioWavesState::~RadioWavesState() {
	//don't delete the sprite animation, SpriteRegistry owns it
}
//initialize and return a RadioWavesState
MapState::RadioWavesState* MapState::RadioWavesState::produce(objCounterParameters()) {
	initializeWithNewFromPool(r, MapState::RadioWavesState)
	return r;
}
//copy the other state
void MapState::RadioWavesState::copyRadioWavesState(RadioWavesState* other) {
	copyEntityState(other);
	setSpriteAnimation(other->spriteAnimation, other->spriteAnimationStartTicksTime);
}
pooledReferenceCounterDefineRelease(MapState::RadioWavesState)
//set the animation to the given animation at the given time
void MapState::RadioWavesState::setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime) {
	spriteAnimation = pSpriteAnimation;
	spriteAnimationStartTicksTime = pSpriteAnimationStartTicksTime;
}
//update this radio waves state
void MapState::RadioWavesState::updateWithPreviousRadioWavesState(RadioWavesState* prev, int ticksTime) {
	copyRadioWavesState(prev);
	//if we have an entity animation, update with that instead
	if (prev->entityAnimation.get() != nullptr) {
		if (entityAnimation.get()->update(this, ticksTime))
			return;
		entityAnimation.set(nullptr);
	}
}
//render the radio waves if we've got an animation
void MapState::RadioWavesState::render(EntityState* camera, int ticksTime) {
	if (spriteAnimation == nullptr)
		return;

	float renderCenterX = getRenderCenterScreenX(camera,  ticksTime);
	float renderCenterY = getRenderCenterScreenY(camera,  ticksTime);
	spriteAnimation->renderUsingCenter(renderCenterX, renderCenterY, ticksTime - spriteAnimationStartTicksTime, 0, 0);
}

//////////////////////////////// MapState ////////////////////////////////
const char* MapState::floorFileName = "floor.png";
const float MapState::smallDistance = 1.0f / 256.0f;
const float MapState::introAnimationCameraCenterX = (float)(MapState::tileSize * introAnimationBootTileX) + 4.5f;
const float MapState::introAnimationCameraCenterY = (float)(MapState::tileSize * introAnimationBootTileY) - 4.5f;
const string MapState::railOffsetFilePrefix = "rail ";
const string MapState::lastActivatedSwitchColorFilePrefix = "lastActivatedSwitchColor ";
const string MapState::finishedConnectionsTutorialFilePrefix = "finishedConnectionsTutorial ";
char* MapState::tiles = nullptr;
char* MapState::heights = nullptr;
short* MapState::railSwitchIds = nullptr;
vector<Rail*> MapState::rails;
vector<Switch*> MapState::switches;
vector<ResetSwitch*> MapState::resetSwitches;
int MapState::width = 1;
int MapState::height = 1;
int MapState::editorNonTilesHidingState = 1;
MapState::MapState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, railStates()
, railStatesByHeight()
, railsBelowPlayerZ(0)
, switchStates()
, resetSwitchStates()
, lastActivatedSwitchColor(-1)
, finishedConnectionsTutorial(false)
, switchesAnimationFadeInStartTicksTime(0)
, shouldPlayRadioTowerAnimation(false)
, radioWavesState(nullptr) {
	for (int i = 0; i < (int)rails.size(); i++)
		railStates.push_back(newRailState(rails[i], i));
	for (Switch* switch0 : switches)
		switchStates.push_back(newSwitchState(switch0));
	for (ResetSwitch* resetSwitch : resetSwitches)
		resetSwitchStates.push_back(newResetSwitchState(resetSwitch));

	//add all the rail states to their appropriate switch states
	vector<SwitchState*> switchStatesByGroup;
	for (SwitchState* switchState : switchStates) {
		int group = (int)switchState->getSwitch()->getGroup();
		while ((int)switchStatesByGroup.size() <= group)
			switchStatesByGroup.push_back(nullptr);
		switchStatesByGroup[group] = switchState;
	}
	for (RailState* railState : railStates) {
		for (char group : railState->getRail()->getGroups())
			switchStatesByGroup[group]->addConnectedRailState(railState);
	}
}
MapState::~MapState() {
	for (RailState* railState : railStates)
		delete railState;
	for (SwitchState* switchState : switchStates)
		delete switchState;
	for (ResetSwitchState* resetSwitchState : resetSwitchStates)
		delete resetSwitchState;
}
//initialize and return a MapState
MapState* MapState::produce(objCounterParameters()) {
	initializeWithNewFromPool(m, MapState)
	m->radioWavesState.set(newRadioWavesState());
	return m;
}
pooledReferenceCounterDefineRelease(MapState)
//release the radio waves state before this is returned to the pool
void MapState::prepareReturnToPool() {
	radioWavesState.set(nullptr);
}
//load the map and extract all the map data from it
void MapState::buildMap() {
	SDL_Surface* floor = FileUtils::loadImage(floorFileName);
	width = floor->w;
	height = floor->h;
	int totalTiles = width * height;
	tiles = new char[totalTiles];
	heights = new char[totalTiles];
	railSwitchIds = new short[totalTiles];

	//these need to be initialized so that we can update them without erasing already-updated ids
	for (int i = 0; i < totalTiles; i++)
		railSwitchIds[i] = 0;

	int redShift = (int)floor->format->Rshift;
	int redMask = (int)floor->format->Rmask;
	int greenShift = (int)floor->format->Gshift;
	int greenMask = (int)floor->format->Gmask;
	int blueShift = (int)floor->format->Bshift;
	int blueMask = (int)floor->format->Bmask;
	int* pixels = static_cast<int*>(floor->pixels);

	//set all tiles and heights before loading rails/switches
	for (int i = 0; i < totalTiles; i++) {
		tiles[i] = (char)(((pixels[i] & greenMask) >> greenShift) / tileDivisor);
		heights[i] = (char)(((pixels[i] & blueMask) >> blueShift) / heightDivisor);
		railSwitchIds[i] = absentRailSwitchId;
	}

	for (int i = 0; i < totalTiles; i++) {
		char railSwitchValue = (char)((pixels[i] & redMask) >> redShift);

		//rail/switch data occupies 2+ red bytes, each byte has bit 0 set (non-switch/rail bytes have bit 0 unset)
		//head bytes that start a rail/switch have bit 1 set, we only build rails/switches when we get to one of these
		if ((railSwitchValue & floorIsRailSwitchAndHeadBitmask) != floorRailSwitchAndHeadValue)
			continue;
		int headX = i % width;
		int headY = i / width;

		//head byte 1:
		//	bit 1: 1 (indicates head byte)
		//	bit 2 indicates rail (0) vs switch (1)
		//	bits 3-4: color (R/B/G/W)
		//	switch bit 5 indicates switch (0) vs reset switch (1)
		//	rail bits 5-7: initial tile offset
		//rail secondary byte 2:
		//	bit 1: 0 (indicates tail byte)
		//	bit 2: direction (0 for down, 1 for up)
		//switch tail byte 2 / rail tail byte 3+:
		//	bit 1: 0 (indicates tail byte)
		//	bits 2-7: group number
		char color = (railSwitchValue >> floorRailSwitchColorDataShift) & floorRailSwitchColorPostShiftBitmask;
		//this is a reset switch
		if (HAS_BITMASK(railSwitchValue, floorIsSwitchAndResetSwitchBitmask)) {
			short newResetSwitchId = (short)resetSwitches.size() | resetSwitchIdValue;
			ResetSwitch* resetSwitch = newResetSwitch(headX, headY);
			resetSwitches.push_back(resetSwitch);
			railSwitchIds[i - width] = newResetSwitchId;
			railSwitchIds[i] = newResetSwitchId;
			Holder_RessetSwitchSegmentVector leftSegmentsHolder (&resetSwitch->leftSegments);
			Holder_RessetSwitchSegmentVector bottomSegmentsHolder (&resetSwitch->bottomSegments);
			Holder_RessetSwitchSegmentVector rightSegmentsHolder (&resetSwitch->rightSegments);
			addResetSwitchSegments(pixels, redShift, headX, headY, i - 1, newResetSwitchId, &leftSegmentsHolder);
			addResetSwitchSegments(pixels, redShift, headX, headY, i + width, newResetSwitchId, &bottomSegmentsHolder);
			addResetSwitchSegments(pixels, redShift, headX, headY, i + 1, newResetSwitchId, &rightSegmentsHolder);
		//this is a regular switch
		} else if (HAS_BITMASK(railSwitchValue, floorIsSwitchBitmask)) {
			char switchByte2 = (char)((pixels[i + 1] & redMask) >> redShift);
			char group = (switchByte2 >> floorRailSwitchGroupDataShift) & floorRailSwitchGroupPostShiftBitmask;
			short newSwitchId = (short)switches.size() | switchIdValue;
			//add the switch and set all the ids
			switches.push_back(newSwitch(headX, headY, color, group));
			for (int yOffset = 0; yOffset <= 1; yOffset++) {
				for (int xOffset = 0; xOffset <= 1; xOffset++) {
					railSwitchIds[i + xOffset + yOffset * width] = newSwitchId;
				}
			}
		//this is a rail
		} else {
			short newRailId = (short)rails.size() | railIdValue;
			railSwitchIds[i] = newRailId;
			//rails can extend in any direction after this head byte tile
			vector<int> railIndices = parseRail(pixels, redShift, i, newRailId);
			int railByte2Index = railIndices[0];
			railIndices.erase(railIndices.begin());
			//collect secondary data and build the rail
			char initialTileOffset =
				(railSwitchValue >> floorRailInitialTileOffsetDataShift) & floorRailInitialTileOffsetPostShiftBitmask;
			char railByte2PostShift = (char)((pixels[railByte2Index] & redMask) >> (redShift + floorRailByte2DataShift));
			char movementDirection = (railByte2PostShift & floorRailMovementDirectionPostShiftBitmask) * 2 - 1;
			Rail* rail = newRail(headX, headY, heights[i], color, initialTileOffset, movementDirection);
			rails.push_back(rail);
			rail->addSegment(railByte2Index % width, railByte2Index / width);
			//add all the groups
			int floorRailGroupShiftedShift = redShift + floorRailSwitchGroupDataShift;
			for (int railIndex : railIndices) {
				rail->addSegment(railIndex % width, railIndex / width);
				rail->addGroup((char)(pixels[railIndex] >> floorRailGroupShiftedShift) & floorRailSwitchGroupPostShiftBitmask);
			}
		}
	}

	SDL_FreeSurface(floor);
}
//go along the path and add rail segment indices to a list, and set the given id over those tiles
//the given rail index is assumed to already have its tile marked
//returns the indices of the parsed segments
vector<int> MapState::parseRail(int* pixels, int redShift, int segmentIndex, int railSwitchId) {
	//cache shift values so that we can iterate the floor data quicker
	int floorIsRailSwitchAndHeadShiftedBitmask = floorIsRailSwitchAndHeadBitmask << redShift;
	int floorRailSwitchTailShiftedValue = floorIsRailSwitchBitmask << redShift;
	vector<int> segmentIndices;
	while (true) {
		if ((pixels[segmentIndex + 1] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
				&& railSwitchIds[segmentIndex + 1] == 0)
			segmentIndex++;
		else if ((pixels[segmentIndex - 1] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
				&& railSwitchIds[segmentIndex - 1] == 0)
			segmentIndex--;
		else if ((pixels[segmentIndex + width] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
				&& railSwitchIds[segmentIndex + width] == 0)
			segmentIndex += width;
		else if ((pixels[segmentIndex - width] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
				&& railSwitchIds[segmentIndex - width] == 0)
			segmentIndex -= width;
		else
			return segmentIndices;
		segmentIndices.push_back(segmentIndex);
		railSwitchIds[segmentIndex] = railSwitchId;
	}
}
//read rail groups starting from the given tile, if there is a rail there
void MapState::addResetSwitchSegments(
	int* pixels,
	int redShift,
	int resetSwitchX,
	int resetSwitchBottomY,
	int firstSegmentIndex,
	int resetSwitchId,
	Holder_RessetSwitchSegmentVector* segmentsHolder)
{
	vector<ResetSwitch::Segment>* segments = segmentsHolder->val;
	int resetSwitchValue = pixels[firstSegmentIndex] >> redShift;
	//no rails here
	if ((resetSwitchValue & floorIsRailSwitchAndHeadBitmask) != floorRailSwitchTailValue)
		return;

	//in each direction (down, left, or right), the first byte indicates colors, 1 or 2:
	//	bits 2-3 indicate the first color and bits 4-5 indicate the second color (if there is one)
	//	rail groups belong to the first color until group 0 is read, at which point groups are now the second color
	char color1 = (char)((resetSwitchValue >> floorRailSwitchGroupDataShift) & floorRailSwitchColorPostShiftBitmask);
	char color2 = (char)((resetSwitchValue >> (floorRailSwitchGroupDataShift + 2)) & floorRailSwitchColorPostShiftBitmask);
	railSwitchIds[firstSegmentIndex] = resetSwitchId;
	int lastSegmentX = resetSwitchX;
	int lastSegmentY = resetSwitchBottomY;
	int segmentX = firstSegmentIndex % width;
	int segmentY = firstSegmentIndex / width;
	int firstSegmentSpriteHorizontalIndex =
		Rail::extentSegmentSpriteHorizontalIndex(lastSegmentX, lastSegmentY, segmentX, segmentY);
	segments->push_back(ResetSwitch::Segment(segmentX, segmentY, color1, 0, firstSegmentSpriteHorizontalIndex));

	char segmentColor = color1;
	vector<int> railIndices = parseRail(pixels, redShift, firstSegmentIndex, resetSwitchId);
	for (int segmentIndex : railIndices) {
		int secondLastSegmentX = lastSegmentX;
		int secondLastSegmentY = lastSegmentY;
		lastSegmentX = segmentX;
		lastSegmentY = segmentY;
		segmentX = segmentIndex % width;
		segmentY = segmentIndex / width;
		segments->back().spriteHorizontalIndex = Rail::middleSegmentSpriteHorizontalIndex(
			secondLastSegmentX, secondLastSegmentY, lastSegmentX, lastSegmentY, segmentX, segmentY);

		char group = (char)((pixels[segmentIndex] >> floorRailSwitchGroupDataShift) & floorRailSwitchGroupPostShiftBitmask);
		if (group == 0)
			segmentColor = color2;
		int segmentSpriteHorizontalIndex =
			Rail::extentSegmentSpriteHorizontalIndex(lastSegmentX, lastSegmentY, segmentX, segmentY);
		segments->push_back(ResetSwitch::Segment(segmentX, segmentY, segmentColor, group, segmentSpriteHorizontalIndex));
	}
}
//delete the resources used to handle the map
void MapState::deleteMap() {
	delete[] tiles;
	delete[] heights;
	delete[] railSwitchIds;
	for (Rail* rail : rails)
		delete rail;
	rails.clear();
	for (Switch* switch0 : switches)
		delete switch0;
	switches.clear();
	for (ResetSwitch* resetSwitch : resetSwitches)
		delete resetSwitch;
	resetSwitches.clear();
}
//get the world position of the left edge of the screen using the camera as the center of the screen
int MapState::getScreenLeftWorldX(EntityState* camera, int ticksTime) {
	//we convert the camera center to int first because with a position with 0.5 offsets, we render all pixels aligned (because
	//	the screen/player width is odd); once we get to a .0 position, then we render one pixel over
	return (int)(camera->getRenderCenterWorldX(ticksTime)) - Config::gameScreenWidth / 2;
}
//get the world position of the top edge of the screen using the camera as the center of the screen
int MapState::getScreenTopWorldY(EntityState* camera, int ticksTime) {
	//(see getScreenLeftWorldX)
	return (int)(camera->getRenderCenterWorldY(ticksTime)) - Config::gameScreenHeight / 2;
}
#ifdef DEBUG
	//find the switch for the given index and write its top left corner
	//does not write map coordinates if it doesn't find any
	void MapState::getSwitchMapTopLeft(short switchIndex, int* outMapLeftX, int* outMapTopY) {
		short targetSwitchId = switchIndex | switchIdValue;
		for (int mapY = 0; mapY < height; mapY++) {
			for (int mapX = 0; mapX < width; mapX++) {
				if (getRailSwitchId(mapX, mapY) == targetSwitchId) {
					*outMapLeftX = mapX;
					*outMapTopY = mapY;
					return;
				}
			}
		}
	}
	//find the reset switch for the given index and write its center bottom coordinate
	//does not write map coordinates if it doesn't find any
	void MapState::getResetSwitchMapTopCenter(short resetSwitchIndex, int* outMapCenterX, int* outMapTopY) {
		short targetResetSwitchId = resetSwitchIndex | resetSwitchIdValue;
		for (int mapY = 0; mapY < height; mapY++) {
			for (int mapX = 0; mapX < width; mapX++) {
				if (getRailSwitchId(mapX, mapY) == targetResetSwitchId) {
					*outMapCenterX = mapX;
					*outMapTopY = mapY;
					return;
				}
			}
		}
	}
#endif
//get the center x of the radio tower antenna
float MapState::antennaCenterWorldX() {
	return (float)(radioTowerLeftXOffset + SpriteRegistry::radioTower->getSpriteWidth() / 2);
}
//get the center y of the radio tower antenna
float MapState::antennaCenterWorldY() {
	return (float)(radioTowerTopYOffset + 2);
}
//check that the tile has the main section of the reset switch, not just one of the rail segments
bool MapState::tileHasResetSwitchBody(int x, int y) {
	if (!tileHasResetSwitch(x, y))
		return false;
	ResetSwitch* resetSwitch = resetSwitches[getRailSwitchId(x, y) & railSwitchIndexBitmask];
	return (x == resetSwitch->getCenterX() && (y == resetSwitch->getBottomY() || y == resetSwitch->getBottomY() - 1));
}
//a switch can only be kicked if it's group 0 or if its color is activate
KickActionType MapState::getSwitchKickActionType(short switchId) {
	Switch* switch0 = switches[switchId & railSwitchIndexBitmask];
	bool group0 = switch0->getGroup() == 0;
	//we've already triggered this group 0 switch, or can't yet kick this puzzle switch
	if (group0 == (lastActivatedSwitchColor >= switch0->getColor()))
		return KickActionType::None;
	if (group0) {
		switch (switch0->getColor()) {
			case squareColor: return KickActionType::Square;
			case triangleColor: return KickActionType::Triangle;
			case sawColor: return KickActionType::Saw;
			case sineColor: return KickActionType::Sine;
			default: return KickActionType::None;
		}
	} else
		return KickActionType::Switch;
}
//check the height of all the tiles in the row, and return it if they're all the same or -1 if they differ
char MapState::horizontalTilesHeight(int lowMapX, int highMapX, int mapY) {
	char foundHeight = getHeight(lowMapX, mapY);
	for (int mapX = lowMapX + 1; mapX <= highMapX; mapX++) {
		if (getHeight(mapX, mapY) != foundHeight)
			return invalidHeight;
	}
	return foundHeight;
}
//change one of the tiles to be the boot tile
void MapState::setIntroAnimationBootTile(bool showBootTile) {
	//if we're not showing the boot tile, just show a default tile instead of showing the tile from the floor file
	tiles[introAnimationBootTileY * width + introAnimationBootTileX] = showBootTile ? tileBoot : tileFloorFirst;
}
//update the rails and switches
void MapState::updateWithPreviousMapState(MapState* prev, int ticksTime) {
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	lastActivatedSwitchColor = prev->lastActivatedSwitchColor;
	finishedConnectionsTutorial =
		prev->finishedConnectionsTutorial || (keyboardState[Config::keyBindings.showConnectionsKey] != 0);
	shouldPlayRadioTowerAnimation = false;
	switchesAnimationFadeInStartTicksTime = prev->switchesAnimationFadeInStartTicksTime;

	if (Editor::isActive) {
		//since the editor can add switches and rails, make sure we update our list to track them
		//we won't connect rail states to switch states since we can't kick switches in the editor
		while (railStates.size() < rails.size())
			railStates.push_back(newRailState(rails[railStates.size()], railStates.size()));
		while (switchStates.size() < switches.size())
			switchStates.push_back(newSwitchState(switches[switchStates.size()]));
		while (resetSwitchStates.size() < resetSwitches.size())
			resetSwitchStates.push_back(newResetSwitchState(resetSwitches[resetSwitchStates.size()]));
	}
	for (int i = 0; i < (int)prev->switchStates.size(); i++)
		switchStates[i]->updateWithPreviousSwitchState(prev->switchStates[i]);
	for (int i = 0; i < (int)prev->resetSwitchStates.size(); i++)
		resetSwitchStates[i]->updateWithPreviousResetSwitchState(prev->resetSwitchStates[i]);
	railStatesByHeight.clear();
	for (RailState* otherRailState : prev->railStatesByHeight) {
		RailState* railState = railStates[otherRailState->getRailIndex()];
		railState->updateWithPreviousRailState(otherRailState, ticksTime);
		insertRailByHeight(railState);
	}
	if (Editor::isActive) {
		//if we added rail states this update, add them to the height list too
		//we know they're the last set of rails in the list
		//if we added them in a previous state, they'll already be sorted
		while (railStatesByHeight.size() < railStates.size())
			railStatesByHeight.push_back(railStates[railStatesByHeight.size()]);
	}

	radioWavesState.get()->updateWithPreviousRadioWavesState(prev->radioWavesState.get(), ticksTime);
}
//via insertion sort, add a rail state to the list of above- or below-player rail states
//compare starting at the end since we expect the rails to mostly be already sorted
void MapState::insertRailByHeight(RailState* railState) {
	float effectiveHeight = railState->getEffectiveHeight();
	//no insertion needed if there are no higher rails
	if (railStatesByHeight.size() == 0 || railStatesByHeight.back()->getEffectiveHeight() <= effectiveHeight) {
		railStatesByHeight.push_back(railState);
		return;
	}
	int insertionIndex = (int)railStatesByHeight.size() - 1;
	railStatesByHeight.push_back(railStatesByHeight.back());
	for (; insertionIndex > 0; insertionIndex--) {
		RailState* other = railStatesByHeight[insertionIndex - 1];
		if (other->getEffectiveHeight() <= effectiveHeight)
			break;
		railStatesByHeight[insertionIndex] = other;
	}
	railStatesByHeight[insertionIndex] = railState;
}
//flip a switch
void MapState::flipSwitch(short switchId, bool allowRadioTowerAnimation, int ticksTime) {
	SwitchState* switchState = switchStates[switchId & railSwitchIndexBitmask];
	Switch* switch0 = switchState->getSwitch();
	//this is a turn-on-other-switches switch, flip it if we haven't done so already
	if (switch0->getGroup() == 0) {
		if (lastActivatedSwitchColor < switch0->getColor()) {
			lastActivatedSwitchColor = switch0->getColor();
			if (allowRadioTowerAnimation)
				shouldPlayRadioTowerAnimation = true;
		}
	//this is just a regular switch and we've turned on the parent switch, flip it
	} else if (lastActivatedSwitchColor >= switch0->getColor())
		switchState->flip(ticksTime);
}
//flip a reset switch
void MapState::flipResetSwitch(short resetSwitchId, int ticksTime) {
	ResetSwitchState* resetSwitchState = resetSwitchStates[resetSwitchId & railSwitchIndexBitmask];
	ResetSwitch* resetSwitch = resetSwitchState->getResetSwitch();
	Holder_RessetSwitchSegmentVector leftSegmentsHolder (&resetSwitch->leftSegments);
	Holder_RessetSwitchSegmentVector bottomSegmentsHolder (&resetSwitch->bottomSegments);
	Holder_RessetSwitchSegmentVector rightSegmentsHolder (&resetSwitch->rightSegments);
	resetMatchingRails(&leftSegmentsHolder);
	resetMatchingRails(&bottomSegmentsHolder);
	resetMatchingRails(&rightSegmentsHolder);
	resetSwitchState->flip(ticksTime);
}
//begin a radio waves animation
void MapState::startRadioWavesAnimation(int initialTicksDelay, int ticksTime) {
	vector<ReferenceCounterHolder<EntityAnimation::Component>> radioWavesAnimationComponents ({
		spriteAnimationAfterDelay(SpriteRegistry::radioWavesAnimation, initialTicksDelay),
		spriteAnimationAfterDelay(nullptr, SpriteRegistry::radioWavesAnimation->getTotalTicksDuration()),
		spriteAnimationAfterDelay(SpriteRegistry::radioWavesAnimation, RadioWavesState::interRadioWavesAnimationTicks),
		spriteAnimationAfterDelay(nullptr, SpriteRegistry::radioWavesAnimation->getTotalTicksDuration())
	});
	Holder_EntityAnimationComponentVector radioWavesAnimationComponentsHolder (&radioWavesAnimationComponents);
	radioWavesState.get()->beginEntityAnimation(&radioWavesAnimationComponentsHolder, ticksTime);
}
//activate the next switch color and set the start of the animation
void MapState::startSwitchesFadeInAnimation(int ticksTime) {
	shouldPlayRadioTowerAnimation = false;
	switchesAnimationFadeInStartTicksTime = ticksTime;
}
//reset any rails that match the segments
void MapState::resetMatchingRails(Holder_RessetSwitchSegmentVector* segmentsHolder) {
	vector<ResetSwitch::Segment>* segments = segmentsHolder->val;
	for (ResetSwitch::Segment& segment : *segments) {
		if (segment.group == 0)
			continue;
		for (RailState* railState : railStates) {
			Rail* rail = railState->getRail();
			if (rail->getColor() != segment.color)
				continue;
			for (char group : rail->getGroups()) {
				if (group == segment.group) {
					railState->moveToDefaultTileOffset();
					break;
				}
			}
		}
	}
}
//draw the map
void MapState::render(EntityState* camera, char playerZ, bool showConnections, int ticksTime) {
	glDisable(GL_BLEND);
	//render the map
	//these values are just right so that every tile rendered is at least partially in the window and no tiles are left out
	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	int tileMinX = MathUtils::max(screenLeftWorldX / tileSize, 0);
	int tileMinY = MathUtils::max(screenTopWorldY / tileSize, 0);
	int tileMaxX = MathUtils::min((Config::gameScreenWidth + screenLeftWorldX - 1) / tileSize + 1, width);
	int tileMaxY = MathUtils::min((Config::gameScreenHeight + screenTopWorldY - 1) / tileSize + 1, height);
	char editorSelectedHeight = Editor::isActive ? Editor::getSelectedHeight() : -1;
	for (int y = tileMinY; y < tileMaxY; y++) {
		for (int x = tileMinX; x < tileMaxX; x++) {
			//consider any tile at the max height to be filler
			int mapIndex = y * width + x;
			char mapHeight = heights[mapIndex];
			if (mapHeight == emptySpaceHeight)
				continue;

			GLint leftX = (GLint)(x * tileSize - screenLeftWorldX);
			GLint topY = (GLint)(y * tileSize - screenTopWorldY);
			SpriteRegistry::tiles->renderSpriteAtScreenPosition((int)(tiles[mapIndex]), 0, leftX, topY);
			if (Editor::isActive && editorSelectedHeight != -1 && editorSelectedHeight != mapHeight)
				SpriteSheet::renderFilledRectangle(
					0.0f, 0.0f, 0.0f, 0.5f, leftX, topY, leftX + (GLint)tileSize, topY + (GLint)tileSize);
		}
	}

	if (Editor::isActive) {
		if (showConnections == (editorNonTilesHidingState % 2 == 1))
			editorNonTilesHidingState = (editorNonTilesHidingState + 1) % 6;
		//by default, show the connections
		if (editorNonTilesHidingState / 2 == 0)
			showConnections = true;
		//on the first press of the show-connections button, hide the connections
		else if (editorNonTilesHidingState / 2 == 1)
			showConnections = false;
		//on the 2nd press of the show-connections button, hide everything other than the tiles
		else
			return;
	}

	//draw rail shadows, rails (that are below the player), and switches
	for (RailState* railState : railStates)
		railState->renderShadow(screenLeftWorldX, screenTopWorldY);
	railsBelowPlayerZ = 0;
	for (RailState* railState : railStatesByHeight) {
		if (railState->isAbovePlayerZ(playerZ))
			break;
		railsBelowPlayerZ++;
		railState->render(screenLeftWorldX, screenTopWorldY);
	}
	for (SwitchState* switchState : switchStates)
		switchState->render(
			screenLeftWorldX,
			screenTopWorldY,
			lastActivatedSwitchColor,
			ticksTime - switchesAnimationFadeInStartTicksTime,
			showConnections,
			ticksTime);
	for (ResetSwitchState* resetSwitchState : resetSwitchStates)
		resetSwitchState->render(screenLeftWorldX, screenTopWorldY, showConnections, ticksTime);

	//draw the radio tower after drawing everything else
	glEnable(GL_BLEND);
	if (Editor::isActive)
		glColor4f(1.0f, 1.0f, 1.0f, 2.0f / 3.0f);
	SpriteRegistry::radioTower->renderSpriteAtScreenPosition(
		0, 0, (GLint)(radioTowerLeftXOffset - screenLeftWorldX), (GLint)(radioTowerTopYOffset - screenTopWorldY));
	if (Editor::isActive)
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glColor4f(
		(lastActivatedSwitchColor == MapState::squareColor || lastActivatedSwitchColor == MapState::sineColor) ? 1.0f : 0.0f,
		(lastActivatedSwitchColor == MapState::sawColor || lastActivatedSwitchColor == MapState::sineColor) ? 1.0f : 0.0f,
		(lastActivatedSwitchColor == MapState::triangleColor || lastActivatedSwitchColor == MapState::sineColor) ? 1.0f : 0.0f,
		1.0f);
	radioWavesState.get()->render(camera, ticksTime);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
//draw anything (rails, groups) that render above the player
//assumes render() has already been called to set the rails above the player
void MapState::renderAbovePlayer(EntityState* camera, bool showConnections, int ticksTime) {
	if (Editor::isActive) {
		//by default, show the connections
		if (editorNonTilesHidingState / 2 == 0)
			showConnections = true;
		//on the first press of the show-connections button, hide the connections
		else if (editorNonTilesHidingState / 2 == 1)
			showConnections = false;
		//on the 2nd press of the show-connections button, hide everything other than the tiles
		else
			return;
	}

	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	for (int i = railsBelowPlayerZ; i < (int)railStatesByHeight.size(); i++)
		railStatesByHeight[i]->render(screenLeftWorldX, screenTopWorldY);
	if (showConnections) {
		//show groups above the player for all rails
		for (RailState* railState : railStates)
			railState->renderGroups(screenLeftWorldX, screenTopWorldY);
		if (!finishedConnectionsTutorial && SDL_GetKeyboardState(nullptr)[Config::keyBindings.showConnectionsKey] == 0) {
			const char* showConnectionsText = "show connections: ";
			const float leftX = 10.0f;
			const float baselineY = 20.0f;
			Text::Metrics showConnectionsMetrics = Text::getMetrics(showConnectionsText, 1.0f);
			Text::render(showConnectionsText, leftX, baselineY, 1.0f);
			Text::renderWithKeyBackground(
				Config::KeyBindings::getKeyName(Config::keyBindings.showConnectionsKey),
				leftX + showConnectionsMetrics.charactersWidth,
				baselineY,
				1.0f);
		}
	}
}
//render the groups for rails that are not in their default position that have a group that this reset switch also has
//return whether any groups were drawn
bool MapState::renderGroupsForRailsToReset(EntityState* camera, short resetSwitchId, int ticksTime) {
	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	ResetSwitch* resetSwitch = resetSwitches[resetSwitchId & railSwitchIndexBitmask];
	bool hasRailsToReset = false;
	for (RailState* railState : railStates) {
		Rail* rail = railState->getRail();
		if (railState->getTargetTileOffset() == (float)rail->getInitialTileOffset())
			continue;
		char railColor = rail->getColor();
		for (char railGroup : rail->getGroups()) {
			//the reset switch has a group for this rail, render groups for it
			if (resetSwitch->hasGroupForColor(railGroup, railColor)) {
				railState->renderGroups(screenLeftWorldX, screenTopWorldY);
				hasRailsToReset = true;
				break;
			}
		}
	}
	if (hasRailsToReset)
		resetSwitchStates[resetSwitchId & railSwitchIndexBitmask]->render(screenLeftWorldX, screenTopWorldY, true, ticksTime);
	return hasRailsToReset;
}
//draw a graphic to represent this rail/switch group
void MapState::renderGroupRect(char group, GLint leftX, GLint topY, GLint rightX, GLint bottomY) {
	GLint midX = (leftX + rightX) / 2;
	//bits 0-2
	SpriteSheet::renderFilledRectangle(
		(float)(group & 1), (float)((group & 2) >> 1), (float)((group & 4) >> 2), 1.0f, leftX, topY, midX, bottomY);
	//bits 3-5
	SpriteSheet::renderFilledRectangle(
		(float)((group & 8) >> 3), (float)((group & 16) >> 4), (float)((group & 32) >> 5), 1.0f, midX, topY, rightX, bottomY);
}
//log the colors of the group to the message
void MapState::logGroup(char group, stringstream* message) {
	for (int i = 0; i < 2; i++) {
		if (i > 0)
			*message << " ";
		switch (group % 8) {
			case 0: *message << "black"; break;
			case 1: *message << "red"; break;
			case 2: *message << "green"; break;
			case 3: *message << "yellow"; break;
			case 4: *message << "blue"; break;
			case 5: *message << "magenta"; break;
			case 6: *message << "cyan"; break;
			case 7: *message << "white"; break;
		}
		group /= 8;
	}
}
//log that the switch was kicked
void MapState::logSwitchKick(short switchId) {
	short switchIndex = switchId & railSwitchIndexBitmask;
	stringstream message;
	message << "  switch " << switchIndex << " (";
	logGroup(switches[switchIndex]->getGroup(), &message);
	message << ")";
	Logger::gameplayLogger.logString(message.str());
}
//log that the reset switch was kicked
void MapState::logResetSwitchKick(short resetSwitchId) {
	short resetSwitchIndex = resetSwitchId & railSwitchIndexBitmask;
	stringstream message;
	message << "  reset switch " << resetSwitchIndex;
	Logger::gameplayLogger.logString(message.str());
}
//log that the player rode on the rail
void MapState::logRailRide(short railId, int playerX, int playerY) {
	short railIndex = railId & railSwitchIndexBitmask;
	stringstream message;
	message << "  rail " << railIndex << " (";
	bool skipComma = true;
	for (char group : rails[railIndex]->getGroups()) {
		if (skipComma)
			skipComma = false;
		else
			message << ", ";
		logGroup(group, &message);
	}
	message << ")" << " " << playerX << " " << playerY;
	Logger::gameplayLogger.logString(message.str());
}
//save the map state to the file
void MapState::saveState(ofstream& file) {
	if (lastActivatedSwitchColor >= 0)
		file << lastActivatedSwitchColorFilePrefix << (int)lastActivatedSwitchColor << "\n";
	if (finishedConnectionsTutorial)
		file << finishedConnectionsTutorialFilePrefix << "true\n";

	//don't save the rail states if we're saving the floor file
	//also write that we unlocked all the switches
	if (Editor::needsGameStateSave) {
		file << lastActivatedSwitchColorFilePrefix << "100\n";
		return;
	}
	for (int i = 0; i < (int)railStates.size(); i++) {
		RailState* railState = railStates[i];
		char targetTileOffset = (char)railState->getTargetTileOffset();
		if (targetTileOffset != railState->getRail()->getInitialTileOffset())
			file << railOffsetFilePrefix << i << ' ' << (int)targetTileOffset << "\n";
	}
}
//try to load state from the line of the file, return whether state was loaded
bool MapState::loadState(string& line) {
	if (StringUtils::startsWith(line, lastActivatedSwitchColorFilePrefix))
		lastActivatedSwitchColor = (char)atoi(line.c_str() + lastActivatedSwitchColorFilePrefix.size());
	else if (StringUtils::startsWith(line, finishedConnectionsTutorialFilePrefix))
		finishedConnectionsTutorial = strcmp(line.c_str() + finishedConnectionsTutorialFilePrefix.size(), "true") == 0;
	else if (StringUtils::startsWith(line, railOffsetFilePrefix)) {
		const char* railIndexString = line.c_str() + railOffsetFilePrefix.size();
		const char* offsetString = strchr(railIndexString, ' ') + 1;
		int railIndex = atoi(railIndexString);
		railStates[railIndex]->loadState((float)atoi(offsetString));
	} else
		return false;
	return true;
}
//we don't have previous rails to update from so sort the rails from our base list
void MapState::sortInitialRails() {
	for (RailState* railState : railStates)
		insertRailByHeight(railState);
}
//reset the rails and switches
void MapState::resetMap() {
	lastActivatedSwitchColor = -1;
	finishedConnectionsTutorial = false;
	for (RailState* railState : railStates)
		railState->reset();
}
//examine the neighboring tiles and pick an appropriate default tile, but only if we match the expected floor height
//wall tiles and floor tiles of a different height will be ignored
void MapState::editorSetAppropriateDefaultFloorTile(int x, int y, char expectedFloorHeight) {
	char height = getHeight(x, y);
	if (height != expectedFloorHeight)
		return;

	int leftX = x - 1;
	int rightX = x + 1;
	char leftHeight = getHeight(leftX, y);
	char rightHeight = getHeight(rightX, y);
	bool leftIsBelow = leftHeight < height || leftHeight == emptySpaceHeight;
	bool leftIsAbove = leftHeight > height && editorHasFloorTileCreatingShadowForHeight(leftX, y, height);
	bool rightIsBelow = rightHeight < height || rightHeight == emptySpaceHeight;
	bool rightIsAbove = rightHeight > height && editorHasFloorTileCreatingShadowForHeight(rightX, y, height);

	//this tile is higher than the one above it
	char topHeight = getHeight(x, y - 1);
	if (height > topHeight || topHeight == emptySpaceHeight) {
		if (leftIsAbove)
			editorSetTile(x, y, tilePlatformTopGroundLeftFloor);
		else if (leftIsBelow)
			editorSetTile(x, y, tilePlatformTopLeftFloor);
		else if (rightIsAbove)
			editorSetTile(x, y, tilePlatformTopGroundRightFloor);
		else if (rightIsBelow)
			editorSetTile(x, y, tilePlatformTopRightFloor);
		else
			editorSetTile(x, y, tilePlatformTopFloorFirst);
	//this tile is the same height or lower than the one above it
	} else {
		if (leftIsAbove)
			editorSetTile(x, y, tileGroundLeftFloorFirst);
		else if (leftIsBelow)
			editorSetTile(x, y, tilePlatformLeftFloorFirst);
		else if (rightIsAbove)
			editorSetTile(x, y, tileGroundRightFloorFirst);
		else if (rightIsBelow)
			editorSetTile(x, y, tilePlatformRightFloorFirst);
		else
			editorSetTile(x, y, tileFloorFirst);
	}
}
//check to see if there is a floor tile at this x that is effectively "above" an adjacent tile at the given y
//go up the tiles, and if we find a floor tile with the right height, return true, or if it's too low, return false
bool MapState::editorHasFloorTileCreatingShadowForHeight(int x, int y, char height) {
	for (char tileOffset = 1; true; tileOffset++) {
		char heightDiff = getHeight(x, y - (int)tileOffset) - height;
		//too high to match, keep going
		if (heightDiff > tileOffset * 2)
			continue;

		//if it's an exact match then we found the tile above this one, otherwise there is no tile above this one
		return heightDiff == tileOffset * 2;
	}
}
//set a switch if there's room, or delete a switch if we can
void MapState::editorSetSwitch(int leftX, int topY, char color, char group) {
	//a switch occupies a 2x2 square, and must be surrounded by a 1-tile ring of no-swich-or-rail tiles
	if (leftX - 1 < 0 || topY - 1 < 0 || leftX + 2 >= width || topY + 2 >= height)
		return;

	short newSwitchId = (short)switches.size() | switchIdValue;
	Switch* matchedSwitch = nullptr;
	int matchedSwitchX = -1;
	int matchedSwitchY = -1;
	for (int checkY = topY - 1; checkY <= topY + 2; checkY++) {
		for (int checkX = leftX - 1; checkX <= leftX + 2; checkX++) {
			//no rail or switch here, keep looking
			if (!tileHasRailOrSwitch(checkX, checkY))
				continue;

			//this is not a switch, we can't delete, move, or place a switch here
			short otherRailSwitchId = getRailSwitchId(checkX, checkY);
			if ((otherRailSwitchId & railSwitchIdBitmask) != switchIdValue)
				return;

			//there is a switch here but we won't delete it because it isn't exactly the same as the switch we're placing
			Switch* switch0 = switches[otherRailSwitchId & railSwitchIndexBitmask];
			if (switch0->getColor() != color || switch0->getGroup() != group)
				return;

			int moveDist = abs(checkX - leftX) + abs(checkY - topY);
			//we found this switch already, keep going
			if (matchedSwitch != nullptr)
				continue;
			//we clicked on a switch exactly the same as the one we're placing, mark it as deleted and set newSwitchId to 0 so
			//	that we can use the regular switch-placing logic to clear the switch
			else if (moveDist == 0) {
				switch0->editorIsDeleted = true;
				newSwitchId = 0;
				checkY = topY + 3;
				break;
			//we clicked 1 square adjacent to a switch exactly the same as the one we're placing, save the position and keep
			//	going
			} else if (moveDist == 1) {
				matchedSwitch = switch0;
				newSwitchId = otherRailSwitchId;
				matchedSwitchX = checkX;
				matchedSwitchY = checkY;
			//it's the same switch but it's too far, we can't move it or delete it
			} else
				return;
		}
	}

	//we've moving a switch, erase the ID from the old tiles and update the position on the switch
	if (matchedSwitch != nullptr) {
		for (int eraseY = matchedSwitchY; eraseY < matchedSwitchY + 2; eraseY++) {
			for (int eraseX = matchedSwitchX; eraseX < matchedSwitchX + 2; eraseX++) {
				railSwitchIds[eraseY * width + eraseX] = 0;
			}
		}
		matchedSwitch->editorMoveTo(leftX, topY);
	//we're deleting a switch, remove this group from any matching rails
	} else if (newSwitchId == 0) {
		for (Rail* rail : rails) {
			if (rail->getColor() == color)
				rail->editorRemoveGroup(group);
		}
	//we're setting a new switch
	} else {
		//but don't set it if we already have a switch exactly like this one
		for (int i = 0; i < (int)switches.size(); i++) {
			Switch* switch0 = switches[i];
			if (switch0->getColor() == color && switch0->getGroup() == group && !switch0->editorIsDeleted)
				return;
		}
		switches.push_back(newSwitch(leftX, topY, color, group));
	}

	//go through and set the new switch ID, whether we're adding, moving, or removing a switch
	for (int switchIdY = topY; switchIdY < topY + 2; switchIdY++) {
		for (int switchIdX = leftX; switchIdX < leftX + 2; switchIdX++) {
			railSwitchIds[switchIdY * width + switchIdX] = newSwitchId;
		}
	}
}
//set a rail, or delete a rail if we can
void MapState::editorSetRail(int x, int y, char color, char group) {
	//a rail can't go along the edge of the map
	if (x - 1 < 0 || y - 1 < 0 || x + 1 >= width || y + 1 >= height)
		return;
	//if there is no switch that matches this rail, don't do anything
	bool foundMatchingSwitch = false;
	for (Switch* switch0 : switches) {
		if (switch0->getGroup() == group && switch0->getColor() == color && !switch0->editorIsDeleted) {
			foundMatchingSwitch = true;
			break;
		}
	}
	if (!foundMatchingSwitch)
		return;

	short editingRailSwitchId = -1;
	bool editingAdjacentRailSwitch = false;
	bool clickedOnRailSwitch = false;
	int minCheckX = 1000000;
	int minCheckY = 1000000;
	int maxCheckX = -1;
	int maxCheckY = -1;
	for (int checkY = y - 1; checkY <= y + 1; checkY++) {
		for (int checkX = x - 1; checkX <= x + 1; checkX++) {
			//no rail or switch here, keep looking
			if (!tileHasRailOrSwitch(checkX, checkY))
				continue;

			//if this is not a rail or reset switch, we can't place a new rail here
			short otherRailSwitchId = getRailSwitchId(checkX, checkY);
			short otherRailSwitchIdValue = otherRailSwitchId & railSwitchIdBitmask;
			if (otherRailSwitchIdValue != railIdValue && otherRailSwitchIdValue != resetSwitchIdValue)
				return;

			//we haven't seen any rail or switch yet, save it
			if (editingRailSwitchId == -1)
				editingRailSwitchId = otherRailSwitchId;
			//we saw an id before and we found a different id now, we can't place a rail here
			else if (editingRailSwitchId != otherRailSwitchId)
				return;

			if (abs(x - checkX) + abs(y - checkY) == 1)
				editingAdjacentRailSwitch = true;
			if (checkX == x && checkY == y)
				clickedOnRailSwitch = true;
			minCheckX = MathUtils::min(minCheckX, checkX);
			minCheckY = MathUtils::min(minCheckY, checkY);
			maxCheckX = MathUtils::max(maxCheckX, checkX);
			maxCheckY = MathUtils::max(maxCheckY, checkY);
		}
	}

	int railSwitchIndex = y * width + x;
	//we're adding a new rail
	if (editingRailSwitchId == -1) {
		railSwitchIds[railSwitchIndex] = (short)rails.size() | railIdValue;
		rails.push_back(newRail(x, y, getHeight(x, y), color, 0, 1));
		return;
	}

	int xSpan = maxCheckX - minCheckX + 1;
	int ySpan = maxCheckY - minCheckY + 1;
	short editingRailSwitchIdValue = editingRailSwitchId & railSwitchIdBitmask;
	//we're editing a rail
	if (editingRailSwitchIdValue == railIdValue) {
		Rail* editingRail = rails[editingRailSwitchId & railSwitchIndexBitmask];
		//don't do anything if the colors don't match, or if the adjacent rails span more than a 1x2 rect (because that means
		//	we're touching more than just the end of a rail)
		if (editingRail->getColor() != color || (xSpan + ySpan > 3))
			return;

		int segmentCount = editingRail->getSegmentCount();
		Rail::Segment* firstSegment = editingRail->getSegment(0);
		Rail::Segment* lastSegment = editingRail->getSegment(segmentCount - 1);
		//we clicked next to a rail, add to it
		if (!clickedOnRailSwitch) {
			int tileHeight = getHeight(x, y);
			//don't add a segment if the rail is not adjacent, or on a non-empty-space tile above the rail
			if (!editingAdjacentRailSwitch || (tileHeight > editingRail->getBaseHeight() && tileHeight != emptySpaceHeight))
				return;

			editingRail->addGroup(group);
			editingRail->addSegment(x, y);
			railSwitchIds[railSwitchIndex] = editingRailSwitchId;
		//we clicked on the last segment of a rail, delete it
		} else if (segmentCount == 1) {
			editingRail->editorIsDeleted = true;
			railSwitchIds[railSwitchIndex] = 0;
		//we clicked at the end of a rail, delete that segment
		} else if ((firstSegment->x == x && firstSegment->y == y) || (lastSegment->x == x && lastSegment->y == y)) {
			editingRail->editorRemoveGroup(group);
			editingRail->editorRemoveSegment(x, y);
			railSwitchIds[railSwitchIndex] = 0;
		}
	//we're editing a reset switch
	} else if (editingRailSwitchIdValue == resetSwitchIdValue) {
		ResetSwitch* editingResetSwitch = resetSwitches[editingRailSwitchId & railSwitchIndexBitmask];
		int editingResetSwitchX = editingResetSwitch->getCenterX();
		int editingResetSwitchBottomY = editingResetSwitch->getBottomY();
		//don't do anything if we the reset switch isn't adjacent to this, or if the adjacent reset switch spans more than 2 in
		//	both directions (a 1x3 rect is fine if we clicked next to the reset switch with a rail below it, and if it's the
		//	middle of a rail we'll just end up passing it)
		if (!editingAdjacentRailSwitch || (xSpan > 1 && ySpan > 1 && !clickedOnRailSwitch))
			return;
		//one side at a time, see if we can add a segment to end of the reset switch
		auto editorUpdateResetSwitchGroupsAt =
			[=](int newRailGroupX, int newRailGroupY, std::vector<ResetSwitch::Segment>* segments) {
				Holder_RessetSwitchSegmentVector segmentsHolder (segments);
				return editorUpdateResetSwitchGroups(
					x,
					y,
					color,
					group,
					editingRailSwitchId,
					railSwitchIndex,
					editingResetSwitchX,
					editingResetSwitchBottomY,
					newRailGroupX,
					newRailGroupY,
					&segmentsHolder);
			};
		editorUpdateResetSwitchGroupsAt(editingResetSwitchX - 1, editingResetSwitchBottomY, &editingResetSwitch->leftSegments)
			|| editorUpdateResetSwitchGroupsAt(
				editingResetSwitchX, editingResetSwitchBottomY + 1, &editingResetSwitch->bottomSegments)
			|| editorUpdateResetSwitchGroupsAt(
				editingResetSwitchX + 1, editingResetSwitchBottomY, &editingResetSwitch->rightSegments);
	}
}
//remove a rail segment if we clicked on the last segment of the list, or add a segment if it's adjacent to the last segment of
//	the list, or add the first segment if there are none and the new one matches the other coordinates
//return whether we updated the segments or determined there was nothing to update
bool MapState::editorUpdateResetSwitchGroups(
	int x,
	int y,
	char color,
	char group,
	short resetSwitchId,
	int resetSwitchIndex,
	int baseX,
	int baseY,
	int newRailGroupX,
	int newRailGroupY,
	Holder_RessetSwitchSegmentVector* segmentsHolder)
{
	vector<ResetSwitch::Segment>* segments = segmentsHolder->val;
	if (segments->size() == 0) {
		//if we have no segments, we can only add a segment with group 0 at the given coordinates
		if (x != newRailGroupX || y != newRailGroupY || group != 0)
			return false;
	} else {
		ResetSwitch::Segment& lastSegment = segments->back();
		int segmentDist = abs(lastSegment.x - x) + abs(lastSegment.y - y);
		//we clicked directly on the last segment, we can't add a segment here
		if (segmentDist == 0) {
			//delete it if it matches the color and group of this rail
			if (lastSegment.color == color && lastSegment.group == group) {
				segments->pop_back();
				railSwitchIds[resetSwitchIndex] = 0;
			}
			return true;
		//we clicked next to the last segment, check if we can add a segment here
		} else if (segmentDist == 1) {
			//the color matches the last segment- we can add a segment if the group isn't already there
			if (color == lastSegment.color) {
				for (ResetSwitch::Segment& segment : (*segments)) {
					if (segment.color == color && segment.group == group)
						return false;
				}
			//this is the start of the second color, we can add it if it's group 0
			} else if (lastSegment.color == segments->front().color) {
				if (group != 0)
					return false;
			//this would be a third color, we can't add a segment here
			} else
				return false;
		//we clicked too far from the last segment, we can't add a segment here
		} else
			return false;
	}

	//if we get here, we can add this segment
	railSwitchIds[resetSwitchIndex] = resetSwitchId;
	segments->push_back(ResetSwitch::Segment(x, y, color, group, 0));
	//fix sprite indices
	int lastEndX = baseX;
	int lastEndY = baseY;
	ResetSwitch::Segment& end = segments->back();
	if (segments->size() > 1) {
		ResetSwitch::Segment& lastEnd = (*segments)[segments->size() - 2];
		lastEndX = lastEnd.x;
		lastEndY = lastEnd.y;
		int secondLastX = baseX;
		int secondLastY = baseY;
		if (segments->size() >= 3) {
			ResetSwitch::Segment& secondLastEnd = (*segments)[segments->size() - 3];
			secondLastX = secondLastEnd.x;
			secondLastY = secondLastEnd.y;
		}
		lastEnd.spriteHorizontalIndex =
			Rail::middleSegmentSpriteHorizontalIndex(secondLastX, secondLastY, lastEndX, lastEndY, x, y);
	}
	end.spriteHorizontalIndex = Rail::extentSegmentSpriteHorizontalIndex(lastEndX, lastEndY, x, y);
	return true;
}
//set a reset switch, or delete one if we can
void MapState::editorSetResetSwitch(int x, int bottomY) {
	//a reset switch occupies a 1x2 square, and must be surrounded by a 1-tile ring of no-swich-or-rail tiles
	if (x - 1 < 0 || bottomY - 2 < 0 || x + 1 >= width || bottomY + 1 >= height)
		return;

	short newResetSwitchId = (short)resetSwitches.size() | resetSwitchIdValue;
	for (int checkY = bottomY - 2; checkY <= bottomY + 1; checkY++) {
		for (int checkX = x - 1; checkX <= x + 1; checkX++) {
			//no rail or switch here, keep looking
			if (!tileHasRailOrSwitch(checkX, checkY))
				continue;

			//this is not a reset switch, we can't place a new rail here
			short otherRailSwitchId = getRailSwitchId(checkX, checkY);
			if ((otherRailSwitchId & railSwitchIdBitmask) != resetSwitchIdValue)
				return;

			ResetSwitch* resetSwitch = resetSwitches[otherRailSwitchId & railSwitchIndexBitmask];
			//we clicked on a reset switch in the same spot as the one we're placing, mark it as deleted and set
			//	newResetSwitchId to 0 so that we can use the regular reset-switch-placing logic to clear the reset switch
			if (checkX == x && checkY == bottomY - 1) {
				//but if it has segments, don't do anything to it
				if (resetSwitch->leftSegments.size() > 0
						|| resetSwitch->bottomSegments.size() > 0
						|| resetSwitch->rightSegments.size() > 0)
					return;
				resetSwitch->editorIsDeleted = true;
				newResetSwitchId = 0;
				checkY = bottomY + 2;
				break;
			//we didn't click on a reset switch in the same spot as this one, don't try to place a new one
			} else
				return;
		}
	}

	railSwitchIds[bottomY * width + x] = newResetSwitchId;
	railSwitchIds[(bottomY - 1) * width + x] = newResetSwitchId;
	if (newResetSwitchId != 0)
		resetSwitches.push_back(newResetSwitch(x, bottomY));
}
//adjust the tile offset of the rail here, if there is one
void MapState::editorAdjustRailInitialTileOffset(int x, int y, char tileOffset) {
	int railSwitchId = getRailSwitchId(x, y);
	if ((railSwitchId & railSwitchIdBitmask) == railIdValue)
		rails[railSwitchId & railSwitchIndexBitmask]->editorAdjustInitialTileOffset(x, y, tileOffset);
}
//check if we're saving a rail or switch to the floor file, and if so get the data we need at this tile
char MapState::editorGetRailSwitchFloorSaveData(int x, int y) {
	int railSwitchId = getRailSwitchId(x, y);
	int railSwitchIdValue = railSwitchId & railSwitchIdBitmask;
	if (railSwitchIdValue == railIdValue)
		return rails[railSwitchId & railSwitchIndexBitmask]->editorGetFloorSaveData(x, y);
	else if (railSwitchIdValue == switchIdValue)
		return switches[railSwitchId & railSwitchIndexBitmask]->editorGetFloorSaveData(x, y);
	else if (railSwitchIdValue == resetSwitchIdValue)
		return resetSwitches[railSwitchId & railSwitchIndexBitmask]->editorGetFloorSaveData(x, y);
	else
		return 0;
}
