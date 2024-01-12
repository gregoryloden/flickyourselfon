#include "MapState.h"
#include "Editor/Editor.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/EntityState.h"
#include "GameState/KickAction.h"
#include "GameState/UndoState.h"
#include "GameState/MapState/Level.h"
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

#define entityAnimationSpriteAnimationWithDelay(animation) \
	newEntityAnimationSetSpriteAnimation(animation), \
	newEntityAnimationDelay(animation->getTotalTicksDuration())

//////////////////////////////// MapState::PlaneConnection ////////////////////////////////
MapState::PlaneConnection::PlaneConnection(LevelTypes::Plane* pFromPlane, int pToTile, short pRailId)
: fromPlane(pFromPlane)
, toTile(pToTile)
, railId(pRailId) {
}
MapState::PlaneConnection::~PlaneConnection() {}

//////////////////////////////// MapState::PlaneConnectionSwitch ////////////////////////////////
MapState::PlaneConnectionSwitch::PlaneConnectionSwitch(
	Switch* pSwitch0, LevelTypes::Plane* pPlane, int pPlaneConnectionSwitchIndex)
: switch0(pSwitch0)
, plane(pPlane)
, planeConnectionSwitchIndex(pPlaneConnectionSwitchIndex) {
}
MapState::PlaneConnectionSwitch::~PlaneConnectionSwitch() {
	//don't delete the switch, it's owned by MapState
	//don't delete the plane, it's owned by a Level
}

