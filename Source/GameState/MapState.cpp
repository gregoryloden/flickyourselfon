#include "MapState.h"
#include "Editor/Editor.h"
#include "GameState/EntityState.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

#define newRail(x, y, baseHeight, color, initialTileOffset) \
	newWithArgs(MapState::Rail, x, y, baseHeight, color, initialTileOffset)
#define newSwitch(leftX, topY, color, group) newWithArgs(MapState::Switch, leftX, topY, color, group)
#define newRailState(rail) newWithArgs(MapState::RailState, rail)
#define newSwitchState(switch0) newWithArgs(MapState::SwitchState, switch0)

//////////////////////////////// MapState::Rail::Segment ////////////////////////////////
MapState::Rail::Segment::Segment(int pX, int pY, char pMaxTileOffset)
: x(pX)
, y(pY)
//use the bottom end sprite as the default, we'll fix it later
, spriteHorizontalIndex(endSegmentSpriteHorizontalIndex(0, -1))
, maxTileOffset(pMaxTileOffset) {
}
MapState::Rail::Segment::~Segment() {}

//////////////////////////////// MapState::Rail ////////////////////////////////
MapState::Rail::Rail(objCounterParametersComma() int x, int y, char pBaseHeight, char pColor, char pInitialTileOffset)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
baseHeight(pBaseHeight)
, color(pColor)
, segments(new vector<Segment>())
, groups()
, initialTileOffset(pInitialTileOffset)
//give a default tile offset extending to the lowest height
, maxTileOffset(pBaseHeight / 2)
#ifdef EDITOR
	, groupIndexToRender(0)
	, isDeleted(false)
