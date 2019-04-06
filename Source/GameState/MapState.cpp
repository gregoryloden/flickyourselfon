#include "MapState.h"
#include "Editor/Editor.h"
#include "GameState/EntityState.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

#define newSwitch(leftX, topY, color, group) newWithArgs(MapState::Switch, leftX, topY, color, group)
#define newRailState(rail, position) newWithArgs(MapState::RailState, rail, position)
#define newSwitchState(switch0) newWithArgs(MapState::SwitchState, switch0)

//////////////////////////////// MapState::Rail::Rect ////////////////////////////////
MapState::Rail::Rect::Rect(int pLeftX, int pTopY, int pRightX, int pBottomY)
: leftX(pLeftX)
, topY(pTopY)
, rightX(pRightX)
, bottomY(pBottomY) {
}
MapState::Rail::Rect::~Rect() {}

//////////////////////////////// MapState::Rail ////////////////////////////////
MapState::Rail::Rail(objCounterParametersComma() vector<Rect> pRects, char pColor, vector<int> pGroups)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
rects(pRects)
, color(pColor)
, groups(pGroups) {
}
MapState::Rail::~Rail() {}
//render this rail at its position, clipping it if part of the map
void MapState::Rail::render(int screenLeftWorldX, int screenTopWorldY, float offset) {
	//TODO: render the rail
}
#ifdef EDITOR
	//we're saving this rail to the floor file, get the data we need at this tile
	char MapState::Rail::getFloorSaveData(int x, int y) {
		//TODO: get the data
		return 0;
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
		//group 0 is the turn-on-all-switches switch, don't render it unless we're in the editor
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
			return (color << floorRailSwitchDataShift) | floorSwitchHeadValue;
		//tail byte, write our number
		else if (x == leftX + 1&& y == topY)
			return (group << floorRailSwitchDataShift) | floorSwitchTailValue;
		//no data but still part of this switch
		else
			return floorSwitchTailValue;
	}
#endif