//////////////////////////////// MapState ////////////////////////////////
char* MapState::tiles = nullptr;
char* MapState::heights = nullptr;
short* MapState::railSwitchIds = nullptr;
short* MapState::planeIds = nullptr;
vector<Rail*> MapState::rails;
vector<Switch*> MapState::switches;
vector<ResetSwitch*> MapState::resetSwitches;
vector<LevelTypes::Plane*> MapState::planes;
vector<Level*> MapState::levels;
int MapState::width = 1;
int MapState::height = 1;
bool MapState::editorHideNonTiles = false;
MapState::MapState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, railStates()
, switchStates()
, resetSwitchStates()
, lastActivatedSwitchColor(-1)
, showConnectionsEnabled(false)
, finishedConnectionsTutorial(false)
, finishedMapCameraTutorial(false)
//prevent the switches fade-in animation from playing on load by ensuring tick 0 is after the fade-in is over
, switchesAnimationFadeInStartTicksTime(-switchesFadeInDuration)
, shouldPlayRadioTowerAnimation(false)
, particles()
, radioWavesColor(-1)
, waveformStartTicksTime(0)
, waveformEndTicksTime(0) {
	for (int i = 0; i < (int)rails.size(); i++)
		railStates.push_back(newRailState(rails[i]));
	for (Switch* switch0 : switches)
		switchStates.push_back(newSwitchState(switch0));
	for (ResetSwitch* resetSwitch : resetSwitches)
		resetSwitchStates.push_back(newResetSwitchState(resetSwitch));

	//add all the rail states to their appropriate switch states
	vector<SwitchState*> switchStatesByGroupByColor[4];
	for (SwitchState* switchState : switchStates) {
		Switch* switch0 = switchState->getSwitch();
		vector<SwitchState*>& switchStatesByGroup = switchStatesByGroupByColor[switch0->getColor()];
		int group = (int)switch0->getGroup();
		while ((int)switchStatesByGroup.size() <= group)
			switchStatesByGroup.push_back(nullptr);
		switchStatesByGroup[group] = switchState;
	}
	for (RailState* railState : railStates) {
		Rail* rail = railState->getRail();
		vector<SwitchState*>& switchStatesByGroup = switchStatesByGroupByColor[rail->getColor()];
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
MapState* MapState::produce(objCounterParameters()) {
	initializeWithNewFromPool(m, MapState)
	return m;
}
pooledReferenceCounterDefineRelease(MapState)
void MapState::prepareReturnToPool() {
	particles.clear();
}
void MapState::buildMap() {
	SDL_Surface* floor = FileUtils::loadImage(floorFileName);
	width = floor->w;
	height = floor->h;
	int totalTiles = width * height;
	tiles = new char[totalTiles];
	heights = new char[totalTiles];
	railSwitchIds = new short[totalTiles];

	int redShift = (int)floor->format->Rshift;
	int redMask = (int)floor->format->Rmask;
	int greenShift = (int)floor->format->Gshift;
	int greenMask = (int)floor->format->Gmask;
	int blueShift = (int)floor->format->Bshift;
	int blueMask = (int)floor->format->Bmask;
	int* pixels = static_cast<int*>(floor->pixels);

	//set all tiles, heights, and IDs before loading rails/switches
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
		//	bit 2: direction (0 for up/-1, 1 for down/1)
		//	bits 3-5: movement magnitude
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
			addResetSwitchSegments(pixels, redShift, i - 1, newResetSwitchId, resetSwitch, -1);
			addResetSwitchSegments(pixels, redShift, i + width, newResetSwitchId, resetSwitch, 0);
			addResetSwitchSegments(pixels, redShift, i + 1, newResetSwitchId, resetSwitch, 1);
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
			char movementMagnitude = (railByte2PostShift >> 1) & floorRailMovementMagnitudePostShiftBitmask;
			Rail* rail = newRail(headX, headY, heights[i], color, initialTileOffset, movementDirection, movementMagnitude);
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

	//link reset switches to their affected rails
	for (int i = 0; i < (int)rails.size(); i++) {
		Rail* rail = rails[i];
		short railId = (short)i | railIdValue;
		char color = rail->getColor();
		for (char group : rail->getGroups()) {
			for (ResetSwitch* resetSwitch : resetSwitches) {
				if (resetSwitch->hasGroupForColor(group, color))
					resetSwitch->getAffectedRailIds()->push_back(railId);
			}
		}
	}

	buildLevels();

	SDL_FreeSurface(floor);
}
vector<int> MapState::parseRail(int* pixels, int redShift, int segmentIndex, int railSwitchId) {
	//cache shift values so that we can iterate the floor data quicker
	int floorIsRailSwitchAndHeadShiftedBitmask = floorIsRailSwitchAndHeadBitmask << redShift;
	int floorRailSwitchTailShiftedValue = floorIsRailSwitchBitmask << redShift;
	vector<int> segmentIndices;
	while (true) {
		//rails are never placed at the edge of the map, so we don't need to check whether we wrapped around the edge
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
void MapState::addResetSwitchSegments(
	int* pixels, int redShift, int firstSegmentIndex, int resetSwitchId, ResetSwitch* resetSwitch, char segmentsSection)
{
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
	int segmentX = firstSegmentIndex % width;
	int segmentY = firstSegmentIndex / width;
	resetSwitch->addSegment(segmentX, segmentY, color1, 0, segmentsSection);

	char segmentColor = color1;
	vector<int> railIndices = parseRail(pixels, redShift, firstSegmentIndex, resetSwitchId);
	for (int segmentIndex : railIndices) {
		segmentX = segmentIndex % width;
		segmentY = segmentIndex / width;

		char group = (char)((pixels[segmentIndex] >> floorRailSwitchGroupDataShift) & floorRailSwitchGroupPostShiftBitmask);
		if (group == 0)
			segmentColor = color2;
		resetSwitch->addSegment(segmentX, segmentY, segmentColor, group, segmentsSection);
	}
}
void MapState::buildLevels() {
	//initialize the base levels state
	planeIds = new short[width * height] {};
	Level* activeLevel = newLevel();
	levels.push_back(activeLevel);
	vector<PlaneConnection> planeConnections;

	//the first tile we'll check is the tile where the boot starts
	deque<int> tileChecks ({ introAnimationBootTileY * width + introAnimationBootTileX });
	while (!tileChecks.empty()) {
		int nextTile = tileChecks.front();
		tileChecks.pop_front();
		//skip any plane that's already filled
		if (planeIds[nextTile] != 0)
			continue;

		//this tile is a victory tile - start a new level if this is the last tile we know about, otherwise keep building planes
		bool isVictoryTile = tiles[nextTile] == tilePuzzleEnd;
		if (isVictoryTile) {
			if (!tileChecks.empty()) {
				tileChecks.push_back(nextTile);
				continue;
			}
			levels.push_back(activeLevel = newLevel());
		}
		LevelTypes::Plane* newPlane = buildPlane(nextTile, activeLevel, tileChecks, planeConnections);
		if (isVictoryTile)
			levels[levels.size() - 2]->assignVictoryPlane(newPlane);
	}

	//add switches to planes
	vector<PlaneConnectionSwitch> planeConnectionSwitches;
	for (Switch* switch0 : switches) {
		LevelTypes::Plane* plane = planes[planeIds[switch0->getTopY() * width + switch0->getLeftX()] - 1];
		planeConnectionSwitches.push_back(PlaneConnectionSwitch(switch0, plane, plane->addConnectionSwitch()));
	}

	//organize switch/plane combinations so that we can refer to them when adding rail connections
	vector<PlaneConnectionSwitch*> planeConnectionSwitchesByGroupByColor[4];
	for (PlaneConnectionSwitch& planeConnectionSwitch : planeConnectionSwitches) {
		vector<PlaneConnectionSwitch*>& planeConnectionSwitchesByGroup =
			planeConnectionSwitchesByGroupByColor[planeConnectionSwitch.switch0->getColor()];
		char group = planeConnectionSwitch.switch0->getGroup();
		while ((int)planeConnectionSwitchesByGroup.size() <= group)
			planeConnectionSwitchesByGroup.push_back(nullptr);
		planeConnectionSwitchesByGroup[group] = &planeConnectionSwitch;
	}

	//add connections between planes
	for (PlaneConnection& planeConnection : planeConnections) {
		LevelTypes::Plane* toPlane = planes[planeIds[planeConnection.toTile] - 1];
		if (planeConnection.fromPlane->addConnection(toPlane, planeConnection.railId != absentRailSwitchId))
			continue;

		//we have a new rail - add a connection to it and add the data to all applicable switches
		LevelTypes::RailByteMaskData railByteMaskData = planeConnection.fromPlane->getOwningLevel()->getNextRailByteMask();
		planeConnection.fromPlane->addRailConnection(toPlane, railByteMaskData);
		Rail* rail = rails[planeConnection.railId & railSwitchIndexBitmask];
		vector<PlaneConnectionSwitch*>& planeConnectionSwitchesByGroup =
			planeConnectionSwitchesByGroupByColor[rail->getColor()];
		for (char group : rail->getGroups()) {
			PlaneConnectionSwitch* planeConnectionSwitch = planeConnectionSwitchesByGroup[group];
			planeConnectionSwitch->plane->addRailConnectionToSwitch(
				railByteMaskData, planeConnectionSwitch->planeConnectionSwitchIndex);
		}
	}
}
LevelTypes::Plane* MapState::buildPlane(
	int tile, Level* activeLevel, deque<int>& tileChecks, vector<PlaneConnection>& planeConnections)
{
	//prep a new plane
	LevelTypes::Plane* plane = activeLevel->addNewPlane();
	planes.push_back(plane);
	int planeId = (int)planes.size();
	planeIds[tile] = planeId;
	int planeHeight = heights[tile];

	//mark tiles with it via breadth-first search
	deque<int> planeTileChecks ({ tile });
	while (!planeTileChecks.empty()) {
		tile = planeTileChecks.front();
		planeTileChecks.pop_front();
		plane->addTile(tile % width, tile / width);
		//planes never extend to the edge of the map, so we don't need to check whether we wrapped around the edge
		int upNeighbor = tile - width;
		int downNeighbor = tile + width;
		int neighbors[] = { tile - 1, tile + 1, upNeighbor, downNeighbor };

		//check neighboring tiles of interest to add to this plane or a climb/fall plane
		for (int neighbor : neighbors) {
			char neighborHeight = heights[neighbor];
			//check for neighboring tiles at the same height to include in the plane
			if (neighborHeight == planeHeight) {
				if (planeIds[neighbor] == 0) {
					planeTileChecks.push_back(neighbor);
					planeIds[neighbor] = planeId;
				}
			//check for neighboring tiles to climb to
			} else if (neighborHeight > planeHeight && neighborHeight != emptySpaceHeight) {
				//for all neighbors but the down neighbor, the tile to climb to is one tile up
				if (neighbor != downNeighbor) {
					neighbor -= width;
					neighborHeight = heights[neighbor];
				}
				if (neighborHeight != planeHeight + 2)
					continue;
				planeConnections.push_back(PlaneConnection(plane, neighbor, absentRailSwitchId));
				tileChecks.push_back(neighbor);
			//check for neighboring tiles to fall to
			} else {
				//no falling if there was a rail on the center tile
				if ((railSwitchIds[tile] & railSwitchIdBitmask) == railIdValue)
					continue;
				//for all neighbors but the up neighbor, the tile to climb to is further down
				else if (neighbor != upNeighbor) {
					int fallX = neighbor % width;
					int fallY;
					if (!tileFalls(fallX, neighbor / width, planeHeight, &fallY, nullptr))
						continue;
					neighbor = fallY * width + fallX;
				} else if (neighborHeight % 2 != 0)
					continue;
				planeConnections.push_back(PlaneConnection(plane, neighbor, absentRailSwitchId));
				tileChecks.push_back(neighbor);
			}
		}

		//check for other tiles of interest to connect planes
		//if there's a rail, check if it's the start/end segment
		if ((railSwitchIds[tile] & railSwitchIdBitmask) == railIdValue) {
			short railId = railSwitchIds[tile];
			Rail* rail = rails[railId & railSwitchIndexBitmask];
			Rail::Segment* startSegment = rail->getSegment(0);
			Rail::Segment* endSegment = rail->getSegment(rail->getSegmentCount() - 1);
			int startTile = startSegment->y * width + startSegment->x;
			int endTile = endSegment->y * width + endSegment->x;
			if (tile == startTile)
				addRailPlaneConnection(
					plane, endTile, railId, planeConnections, activeLevel, rail, rail->getSegmentCount() - 2, tileChecks);
			else if (tile == endTile)
				addRailPlaneConnection(plane, startTile, railId, planeConnections, activeLevel, rail, 1, tileChecks);
		}
	}
	return plane;
}
void MapState::addRailPlaneConnection(
	LevelTypes::Plane* plane,
	int toTile,
	short railId,
	vector<PlaneConnection>& planeConnections,
	Level* activeLevel,
	Rail* rail,
	int adjacentRailSegmentIndex,
	deque<int>& tileChecks)
{
	planeConnections.push_back(PlaneConnection(plane, toTile, railId));
	if (planeIds[toTile] == 0) {
		activeLevel->setMinimumRailColor(rail->getColor());
		Rail::Segment* toAdjacentSegment = rail->getSegment(adjacentRailSegmentIndex);
		int toAdjacentTile = toTile * 2 - toAdjacentSegment->y * width - toAdjacentSegment->x;
		if (tiles[toAdjacentTile] == tilePuzzleEnd)
			tileChecks.push_back(toAdjacentTile);
		else
			tileChecks.push_back(toTile);
	}
}
void MapState::deleteMap() {
	delete[] tiles;
	delete[] heights;
	delete[] railSwitchIds;
	delete[] planeIds;
	for (Rail* rail : rails)
		delete rail;
	rails.clear();
	for (Switch* switch0 : switches)
		delete switch0;
	switches.clear();
	for (ResetSwitch* resetSwitch : resetSwitches)
		delete resetSwitch;
	resetSwitches.clear();
	//don't delete planes, they are owned by the Levels
	planes.clear();
	for (Level* level : levels)
		delete level;
	levels.clear();
}
int MapState::getScreenLeftWorldX(EntityState* camera, int ticksTime) {
	//we convert the camera center to int first because with a position with 0.5 offsets, we render all pixels aligned (because
	//	the screen/player width is odd); once we get to a .0 position, then we render one pixel over
	return (int)(camera->getRenderCenterWorldX(ticksTime)) - Config::gameScreenWidth / 2;
}
int MapState::getScreenTopWorldY(EntityState* camera, int ticksTime) {
	//(see getScreenLeftWorldX)
	return (int)(camera->getRenderCenterWorldY(ticksTime)) - Config::gameScreenHeight / 2;
}
#ifdef DEBUG
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
float MapState::antennaCenterWorldX() {
	return (float)(radioTowerLeftXOffset + SpriteRegistry::radioTower->getSpriteWidth() / 2);
}
float MapState::antennaCenterWorldY() {
	return (float)(radioTowerTopYOffset + 2);
}
float MapState::radioTowerPlatformCenterWorldY() {
	return (float)(radioTowerTopYOffset + 103);
}
bool MapState::tileHasResetSwitchBody(int x, int y) {
	if (!tileHasResetSwitch(x, y))
		return false;
	ResetSwitch* resetSwitch = resetSwitches[getRailSwitchId(x, y) & railSwitchIndexBitmask];
	return (x == resetSwitch->getCenterX() && (y == resetSwitch->getBottomY() || y == resetSwitch->getBottomY() - 1));
}
bool MapState::tileHasRailEnd(int x, int y) {
	if (!tileHasRail(x, y))
		return false;
	Rail* rail = getRailFromId(getRailSwitchId(x, y));
	Rail::Segment* startSegment = rail->getSegment(0);
	if (startSegment->x == x && startSegment->y == y)
		return true;
	Rail::Segment* endSegment = rail->getSegment(rail->getSegmentCount() - 1);
	return endSegment->x == x && endSegment->y == y;
}
bool MapState::tileFalls(int x, int y, char initialHeight, int* outFallY, char* outFallHeight) {
	//start one tile down and look for an eligible floor below our current height
	for (char tileOffset = 1; true; tileOffset++) {
		char fallHeight = getHeight(x, y + tileOffset);
		char targetHeight = initialHeight - tileOffset * 2;
		//an empty tile height is fine...
		if (fallHeight == MapState::emptySpaceHeight) {
			//...unless we reached the lowest height, in which case there is no longer a possible fall height
			if (targetHeight <= 0)
				return false;
			continue;
		//the tile is higher than us, we can't fall here
		} else if (fallHeight > targetHeight)
			return false;
		//this is a cliff face or lower floor, keep looking
		else if (fallHeight < targetHeight)
			continue;

		//we found a matching floor tile
		*outFallY = y + tileOffset;
		if (outFallHeight != nullptr)
			*outFallHeight = fallHeight;
		return true;
	}
}
KickActionType MapState::getSwitchKickActionType(short switchId) {
	Switch* switch0 = switches[switchId & railSwitchIndexBitmask];
	bool group0 = switch0->getGroup() == 0;
	//we've already triggered this group 0 switch, or can't yet kick this puzzle switch
	if (group0 == (lastActivatedSwitchColor >= switch0->getColor()))
		//if we've already triggered this group 0 switch, there's nothing to show, otherwise this is a switch that the player
		//	can't kick yet
		return group0 ? KickActionType::None : KickActionType::NoSwitch;
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
char MapState::horizontalTilesHeight(int lowMapX, int highMapX, int mapY) {
	char foundHeight = getHeight(lowMapX, mapY);
	for (int mapX = lowMapX + 1; mapX <= highMapX; mapX++) {
		if (getHeight(mapX, mapY) != foundHeight)
			return invalidHeight;
	}
	return foundHeight;
}
char MapState::verticalTilesHeight(int mapX, int lowMapY, int highMapY) {
	char foundHeight = getHeight(mapX, lowMapY);
	for (int mapY = lowMapY + 1; mapY <= lowMapY; lowMapY++) {
		if (getHeight(mapX, mapY) != foundHeight)
			return invalidHeight;
	}
	return foundHeight;
}
void MapState::setIntroAnimationBootTile(bool showBootTile) {
	//if we're not showing the boot tile, just show a default tile instead of showing the tile from the floor file
	tiles[introAnimationBootTileY * width + introAnimationBootTileX] = showBootTile ? tileBoot : tileFloorFirst;
}
void MapState::updateWithPreviousMapState(MapState* prev, int ticksTime) {
	lastActivatedSwitchColor = prev->lastActivatedSwitchColor;
	showConnectionsEnabled = prev->showConnectionsEnabled;
	finishedConnectionsTutorial = prev->finishedConnectionsTutorial;
	finishedMapCameraTutorial = prev->finishedMapCameraTutorial;
	shouldPlayRadioTowerAnimation = false;
	switchesAnimationFadeInStartTicksTime = prev->switchesAnimationFadeInStartTicksTime;
	radioWavesColor = prev->radioWavesColor;
	waveformStartTicksTime = prev->waveformStartTicksTime;
	waveformEndTicksTime = prev->waveformEndTicksTime;

	if (Editor::isActive) {
		//since the editor can add switches and rails, make sure we update our list to track them
		//we won't connect rail states to switch states since we can't kick switches in the editor
		while (railStates.size() < rails.size())
			railStates.push_back(newRailState(rails[railStates.size()]));
		while (switchStates.size() < switches.size())
			switchStates.push_back(newSwitchState(switches[switchStates.size()]));
		while (resetSwitchStates.size() < resetSwitches.size())
			resetSwitchStates.push_back(newResetSwitchState(resetSwitches[resetSwitchStates.size()]));
	}
	for (int i = 0; i < (int)prev->switchStates.size(); i++)
		switchStates[i]->updateWithPreviousSwitchState(prev->switchStates[i]);
	for (int i = 0; i < (int)prev->resetSwitchStates.size(); i++)
		resetSwitchStates[i]->updateWithPreviousResetSwitchState(prev->resetSwitchStates[i]);
	for (int i = 0; i < (int)prev->railStates.size(); i++)
		railStates[i]->updateWithPreviousRailState(prev->railStates[i], ticksTime);

	while (particles.size() < prev->particles.size())
		particles.push_back(newParticle(0, 0, false));
	while (particles.size() > prev->particles.size())
		particles.pop_back();
	for (int i = (int)particles.size() - 1; i >= 0; i--) {
		if (!particles[i].get()->updateWithPreviousParticle(prev->particles[i].get(), ticksTime))
			particles.erase(particles.begin() + i);
	}
	if (particles.empty())
		radioWavesColor = -1;
}
Particle* MapState::queueParticle(
	float centerX,
	float centerY,
	bool isAbovePlayer,
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> components,
	int ticksTime)
{
	Particle* particle = newParticle(centerX, centerY, isAbovePlayer);
	particle->beginEntityAnimation(&components, ticksTime);
	particles.push_back(particle);
	return particle;
}
void MapState::flipSwitch(short switchId, bool moveRailsForward, bool allowRadioTowerAnimation, int ticksTime) {
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
	} else if (lastActivatedSwitchColor >= switch0->getColor()) {
		switchState->flip(moveRailsForward, ticksTime);

		radioWavesColor = switch0->getColor();
		queueParticle(
			switch0->getSwitchWavesCenterX(),
			switch0->getSwitchWavesCenterY(),
			true,
			{
				entityAnimationSpriteAnimationWithDelay(SpriteRegistry::switchWavesAnimation),
			},
			ticksTime);

		for (RailState* railState : *switchState->getConnectedRailStates()) {
			Rail* rail = railState->getRail();
			Rail::Segment* segments[] = { rail->getSegment(0), rail->getSegment(rail->getSegmentCount() - 1) };
			for (Rail::Segment* segment : segments)
				queueParticle(
					segment->tileCenterX(),
					segment->tileCenterY(),
					true,
					{
						newEntityAnimationDelay(SpriteRegistry::radioWaveAnimationTicksPerFrame),
						entityAnimationSpriteAnimationWithDelay(SpriteRegistry::railWavesAnimation),
					},
					ticksTime);
		}
	}
}
void MapState::flipResetSwitch(short resetSwitchId, KickResetSwitchUndoState* kickResetSwitchUndoState, int ticksTime) {
	ResetSwitchState* resetSwitchState = resetSwitchStates[resetSwitchId & railSwitchIndexBitmask];
	if (kickResetSwitchUndoState != nullptr) {
		for (KickResetSwitchUndoState::RailUndoState& railUndoState : *kickResetSwitchUndoState->getRailUndoStates())
			railStates[railUndoState.railId & railSwitchIndexBitmask]->loadState(
				railUndoState.fromTargetTileOffset, railUndoState.fromMovementDirection, true);
	} else {
		for (short railId : *resetSwitchState->getResetSwitch()->getAffectedRailIds())
			railStates[railId & railSwitchIndexBitmask]->reset(true);
	}
	resetSwitchState->flip(ticksTime);
}
void MapState::writeCurrentRailStates(short resetSwitchId, KickResetSwitchUndoState* kickResetSwitchUndoState) {
	vector<KickResetSwitchUndoState::RailUndoState>* railUndoStates = kickResetSwitchUndoState->getRailUndoStates();
	for (short railId : *resetSwitches[resetSwitchId & railSwitchIndexBitmask]->getAffectedRailIds()) {
		RailState* railState = railStates[railId & railSwitchIndexBitmask];
		railUndoStates->push_back(
			KickResetSwitchUndoState::RailUndoState(
				railId, railState->getTargetTileOffset(), railState->getNextMovementDirection()));
	}
}
int MapState::startRadioWavesAnimation(int initialTicksDelay, int ticksTime) {
	radioWavesColor = lastActivatedSwitchColor;
	Particle* particle = queueParticle(
		antennaCenterWorldX(),
		antennaCenterWorldY(),
		true,
		{
			newEntityAnimationDelay(initialTicksDelay),
			entityAnimationSpriteAnimationWithDelay(SpriteRegistry::radioWavesAnimation),
			newEntityAnimationSetSpriteAnimation(nullptr),
			newEntityAnimationDelay(interRadioWavesAnimationTicks),
			entityAnimationSpriteAnimationWithDelay(SpriteRegistry::radioWavesAnimation),
		},
		ticksTime);
	int radioWavesDuration = particle->getAnimationTicksDuration() - initialTicksDelay;
	int waveformFadedInStartTicksTime = ticksTime + initialTicksDelay;
	waveformStartTicksTime = waveformFadedInStartTicksTime - waveformStartEndBufferTicks;
	waveformEndTicksTime = waveformFadedInStartTicksTime + radioWavesDuration + waveformStartEndBufferTicks;
	return radioWavesDuration;
}
void MapState::startSwitchesFadeInAnimation(int initialTicksDelay, int ticksTime) {
	shouldPlayRadioTowerAnimation = false;
	switchesAnimationFadeInStartTicksTime = ticksTime + initialTicksDelay;
	for (Switch* switch0 : switches) {
		if (switch0->getColor() != lastActivatedSwitchColor || switch0->getGroup() == 0)
			continue;
		queueParticle(
			switch0->getSwitchWavesCenterX(),
			switch0->getSwitchWavesCenterY(),
			true,
			{
				newEntityAnimationDelay(initialTicksDelay),
				entityAnimationSpriteAnimationWithDelay(SpriteRegistry::switchWavesShortAnimation),
			},
			ticksTime);
	}
}
void MapState::toggleShowConnections() {
	if (Editor::isActive) {
		//if only tiles were visible, restore everything and turn connections on
		if (editorHideNonTiles) {
			showConnectionsEnabled = true;
			editorHideNonTiles = false;
		//if connections were on, turn them off
		} else if (showConnectionsEnabled)
			showConnectionsEnabled = false;
		//if connections were off, also hide everything other than the tiles
		else
			editorHideNonTiles = true;
	} else {
		showConnectionsEnabled = !showConnectionsEnabled;
		finishedConnectionsTutorial = true;
	}
}
void MapState::renderBelowPlayer(EntityState* camera, float playerWorldGroundY, char playerZ, int ticksTime) {
	glDisable(GL_BLEND);
	//render the map
	//these values are just right so that every tile rendered is at least partially in the window and no tiles are left out
	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	int tileMinX = MathUtils::max(screenLeftWorldX / tileSize, 0);
	int tileMinY = MathUtils::max(screenTopWorldY / tileSize, 0);
	int tileMaxX = MathUtils::min((Config::gameScreenWidth + screenLeftWorldX - 1) / tileSize + 1, width);
	int tileMaxY = MathUtils::min((Config::gameScreenHeight + screenTopWorldY - 1) / tileSize + 1, height);
	char editorSelectedHeight = Editor::isActive ? Editor::getSelectedHeight() : invalidHeight;
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
			if (Editor::isActive && editorSelectedHeight != invalidHeight && editorSelectedHeight != mapHeight)
				SpriteSheet::renderFilledRectangle(
					0.0f, 0.0f, 0.0f, 0.5f, leftX, topY, leftX + (GLint)tileSize, topY + (GLint)tileSize);
		}
	}

	if (Editor::isActive && editorHideNonTiles)
		return;

	//draw rail shadows, rails (that are below the player), and switches
	for (RailState* railState : railStates)
		railState->getRail()->renderShadow(screenLeftWorldX, screenTopWorldY);
	for (RailState* railState : railStates) {
		//guarantee that the rail renders behind the player if it has a lower height than the player
		float effectivePlayerWorldGroundY =
			railState->getRail()->getBaseHeight() <= playerZ ? playerWorldGroundY + height : playerWorldGroundY;
		railState->renderBelowPlayer(screenLeftWorldX, screenTopWorldY, effectivePlayerWorldGroundY);
	}
	for (SwitchState* switchState : switchStates)
		switchState->render(
			screenLeftWorldX,
			screenTopWorldY,
			lastActivatedSwitchColor,
			ticksTime - switchesAnimationFadeInStartTicksTime,
			ticksTime);
	for (ResetSwitchState* resetSwitchState : resetSwitchStates)
		resetSwitchState->render(screenLeftWorldX, screenTopWorldY, ticksTime);

	//draw the radio tower after drawing everything else
	glEnable(GL_BLEND);
	if (Editor::isActive)
		glColor4f(1.0f, 1.0f, 1.0f, 2.0f / 3.0f);
	SpriteRegistry::radioTower->renderSpriteAtScreenPosition(
		0, 0, (GLint)(radioTowerLeftXOffset - screenLeftWorldX), (GLint)(radioTowerTopYOffset - screenTopWorldY));
	if (Editor::isActive)
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	//draw particles below the player
	for (ReferenceCounterHolder<Particle>& particle : particles) {
		if (!particle.get()->getIsAbovePlayer())
			particle.get()->render(camera, ticksTime);
	}
}
void MapState::renderAbovePlayer(EntityState* camera, bool showConnections, int ticksTime) {
	if (Editor::isActive && editorHideNonTiles)
		return;

	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	for (RailState* railState : railStates)
		railState->renderAbovePlayer(screenLeftWorldX, screenTopWorldY);

	if (showConnections) {
		//show movement directions and groups above the player for all rails
		for (RailState* railState : railStates) {
			Rail* rail = railState->getRail();
			if (rail->getGroups().size() == 0)
				continue;
			if (rail->getColor() >= triangleColor)
				railState->renderMovementDirections(screenLeftWorldX, screenTopWorldY);
			rail->renderGroups(screenLeftWorldX, screenTopWorldY);
		}
		for (Switch* switch0 : switches)
			switch0->renderGroup(screenLeftWorldX, screenTopWorldY);
		for (ResetSwitch* resetSwitch : resetSwitches)
			resetSwitch->renderGroups(screenLeftWorldX, screenTopWorldY);
	}

	//draw particles above the player
	glEnable(GL_BLEND);
	float radioWavesR = (radioWavesColor == squareColor || radioWavesColor == sineColor) ? 1.0f : 0.0f;
	float radioWavesG = (radioWavesColor == sawColor || radioWavesColor == sineColor) ? 1.0f : 0.0f;
	float radioWavesB = (radioWavesColor == triangleColor || radioWavesColor == sineColor) ? 1.0f : 0.0f;
	if (radioWavesColor != -1)
		glColor4f(radioWavesR, radioWavesG, radioWavesB, 1.0f);
	for (ReferenceCounterHolder<Particle>& particle : particles) {
		if (particle.get()->getIsAbovePlayer())
			particle.get()->render(camera, ticksTime);
	}

	//draw the waveform graphic if applicable
	if (ticksTime < waveformEndTicksTime && ticksTime > waveformStartTicksTime) {
		float alpha = 0.625f;
		if (ticksTime - waveformStartTicksTime < waveformStartEndBufferTicks)
			alpha *= (float)(ticksTime - waveformStartTicksTime) / (float)waveformStartEndBufferTicks;
		else if (waveformEndTicksTime - ticksTime < waveformStartEndBufferTicks)
			alpha *= (float)(waveformEndTicksTime - ticksTime) / (float)waveformStartEndBufferTicks;
		glColor4f(radioWavesR, radioWavesG, radioWavesB, alpha);
		float animationPeriodCycle =
			(float)(ticksTime - waveformStartTicksTime)
				/ (float)(waveformEndTicksTime - waveformStartTicksTime)
				* waveformAnimationPeriods;
		GLint waveformLeft = (GLint)(antennaCenterWorldX() - screenLeftWorldX - waveformWidth / 2);
		GLint waveformTop =
			(GLint)(antennaCenterWorldY() - screenTopWorldY)
				- (GLint)(SpriteRegistry::radioWaves->getSpriteHeight() / 2 + waveformBottomRadioWavesOffset)
				- (GLint)(waveformHeight + 1);
		GLint lastPointTop = 0;
		GLint lastPointBottom = waveformTop + (GLint)waveformHeight;
		for (int i = 0; i < waveformWidth; i++) {
			//divide by the height because we want one period to be a square
			float basePeriodX = (i + 0.5f) / waveformHeight + animationPeriodCycle;
			GLint pointLeft = (GLint)(waveformLeft + i);
			GLint basePointTop = (GLint)(waveformY(radioWavesColor, fmodf(basePeriodX, 1.0f)) * waveformHeight) + waveformTop;
			GLint pointTop = MathUtils::min(basePointTop, lastPointBottom);
			GLint pointBottom = MathUtils::max(basePointTop + 1, lastPointTop);
			SpriteSheet::renderPreColoredRectangle(pointLeft, pointTop, pointLeft + 1, pointBottom);
			lastPointTop = pointTop;
			lastPointBottom = pointBottom;
		}
		Rail::setSegmentColor(0.0f, radioWavesColor, alpha);
		GLint railBaseTopY = waveformTop - (GLint)SpriteRegistry::rails->getSpriteHeight() / 2;
		SpriteRegistry::rails->renderSpriteAtScreenPosition(
			Rail::Segment::spriteHorizontalIndexHorizontal,
			0,
			waveformLeft - (GLint)(SpriteRegistry::rails->getSpriteWidth() + waveformRailSpacing),
			railBaseTopY + (GLint)(waveformY(radioWavesColor, fmodf(animationPeriodCycle, 1.0f)) * waveformHeight));
		SpriteRegistry::rails->renderSpriteAtScreenPosition(
			Rail::Segment::spriteHorizontalIndexHorizontal,
			0,
			waveformLeft + (GLint)(waveformWidth + waveformRailSpacing),
			railBaseTopY
				+ (GLint)(waveformY(radioWavesColor, fmodf(animationPeriodCycle + waveformAspectRatio, 1.0f))
					* waveformHeight));
	}
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
bool MapState::renderGroupsForRailsToReset(EntityState* camera, short resetSwitchId, int ticksTime) {
	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	ResetSwitch* resetSwitch = resetSwitches[resetSwitchId & railSwitchIndexBitmask];
	bool hasRailsToReset = false;
	for (short railId : *resetSwitch->getAffectedRailIds()) {
		RailState* railState = railStates[railId & railSwitchIndexBitmask];
		if (railState->isInDefaultState())
			continue;
		railState->getRail()->renderGroups(screenLeftWorldX, screenTopWorldY);
		hasRailsToReset = true;
	}
	if (hasRailsToReset)
		resetSwitch->renderGroups(screenLeftWorldX, screenTopWorldY);
	return hasRailsToReset;
}
void MapState::renderGroupsForRailsFromSwitch(EntityState* camera, short switchId, int ticksTime) {
	if (!Config::switchKickIndicator.isOn())
		return;
	Switch* switch0 = switches[switchId & railSwitchIndexBitmask];
	char group = switch0->getGroup();
	char color = switch0->getColor();
	if (group == 0 || lastActivatedSwitchColor < color)
		return;
	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	for (Rail* rail : rails) {
		if (rail->getColor() != color)
			continue;
		for (char railGroup : rail->getGroups()) {
			if (railGroup == group) {
				rail->renderGroups(screenLeftWorldX, screenTopWorldY);
				break;
			}
		}
	}
	switch0->renderGroup(screenLeftWorldX, screenTopWorldY);
}
void MapState::renderTutorials(bool showConnections) {
	if (lastActivatedSwitchColor < 0)
		return;
	if (!finishedMapCameraTutorial)
		renderControlsTutorial(mapCameraTutorialText, { Config::mapCameraKeyBinding.value });
	else if (showConnections && !finishedConnectionsTutorial)
		renderControlsTutorial(showConnectionsTutorialText, { Config::showConnectionsKeyBinding.value });
}
void MapState::renderGroupRect(char group, GLint leftX, GLint topY, GLint rightX, GLint bottomY) {
	GLint midX = (leftX + rightX) / 2;
	//bits 0-2
	SpriteSheet::renderFilledRectangle(
		(float)(group & 1), (float)((group >> 1) & 1), (float)((group >> 2) & 1), 1.0f, leftX, topY, midX, bottomY);
	//bits 3-5
	SpriteSheet::renderFilledRectangle(
		(float)((group >> 3) & 1), (float)((group >> 4) & 1), (float)((group >> 5) & 1), 1.0f, midX, topY, rightX, bottomY);
}
void MapState::renderControlsTutorial(const char* text, vector<SDL_Scancode> keys) {
	Text::render(text, tutorialLeftX, tutorialBaselineY, 1.0f);
	float leftX = tutorialLeftX + Text::getMetrics(text, 1.0f).charactersWidth;
	for (SDL_Scancode keyCode : keys) {
		const char* key = ConfigTypes::KeyBindingSetting::getKeyName(keyCode);
		Text::Metrics keyMetrics = Text::getMetrics(key, 1.0f);
		Text::Metrics keyBackgroundMetrics = Text::getKeyBackgroundMetrics(&keyMetrics);
		Text::renderWithKeyBackground(key, leftX, tutorialBaselineY, 1.0f);
		leftX += keyBackgroundMetrics.charactersWidth + 1;
	}
}
float MapState::waveformY(char color, float periodX) {
	switch (color) {
		case squareColor: return periodX < 0.5f ? 0.0f : 1.0f;
		case triangleColor:
			return periodX < 0.25f ? (0.25f - periodX) * 2.0f
				: periodX < 0.75f ? (periodX - 0.25f) * 2.0f
				: (1.25f - periodX) * 2.0f;
		case sawColor: return periodX < 0.5f ? 0.5f - periodX : 1.5f - periodX;
		default: return (1.0f - sinf(MathUtils::twoPi * periodX)) * 0.5f;
	}
}
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
void MapState::logSwitchKick(short switchId) {
	short switchIndex = switchId & railSwitchIndexBitmask;
	stringstream message;
	message << "  switch " << switchIndex << " (";
	logGroup(switches[switchIndex]->getGroup(), &message);
	message << ")";
	Logger::gameplayLogger.logString(message.str());
}
void MapState::logResetSwitchKick(short resetSwitchId) {
	short resetSwitchIndex = resetSwitchId & railSwitchIndexBitmask;
	stringstream message;
	message << "  reset switch " << resetSwitchIndex;
	Logger::gameplayLogger.logString(message.str());
}
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
void MapState::saveState(ofstream& file) {
	if (lastActivatedSwitchColor >= 0)
		file << lastActivatedSwitchColorFilePrefix << (int)lastActivatedSwitchColor << "\n";
	if (finishedConnectionsTutorial)
		file << finishedConnectionsTutorialFileValue << "\n";
	if (finishedMapCameraTutorial)
		file << finishedMapCameraTutorialFileValue << "\n";
	if (showConnectionsEnabled)
		file << showConnectionsFileValue << "\n";

	//don't save the rail states if we're saving the floor file
	//also write that we unlocked all the switches
	if (Editor::needsGameStateSave) {
		file << lastActivatedSwitchColorFilePrefix << "100\n";
		return;
	}
	for (int i = 0; i < (int)railStates.size(); i++) {
		RailState* railState = railStates[i];
		if (!railState->isInDefaultState())
			file << railOffsetFilePrefix << i << ' '
				<< (int)railState->getTargetTileOffset() << ' ' << (int)railState->getNextMovementDirection() << "\n";
	}
}
bool MapState::loadState(string& line) {
	if (StringUtils::startsWith(line, lastActivatedSwitchColorFilePrefix))
		lastActivatedSwitchColor = (char)atoi(line.c_str() + StringUtils::strlenConst(lastActivatedSwitchColorFilePrefix));
	else if (StringUtils::startsWith(line, finishedConnectionsTutorialFileValue))
		finishedConnectionsTutorial = true;
	else if (StringUtils::startsWith(line, finishedMapCameraTutorialFileValue))
		finishedMapCameraTutorial = true;
	else if (StringUtils::startsWith(line, showConnectionsFileValue))
		showConnectionsEnabled = true;
	else if (StringUtils::startsWith(line, railOffsetFilePrefix)) {
		const char* dataString = line.c_str() + StringUtils::strlenConst(railOffsetFilePrefix);
		int railIndex, tileOffset, movementDirection;
		dataString = StringUtils::parseNextInt(dataString, &railIndex);
		dataString = StringUtils::parseNextInt(dataString, &tileOffset);
		StringUtils::parseNextInt(dataString, &movementDirection);
		railStates[railIndex]->loadState(tileOffset, movementDirection, false);
	} else
		return false;
	return true;
}
void MapState::resetMap() {
	lastActivatedSwitchColor = -1;
	finishedConnectionsTutorial = false;
	finishedMapCameraTutorial = false;
	for (RailState* railState : railStates)
		railState->reset(false);
}
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
bool MapState::editorHasSwitch(char color, char group) {
	for (Switch* switch0 : switches) {
		if (switch0->getColor() == color && switch0->getGroup() == group && !switch0->editorIsDeleted)
			return true;
	}
	return false;
}
void MapState::editorSetSwitch(int leftX, int topY, char color, char group) {
	//a switch occupies a 2x2 square, and must be surrounded by a 1-tile ring of no-swich-or-rail tiles
	if (leftX < 1 || topY < 1 || leftX + 2 >= width || topY + 2 >= height)
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
			if (!tileHasSwitch(checkX, checkY))
				return;

			//there is a switch here but we won't delete it because it isn't exactly the same as the switch we're placing
			short otherRailSwitchId = getRailSwitchId(checkX, checkY);
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
	//we're deleting a switch, remove this group from any matching rails and reset switches
	} else if (newSwitchId == 0) {
		for (Rail* rail : rails) {
			if (rail->getColor() == color)
				rail->editorRemoveGroup(group);
		}
		for (ResetSwitch* resetSwitch : resetSwitches) {
			resetSwitch->editorRemoveSwitchSegment(color, group);
		}
	//we're setting a new switch
	} else {
		//but don't set it if we already have a switch exactly like this one
		if (editorHasSwitch(color, group))
			return;
		switches.push_back(newSwitch(leftX, topY, color, group));
	}

	//go through and set the new switch ID, whether we're adding, moving, or removing a switch
	for (int switchIdY = topY; switchIdY < topY + 2; switchIdY++) {
		for (int switchIdX = leftX; switchIdX < leftX + 2; switchIdX++) {
			railSwitchIds[switchIdY * width + switchIdX] = newSwitchId;
		}
	}
}
void MapState::editorSetRail(int x, int y, char color, char group) {
	//a rail can't go along the edge of the map
	if (x < 1 || y < 1 || x + 1 >= width || y + 1 >= height)
		return;
	//if there is no switch that matches the selected group, don't do anything
	//even group 0 needs a switch, but we expect that switch to already exist
	bool foundMatchingSwitch = false;
	for (Switch* switch0 : switches) {
		if (switch0->getGroup() == group && switch0->getColor() == color && !switch0->editorIsDeleted) {
			foundMatchingSwitch = true;
			break;
		}
	}
	if (!foundMatchingSwitch)
		return;

	//delete a segment from a rail
	if (tileHasRail(x, y)) {
		if (rails[getRailSwitchId(x, y) & railSwitchIndexBitmask]->editorRemoveSegment(x, y, color, group))
			editorSetRailSwitchId(x, y, 0);
		return;
	//delete a segment from a reset switch
	} else if (tileHasResetSwitch(x, y)) {
		if (resetSwitches[getRailSwitchId(x, y) & railSwitchIndexBitmask]->editorRemoveEndSegment(x, y, color, group))
			editorSetRailSwitchId(x, y, 0);
		return;
	}

	//make sure that there is at most 1 rail/reset switch in range of this rail
	short editingRailSwitchId = absentRailSwitchId;
	Rail* editingRail = nullptr;
	ResetSwitch* editingResetSwitch = nullptr;
	for (int checkY = y - 1; checkY <= y + 1; checkY++) {
		for (int checkX = x - 1; checkX <= x + 1; checkX++) {
			//no rail or switch here, keep looking
			if (!tileHasRailOrSwitch(checkX, checkY))
				continue;

			short otherRailSwitchId = getRailSwitchId(checkX, checkY);
			if (editingRailSwitchId == absentRailSwitchId) {
				editingRailSwitchId = otherRailSwitchId;
				if (tileHasRail(checkX, checkY))
					editingRail = rails[editingRailSwitchId & railSwitchIndexBitmask];
				else if (tileHasResetSwitch(checkX, checkY))
					editingResetSwitch = resetSwitches[editingRailSwitchId & railSwitchIndexBitmask];
				//only rails and reset switches can add rail segments
				else
					return;
			//don't attempt to place a rail if there are other rails or switches in range
			} else if (otherRailSwitchId != editingRailSwitchId)
				return;
		}
	}

	//add to a reset switch
	if (editingResetSwitch != nullptr) {
		if (editingResetSwitch->editorAddSegment(x, y, color, group))
			editorSetRailSwitchId(x, y, editingRailSwitchId);
	//add to a rail or create a new rail
	} else {
		//add to a rail
		if (editingRail != nullptr) {
			if (editingRail->editorAddSegment(x, y, color, group, getHeight(x, y)))
				editorSetRailSwitchId(x, y, editingRailSwitchId);
		//create a new rail
		} else {
			editorSetRailSwitchId(x, y, (short)rails.size() | railIdValue);
			editingRail = newRail(x, y, getHeight(x, y), color, 0, 1, 1);
			editingRail->addGroup(group);
			rails.push_back(editingRail);
		}
	}
}
void MapState::editorSetResetSwitch(int x, int bottomY) {
	//a reset switch occupies a 1x2 square, and must be surrounded by a 1-tile ring of no-swich-or-rail tiles
	if (x < 1 || bottomY < 2 || x + 1 >= width || bottomY + 1 >= height)
		return;

	short newResetSwitchId = (short)resetSwitches.size() | resetSwitchIdValue;
	for (int checkY = bottomY - 2; checkY <= bottomY + 1; checkY++) {
		for (int checkX = x - 1; checkX <= x + 1; checkX++) {
			//no rail or switch here, keep looking
			if (!tileHasRailOrSwitch(checkX, checkY))
				continue;

			//this is not a reset switch, we can't place a new rail here
			if (!tileHasResetSwitch(checkX, checkY))
				return;

			short otherRailSwitchId = getRailSwitchId(checkX, checkY);
			ResetSwitch* resetSwitch = resetSwitches[otherRailSwitchId & railSwitchIndexBitmask];
			//we clicked on a reset switch in the same spot as the one we're placing, mark it as deleted and set
			//	newResetSwitchId to 0 so that we can use the regular reset-switch-placing logic to clear the reset switch
			if (checkX == x && checkY == bottomY - 1) {
				//but if it has segments, don't do anything to it
				if (resetSwitch->hasAnySegments())
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
void MapState::editorAdjustRailMovementMagnitude(int x, int y, char magnitudeAdd) {
	if (tileHasRail(x, y))
		rails[getRailSwitchId(x, y) & railSwitchIndexBitmask]->editorAdjustMovementMagnitude(x, y, magnitudeAdd);
}
void MapState::editorToggleRailMovementDirection(int x, int y) {
	if (tileHasRail(x, y))
		rails[getRailSwitchId(x, y) & railSwitchIndexBitmask]->editorToggleMovementDirection();
}
void MapState::editorAdjustRailInitialTileOffset(int x, int y, char tileOffset) {
	if (tileHasRail(x, y))
		rails[getRailSwitchId(x, y) & railSwitchIndexBitmask]->editorAdjustInitialTileOffset(x, y, tileOffset);
}
char MapState::editorGetRailSwitchFloorSaveData(int x, int y) {
	if (tileHasRail(x, y))
		return rails[getRailSwitchId(x, y) & railSwitchIndexBitmask]->editorGetFloorSaveData(x, y);
	else if (tileHasSwitch(x, y))
		return switches[getRailSwitchId(x, y) & railSwitchIndexBitmask]->editorGetFloorSaveData(x, y);
	else if (tileHasResetSwitch(x, y))
		return resetSwitches[getRailSwitchId(x, y) & railSwitchIndexBitmask]->editorGetFloorSaveData(x, y);
	return 0;
}