#endif
{
	segments->push_back(Segment(x, y, 0));
}
MapState::Rail::~Rail() {
	delete segments;
}
//get the sprite index based on which direction this end segment extends towards
int MapState::Rail::endSegmentSpriteHorizontalIndex(int xExtents, int yExtents) {
	return yExtents != 0 ? 8 + (1 - yExtents) / 2 : 6 + (1 - xExtents) / 2;
}
//reverse the order of the segments
void MapState::Rail::reverseSegments() {
	vector<Segment>* newSegments = new vector<Segment>();
	for (int i = segments->size() - 1; i >= 0; i--)
		newSegments->push_back((*segments)[i]);
	delete segments;
	segments = newSegments;
}
//add this group to the rail if it does not already contain it
void MapState::Rail::addGroup(char group) {
	if (group == 0)
		return;
	for (int i = 0; i < (int)groups.size(); i++) {
		if (groups[i] == group)
			return;
	}
	groups.push_back(group);
}
//add a segment on this tile to the rail
void MapState::Rail::addSegment(int x, int y) {
	//if we aren't adding at the end, reverse the list before continuing
	Segment* end = &segments->back();
	if (abs(y - end->y) + abs(x - end->x) != 1)
		reverseSegments();

	//find the tile where the shadow should go
	for (char segmentMaxTileOffset = 0; true; segmentMaxTileOffset++) {
		//in the event we actually went past the bottom of the map, don't try to find the tile height,
		//	this segment does not have an offset and we're done searching
		if (y + segmentMaxTileOffset >= height)
			segmentMaxTileOffset = Segment::absentTileOffset;
		else {
			char railGroundHeight = baseHeight - (segmentMaxTileOffset * 2);
			char tileHeight = getHeight(x, y + segmentMaxTileOffset);
			//keep looking if we haven't gone down to the lowest level
			//	and we found an empty space tile or a non-empty-space tile that's too low
			if (tileHeight == emptySpaceHeight || tileHeight < railGroundHeight)
				continue;

			//we ended up on a tile above what would be a ground tile for this rail
			//this is an empty space or a tile that the rail hides behind, mark that this segment does not have an offset
			if (tileHeight > railGroundHeight)
				segmentMaxTileOffset = Segment::absentTileOffset;
		}
		segments->push_back(Segment(x, y, segmentMaxTileOffset));
		break;
	}

	//our newest segment is an end segment, figure out which way it should face
	Segment* lastEnd = &(*segments)[segments->size() - 2];
	end = &segments->back();
	end->spriteHorizontalIndex = endSegmentSpriteHorizontalIndex(lastEnd->x - end->x, lastEnd->y - end->y);

	//our last end segment is now a middle rail, find its sprite and account for its max height offset
	if (segments->size() >= 3) {
		Segment* secondLastEnd = &(*segments)[segments->size() - 3];
		if (secondLastEnd->x == end->x)
			lastEnd->spriteHorizontalIndex = 0;
		else if (secondLastEnd->y == end->y)
			lastEnd->spriteHorizontalIndex = 1;
		else {
			int xExtents = secondLastEnd->x - lastEnd->x + end->x - lastEnd->x;
			int yExtents = secondLastEnd->y - lastEnd->y + end->y - lastEnd->y;
			lastEnd->spriteHorizontalIndex = 3 - yExtents + (1 - xExtents) / 2;
		}
		if (lastEnd->maxTileOffset != Segment::absentTileOffset) {
			maxTileOffset = MathUtils::min(lastEnd->maxTileOffset, maxTileOffset);
			initialTileOffset = MathUtils::min(maxTileOffset, initialTileOffset);
		}
	//we only have 2 segments so the other rail is just the end segment that complements this
	} else
		lastEnd->spriteHorizontalIndex = endSegmentSpriteHorizontalIndex(end->x - lastEnd->x, end->y - lastEnd->y);
}
//render this rail at its position by rendering each segment
void MapState::Rail::render(int screenLeftWorldX, int screenTopWorldY, float tileOffset, bool renderShadow) {
	#ifdef EDITOR
		if (isDeleted)
			return;
		groupIndexToRender = 0;
	#endif
	glEnable(GL_BLEND);

	int lastSegmentIndex = (int)segments->size() - 1;
	for (int i = 0; i <= lastSegmentIndex; i++) {
		Segment& segment = (*segments)[i];
		if (i == 0 || i == lastSegmentIndex) {
			//no shadows for end segments
			if (!renderShadow)
				renderSegment(screenLeftWorldX, screenTopWorldY, 0.0f, segment);
		} else {
			if (!renderShadow)
				renderSegment(screenLeftWorldX, screenTopWorldY, tileOffset, segment);
			else if (segment.maxTileOffset > 0)
				SpriteRegistry::rails->renderSpriteAtScreenPosition(
					segment.spriteHorizontalIndex + 10,
					0,
					(GLint)(segment.x * tileSize - screenLeftWorldX),
					(GLint)((segment.y + (int)segment.maxTileOffset) * tileSize - screenTopWorldY));
		}
	}
}
//render the rail segment at its position, clipping it if part of the map is higher than it
void MapState::Rail::renderSegment(int screenLeftWorldX, int screenTopWorldY, float tileOffset, Rail::Segment& segment) {
	int pixelOffset = (int)(tileOffset * (float)tileSize + 0.5f);
	int topWorldY = segment.y * tileSize + pixelOffset;
	int bottomTileY = (topWorldY + tileSize - 1) / tileSize;
	GLint drawLeftX = (GLint)(segment.x * tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(topWorldY - screenTopWorldY);
	char topTileHeight = getHeight(segment.x, topWorldY / tileSize);
	char bottomTileHeight = getHeight(segment.x, bottomTileY);

	#ifdef EDITOR
		if (&segment == &segments->front() || &segment == &segments->back())
			SpriteSheet::renderFilledRectangle(
				(color == 0 || color == 3) ? 0.75f : 0.0f,
				(color == 2 || color == 3) ? 0.75f : 0.0f,
				(color == 1 || color == 3) ? 0.75f : 0.0f,
				0.75f,
				drawLeftX + 1,
				drawTopY + 1,
				drawLeftX + 5,
				drawTopY + 5);
	#endif
	//this segment is completely visible
	if (bottomTileHeight == emptySpaceHeight
			|| bottomTileHeight <= baseHeight - (char)((pixelOffset + tileSize - 1) / tileSize) * 2)
		SpriteRegistry::rails->renderSpriteAtScreenPosition(segment.spriteHorizontalIndex, 0, drawLeftX, drawTopY);
	//the top part of this segment is visible
	else if (topTileHeight == emptySpaceHeight || topTileHeight <= baseHeight - (char)(pixelOffset / tileSize) * 2) {
		int spriteX = segment.spriteHorizontalIndex * tileSize;
		int spriteHeight = bottomTileY * tileSize - topWorldY;
		SpriteRegistry::rails->renderSpriteSheetRegionAtScreenRegion(
			spriteX,
			0,
			spriteX + tileSize,
			spriteHeight,
			drawLeftX,
			drawTopY,
			drawLeftX + (GLint)tileSize,
			drawTopY + (GLint)spriteHeight);
	}
	#ifdef EDITOR
		if (groups.size() > 0) {
			Editor::renderGroupRect(groups[groupIndexToRender], drawLeftX + 2, drawTopY + 2, drawLeftX + 4, drawTopY + 4);
			glEnable(GL_BLEND);
			groupIndexToRender = (groupIndexToRender + 1) % groups.size();
		}
	#endif
}
#ifdef EDITOR
	//remove this group from the rail if it contains it
	void MapState::Rail::removeGroup(char group) {
		for (int i = 0; i < (int)groups.size(); i++) {
			if (groups[i] == group) {
				groups.erase(groups.begin() + i);
				return;
			}
		}
	}
	//remove the segment on this tile from the rail
	void MapState::Rail::removeSegment(int x, int y) {
		Segment& end = segments->back();
		if (y == end.y && x == end.x)
			segments->pop_back();
		else
			segments->erase(segments->begin());
		//reset the max tile offset, find the smallest offset among the non-end segments
		maxTileOffset = baseHeight / 2;
		for (int i = 1; i < (int)segments->size() - 1; i++) {
			Segment& segment = (*segments)[i];
			if (segment.maxTileOffset != Segment::absentTileOffset)
				maxTileOffset = MathUtils::min(segment.maxTileOffset, maxTileOffset);
		}
	}
	//adjust the initial tile offset of this rail if we're clicking on one of its end segments
	void MapState::Rail::adjustInitialTileOffset(int x, int y, char tileOffset) {
		Segment& start = segments->front();
		Segment& end = segments->back();
		if ((x == start.x && y == start.y) || (x == end.x && y == end.y))
			initialTileOffset = MathUtils::max(0, MathUtils::min(maxTileOffset, initialTileOffset + tileOffset));
	}
	//we're saving this rail to the floor file, get the data we need at this tile
	char MapState::Rail::getFloorSaveData(int x, int y) {
		Segment& start = segments->front();
		if (x == start.x && y == start.y)
			return (color << floorRailSwitchColorDataShift)
				| (initialTileOffset << floorRailInitialTileOffsetDataShift)
				| floorRailHeadValue;
		else {
			for (int i = 1; i - 1 < (int)groups.size() && i < (int)segments->size(); i++) {
				Segment& segment = (*segments)[i];
				if (segment.x == x && segment.y == y)
					return groups[i - 1] << floorRailSwitchGroupDataShift | floorIsRailSwitchBitmask;
			}
		}
		//no data but still part of this rail
		return floorIsRailSwitchBitmask;
	}
#endif

//////////////////////////////// MapState::Switch ////////////////////////////////
MapState::Switch::Switch(objCounterParametersComma() int pLeftX, int pTopY, char pColor, char pGroup)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
leftX(pLeftX)
, topY(pTopY)
, color(pColor)
, group(pGroup)
, rails()
#ifdef EDITOR
	, isDeleted(false)
#endif
{
}
MapState::Switch::~Switch() {
	//don't delete the rails, they're owned by MapState
}
//render the switch
void MapState::Switch::render(int screenLeftWorldX, int screenTopWorldY, bool isOn) {
	#ifdef EDITOR
		if (isDeleted)
			return;
	#else
		//group 0 is the turn-on-all-switches switch, don't render it if we're not in the editor
		if (group == 0)
			return;
	#endif

	glEnable(GL_BLEND);
	GLint drawLeftX = (GLint)(leftX * tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(topY * tileSize - screenTopWorldY);
	SpriteRegistry::switches->renderSpriteAtScreenPosition((int)(color * 2 + (isOn ? 1 : 2)), 0, drawLeftX, drawTopY);
	#ifdef EDITOR
		Editor::renderGroupRect(group, drawLeftX + 4, drawTopY + 4, drawLeftX + 8, drawTopY + 8);
	#endif
}
#ifdef EDITOR
	//we're saving this switch to the floor file, get the data we need at this tile
	char MapState::Switch::getFloorSaveData(int x, int y) {
		//head byte, write our color
		if (x == leftX && y == topY)
			return (color << floorRailSwitchColorDataShift) | floorSwitchHeadValue;
		//tail byte, write our number
		else if (x == leftX + 1&& y == topY)
			return (group << floorRailSwitchGroupDataShift) | floorIsRailSwitchBitmask;
		//no data but still part of this switch
		else
			return floorIsRailSwitchBitmask;
	}
#endif

//////////////////////////////// MapState::RailState ////////////////////////////////
MapState::RailState::RailState(objCounterParametersComma() Rail* pRail)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
rail(pRail)
, tileOffset((float)pRail->getInitialTileOffset()) {
}
MapState::RailState::~RailState() {
	//don't delete the rail, it's owned by MapState
}
//check if we need to start/stop moving
void MapState::RailState::updateWithPreviousRailState(RailState* prev, int ticksTime) {
	//TODO: DynamicValue for tile offset
	#ifdef EDITOR
		tileOffset = (float)rail->getInitialTileOffset();
	#endif
}
//render the rail
void MapState::RailState::render(int screenLeftWorldX, int screenTopWorldY, bool renderShadow) {
	rail->render(screenLeftWorldX, screenTopWorldY, tileOffset, renderShadow);
}

//////////////////////////////// MapState::SwitchState ////////////////////////////////
MapState::SwitchState::SwitchState(objCounterParametersComma() Switch* pSwitch0)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
switch0(pSwitch0)
, isOn(false) {
}
MapState::SwitchState::~SwitchState() {
	//don't delete the switch, it's owned by MapState
}
//activate rails if this switch was kicked
void MapState::SwitchState::updateWithPreviousSwitchState(SwitchState* prev, int ticksTime) {
	//TODO: handle getting kicked
}
//render the switch
void MapState::SwitchState::render(int screenLeftWorldX, int screenTopWorldY) {
	switch0->render(screenLeftWorldX, screenTopWorldY, isOn);
}

//////////////////////////////// MapState ////////////////////////////////
const char* MapState::floorFileName = "images/floor.png";
const float MapState::smallDistance = 1.0f / 256.0f;
const float MapState::introAnimationCameraCenterX = (float)(MapState::tileSize * introAnimationBootTileX) + 4.5f;
const float MapState::introAnimationCameraCenterY = (float)(MapState::tileSize * introAnimationBootTileY) - 4.5f;
char* MapState::tiles = nullptr;
char* MapState::heights = nullptr;
short* MapState::railSwitchIds = nullptr;
vector<MapState::Rail*> MapState::rails;
vector<MapState::Switch*> MapState::switches;
int MapState::width = 1;
int MapState::height = 1;
MapState::MapState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, railStates()
, switchStates() {
	for (Rail* rail : rails)
		railStates.push_back(newRailState(rail));
	for (Switch* switch0 : switches)
		switchStates.push_back(newSwitchState(switch0));
}
MapState::~MapState() {
	for (RailState* railState : railStates)
		delete railState;
	for (SwitchState* switchState : switchStates)
		delete switchState;
}
//initialize and return a MapState
MapState* MapState::produce(objCounterParameters()) {
	initializeWithNewFromPool(m, MapState)
	return m;
}
pooledReferenceCounterDefineRelease(MapState)
//load the map and extract all the map data from it
void MapState::buildMap() {
	SDL_Surface* floor = IMG_Load(floorFileName);
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
	}

	for (int i = 0; i < totalTiles; i++) {
		char railSwitchValue = (char)((pixels[i] & redMask) >> redShift);

		//rail/switch data occupies 2+ red bytes, each byte has bit 0 set (non-switch/rail bytes have bit 0 unset)
		//head bytes that start a rail/switch have bit 1 set, we only build rails/switches when we get to one of these
		if ((railSwitchValue & floorIsRailSwitchAndHeadBitmask) != floorRailSwitchAndHeadValue)
			continue;

		//head byte 1:
		//	bit 1: 1 (indicates head byte)
		//	bit 2 indicates rail (0) vs switch (1)
		//	bits 3-4: color (R/B/G/W)
		//	rail bits 5-7: initial tile offset
		//tail byte 2+:
		//	bit 1: 0 (indicates tail byte)
		//	bits 2-7: group number
		char color = (railSwitchValue >> floorRailSwitchColorDataShift) & floorRailSwitchColorPostShiftBitmask;
		//switches have only 2 bytes, head byte 1 at its top-left tile, and tail byte 2 at its top-right tile
		if ((railSwitchValue & floorIsSwitchBitmask) != 0) {
			char switchByte2 = (char)((pixels[i + 1] & redMask) >> redShift);
			char group = (switchByte2 >> floorRailSwitchGroupDataShift) & floorRailSwitchGroupPostShiftBitmask;
			short newSwitchId = (short)switches.size() | switchIdBitmask;
			//add the switch and set all the ids
			switches.push_back(newSwitch(i % width, i / width, color, group));
			for (int yOffset = 0; yOffset <= 1; yOffset++) {
				for (int xOffset = 0; xOffset <= 1; xOffset++) {
					railSwitchIds[i + xOffset + yOffset * width] = newSwitchId;
				}
			}
		//rails can extend in any direction after this head byte tile
		} else {
			short newRailId = (short)rails.size() | railIdBitmask;
			char initialTileOffset =
				(railSwitchValue >> floorRailInitialTileOffsetDataShift) & floorRailInitialTileOffsetPostShiftBitmask;
			Rail* rail = newRail(i % width, i / width, heights[i], color, initialTileOffset);
			rails.push_back(rail);
			railSwitchIds[i] = newRailId;
			int railI = i;
			//cache shift values so that we can iterate the floor data quicker
			int floorIsRailSwitchAndHeadShiftedBitmask = floorIsRailSwitchAndHeadBitmask << redShift;
			int floorRailSwitchTailShiftedValue = floorIsRailSwitchBitmask << redShift;
			int floorRailGroupShiftedShift = redShift + floorRailSwitchGroupDataShift;
			while (true) {
				if ((pixels[railI + 1] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
						&& railSwitchIds[railI + 1] == 0)
					railI++;
				else if ((pixels[railI - 1] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
						&& railSwitchIds[railI - 1] == 0)
					railI--;
				else if ((pixels[railI + width] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
						&& railSwitchIds[railI + width] == 0)
					railI += width;
				else if ((pixels[railI - width] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
						&& railSwitchIds[railI - width] == 0)
					railI -= width;
				else
					break;
				rail->addSegment(railI % width, railI / width);
				rail->addGroup((char)(pixels[railI] >> floorRailGroupShiftedShift) & floorRailSwitchGroupPostShiftBitmask);
				railSwitchIds[railI] = newRailId;
			}
		}
	}

	SDL_FreeSurface(floor);
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
void MapState::setIntroAnimationBootTile(bool startingAnimation) {
	tiles[introAnimationBootTileY * width + introAnimationBootTileX] = startingAnimation ? introAnimationBootTile : 0;
}
//update the rails and switches
void MapState::updateWithPreviousMapState(MapState* prev, int ticksTime) {
	for (int i = 0; i < (int)railStates.size(); i++)
		railStates[i]->updateWithPreviousRailState(prev->railStates[i], ticksTime);
	for (int i = 0; i < (int)switchStates.size(); i++)
		switchStates[i]->updateWithPreviousSwitchState(prev->switchStates[i], ticksTime);
	#ifdef EDITOR
		//since the editor can add switches and rails, make sure we update our list to track them
		while (railStates.size() < rails.size())
			railStates.push_back(newRailState(rails[railStates.size()]));
		while (switchStates.size() < switches.size())
			switchStates.push_back(newSwitchState(switches[switchStates.size()]));
	#endif
}
//draw the map
void MapState::render(EntityState* camera, char playerZ, int ticksTime) {
	glDisable(GL_BLEND);
	//render the map
	//these values are just right so that every tile rendered is at least partially in the window and no tiles are left out
	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	int tileMinX = MathUtils::max(screenLeftWorldX / tileSize, 0);
	int tileMinY = MathUtils::max(screenTopWorldY / tileSize, 0);
	int tileMaxX = MathUtils::min((Config::gameScreenWidth + screenLeftWorldX - 1) / tileSize + 1, width);
	int tileMaxY = MathUtils::min((Config::gameScreenHeight + screenTopWorldY - 1) / tileSize + 1, height);
	#ifdef EDITOR
		char editorSelectedHeight = Editor::getSelectedHeight();
	#endif
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
			#ifdef EDITOR
				if (editorSelectedHeight != -1 && editorSelectedHeight != mapHeight)
					SpriteSheet::renderFilledRectangle(
						0.0f, 0.0f, 0.0f, 0.5f, leftX, topY, leftX + (GLint)tileSize, topY + (GLint)tileSize);
			#endif
		}
	}

	//draw rail shadows, rails (that are below the player, and switches
	for (RailState* railState : railStates)
		railState->render(screenLeftWorldX, screenTopWorldY, true);
	for (RailState* railState : railStates) {
		if (!railState->isAbovePlayerZ(playerZ))
			railState->render(screenLeftWorldX, screenTopWorldY, false);
	}
	for (SwitchState* switchState : switchStates)
		switchState->render(screenLeftWorldX, screenTopWorldY);

	//draw the radio tower after drawing everything else
	glEnable(GL_BLEND);
	SpriteRegistry::radioTower->renderSpriteAtScreenPosition(
		0, 0, (GLint)(radioTowerLeftXOffset - screenLeftWorldX), (GLint)(radioTowerTopYOffset - screenTopWorldY));
}
//draw any rails that are above the player
void MapState::renderRailsAbovePlayer(EntityState* camera, char playerZ, int ticksTime) {
	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	for (RailState* railState : railStates) {
		if (railState->isAbovePlayerZ(playerZ))
			railState->render(screenLeftWorldX, screenTopWorldY, false);
	}
}
#ifdef EDITOR
	//set a switch if there's room, or delete a switch if we can
	void MapState::setSwitch(int leftX, int topY, char color, char group) {
		//a switch occupies a 2x2 square, and must be surrounded by a 1-tile ring of no-swich-or-rail tiles
		if (leftX - 1 < 0 || topY - 1 < 0 || leftX + 3 > width || topY + 3 > height)
			return;

		short newSwitchId = (short)switches.size() | switchIdBitmask;
		for (int checkY = topY - 1; checkY < topY + 3; checkY++) {
			for (int checkX = leftX - 1; checkX < leftX + 3; checkX++) {
				//no rail or switch here, keep looking
				if (!tileHasRailOrSwitch(checkX, checkY))
					continue;

				//there's a rail or switch here and it doesn't match this switch, we won't place a switch or delete a switch
				if (checkX != leftX || checkY != topY)
					return;

				//this is a rail, not a switch, we can't delete this or place a new switch here
				short otherRailSwitchId = getRailSwitchId(checkX, checkY);
				if ((otherRailSwitchId & switchIdBitmask) == 0)
					return;

				//there is a switch here but we won't delete it because it isn't exactly the same as the switch we're placing
				Switch* switch0 = switches[otherRailSwitchId & railSwitchIndexBitmask];
				if (switch0->getColor() != color || switch0->getGroup() != group)
					return;

				//we clicked on a switch exactly the same as the one we're placing, mark it as deleted and set newSwitchId to 0
				//	so that we can use the regular switch-placing logic to clear the switch
				switch0->isDeleted = true;
				newSwitchId = 0;
				checkY = topY + 3;
				break;
			}
		}

		//we're clear to set the switch (assuming we're not actually deleting a switch)
		if (newSwitchId != 0) {
			//but if we already have a switch exactly like this one, don't set a new one
			for (int i = 0; i < (int)switches.size(); i++) {
				Switch* switch0 = switches[i];
				if (switch0->getColor() == color && switch0->getGroup() == group && !switch0->isDeleted)
					return;
			}
			switches.push_back(newSwitch(leftX, topY, color, group));
		}

		//go through and either set the switch to the new one, or reset it since we're deleting one
		for (int switchIdY = topY; switchIdY <= topY + 1; switchIdY++) {
			for (int switchIdX = leftX; switchIdX <= leftX + 1; switchIdX++) {
				int railSwitchIndex = switchIdY * width + switchIdX;
				railSwitchIds[railSwitchIndex] = newSwitchId;
			}
		}
	}
	//set a rail, or delete a rail if we can
	void MapState::setRail(int x, int y, char color, char group) {
		short editingRailId = -1;
		Rail* editingRail = nullptr;
		int orthogonalRails = 0;
		int diagonalRails = 0;
		bool clickedOnRail = false;
		for (int checkY = y - 1; checkY <= y + 1; checkY++) {
			for (int checkX = x - 1; checkX <= x + 1; checkX++) {
				//no rail or switch here, keep looking
				if (!tileHasRailOrSwitch(checkX, checkY))
					continue;

				//this is a switch, not a rail, we can't place a new rail here
				short otherRailSwitchId = getRailSwitchId(checkX, checkY);
				if ((otherRailSwitchId & railIdBitmask) == 0)
					return;

				if (editingRailId == -1) {
					editingRailId = otherRailSwitchId;
					editingRail = rails[otherRailSwitchId & railSwitchIndexBitmask];
				//we saw one rail id before and we found another rail id now, we can't place a rail here
				} else if (editingRailId != otherRailSwitchId)
					return;

				if (checkX == x && checkY == y)
					clickedOnRail = true;
				else if (checkX == x || checkY == y)
					orthogonalRails++;
				else
					diagonalRails++;
			}
		}

		//don't make any changes except at the end of a rail, which will have exactly one orthogonal rail and at most one
		//	diagonal rail, or on an empty space/single rail, which will have no adjacent rails
		bool endOfRail = orthogonalRails == 1 && diagonalRails <= 1;
		bool hasAdjacentRail = orthogonalRails + diagonalRails > 0;
		if (!endOfRail && hasAdjacentRail)
			return;

		//if we have a diagonal rail, make sure our orthogonal rail isn't actually connected to a rail on the other side
		if (diagonalRails == 1
				&& ((tileHasRailOrSwitch(x - 1, y) && tileHasRailOrSwitch(x - 2, y))
					|| (tileHasRailOrSwitch(x + 1, y) && tileHasRailOrSwitch(x + 2, y))
					|| (tileHasRailOrSwitch(x, y - 1) && tileHasRailOrSwitch(x, y - 2))
					|| (tileHasRailOrSwitch(x, y + 1) && tileHasRailOrSwitch(x, y + 2))))
			return;

		//don't modify a rail if it doesn't match the color that we selected
		if (editingRail != nullptr && editingRail->getColor() != color)
			return;

		int railSwitchIndex = y * width + x;

		//this is the end of a rail, find the rail and modify it
		if (endOfRail) {
			//delete this end segment of this rail
			if (clickedOnRail) {
				editingRail->removeGroup(group);
				editingRail->removeSegment(x, y);
				railSwitchIds[railSwitchIndex] = 0;
			//add a segment to the end of this rail
			} else {
				editingRail->addGroup(group);
				editingRail->addSegment(x, y);
				railSwitchIds[railSwitchIndex] = editingRailId;
			}
		//there are no rails around this square, create a new rail, or remove the last segment of this rail
		} else {
			//mark this rail as deleted
			if (clickedOnRail) {
				editingRail->isDeleted = true;
				railSwitchIds[railSwitchIndex] = 0;
			//add a new rail here
			} else {
				railSwitchIds[railSwitchIndex] = (short)rails.size() | railIdBitmask;
				rails.push_back(newRail(x, y, getHeight(x, y), color, 0));
			}
		}
	}
	//adjust the tile offset of the rail here, if there is one
	void MapState::adjustRailInitialTileOffset(int x, int y, char tileOffset) {
		int railSwitchId = getRailSwitchId(x, y);
		if ((railSwitchId & railIdBitmask) != 0)
			rails[railSwitchId & railSwitchIndexBitmask]->adjustInitialTileOffset(x, y, tileOffset);
	}
	//check if we're saving a rail or switch to the floor file, and if so get the data we need at this tile
	char MapState::getRailSwitchFloorSaveData(int x, int y) {
		int railSwitchId = getRailSwitchId(x, y);
		if ((railSwitchId & railIdBitmask) != 0)
			return rails[railSwitchId & railSwitchIndexBitmask]->getFloorSaveData(x, y);
		else if ((railSwitchId & switchIdBitmask) != 0)
			return switches[railSwitchId & railSwitchIndexBitmask]->getFloorSaveData(x, y);
		else
			return 0;
	}
#endif