//////////////////////////////// MapState::RailState ////////////////////////////////
MapState::RailState::RailState(objCounterParametersComma() Rail* pRail, char pPosition)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
rail(pRail)
, position(pPosition) {
}
MapState::RailState::~RailState() {
	//don't delete the rail, it's owned by MapState
}
//render the rail
void MapState::RailState::render(int screenLeftWorldX, int screenTopWorldY) {
	//TODO: pass the position
	rail->render(screenLeftWorldX, screenTopWorldY, 0.0f);
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
//render the switch
void MapState::SwitchState::render(int screenLeftWorldX, int screenTopWorldY) {
	switch0->render(screenLeftWorldX, screenTopWorldY, isOn);
}

//////////////////////////////// MapState ////////////////////////////////
const char* MapState::floorFileName = "images/floor.png";
const float MapState::speedPerSecond = 40.0f;
const float MapState::diagonalSpeedPerSecond = MapState::speedPerSecond * sqrt(0.5f);
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
		//TODO: put the initial position of the rail
		railStates.push_back(newRailState(rail, 0));
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

	for (int i = 0; i < totalTiles; i++) {
		tiles[i] = (char)(((pixels[i] & greenMask) >> greenShift) / tileDivisor);
		heights[i] = (char)(((pixels[i] & blueMask) >> blueShift) / heightDivisor);

		//max-height tiles have red bytes set, but we should ignore them
		if (heights[i] == MapState::emptySpaceHeight)
			continue;

		//rail/switch data occupies 2+ red bytes, each byte has bit 0 set
		//head bytes that start a rail/switch have bit 1 set, we only build rails/switches when we get to one of these
		char railSwitchValue = (char)((pixels[i] & redMask) >> redShift);
		if ((railSwitchValue & floorIsRailSwitchOrHeadBitmask) == floorRailSwitchAndHeadValue) {
			//the topmost (and then leftmost) tile is byte 1 (the head byte)
			//for switches, there is only 1 tail byte to the right, byte 2
			//for rails, the rail can extend to the right and/or below, specifying which groups the rail responds to
			//byte 1:
			//	bit 1: 1 (indicates head byte)
			//	bit 2 indicates rail (0) vs switch (1)
			//	bits 3-4: color (R/B/G/W)
			char color = (railSwitchValue & floorRailSwitchColorBitmask) >> floorRailSwitchDataShift;
			//	rail bit 5: vertical starting position
			//byte 2+:
			//	bit 1: 0 (indicates tail byte)
			//	bits 2-7: group number
			if ((railSwitchValue & floorIsSwitchBitmask) != 0) {
				char switchByte2 = (char)((pixels[i + 1] & redMask) >> redShift);
				char group = (switchByte2 & floorRailSwitchGroupBitmask) >> floorRailSwitchDataShift;
				short newSwitchId = (short)switches.size() | switchIdBitmask;
				switches.push_back(newSwitch(i % width, i / width, color, group));
				for (int yOffset = 0; yOffset <= 1; yOffset++) {
					for (int xOffset = 0; xOffset <= 1; xOffset++) {
						railSwitchIds[i + xOffset + yOffset * width] = newSwitchId;
					}
				}
			} else {
				//TODO: load rails
			}
		}
	}

	//go through all the tiles again, any tiles with switches need to be set at the switch height
	for (int i = 0; i < totalTiles; i++) {
		if ((railSwitchIds[i] & switchIdBitmask) != 0)
			heights[i] = switchHeight;
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
	#ifdef EDITOR
		//since the editor can add switches and rails, make sure we update our list to track them
		while (railStates.size() < rails.size())
			//TODO: rail initial position
			railStates.push_back(newRailState(rails[railStates.size()], 0));
		while (switchStates.size() < switches.size())
			switchStates.push_back(newSwitchState(switches[switchStates.size()]));
	#endif
}
//draw the map
void MapState::render(EntityState* camera, int ticksTime) {
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

	//draw the radio tower after drawing the map
	glEnable(GL_BLEND);
	SpriteRegistry::radioTower->renderSpriteAtScreenPosition(
		0, 0, (GLint)(radioTowerLeftXOffset - screenLeftWorldX), (GLint)(radioTowerTopYOffset - screenTopWorldY));

	//draw rails and switches after everything else
	for (RailState* railState : railStates)
		railState->render(screenLeftWorldX, screenTopWorldY);
	for (SwitchState* switchState : switchStates)
		switchState->render(screenLeftWorldX, screenTopWorldY);
}
#ifdef EDITOR
	//set a switch if there's room, or delete a switch if we can
	void MapState::setSwitch(int leftX, int topY, char color, char group) {
		//a switch occupies a 2x2 square, and must be surrounded by a 1-tile ring of no-swich-or-rail tiles
		if (leftX - 1 < 0 || topY - 1 < 0 || leftX + 3 > width || topY + 3 > height)
			return;

		short newSwitchId = (short)switches.size() | switchIdBitmask;
		char newHeight = switchHeight;
		for (int checkY = topY - 1; checkY < topY + 3; checkY++) {
			for (int checkX = leftX - 1; checkX < leftX + 3; checkX++) {
				//no rail or switch here, keep looking
				if (!tileHasRailOrSwitch(checkX, checkY))
					continue;

				//there's a rail or switch here and it doesn't match this switch, we won't place a switch or delete a switch
				if (checkX != leftX || checkY != topY)
					return;

				//this is a rail, not a switch, we can't delete this
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
				newHeight = 0;
				checkY = topY + 3;
				break;
			}
		}

		//we're clear to set the switch (assuming we're not actually deleting a switch)
		if (newSwitchId != 0) {
			//but if we already have a switch for this group, don't set a new one
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
				heights[railSwitchIndex] = newHeight;
			}
		}
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
