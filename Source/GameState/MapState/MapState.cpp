#include "MapState.h"
#include "Audio/Audio.h"
#include "Editor/Editor.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/EntityState.h"
#include "GameState/HintState.h"
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
#include "Util/VectorUtils.h"

#define entityAnimationSpriteAnimationWithDelay(animation) \
	newEntityAnimationSetSpriteAnimation(animation), \
	newEntityAnimationDelay(animation->getTotalTicksDuration())

//////////////////////////////// MapState::PlaneConnection ////////////////////////////////
MapState::PlaneConnection::PlaneConnection(int pToTile, Rail* pRail, int pLevelRailByteMaskDataIndex)
: toTile(pToTile)
, rail(pRail)
, levelRailByteMaskDataIndex(pLevelRailByteMaskDataIndex) {
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
char* MapState::tileBorders = nullptr;
char* MapState::heights = nullptr;
short* MapState::railSwitchIds = nullptr;
short* MapState::planeIds = nullptr;
char* MapState::mapZeroes = nullptr;
vector<Rail*> MapState::rails;
vector<Switch*> MapState::switches;
vector<ResetSwitch*> MapState::resetSwitches;
vector<LevelTypes::Plane*> MapState::planes;
vector<Level*> MapState::levels;
int MapState::mapWidth = 1;
int MapState::mapHeight = 1;
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
, waveformStartTicksTime(0)
, waveformEndTicksTime(0)
, hintState(nullptr)
, renderRailStates() {
	for (int i = 0; i < (int)rails.size(); i++)
		railStates.push_back(newRailState(rails[i]));
	for (Switch* switch0 : switches)
		switchStates.push_back(newSwitchState(switch0));
	for (ResetSwitch* resetSwitch : resetSwitches)
		resetSwitchStates.push_back(newResetSwitchState(resetSwitch));

	//add all the rail states to their appropriate switch states
	vector<SwitchState*> switchStatesByGroupByColor[colorCount];
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
	m->hintState.set(newHintState(&Hint::none, 0));
	return m;
}
pooledReferenceCounterDefineRelease(MapState)
void MapState::prepareReturnToPool() {
	particles.clear();
	hintState.set(nullptr);
}
void MapState::buildMap() {
	SDL_Surface* floor = FileUtils::loadImage(floorFileName);
	mapWidth = floor->w;
	mapHeight = floor->h;
	int totalTiles = mapWidth * mapHeight;
	tiles = new char[totalTiles];
	tileBorders = new char[totalTiles];
	heights = new char[totalTiles];
	railSwitchIds = new short[totalTiles];
	mapZeroes = new char[totalTiles];

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
		tileBorders[i] = 0;
		heights[i] = (char)(((pixels[i] & blueMask) >> blueShift) / heightDivisor);
		railSwitchIds[i] = absentRailSwitchId;
		mapZeroes[i] = 0;
	}

	for (int i = 0; i < totalTiles; i++) {
		char railSwitchValue = (char)((pixels[i] & redMask) >> redShift);

		//rail/switch data occupies 2+ red bytes, each byte has bit 0 set (non-switch/rail bytes have bit 0 unset)
		//head bytes that start a rail/switch have bit 1 set, we only build rails/switches when we get to one of these
		if ((railSwitchValue & floorIsRailSwitchAndHeadBitmask) != floorRailSwitchAndHeadValue)
			continue;
		int headX = i % mapWidth;
		int headY = i / mapWidth;

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
		static constexpr int floorIsSwitchAndResetSwitchBitmask = floorIsSwitchBitmask | floorIsResetSwitchBitmask;
		if (HAS_BITMASK(railSwitchValue, floorIsSwitchAndResetSwitchBitmask)) {
			short newResetSwitchId = (short)resetSwitches.size() | resetSwitchIdValue;
			ResetSwitch* resetSwitch = newResetSwitch(headX, headY);
			resetSwitches.push_back(resetSwitch);
			railSwitchIds[i - mapWidth] = newResetSwitchId;
			railSwitchIds[i] = newResetSwitchId;
			addResetSwitchSegments(pixels, redShift, i - 1, newResetSwitchId, resetSwitch, -1);
			addResetSwitchSegments(pixels, redShift, i + mapWidth, newResetSwitchId, resetSwitch, 0);
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
					railSwitchIds[i + xOffset + yOffset * mapWidth] = newSwitchId;
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
			static constexpr char floorRailInitialTileOffsetPostShiftBitmask = 7;
			static constexpr char floorRailMovementDirectionPostShiftBitmask = 1;
			static constexpr char floorRailMovementMagnitudePostShiftBitmask = 7;
			char initialTileOffset =
				(railSwitchValue >> floorRailInitialTileOffsetDataShift) & floorRailInitialTileOffsetPostShiftBitmask;
			char railByte2PostShift = (char)((pixels[railByte2Index] & redMask) >> (redShift + floorRailByte2DataShift));
			char movementDirection = (railByte2PostShift & floorRailMovementDirectionPostShiftBitmask) * 2 - 1;
			char movementMagnitude = (railByte2PostShift >> 1) & floorRailMovementMagnitudePostShiftBitmask;
			Rail* rail = newRail(headX, headY, heights[i], color, initialTileOffset, movementDirection, movementMagnitude);
			rails.push_back(rail);
			rail->addSegment(railByte2Index % mapWidth, railByte2Index / mapWidth);
			//add all the groups
			int floorRailGroupShiftedShift = redShift + floorRailSwitchGroupDataShift;
			for (int railIndex : railIndices) {
				rail->addSegment(railIndex % mapWidth, railIndex / mapWidth);
				rail->addGroup((char)(pixels[railIndex] >> floorRailGroupShiftedShift) & floorRailSwitchGroupPostShiftBitmask);
			}
			rail->assignRenderBox();
		}
	}

	for (int i = 0; i < totalTiles; i++) {
		int height = heights[i];
		if (height == emptySpaceHeight || height % 2 != 0)
			continue;
		int x = i % mapWidth;
		int y = i / mapWidth;
		int fallY;
		char fallHeight;
		//left is blocked
		if (heights[i - 1] < height && tileFalls(x - 1, y, height, &fallY, &fallHeight) == TileFallResult::Blocked)
			tileBorders[i] |= 1 << (int)SpriteDirection::Left;
		//right is blocked
		if (heights[i + 1] < height && tileFalls(x + 1, y, height, &fallY, &fallHeight) == TileFallResult::Blocked)
			tileBorders[i] |= 1 << (int)SpriteDirection::Right;
		//top is blocked
		if (heights[i - mapWidth] % 2 == 1 && heights[i - mapWidth] < height)
			tileBorders[i] |= 1 << (int)SpriteDirection::Up;
		//bottom is blocked
		if (heights[i + mapWidth] % 2 == 1 && tileFalls(x, y + 1, height, &fallY, &fallHeight) == TileFallResult::Blocked)
			tileBorders[i] |= 1 << (int)SpriteDirection::Down;
	}

	SDL_FreeSurface(floor);

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

	if (Editor::isActive) {
		#ifdef VALIDATE_MAP_TILES
			Editor::validateMapTiles();
		#endif
	} else
		buildLevels();
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
		else if ((pixels[segmentIndex + mapWidth] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
				&& railSwitchIds[segmentIndex + mapWidth] == 0)
			segmentIndex += mapWidth;
		else if ((pixels[segmentIndex - mapWidth] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
				&& railSwitchIds[segmentIndex - mapWidth] == 0)
			segmentIndex -= mapWidth;
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
	int segmentX = firstSegmentIndex % mapWidth;
	int segmentY = firstSegmentIndex / mapWidth;
	resetSwitch->addSegment(segmentX, segmentY, color1, 0, segmentsSection);

	char segmentColor = color1;
	vector<int> railIndices = parseRail(pixels, redShift, firstSegmentIndex, resetSwitchId);
	for (int segmentIndex : railIndices) {
		segmentX = segmentIndex % mapWidth;
		segmentY = segmentIndex / mapWidth;

		char group = (char)((pixels[segmentIndex] >> floorRailSwitchGroupDataShift) & floorRailSwitchGroupPostShiftBitmask);
		if (group == 0)
			segmentColor = color2;
		resetSwitch->addSegment(segmentX, segmentY, segmentColor, group, segmentsSection);
	}
}
void MapState::buildLevels() {
	//initialize the base levels state
	planeIds = new short[mapWidth * mapHeight] {};
	int introAnimationBootTile = introAnimationBootTileY * mapWidth + introAnimationBootTileX;
	Level* activeLevel = newLevel(levels.size() + 1, introAnimationBootTile);
	levels.push_back(activeLevel);
	vector<vector<PlaneConnection>> allPlaneConnections;

	//the first tile we'll check is the tile where the boot starts
	deque<int> tileChecks ({ introAnimationBootTile });
	while (!tileChecks.empty()) {
		int nextTile = tileChecks.front();
		tileChecks.pop_front();
		//skip any plane that's already filled
		if (planeIds[nextTile] != 0)
			continue;

		//this tile is a victory tile - start a new level if this is the last tile we know about
		if (tiles[nextTile] == tilePuzzleEnd) {
			//there are more tiles/planes to check, stick this tile at the end and keep building planes
			if (!tileChecks.empty()) {
				tileChecks.push_back(nextTile);
				continue;
			}
			activeLevel->addVictoryPlane();
			levels.push_back(activeLevel = newLevel(levels.size() + 1, nextTile));
		}
		allPlaneConnections.push_back(vector<PlaneConnection>());
		LevelTypes::Plane* newPlane = buildPlane(nextTile, activeLevel, tileChecks, allPlaneConnections.back());
	}

	//add switches to planes
	vector<PlaneConnectionSwitch> planeConnectionSwitches;
	for (Switch* switch0 : switches) {
		//with the editor, it's possible to have switches which aren't on any plane accessible from the start, so skip those
		//should never happen with an umodified floor file once the game is released
		short planeId = getPlaneId(switch0->getLeftX(), switch0->getTopY());
		if (planeId == 0) {
			stringstream message;
			message << "ERROR: no plane found for switch @ " << switch0->getLeftX() << "," << switch0->getTopY();
			logSwitchDescriptor(switch0, &message);
			Logger::debugLogger.logString(message.str());
			continue;
		}
		LevelTypes::Plane* plane = planes[planeId - 1];
		if (switch0->getGroup() == 0)
			plane->getOwningLevel()->assignRadioTowerSwitch(switch0);
		else
			planeConnectionSwitches.push_back(PlaneConnectionSwitch(switch0, plane, plane->addConnectionSwitch(switch0)));
	}

	//add reset switches to planes
	#ifdef DEBUG
		bool* levelHasResetSwitch = new bool[levels.size()] {};
	#endif
	for (ResetSwitch* resetSwitch : resetSwitches) {
		//with the editor, it's possible to have reset switches which aren't on any plane accessible from the start, so skip
		//	those
		//should never happen with an umodified floor file once the game is released
		short planeId = getPlaneId(resetSwitch->getCenterX(), resetSwitch->getBottomY());
		if (planeId == 0)
			Logger::debugLogger.logString(
				"ERROR: no plane found for reset switch @ "
					+ to_string(resetSwitch->getCenterX()) + "," + to_string(resetSwitch->getBottomY()));
		else {
			Level* level = planes[planeId - 1]->getOwningLevel();
			level->assignResetSwitch(resetSwitch);
			#ifdef DEBUG
				//validate reset switches in levels
				level->validateResetSwitch(resetSwitch);
				levelHasResetSwitch[level->getLevelN() - 1] = true;
			#endif
		}
	}
	#ifdef DEBUG
		//validate all levels have reset switches
		for (Level* level : levels) {
			if (!levelHasResetSwitch[level->getLevelN() - 1])
				Logger::debugLogger.logString("ERROR: level " + to_string(level->getLevelN()) + ": missing reset switch");
		}
		delete[] levelHasResetSwitch;
	#endif


	//organize switch/plane combinations so that we can refer to them when adding rail connections
	vector<PlaneConnectionSwitch*> planeConnectionSwitchesByGroupByColor[colorCount];
	for (PlaneConnectionSwitch& planeConnectionSwitch : planeConnectionSwitches) {
		vector<PlaneConnectionSwitch*>& planeConnectionSwitchesByGroup =
			planeConnectionSwitchesByGroupByColor[planeConnectionSwitch.switch0->getColor()];
		char group = planeConnectionSwitch.switch0->getGroup();
		while ((int)planeConnectionSwitchesByGroup.size() <= group)
			planeConnectionSwitchesByGroup.push_back(nullptr);
		planeConnectionSwitchesByGroup[group] = &planeConnectionSwitch;
	}

	//add direct connections to planes
	for (int planeI = 0; planeI < (int)planes.size(); planeI++) {
		LevelTypes::Plane* fromPlane = planes[planeI];
		vector<PlaneConnection>& fromPlaneConnections = allPlaneConnections[planeI];
		for (int fromPlaneConnectionI = 0; fromPlaneConnectionI < (int)fromPlaneConnections.size(); fromPlaneConnectionI++) {
			PlaneConnection& planeConnection = fromPlaneConnections[fromPlaneConnectionI];
			LevelTypes::Plane* toPlane = planes[planeIds[planeConnection.toTile] - 1];
			//plane-plane connection
			if (planeConnection.rail == nullptr)
				fromPlane->addPlaneConnection(toPlane);
			//we have a new rail - add a connection to it and add the data to all applicable switches
			else if (planeConnection.levelRailByteMaskDataIndex >= 0) {
				LevelTypes::RailByteMaskData* railByteMaskData =
					fromPlane->getOwningLevel()->getRailByteMaskData(planeConnection.levelRailByteMaskDataIndex);
				fromPlane->addRailConnection(toPlane, railByteMaskData, planeConnection.rail);
				vector<PlaneConnectionSwitch*>& planeConnectionSwitchesByGroup =
					planeConnectionSwitchesByGroupByColor[planeConnection.rail->getColor()];
				for (char group : planeConnection.rail->getGroups()) {
					//with the editor, it's possible to have switches which aren't on any plane accessible from the start, so
					//	skip rails for those switches
					//should never happen with an umodified floor file once the game is released
					PlaneConnectionSwitch* planeConnectionSwitch =
						group < (int)planeConnectionSwitchesByGroup.size() ? planeConnectionSwitchesByGroup[group] : nullptr;
					if (planeConnectionSwitch == nullptr) {
						stringstream message;
						message << "ERROR: no switch found for rail c" << (int)planeConnection.rail->getColor() << " ";
						logGroup(group, &message);
						Logger::debugLogger.logString(message.str());
						continue;
					}
					planeConnectionSwitch->plane->addRailConnectionToSwitch(
						railByteMaskData, planeConnectionSwitch->planeConnectionSwitchIndex);
				}
			//add a connection going back to a plane that is already connected to this plane
			//because these PlaneConnections are only added when connecting to an already-existing plane, which has already
			//	added all its connections, we can assume that there is a corresponding reverse rail connection to copy from
			} else
				fromPlane->addReverseRailConnection(toPlane, planeConnection.rail);
		}
	}

	//add extended connections between planes
	for (Level* level : levels)
		level->findMilestonesAndExtendConnections();

	for (Level* level : levels)
		level->logStats();

	//initialize utilities for hints
	Level::setupHintSearchHelpers(levels);
	for (Level* level : levels)
		level->preAllocatePotentialLevelStates();
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
		plane->addTile(tile % mapWidth, tile / mapWidth);
		//planes never extend to the edge of the map, so we don't need to check whether we wrapped around the edge
		int upNeighbor = tile - mapWidth;
		int downNeighbor = tile + mapWidth;
		int neighbors[] { tile - 1, tile + 1, upNeighbor, downNeighbor };

		//check neighboring tiles of interest to add to this plane or a climb/fall plane
		for (int neighbor : neighbors) {
			char neighborHeight = heights[neighbor];
			//check for neighboring tiles at the same height to include in the plane
			if (neighborHeight == planeHeight) {
				if (planeIds[neighbor] == 0) {
					planeTileChecks.push_back(neighbor);
					planeIds[neighbor] = planeId;
				}
				continue;
			}

			//if there's a rail, check if it's the start/end segment
			//if so, don't check for tiles to climb or fall to
			if ((railSwitchIds[tile] & railSwitchIdBitmask) == railIdValue) {
				short railId = railSwitchIds[tile];
				Rail* rail = rails[railId & railSwitchIndexBitmask];
				Rail::Segment* startSegment = rail->getSegment(0);
				Rail::Segment* endSegment = rail->getSegment(rail->getSegmentCount() - 1);
				int startTile = startSegment->y * mapWidth + startSegment->x;
				int endTile = endSegment->y * mapWidth + endSegment->x;
				if (tile == startTile) {
					addRailPlaneConnection(
						endTile, railId, planeConnections, activeLevel, rail, rail->getSegmentCount() - 2, tileChecks);
					continue;
				} else if (tile == endTile) {
					addRailPlaneConnection(startTile, railId, planeConnections, activeLevel, rail, 1, tileChecks);
					continue;
				}
			}

			//check for neighboring tiles to climb to
			if (neighborHeight > planeHeight && neighborHeight != emptySpaceHeight) {
				//for all neighbors but the down neighbor, the tile to climb to is one tile up
				if (neighbor != downNeighbor) {
					neighbor -= mapWidth;
					neighborHeight = heights[neighbor];
				}
				if (neighborHeight != planeHeight + 2)
					continue;
			//check for neighboring tiles to fall to
			} else {
				//for all neighbors but the up neighbor, the tile to climb to is further down
				if (neighbor != upNeighbor) {
					int fallX = neighbor % mapWidth;
					int fallY;
					if (tileFalls(fallX, neighbor / mapWidth, planeHeight, &fallY, nullptr) != TileFallResult::Floor)
						continue;
					neighbor = fallY * mapWidth + fallX;
				} else if (neighborHeight % 2 != 0)
					continue;
			}

			//we have a neighboring tile that we can climb or fall to, but if it belongs to a plane on a previous level, drop it
			if (planeIds[neighbor] != 0 && planes[planeIds[neighbor] - 1]->getOwningLevel() != activeLevel)
				continue;
			planeConnections.push_back(PlaneConnection(neighbor, nullptr, -1));
			tileChecks.push_back(neighbor);
		}
	}
	return plane;
}
void MapState::addRailPlaneConnection(
	int toTile,
	short railId,
	vector<PlaneConnection>& planeConnections,
	Level* activeLevel,
	Rail* rail,
	int adjacentRailSegmentIndex,
	deque<int>& tileChecks)
{
	if (planeIds[toTile] == 0) {
		planeConnections.push_back(PlaneConnection(toTile, rail, activeLevel->trackNextRail(railId, rail)));
		Rail::Segment* toAdjacentSegment = rail->getSegment(adjacentRailSegmentIndex);
		int toAdjacentTile = toTile * 2 - toAdjacentSegment->y * mapWidth - toAdjacentSegment->x;
		if (tiles[toAdjacentTile] == tilePuzzleEnd)
			tileChecks.push_back(toAdjacentTile);
		else
			tileChecks.push_back(toTile);
	//only add reverse rail connections to planes in the same level
	} else if (planes[planeIds[toTile] - 1]->getOwningLevel() == activeLevel)
		planeConnections.push_back(PlaneConnection(toTile, rail, -1));
}
void MapState::deleteMap() {
	delete[] tiles;
	delete[] tileBorders;
	delete[] heights;
	delete[] railSwitchIds;
	delete[] planeIds;
	delete[] mapZeroes;
	for (Rail* rail : rails)
		delete rail;
	rails.clear();
	for (Switch* switch0 : switches)
		delete switch0;
	switches.clear();
	for (ResetSwitch* resetSwitch : resetSwitches)
		delete resetSwitch;
	resetSwitches.clear();
	Level::deleteHelpers();
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
		for (int mapY = 0; mapY < mapHeight; mapY++) {
			for (int mapX = 0; mapX < mapWidth; mapX++) {
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
		for (int mapY = 0; mapY < mapHeight; mapY++) {
			for (int mapX = 0; mapX < mapWidth; mapX++) {
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
MapState::TileFallResult MapState::tileFalls(int x, int y, char initialHeight, int* outFallY, char* outFallHeight) {
	TileFallResult blockedFallResult = TileFallResult::Blocked;
	//start one tile down and look for an eligible floor below our current height
	for (char tileOffset = 1; true; tileOffset++) {
		char fallHeight = getHeight(x, y + tileOffset);
		char targetHeight = initialHeight - tileOffset * 2;
		//an empty tile height is fine...
		if (fallHeight == emptySpaceHeight) {
			//...unless we reached the lowest height, in which case there is no longer a possible fall height
			if (targetHeight <= 0)
				return TileFallResult::Empty;
			blockedFallResult = TileFallResult::Empty;
			continue;
		//this is a cliff face or lower floor, keep looking
		} else if (fallHeight % 2 == 1 || fallHeight < targetHeight)
			continue;
		//the tile is higher than us, we can't fall here
		else if (fallHeight > targetHeight)
			return blockedFallResult;

		//we found a matching floor tile
		*outFallY = y + tileOffset;
		if (outFallHeight != nullptr)
			*outFallHeight = fallHeight;
		return TileFallResult::Floor;
	}
}
KickActionType MapState::getSwitchKickActionType(short switchId) {
	Switch* switch0 = switches[switchId & railSwitchIndexBitmask];
	if (switch0->getGroup() == 0) {
		//we've already triggered this group 0 switch, there's nothing to show
		if (lastActivatedSwitchColor >= switch0->getColor())
			return KickActionType::None;
		switch (switch0->getColor()) {
			case squareColor: return KickActionType::Square;
			case triangleColor: return KickActionType::Triangle;
			case sawColor: return KickActionType::Saw;
			case sineColor: return KickActionType::Sine;
			default: return KickActionType::None;
		}
	} else
		//we can kick the switch if we've already triggered the group 0 switch for this color
		return lastActivatedSwitchColor >= switch0->getColor() ? KickActionType::Switch : KickActionType::NoSwitch;
}
char MapState::horizontalTilesHeight(int lowMapX, int highMapX, int mapY) {
	if (tileHasResetSwitchBody(lowMapX, mapY) || tileHasSwitch(lowMapX, mapY))
		return invalidHeight;
	char foundHeight = getHeight(lowMapX, mapY);
	for (int mapX = lowMapX + 1; mapX <= highMapX; mapX++) {
		if (getHeight(mapX, mapY) != foundHeight || tileHasResetSwitchBody(mapX, mapY) || tileHasSwitch(mapX, mapY))
			return invalidHeight;
	}
	return foundHeight;
}
char MapState::verticalTilesHeight(int mapX, int lowMapY, int highMapY) {
	if (tileHasResetSwitchBody(mapX, lowMapY) || tileHasSwitch(mapX, lowMapY))
		return invalidHeight;
	char foundHeight = getHeight(mapX, lowMapY);
	for (int mapY = lowMapY + 1; mapY <= lowMapY; lowMapY++) {
		if (getHeight(mapX, mapY) != foundHeight || tileHasResetSwitchBody(mapX, mapY) || tileHasSwitch(mapX, mapY))
			return invalidHeight;
	}
	return foundHeight;
}
void MapState::setIntroAnimationBootTile(bool showBootTile) {
	//if we're not showing the boot tile, just show a default tile instead of showing the tile from the floor file
	tiles[introAnimationBootTileY * mapWidth + introAnimationBootTileX] = showBootTile ? tileBoot : tileFloorFirst;
}
void MapState::getLevelStartPosition(int levelN, int* outMapX, int* outMapY, char* outZ) {
	int startTile = levels[levelN - 1]->getStartTile();
	*outMapX = startTile % mapWidth;
	*outMapY = startTile / mapWidth;
	*outZ = getHeight(*outMapX, *outMapY);
}
void MapState::updateWithPreviousMapState(MapState* prev, int ticksTime) {
	lastActivatedSwitchColor = prev->lastActivatedSwitchColor;
	showConnectionsEnabled = prev->showConnectionsEnabled;
	finishedConnectionsTutorial = prev->finishedConnectionsTutorial;
	finishedMapCameraTutorial = prev->finishedMapCameraTutorial;
	shouldPlayRadioTowerAnimation = false;
	switchesAnimationFadeInStartTicksTime = prev->switchesAnimationFadeInStartTicksTime;
	waveformStartTicksTime = prev->waveformStartTicksTime;
	waveformEndTicksTime = prev->waveformEndTicksTime;
	hintState.set(prev->hintState.get());

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
		particles.push_back(newParticle(0, 0, 1, 1, 1, false));
	while (particles.size() > prev->particles.size())
		particles.pop_back();
	for (int i = (int)particles.size() - 1; i >= 0; i--) {
		if (!particles[i].get()->updateWithPreviousParticle(prev->particles[i].get(), ticksTime))
			particles.erase(particles.begin() + i);
	}
}
Particle* MapState::queueParticle(
	float centerX,
	float centerY,
	float r,
	float g,
	float b,
	bool isAbovePlayer,
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> components,
	int ticksTime)
{
	Particle* particle = newParticle(centerX, centerY, r, g, b, isAbovePlayer);
	particle->beginEntityAnimation(&components, ticksTime);
	particles.push_back(particle);
	return particle;
}
Particle* MapState::queueParticleWithWaveColor(
	float centerX,
	float centerY,
	char color,
	bool isAbovePlayer,
	vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> components,
	int ticksTime)
{
	float r = (color == squareColor || color == sineColor) ? 1.0f : 0.0f;
	float g = (color == sawColor || color == sineColor) ? 1.0f : 0.0f;
	float b = (color == triangleColor || color == sineColor) ? 1.0f : 0.0f;
	return queueParticle(centerX, centerY, r, g, b, isAbovePlayer, components, ticksTime);
}
void MapState::queueEndSegmentParticles(Rail* rail, int ticksTime) {
	Rail::Segment* segments[] { rail->getSegment(0), rail->getSegment(rail->getSegmentCount() - 1) };
	for (Rail::Segment* segment : segments)
		queueParticleWithWaveColor(
			segment->tileCenterX(),
			segment->tileCenterY(),
			rail->getColor(),
			true,
			{
				newEntityAnimationDelay(SpriteRegistry::radioWaveAnimationTicksPerFrame),
				entityAnimationSpriteAnimationWithDelay(SpriteRegistry::railWavesAnimation),
			},
			ticksTime);
}
void MapState::flipSwitch(short switchId, bool moveRailsForward, bool allowRadioTowerAnimation, int ticksTime) {
	SwitchState* switchState = switchStates[switchId & railSwitchIndexBitmask];
	Switch* switch0 = switchState->getSwitch();
	char switchColor = switch0->getColor();
	//this is a turn-on-other-switches switch, flip it if we haven't done so already
	if (switch0->getGroup() == 0) {
		if (lastActivatedSwitchColor < switchColor) {
			lastActivatedSwitchColor = switchColor;
			if (allowRadioTowerAnimation)
				shouldPlayRadioTowerAnimation = true;
		}
	//this is just a regular switch and we've turned on the parent switch, flip it
	} else if (lastActivatedSwitchColor >= switchColor) {
		switchState->flip(moveRailsForward, ticksTime);

		queueParticleWithWaveColor(
			switch0->getSwitchWavesCenterX(),
			switch0->getSwitchWavesCenterY(),
			switchColor,
			true,
			{
				entityAnimationSpriteAnimationWithDelay(SpriteRegistry::switchWavesAnimation),
			},
			ticksTime);

		for (RailState* railState : *switchState->getConnectedRailStates())
			queueEndSegmentParticles(railState->getRail(), ticksTime);

		Audio::railSwitchWavesSounds[switchColor]->play(0);
	}
}
void MapState::flipResetSwitch(short resetSwitchId, KickResetSwitchUndoState* kickResetSwitchUndoState, int ticksTime) {
	ResetSwitchState* resetSwitchState = resetSwitchStates[resetSwitchId & railSwitchIndexBitmask];
	int maxResetRailColor = -1;
	if (kickResetSwitchUndoState != nullptr) {
		for (KickResetSwitchUndoState::RailUndoState& railUndoState : *kickResetSwitchUndoState->getRailUndoStates()) {
			RailState* railState = railStates[railUndoState.railId & railSwitchIndexBitmask];
			if (railState->loadState(railUndoState.fromTargetTileOffset, railUndoState.fromMovementDirection, true)) {
				Rail* rail = railState->getRail();
				queueEndSegmentParticles(rail, ticksTime);
				maxResetRailColor = MathUtils::max(maxResetRailColor, rail->getColor());
			}
		}
	} else {
		for (short railId : *resetSwitchState->getResetSwitch()->getAffectedRailIds()) {
			RailState* railState = railStates[railId & railSwitchIndexBitmask];
			if (railState->reset(true)) {
				Rail* rail = railState->getRail();
				queueEndSegmentParticles(rail, ticksTime);
				maxResetRailColor = MathUtils::max(maxResetRailColor, rail->getColor());
			}
		}
	}
	resetSwitchState->flip(ticksTime);
	if (maxResetRailColor >= 0) {
		queueParticleWithWaveColor(
			(float)(resetSwitchState->getResetSwitch()->getCenterX() * tileSize + halfTileSize),
			(float)((resetSwitchState->getResetSwitch()->getBottomY() - 1) * tileSize),
			maxResetRailColor,
			true,
			{
				entityAnimationSpriteAnimationWithDelay(SpriteRegistry::railWavesAnimation),
			},
			ticksTime);
		Audio::resetSwitchWavesSounds[maxResetRailColor]->play(0);
	}
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
	static constexpr int interRadioWavesAnimationTicks = 1500;
	Particle* particle = queueParticleWithWaveColor(
		antennaCenterWorldX(),
		antennaCenterWorldY(),
		lastActivatedSwitchColor,
		true,
		{
			newEntityAnimationDelay(initialTicksDelay),
			newEntityAnimationPlaySound(Audio::radioWavesSounds[lastActivatedSwitchColor], 0),
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
		queueParticleWithWaveColor(
			switch0->getSwitchWavesCenterX(),
			switch0->getSwitchWavesCenterY(),
			lastActivatedSwitchColor,
			true,
			{
				newEntityAnimationDelay(initialTicksDelay),
				entityAnimationSpriteAnimationWithDelay(SpriteRegistry::switchWavesShortAnimation),
			},
			ticksTime);
	}
}
void MapState::spawnGoalSparks(int levelN, int ticksTime) {
	char ignoreZ;
	int tileX, tileY;
	getLevelStartPosition(levelN, &tileX, &tileY, &ignoreZ);
	SpriteDirection directions[] =
		{ SpriteDirection::Right, SpriteDirection::Up, SpriteDirection::Left, SpriteDirection::Down };
	for (SpriteDirection direction : directions) {
		SpriteAnimation* animations[] { SpriteRegistry::sparksSlowAnimationA, SpriteRegistry::sparksSlowAnimationA };
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
			queueParticle(
				sparkX,
				sparkY,
				1,
				1,
				1,
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
Hint* MapState::generateHint(float playerX, float playerY) {
	if (Editor::isActive)
		return &Hint::none;
	//with noclip or editing the save file, it's possible to be somewhere that isn't a plane accessible from the start, so don't
	//	try to generate a hint
	//should never happen with an umodified save file once the game is released
	short planeId = getPlaneId((int)playerX / tileSize, (int)playerY / tileSize);
	if (planeId == 0) {
		Logger::debugLogger.logString(
			"ERROR: no plane found to generate hint at " + to_string(playerX) + "," + to_string(playerY));
		return &Hint::none;
	}
	LevelTypes::Plane* currentPlane = planes[planeId - 1];
	return currentPlane->getOwningLevel()->generateHint(
		currentPlane,
		[this](short railId, Rail* rail, char* outMovementDirection, char* outTileOffset) {
			RailState* railState = railStates[railId & railSwitchIndexBitmask];
			*outMovementDirection = railState->getNextMovementDirection();
			*outTileOffset = railState->getTargetTileOffset();
		},
		lastActivatedSwitchColor);
}
void MapState::setHint(Hint* hint, int ticksTime) {
	hintState.set(newHintState(hint, ticksTime));
}
bool MapState::requestsHint() {
	return hintState.get()->getHintType() == Hint::Type::CalculatingHint;
}
int MapState::getLevelN(float playerX, float playerY) {
	if (Editor::isActive)
		return 0;
	//don't worry if the player is outside of a level plane (whether due to the intro animation, noclip, or a modified save
	//	file)
	short planeId = getPlaneId((int)playerX / tileSize, (int)playerY / tileSize);
	return planeId == 0 ? 0 : planes[planeId - 1]->getOwningLevel()->getLevelN();
}
void MapState::renderBelowPlayer(EntityState* camera, float playerWorldGroundY, char playerZ, int ticksTime) {
	glDisable(GL_BLEND);
	//render the map
	//these values are just right so that every tile rendered is at least partially in the window and no tiles are left out
	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	int tileMinX = MathUtils::max(screenLeftWorldX / tileSize, 0);
	int tileMinY = MathUtils::max(screenTopWorldY / tileSize, 0);
	int tileMaxX = MathUtils::min((Config::gameScreenWidth + screenLeftWorldX - 1) / tileSize + 1, mapWidth);
	int tileMaxY = MathUtils::min((Config::gameScreenHeight + screenTopWorldY - 1) / tileSize + 1, mapHeight);
	char* useTileBorders = Config::showBlockedFallEdges.isOn() ? tileBorders : mapZeroes;
	for (int y = tileMinY; y < tileMaxY; y++) {
		for (int x = tileMinX; x < tileMaxX; x++) {
			//consider any tile at the max height to be filler
			int mapIndex = y * mapWidth + x;
			if (heights[mapIndex] == emptySpaceHeight)
				continue;

			GLint leftX = (GLint)(x * tileSize - screenLeftWorldX);
			GLint topY = (GLint)(y * tileSize - screenTopWorldY);
			SpriteRegistry::tiles->renderSpriteAtScreenPosition(
				(int)(tiles[mapIndex]), (int)(useTileBorders[mapIndex]), leftX, topY);
		}
	}
	glEnable(GL_BLEND);

	if (Editor::isActive) {
		//darken tiles that don't match the selected height in the editor, if one is selected
		char editorSelectedHeight = Editor::getSelectedHeight();
		if (editorSelectedHeight != invalidHeight) {
			for (int y = tileMinY; y < tileMaxY; y++) {
				for (int x = tileMinX; x < tileMaxX; x++) {
					//consider any tile at the max height to be filler
					char mapHeight = heights[y * mapWidth + x];
					if (mapHeight == emptySpaceHeight || mapHeight == editorSelectedHeight)
						continue;

					GLint leftX = (GLint)(x * tileSize - screenLeftWorldX);
					GLint topY = (GLint)(y * tileSize - screenTopWorldY);
					SpriteSheet::renderFilledRectangle(
						0.0f, 0.0f, 0.0f, 0.5f, leftX, topY, leftX + (GLint)tileSize, topY + (GLint)tileSize);
				}
			}
		}

		//stop here if we only render tiles
		if (editorHideNonTiles)
			return;
	//color tiles that are above or below the player, based on how far above or below the player they are, if the setting is
	//	enabled
	} else if (Config::heightBasedShading.state != Config::heightBasedShadingOffValue) {
		static constexpr int nearHeightsEnd = 4;
		//even distribution from 0%-50% across floor height differences 0-7
		float maxAlpha = 0.5f;
		float nearHeightsMaxAlpha = maxAlpha * nearHeightsEnd / highestFloorHeight;
		if (Config::heightBasedShading.state == Config::heightBasedShadingExtraValue) {
			//distribute 0%-25% across floor height differences 0-n, and 25%-62.5% across floor height differences n-7
			nearHeightsMaxAlpha = 0.25f;
			maxAlpha = 0.625f;
		}
		float nearHeightsMultiplier = nearHeightsMaxAlpha / nearHeightsEnd;
		float farHeightsMultiplier = (maxAlpha - nearHeightsMaxAlpha) / (highestFloorHeight - nearHeightsEnd);
		for (int y = tileMinY; y < tileMaxY; y++) {
			for (int x = tileMinX; x < tileMaxX; x++) {
				//consider any tile at the max height to be filler
				char mapHeight = heights[y * mapWidth + x];
				if (mapHeight == emptySpaceHeight || mapHeight == playerZ)
					continue;

				if (mapHeight > playerZ)
					glColor4f(
						1.0f,
						1.0f,
						1.0f,
						mapHeight - playerZ <= nearHeightsEnd
							? (mapHeight - playerZ) * nearHeightsMultiplier
							: (mapHeight - playerZ - nearHeightsEnd) * farHeightsMultiplier + nearHeightsMaxAlpha);
				else
					glColor4f(
						0.0f,
						0.0f,
						0.0f,
						playerZ - mapHeight <= nearHeightsEnd
							? (playerZ - mapHeight) * nearHeightsMultiplier
							: (playerZ - mapHeight - nearHeightsEnd) * farHeightsMultiplier + nearHeightsMaxAlpha);
				GLint leftX = (GLint)(x * tileSize - screenLeftWorldX);
				GLint topY = (GLint)(y * tileSize - screenTopWorldY);
				SpriteSheet::renderPreColoredRectangle(leftX, topY, leftX + (GLint)tileSize, topY + (GLint)tileSize);
			}
		}
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	//draw the radio tower immediately after drawing and coloring the tiles
	if (Editor::isActive)
		glColor4f(1.0f, 1.0f, 1.0f, 2.0f / 3.0f);
	SpriteRegistry::radioTower->renderSpriteAtScreenPosition(
		0, 0, (GLint)(radioTowerLeftXOffset - screenLeftWorldX), (GLint)(radioTowerTopYOffset - screenTopWorldY));
	if (Editor::isActive)
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	//draw hints below rails, if applicable
	Hint::Type hintType = hintState.get()->getHintType();
	if (hintType == Hint::Type::Plane)
		hintState.get()->render(screenLeftWorldX, screenTopWorldY, ticksTime);

	//draw rail shadows, rails (that are below the player), and switches
	renderRailStates.clear();
	for (RailState* railState : railStates) {
		Rail* rail = railState->getRail();
		if (!rail->canRender(tileMinX, tileMinY, tileMaxX, tileMaxY))
			continue;
		rail->renderShadow(screenLeftWorldX, screenTopWorldY);
		renderRailStates.push_back(railState);
	}
	sort(renderRailStates.begin(), renderRailStates.end(), RailState::effectiveHeightsAreAscending);
	for (RailState* railState : renderRailStates) {
		//guarantee that the rail renders behind the player if it has an equal or lower height than the player
		//this is mainly relevant for rail ends
		float effectivePlayerWorldGroundY =
			railState->getRail()->getBaseHeight() <= playerZ ? playerWorldGroundY + mapHeight : playerWorldGroundY;
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

	//draw particles below the player
	for (ReferenceCounterHolder<Particle>& particle : particles) {
		if (!particle.get()->getIsAbovePlayer())
			particle.get()->render(camera, ticksTime);
	}

	//draw hints above rails, if applicable
	if (hintType == Hint::Type::Rail || hintType == Hint::Type::Switch || hintType == Hint::Type::UndoReset)
		hintState.get()->render(screenLeftWorldX, screenTopWorldY, ticksTime);
}
void MapState::renderAbovePlayer(EntityState* camera, bool showConnections, int ticksTime) {
	if (Editor::isActive && editorHideNonTiles)
		return;

	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	for (RailState* railState : renderRailStates)
		railState->renderAbovePlayer(screenLeftWorldX, screenTopWorldY);

	if (showConnections) {
		//show movement directions and groups above the player for all rails
		for (RailState* railState : renderRailStates) {
			Rail* rail = railState->getRail();
			if (rail->getGroups().size() == 0)
				continue;
			railState->renderMovementDirections(screenLeftWorldX, screenTopWorldY);
			rail->renderGroups(screenLeftWorldX, screenTopWorldY);
		}
		for (Switch* switch0 : switches) {
			if (switch0->getGroup() != 0)
				switch0->renderGroup(screenLeftWorldX, screenTopWorldY);
		}
		for (ResetSwitch* resetSwitch : resetSwitches)
			resetSwitch->renderGroups(screenLeftWorldX, screenTopWorldY);
	}

	//draw particles above the player
	glEnable(GL_BLEND);
	for (ReferenceCounterHolder<Particle>& particle : particles) {
		if (particle.get()->getIsAbovePlayer())
			particle.get()->render(camera, ticksTime);
	}

	//draw the waveform graphic if applicable
	if (ticksTime < waveformEndTicksTime && ticksTime > waveformStartTicksTime) {
		//find where we are in the animation
		static constexpr float waveformAnimationPeriods = 2.5f;
		float radioWavesR = (lastActivatedSwitchColor == squareColor || lastActivatedSwitchColor == sineColor) ? 1.0f : 0.0f;
		float radioWavesG = (lastActivatedSwitchColor == sawColor || lastActivatedSwitchColor == sineColor) ? 1.0f : 0.0f;
		float radioWavesB = (lastActivatedSwitchColor == triangleColor || lastActivatedSwitchColor == sineColor) ? 1.0f : 0.0f;
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

		//draw the waveform
		static constexpr float waveformAspectRatio = 2.5f;
		static constexpr int waveformHeight = 18;
		static constexpr int waveformWidth = (int)(waveformHeight * waveformAspectRatio);
		static constexpr int waveformBottomRadioWavesOffset = 4;
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
			GLint basePointTop =
				(GLint)(waveformY(lastActivatedSwitchColor, fmodf(basePeriodX, 1.0f)) * waveformHeight) + waveformTop;
			GLint pointTop = MathUtils::min(basePointTop, lastPointBottom);
			GLint pointBottom = MathUtils::max(basePointTop + 1, lastPointTop);
			SpriteSheet::renderPreColoredRectangle(pointLeft, pointTop, pointLeft + 1, pointBottom);
			lastPointTop = pointTop;
			lastPointBottom = pointBottom;
		}

		//draw the edge rails
		static constexpr int waveformRailSpacing = 1;
		Rail::setSegmentColor(lastActivatedSwitchColor, 1.0f, alpha);
		GLint railBaseTopY = waveformTop - (GLint)SpriteRegistry::rails->getSpriteHeight() / 2;
		SpriteRegistry::rails->renderSpriteAtScreenPosition(
			Rail::Segment::spriteHorizontalIndexHorizontal,
			0,
			waveformLeft - (GLint)(SpriteRegistry::rails->getSpriteWidth() + waveformRailSpacing),
			railBaseTopY + (GLint)(waveformY(lastActivatedSwitchColor, fmodf(animationPeriodCycle, 1.0f)) * waveformHeight));
		SpriteRegistry::rails->renderSpriteAtScreenPosition(
			Rail::Segment::spriteHorizontalIndexHorizontal,
			0,
			waveformLeft + (GLint)(waveformWidth + waveformRailSpacing),
			railBaseTopY
				+ (GLint)(waveformY(lastActivatedSwitchColor, fmodf(animationPeriodCycle + waveformAspectRatio, 1.0f))
					* waveformHeight));
	}
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	Hint::Type hintType = hintState.get()->getHintType();
	if (hintType == Hint::Type::CalculatingHint || hintType == Hint::Type::SearchCanceledEarly)
		hintState.get()->render(screenLeftWorldX, screenTopWorldY, ticksTime);
	else
		hintState.get()->renderOffscreenArrow(screenLeftWorldX, screenTopWorldY);

	if (Config::showActivatedSwitchWaves.isOn()) {
		static constexpr int wavesActivatedEdgeSpacing = 10;
		GLint wavesActivatedX =
			(GLint)(Config::gameScreenWidth - wavesActivatedEdgeSpacing - SpriteRegistry::wavesActivated->getSpriteWidth());
		int wavesActivatedYSpacing = SpriteRegistry::wavesActivated->getSpriteHeight() + 2;
		for (int i = 0; i <= lastActivatedSwitchColor; i++) {
			if (i < lastActivatedSwitchColor || ticksTime >= switchesAnimationFadeInStartTicksTime)
				SpriteRegistry::wavesActivated->renderSpriteAtScreenPosition(
					0, i, wavesActivatedX, wavesActivatedEdgeSpacing + i * wavesActivatedYSpacing);
		}
	}

	#ifdef RENDER_PLANE_IDS
		for (LevelTypes::Plane* plane : planes)
			plane->renderId(screenLeftWorldX, screenTopWorldY);
	#endif
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
		railState->renderMovementDirections(screenLeftWorldX, screenTopWorldY);
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
	for (RailState* railState : renderRailStates) {
		Rail* rail = railState->getRail();
		if (rail->getColor() != color)
			continue;
		for (char railGroup : rail->getGroups()) {
			if (railGroup == group) {
				railState->renderMovementDirections(screenLeftWorldX, screenTopWorldY);
				rail->renderGroups(screenLeftWorldX, screenTopWorldY);
				break;
			}
		}
	}
	switch0->renderGroup(screenLeftWorldX, screenTopWorldY);
}
void MapState::renderGroupsForSwitchesFromRail(EntityState* camera, short railId, int ticksTime) {
	if (!Config::railKickIndicator.isOn())
		return;
	Rail* rail = rails[railId & railSwitchIndexBitmask];
	char color = rail->getColor();
	if (lastActivatedSwitchColor < color || rail->getGroups().empty())
		return;
	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	for (char group : rail->getGroups()) {
		for (Switch* switch0 : switches) {
			if (switch0->getGroup() != group)
				continue;
			switch0->renderGroup(screenLeftWorldX, screenTopWorldY);
			break;
		}
	}
	rail->renderGroups(screenLeftWorldX, screenTopWorldY);
}
bool MapState::renderTutorials(bool showConnections) {
	if (showConnections && !finishedConnectionsTutorial)
		renderControlsTutorial("Show connections: ", { Config::showConnectionsKeyBinding.value });
	else if (!finishedMapCameraTutorial && lastActivatedSwitchColor >= 0)
		renderControlsTutorial("Map camera: ", { Config::mapCameraKeyBinding.value });
	else
		return false;
	return true;
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
float MapState::renderControlsTutorial(const char* text, vector<SDL_Scancode> keys) {
	Text::render(text, tutorialLeftX, tutorialBaselineY, 1.0f);
	float leftX = tutorialLeftX + Text::getMetrics(text, 1.0f).charactersWidth;
	for (SDL_Scancode keyCode : keys) {
		const char* key = ConfigTypes::KeyBindingSetting::getKeyName(keyCode);
		Text::Metrics keyMetrics = Text::getMetrics(key, 1.0f);
		Text::Metrics keyBackgroundMetrics = Text::getKeyBackgroundMetrics(&keyMetrics);
		Text::renderWithKeyBackground(key, leftX, tutorialBaselineY, 1.0f);
		leftX += keyBackgroundMetrics.charactersWidth + 1;
	}
	return leftX;
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
	static constexpr char* groupSegmentColors[] { "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white" };
	for (int i = 0; true; i++) {
		*message << groupSegmentColors[group % 8];
		if (i >= 1)
			return;
		*message << "-";
		group /= 8;
	}
}
void MapState::logSwitchKick(short switchId) {
	short switchIndex = switchId & railSwitchIndexBitmask;
	Switch* switch0 = switches[switchIndex];
	stringstream message;
	message << "  switch " << switchIndex;
	logSwitchDescriptor(switch0, &message);
	Logger::gameplayLogger.logString(message.str());
}
void MapState::logSwitchDescriptor(Switch* switch0, stringstream* message) {
	*message << " (c" << (int)switch0->getColor() << " ";
	logGroup(switch0->getGroup(), message);
	*message << ")";
}
void MapState::logResetSwitchKick(short resetSwitchId) {
	short resetSwitchIndex = resetSwitchId & railSwitchIndexBitmask;
	Logger::gameplayLogger.logString("  reset switch " + to_string(resetSwitchIndex));
}
void MapState::logRailRide(short railId, int playerX, int playerY) {
	short railIndex = railId & railSwitchIndexBitmask;
	Rail* rail = rails[railIndex];
	stringstream message;
	message << "  rail " << railIndex;
	logRailDescriptor(rail, &message);
	message << " " << playerX << " " << playerY;
	Logger::gameplayLogger.logString(message.str());
}
void MapState::logRailDescriptor(Rail* rail, stringstream* message) {
	*message << " (c" << (int)rail->getColor() << " ";
	bool skipComma = true;
	for (char group : rail->getGroups()) {
		if (skipComma)
			skipComma = false;
		else
			*message << ", ";
		logGroup(group, message);
	}
	*message << ")";
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
		file << lastActivatedSwitchColorFilePrefix << (int)(MapState::colorCount - 1) << "\n";
		return;
	}
	for (int i = 0; i < (int)railStates.size(); i++) {
		RailState* railState = railStates[i];
		if (!railState->isInDefaultState())
			file << railStateFilePrefix << i << ' '
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
	else if (StringUtils::startsWith(line, railStateFilePrefix)) {
		const char* dataString = line.c_str() + StringUtils::strlenConst(railStateFilePrefix);
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
	showConnectionsEnabled = false;
	for (RailState* railState : railStates)
		railState->reset(false);
}
void MapState::editorSetAppropriateDefaultFloorTile(int x, int y, char expectedFloorHeight) {
	char height = getHeight(x, y);
	if (height != expectedFloorHeight)
		return;

	char leftHeightComparison = editorCompareAdjacentFloorHeight(x - 1, y, height);
	char rightHeightComparison = editorCompareAdjacentFloorHeight(x + 1, y, height);

	//this tile is higher than the one above it
	char topHeight = getHeight(x, y - 1);
	if (height > topHeight || topHeight == emptySpaceHeight) {
		if (leftHeightComparison > 0)
			editorSetTile(x, y, tilePlatformTopGroundLeftFloor);
		else if (leftHeightComparison < 0)
			editorSetTile(x, y, tilePlatformTopLeftFloor);
		else if (rightHeightComparison > 0)
			editorSetTile(x, y, tilePlatformTopGroundRightFloor);
		else if (rightHeightComparison < 0)
			editorSetTile(x, y, tilePlatformTopRightFloor);
		else
			editorSetTile(x, y, tilePlatformTopFloorFirst);
	//this tile is the same height or lower than the one above it
	} else {
		if (leftHeightComparison > 0)
			editorSetTile(x, y, tileGroundLeftFloorFirst);
		else if (leftHeightComparison < 0)
			editorSetTile(x, y, tilePlatformLeftFloorFirst);
		else if (rightHeightComparison > 0)
			editorSetTile(x, y, tileGroundRightFloorFirst);
		else if (rightHeightComparison < 0)
			editorSetTile(x, y, tilePlatformRightFloorFirst);
		else
			editorSetTile(x, y, tileFloorFirst);
	}
}
char MapState::editorCompareAdjacentFloorHeight(int x, int y, char floorHeight) {
	for (char tileOffset = 0; true; tileOffset++) {
		char otherHeight = getHeight(x, y - (int)tileOffset);
		//empty space is always lower
		if (otherHeight == emptySpaceHeight)
			return -1;
		char expectedHeight = floorHeight + tileOffset * 2;
		//too high to match, which means it's still hiding tiles; keep going
		if (otherHeight > expectedHeight)
			continue;
		//we found a wall tile, lower than the expected floor height
		if (otherHeight % 2 == 1)
			//if it's lower than the original floor height, assume that there are no "hidden" tiles at this height
			//if it's higher than the original floor height, assume that there are "hidden" tiles at this height
			return otherHeight < floorHeight ? -1 : 0;
		//we found a non-hidden floor tile, compare it to the original height, assuming that the floor continued at that height
		return otherHeight - floorHeight;
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
	if (leftX < 1 || topY < 1 || leftX + 2 >= mapWidth || topY + 2 >= mapHeight)
		return;

	short newSwitchId = (short)switches.size() | switchIdValue;
	Switch* moveSwitch = nullptr;
	int moveSwitchX = -1;
	int moveSwitchY = -1;
	Switch* rewriteSwitch = nullptr;
	for (int checkY = topY - 1; checkY <= topY + 2; checkY++) {
		for (int checkX = leftX - 1; checkX <= leftX + 2; checkX++) {
			//no rail or switch here, keep looking
			if (!tileHasRailOrSwitch(checkX, checkY))
				continue;

			//this is not a switch, we can't delete, move, or place a switch here
			if (!tileHasSwitch(checkX, checkY))
				return;

			//there is a switch here, check if it matches the color of the switch we're placing
			short otherRailSwitchId = getRailSwitchId(checkX, checkY);
			Switch* switch0 = switches[otherRailSwitchId & railSwitchIndexBitmask];
			if (switch0->getColor() != color)
				return;

			int moveDist = abs(checkX - leftX) + abs(checkY - topY);
			//we found this switch already, keep going
			if (moveSwitch != nullptr)
				continue;
			//we clicked on a switch in exactly the same position as the one we're placing
			else if (moveDist == 0) {
				//if it's also the same group, mark it as deleted and set newSwitchId to 0 so that we can use the regular
				//	switch-placing logic to clear the switch
				if (switch0->getGroup() == group) {
					switch0->editorIsDeleted = true;
					newSwitchId = 0;
				//if it's a different group, we'll see if we can change its group to the selected group
				} else
					rewriteSwitch = switch0;
				checkY = topY + 3;
				break;
			//different position and different group, the switches don't match so there's nothing to do here
			} else if (switch0->getGroup() != group)
				return;
			//we clicked 1 square adjacent to a switch exactly the same as the one we're placing, save the position and keep
			//	going, in case the new switch position is invalid
			else if (moveDist == 1) {
				moveSwitch = switch0;
				newSwitchId = otherRailSwitchId;
				moveSwitchX = checkX;
				moveSwitchY = checkY;
			//it matches a switch but it's too far, we can't move it or delete it
			} else
				return;
		}
	}

	//we've moving a switch, erase the ID from the old tiles and update the position on the switch
	if (moveSwitch != nullptr) {
		for (int eraseY = moveSwitchY; eraseY < moveSwitchY + 2; eraseY++) {
			for (int eraseX = moveSwitchX; eraseX < moveSwitchX + 2; eraseX++) {
				railSwitchIds[eraseY * mapWidth + eraseX] = 0;
			}
		}
		moveSwitch->editorMoveTo(leftX, topY);
	//we found a switch to change the group of
	} else if (rewriteSwitch != nullptr) {
		//don't change to the group if it's already in use
		if (editorHasSwitch(color, group))
			return;
		char oldGroup = rewriteSwitch->getGroup();
		//rewrite the group on the switch
		rewriteSwitch->editorSetGroup(group);
		//rewrite the group in rails
		for (Rail* rail : rails) {
			if (rail->getColor() != color)
				continue;
			vector<char>& groups = rail->getGroups();
			for (int i = 0; i < (int)groups.size(); i++) {
				if (groups[i] == oldGroup)
					groups[i] = group;
			}
		}
		//rewrite the group in reset switches
		for (ResetSwitch* resetSwitch : resetSwitches)
			resetSwitch->editorRewriteGroup(color, oldGroup, group);
		//don't write a switch ID, we're done
		return;
	//we're deleting a switch, remove this group from any matching rails and reset switches
	} else if (newSwitchId == 0) {
		for (Rail* rail : rails) {
			if (rail->getColor() == color)
				rail->editorRemoveGroup(group);
		}
		for (ResetSwitch* resetSwitch : resetSwitches) {
			int freeX, freeY;
			if (resetSwitch->editorRemoveSwitchSegment(color, group, &freeX, &freeY))
				editorSetRailSwitchId(freeX, freeY, 0);
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
			railSwitchIds[switchIdY * mapWidth + switchIdX] = newSwitchId;
		}
	}
}
void MapState::editorSetRail(int x, int y, char color, char group) {
	//a rail can't go along the edge of the map
	if (x < 1 || y < 1 || x + 1 >= mapWidth || y + 1 >= mapHeight)
		return;
	//if there is no switch that matches the selected group, don't do anything
	//even group 0 needs a switch, but we expect that switch to already exist
	if (!editorHasSwitch(color, group))
		return;

	//delete a segment from a rail
	if (tileHasRail(x, y)) {
		if (rails[getRailSwitchId(x, y) & railSwitchIndexBitmask]->editorRemoveSegment(x, y, color, group))
			editorSetRailSwitchId(x, y, 0);
		return;
	//delete a segment from a reset switch
	} else if (tileHasResetSwitch(x, y)) {
		int freeX, freeY;
		if (resetSwitches[getRailSwitchId(x, y) & railSwitchIndexBitmask]
				->editorRemoveSegment(x, y, color, group, &freeX, &freeY))
			editorSetRailSwitchId(freeX, freeY, 0);
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
			editingRail = newRail(x, y, getHeight(x, y), color, 0, color == sawColor ? -1 : 1, 1);
			editingRail->addGroup(group);
			rails.push_back(editingRail);
		}
	}
}
void MapState::editorSetResetSwitch(int x, int bottomY) {
	//a reset switch occupies a 1x2 square, and must be surrounded by a 1-tile ring of no-swich-or-rail tiles
	if (x < 1 || bottomY < 2 || x + 1 >= mapWidth || bottomY + 1 >= mapHeight)
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

	railSwitchIds[bottomY * mapWidth + x] = newResetSwitchId;
	railSwitchIds[(bottomY - 1) * mapWidth + x] = newResetSwitchId;
	if (newResetSwitchId != 0)
		resetSwitches.push_back(newResetSwitch(x, bottomY));
}
void MapState::editorAdjustRailMovementMagnitude(int x, int y, char magnitudeAdd) {
	if (tileHasRail(x, y))
		rails[getRailSwitchId(x, y) & railSwitchIndexBitmask]->editorAdjustMovementMagnitude(x, y, magnitudeAdd);
}
void MapState::editorToggleRailMovementDirection(int x, int y) {
	if (tileHasRail(x, y))
		rails[getRailSwitchId(x, y) & railSwitchIndexBitmask]->editorToggleMovementDirection(x, y);
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
void MapState::editorRenderTiles(SDL_Renderer* mapRenderer) {
	SpriteRegistry::tiles->withRendererTexture(
		mapRenderer,
		[mapRenderer](SDL_Texture* tilesTexture) {
			for (int mapY = 0; mapY < mapHeight; mapY++) {
				for (int mapX = 0; mapX < mapWidth; mapX++) {
					if (getHeight(mapX, mapY) == emptySpaceHeight)
						continue;

					int destinationX = mapX * tileSize;
					int destinationY = mapY * tileSize;
					SDL_Rect source { (int)getTile(mapX, mapY) * tileSize, 0, tileSize, tileSize };
					SDL_Rect destination { destinationX, destinationY, tileSize, tileSize };
					SDL_RenderCopy(mapRenderer, tilesTexture, &source, &destination);
				}
			}
		});
}
