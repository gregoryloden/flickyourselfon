#ifndef MAP_STATE_H
#define MAP_STATE_H
#include "Util/PooledReferenceCounter.h"

#define newMapState() produceWithoutArgs(MapState)

class DynamicCameraAnchor;
class EntityState;
class Hint;
class HintState;
class KickResetSwitchUndoState;
class Level;
class Particle;
class Rail;
class RailState;
class ResetSwitch;
class ResetSwitchState;
class Switch;
class SwitchState;
enum class KickActionType: int;
namespace EntityAnimationTypes {
	class Component;
}
namespace LevelTypes {
	class Plane;
}

class MapState: public PooledReferenceCounter {
public:
	enum class TileFallResult: int {
		Floor,
		Empty,
		Blocked,
	};
private:
	//Should only be allocated within an object, on the stack, or as a static object
	class PlaneConnection {
	public:
		int toTile;
		Rail* rail;
		//we need to store an index into the level's RailByteMaskData list when tracking the connection, instead of adding it
		//	when adding the connection to the plane, because the pointers into the list get invalidated as the list grows, since
		//	it stores objects instead of pointers
		int levelRailByteMaskDataIndex;

		PlaneConnection(int pToTile, Rail* pRail, int pLevelRailByteMaskDataIndex);
		virtual ~PlaneConnection();
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class PlaneConnectionSwitch {
	public:
		Switch* switch0;
		LevelTypes::Plane* plane;
		int planeConnectionSwitchIndex;

		PlaneConnectionSwitch(Switch* pSwitch0, LevelTypes::Plane* pPlane, int pPlaneConnectionSwitchIndex);
		virtual ~PlaneConnectionSwitch();
	};

public:
	//map state
	static constexpr int tileCount = 64; // tile = green / 4
	static constexpr int tileDivisor = 256 / tileCount;
	static constexpr int heightCount = 16; // height = blue / 16
	static constexpr int heightDivisor = 256 / heightCount;
	static constexpr int emptySpaceHeight = heightCount - 1;
	static constexpr int highestFloorHeight = heightCount - 2;
	static constexpr int tileSize = 6;
	static constexpr int halfTileSize = tileSize / 2;
	static constexpr int switchSize = 12;
	static constexpr int switchSideInset = 2;
	static constexpr int switchTopInset = 1;
	static constexpr int switchBottomInset = 2;
	static constexpr char invalidHeight = -1;
	//animations
	static constexpr int firstLevelTileOffsetX = 40;
	static constexpr int firstLevelTileOffsetY = 113;
private:
	static constexpr int radioTowerLeftXOffset = 324 + firstLevelTileOffsetX * tileSize;
	static constexpr int radioTowerTopYOffset = -106 + firstLevelTileOffsetY * tileSize;
	static constexpr int introAnimationBootTileX = 29 + firstLevelTileOffsetX;
	static constexpr int introAnimationBootTileY = 26 + firstLevelTileOffsetY;
	static constexpr int waveformStartEndBufferTicks = 500;
public:
	static constexpr int switchesFadeInDuration = 1000;
	static constexpr int switchFlipDuration = 600;
	static constexpr float introAnimationCameraCenterX = (float)(tileSize * introAnimationBootTileX) + 4.5f;
	static constexpr float introAnimationCameraCenterY = (float)(tileSize * introAnimationBootTileY) - 4.5f;
	//tile sections
	static constexpr char tileFloorFirst = 0;
	static constexpr char tileFloorLast = 8;
	static constexpr char tileWallFirst = 9;
	static constexpr char tileWallLast = 14;
	static constexpr char tilePlatformRightFloorFirst = 15;
	static constexpr char tilePlatformRightFloorLast = 18;
	static constexpr char tilePlatformLeftFloorFirst = 19;
	static constexpr char tilePlatformLeftFloorLast = 22;
	static constexpr char tilePlatformTopFloorFirst = 23;
	static constexpr char tilePlatformTopFloorLast = 26;
	static constexpr char tilePlatformTopLeftFloor = 27;
	static constexpr char tilePlatformTopRightFloor = 28;
	static constexpr char tileGroundLeftFloorFirst = 29;
	static constexpr char tileGroundLeftFloorLast = 32;
	static constexpr char tileGroundRightFloorFirst = 33;
	static constexpr char tileGroundRightFloorLast = 36;
	static constexpr char tileBoot = 37;
	static constexpr char tilePuzzleEnd = 38;
	static constexpr char tilePlatformTopGroundLeftFloor = 39;
	static constexpr char tilePlatformTopGroundRightFloor = 40;
	//switch colors
	static constexpr char squareColor = 0;
	static constexpr char triangleColor = 1;
	static constexpr char sawColor = 2;
	static constexpr char sineColor = 3;
	static constexpr char colorCount = 4;
	//rail/switch state serialization
	static constexpr char* floorFileName = "floor.png";
	static constexpr short absentRailSwitchId = 0;
private:
	static constexpr short railSwitchIndexBits = 12;
	static constexpr short railSwitchIdBitmask = 3 << railSwitchIndexBits;
	static constexpr short railIdValue = 1 << railSwitchIndexBits;
	static constexpr short switchIdValue = 2 << railSwitchIndexBits;
	static constexpr short resetSwitchIdValue = 3 << railSwitchIndexBits;
	static constexpr short railSwitchIndexBitmask = (1 << railSwitchIndexBits) - 1;
public:
	//see MapState::buildMap() in MapState.cpp for an explanation of the bitmask layout
	static constexpr int floorIsRailSwitchBitmask = 1;
	static constexpr int floorIsRailSwitchHeadBitmask = 2;
	static constexpr int floorIsRailSwitchAndHeadBitmask = floorIsRailSwitchHeadBitmask | floorIsRailSwitchBitmask;
	static constexpr int floorRailSwitchAndHeadValue = floorIsRailSwitchHeadBitmask | floorIsRailSwitchBitmask;
	static constexpr int floorRailSwitchTailValue = floorIsRailSwitchBitmask;
	static constexpr int floorIsSwitchBitmask = 4;
	static constexpr int floorIsResetSwitchBitmask = 0x20;
	static constexpr char floorRailSwitchColorPostShiftBitmask = 3;
	static constexpr char floorRailSwitchGroupPostShiftBitmask = 0x3F;
	static constexpr int floorRailSwitchColorDataShift = 3;
	static constexpr int floorRailSwitchGroupDataShift = 2;
	static constexpr int floorRailInitialTileOffsetDataShift = 5;
	static constexpr int floorRailByte2DataShift = 2;
	static constexpr int floorRailHeadValue = floorRailSwitchAndHeadValue;
	static constexpr int floorSwitchHeadValue = floorRailSwitchAndHeadValue | floorIsSwitchBitmask;
	static constexpr int floorResetSwitchHeadValue = floorSwitchHeadValue | floorIsResetSwitchBitmask;
private:
	//tutorials and serialization
	static constexpr float tutorialLeftX = 10.0f;
public:
	static constexpr float tutorialBaselineY = 20.0f;
private:
	static constexpr char* lastActivatedSwitchColorFilePrefix = "lastActivatedSwitchColor ";
	static constexpr char* finishedConnectionsTutorialFileValue = "finishedConnectionsTutorial";
	static constexpr char* finishedMapCameraTutorialFileValue = "finishedMapCameraTutorial";
	static constexpr char* showConnectionsFileValue = "showConnections";
	static constexpr char* railStateFilePrefix = "rail ";

	static char* tiles;
	static char* tileBorders;
	static char* heights;
	//bits 0-11 indicate the index in the appropriate rail/switch array, bits 13 and 12 indicate a rail (01), a switch (10), or
	//	a reset switch (11)
	static short* railSwitchIds;
	static short* planeIds;
	static char* mapZeroes;
	static vector<Rail*> rails;
	static vector<Switch*> switches;
	static vector<ResetSwitch*> resetSwitches;
	static vector<LevelTypes::Plane*> planes;
	static vector<Level*> levels;
	static int mapWidth;
	static int mapHeight;
	static bool editorHideNonTiles;

	vector<RailState*> railStates;
	vector<SwitchState*> switchStates;
	vector<ResetSwitchState*> resetSwitchStates;
	char lastActivatedSwitchColor;
	bool showConnectionsEnabled;
	bool finishedConnectionsTutorial;
	bool finishedMapCameraTutorial;
	int switchesAnimationFadeInStartTicksTime;
	bool shouldPlayRadioTowerAnimation;
	vector<ReferenceCounterHolder<Particle>> particles;
	int waveformStartTicksTime;
	int waveformEndTicksTime;
	ReferenceCounterHolder<HintState> hintState;
	vector<RailState*> renderRailStates;

public:
	MapState(objCounterParameters());
	virtual ~MapState();

	static char getTile(int x, int y) { return tiles[y * mapWidth + x]; }
	static char getHeight(int x, int y) { return heights[y * mapWidth + x]; }
	static short getRailSwitchId(int x, int y) { return railSwitchIds[y * mapWidth + x]; }
private:
	static short getPlaneId(int x, int y) { return planeIds[y * mapWidth + x]; }
public:
	static Rail* getRailFromId(int railId) { return rails[railId & railSwitchIndexBitmask]; }
	static short getIdFromRailIndex(short railIndex) { return railIndex | railIdValue; }
	static short getIdFromSwitchIndex(short switchIndex) { return switchIndex | switchIdValue; }
	static short getIdFromResetSwitchIndex(short resetSwitchIndex) { return resetSwitchIndex | switchIdValue; }
	static int getMapWidth() { return mapWidth; }
	static int getMapHeight() { return mapHeight; }
	static bool tileHasRailOrSwitch(int x, int y) { return getRailSwitchId(x, y) != 0; }
	static bool tileHasRail(int x, int y) { return (getRailSwitchId(x, y) & railSwitchIdBitmask) == railIdValue; }
	static bool tileHasSwitch(int x, int y) { return (getRailSwitchId(x, y) & railSwitchIdBitmask) == switchIdValue; }
	static bool tileHasResetSwitch(int x, int y) { return (getRailSwitchId(x, y) & railSwitchIdBitmask) == resetSwitchIdValue; }
	static int getLevelCount() { return (int)levels.size(); }
	RailState* getRailState(int x, int y) { return railStates[getRailSwitchId(x, y) & railSwitchIndexBitmask]; }
	SwitchState* getSwitchState(int x, int y) { return switchStates[getRailSwitchId(x, y) & railSwitchIndexBitmask]; }
	ResetSwitchState* getResetSwitchState(int x, int y) {
		return resetSwitchStates[getRailSwitchId(x, y) & railSwitchIndexBitmask];
	}
	char getLastActivatedSwitchColor() { return lastActivatedSwitchColor; }
	bool getShowConnections(bool showTutorialConnections) {
		return showConnectionsEnabled
			|| (showTutorialConnections && !finishedConnectionsTutorial && lastActivatedSwitchColor >= 0);
	}
	bool getShouldPlayRadioTowerAnimation() { return shouldPlayRadioTowerAnimation; }
	void finishMapCameraTutorial() { finishedMapCameraTutorial = true; }
	static void editorSetTile(int x, int y, char tile) { tiles[y * mapWidth + x] = tile; }
	static void editorSetHeight(int x, int y, char height) { heights[y * mapWidth + x] = height; }
	static void editorSetRailSwitchId(int x, int y, short railSwitchId) { railSwitchIds[y * mapWidth + x] = railSwitchId; }
	//initialize and return a MapState
	static MapState* produce(objCounterParameters());
	//release a reference to this MapState and return it to the pool if applicable
	virtual void release();
protected:
	//release the radio waves states before this is returned to the pool
	virtual void prepareReturnToPool();
public:
	//load the map and extract all the map data from it
	static void buildMap();
private:
	//go along the path and add rail segment indices to a list, and set the given id over those tiles
	//the given rail index is assumed to already have its tile marked, and is not included in the result list
	//returns the indices of the parsed segments
	static vector<int> parseRail(int* pixels, int redShift, int segmentIndex, int railSwitchId);
	//read rail colors and groups from the reset switch segments starting from the given tile, if there is a segment there
	static void addResetSwitchSegments(
		int* pixels, int redShift, int firstSegmentIndex, int resetSwitchId, ResetSwitch* resetSwitch, char segmentsSection);
	//go through the map and figure out which parts of the map belong to which level
	static void buildLevels();
	//breadth-first-search to build a plane
	static LevelTypes::Plane* buildPlane(
		int tile, Level* activeLevel, deque<int>& tileChecks, vector<PlaneConnection>& planeConnections);
	static void addRailPlaneConnection(
		int toTile,
		short railId,
		vector<PlaneConnection>& planeConnections,
		Level* activeLevel,
		Rail* rail,
		int adjacentRailSegmentIndex,
		deque<int>& tileChecks);
public:
	//delete the resources used to handle the map
	static void deleteMap();
	//get the world position of the left edge of the screen using the camera as the center of the screen
	static int getScreenLeftWorldX(EntityState* camera, int ticksTime);
	//get the world position of the top edge of the screen using the camera as the center of the screen
	static int getScreenTopWorldY(EntityState* camera, int ticksTime);
	#ifdef DEBUG
		//find the switch for the given index and write its top left corner
		//does not write map coordinates if it doesn't find any
		static void getSwitchMapTopLeft(short switchIndex, int* outMapLeftX, int* outMapTopY);
		//find the reset switch for the given index and write its top center coordinate
		//does not write map coordinates if it doesn't find any
		static void getResetSwitchMapTopCenter(short resetSwitchIndex, int* outMapCenterX, int* outMapTopY);
	#endif
	//get the center x of the radio tower antenna
	static float antennaCenterWorldX();
	//get the center y of the radio tower antenna
	static float antennaCenterWorldY();
	//get the center y of the radio tower platform
	static float radioTowerPlatformCenterWorldY();
	//check that the tile has the main section of the reset switch, not just one of the rail segments
	static bool tileHasResetSwitchBody(int x, int y);
	//check that the tile has the end segment of a rail
	static bool tileHasRailEnd(int x, int y);
	//check if there is a floor tile that is lower than the given height and corresponds to the world ground Y of the given tile
	//	at the given height, and write it if there is one
	//if there isn't, return why it wasn't (either because there was an empty tile, or because the space was blocked by a hill
	static TileFallResult tileFalls(int x, int y, char initialHeight, int* outFallY, char* outFallHeight);
	//a switch can only be kicked if it's group 0 or if its color is activated
	KickActionType getSwitchKickActionType(short switchId);
	//check the height of all the tiles in the row (indices inclusive), and return it if they're all the same or invalidHeight
	//	if any differ
	static char horizontalTilesHeight(int lowMapX, int highMapX, int mapY);
	//check the height of all the tiles in the column (indices inclusive), and return it if they're all the same or
	//	invalidHeight if any differ
	static char verticalTilesHeight(int mapX, int lowMapY, int highMapY);
	//change one of the tiles to be the boot tile
	static void setIntroAnimationBootTile(bool showBootTile);
	//get the starting position of the given level (1-based)
	static void getLevelStartPosition(int levelN, int* outMapX, int* outMapY, char* outZ);
	//update the rails and switches of the MapState by reading from the previous state
	void updateWithPreviousMapState(MapState* prev, int ticksTime);
	//queue a particle
	//returns the created Particle
	Particle* queueParticle(
		float centerX,
		float centerY,
		float r,
		float g,
		float b,
		bool isAbovePlayer,
		vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> components,
		int ticksTime);
private:
	//queue a particle colored according to the given waveform color
	Particle* queueParticleWithWaveColor(
		float centerX,
		float centerY,
		char color,
		bool isAbovePlayer,
		vector<ReferenceCounterHolder<EntityAnimationTypes::Component>> components,
		int ticksTime);
	//queue particles for the end segments of this rail, after a delay of one animation frame
	void queueEndSegmentParticles(Rail* rail, int ticksTime);
public:
	//flip a switch
	void flipSwitch(short switchId, bool moveRailsForward, bool allowRadioTowerAnimation, int ticksTime);
	//flip a reset switch
	void flipResetSwitch(short resetSwitchId, KickResetSwitchUndoState* kickResetSwitchUndoState, int ticksTime);
	//write the states of all rails affected by the given reset switch into the undo state
	void writeCurrentRailStates(short resetSwitchId, KickResetSwitchUndoState* kickResetSwitchUndoState);
	//begin a radio waves animation
	//returns the duration of the animation that takes place after the initial delay
	int startRadioWavesAnimation(int initialTicksDelay, int ticksTime);
	//set the start of the fade-in animation for the next switch color and show radio waves for them
	void startSwitchesFadeInAnimation(int initialTicksDelay, int ticksTime);
	//spawn sparks over the start tile for the given level, which we just reached
	void spawnGoalSparks(int levelN, int ticksTime);
	//toggle the state of showing connections, and any other relevant state
	void toggleShowConnections();
	//generate a hint based on the state of the map and the given player position
	Hint* generateHint(float playerX, float playerY);
	//set the given hint to be shown
	void setHint(Hint* hint, int ticksTime);
	//returns true if this MapState should receive a new hint this update
	bool requestsHint();
	//get the level for the given position
	int getLevelN(float playerX, float playerY);
	//render the map
	void renderBelowPlayer(EntityState* camera, float playerWorldGroundY, char playerZ, int ticksTime);
	//render anything (rails, groups) that render above the player
	//assumes renderBelowPlayer() has already been called to set the rails above the player
	void renderAbovePlayer(EntityState* camera, bool showConnections, int ticksTime);
	//render the groups for rails that are not in their default position that have a group that this reset switch also has
	//return whether any groups were drawn
	bool renderGroupsForRailsToReset(EntityState* camera, short resetSwitchId, int ticksTime);
	//render the groups for rails that have the group of this switch
	void renderGroupsForRailsFromSwitch(EntityState* camera, short switchId, int ticksTime);
	//render any applicable tutorials
	//returns whether a tutorial was rendered
	bool renderTutorials(bool showConnections);
	//draw a graphic to represent this rail/switch group
	static void renderGroupRect(char group, GLint leftX, GLint topY, GLint rightX, GLint bottomY);
	//render a controls tutorial message
	//returns the x position after the text
	static float renderControlsTutorial(const char* text, vector<SDL_Scancode> keys);
private:
	//get the corresponding waveform value based on which waveform it is, converted to 0 at the top and 1 at the bottom
	//assumes 0 <= periodX < 1
	static float waveformY(char color, float periodX);
public:
	//log the colors of the group to the message
	static void logGroup(char group, stringstream* message);
	//log that the switch was kicked
	static void logSwitchKick(short switchId);
	//log the color and group of a switch
	static void logSwitchDescriptor(Switch* switch0, stringstream* message);
	//log that the reset switch was kicked
	static void logResetSwitchKick(short resetSwitchId);
	//log that the player rode on the rail
	static void logRailRide(short railId, int playerX, int playerY);
	//log the color and groups of a rail
	static void logRailDescriptor(Rail* rail, stringstream* message);
	//save the map state to the file
	void saveState(ofstream& file);
	//try to load state from the line of the file, return whether state was loaded
	bool loadState(string& line);
	//reset the rails and switches
	void resetMap();
	//examine the neighboring tiles and pick an appropriate default tile, but only if we match the expected floor height
	//wall tiles and floor tiles of a different height will be ignored
	static void editorSetAppropriateDefaultFloorTile(int x, int y, char expectedFloorHeight);
	//find the height of the next non-"hidden" tile at the given x and north or equal to the given y, and compare it against the
	//	given height
	//high floor and wall heights "hide" tiles behind them, search for the first tile with a height that isn't "hidden"
	//returns a positive, zero, or negative number if the found tile is higher, equal, or lower than the given height,
	//	respectively
	static char editorCompareAdjacentFloorHeight(int x, int y, char floorHeight);
	//check if there is already a switch with the same color and group
	static bool editorHasSwitch(char color, char group);
	//set a switch if there's room, or delete one if we can
	static void editorSetSwitch(int leftX, int topY, char color, char group);
	//set a rail or reset switch segment, or delete one if we can
	static void editorSetRail(int x, int y, char color, char group);
	//set a reset switch body, or delete one if we can
	static void editorSetResetSwitch(int x, int bottomY);
	//adjust the movement magnitude of the rail here, if there is one
	static void editorAdjustRailMovementMagnitude(int x, int y, char magnitudeAdd);
	//toggle the movement direction of the rail here, if there is one
	static void editorToggleRailMovementDirection(int x, int y);
	//adjust the tile offset of the rail here, if there is one
	static void editorAdjustRailInitialTileOffset(int x, int y, char tileOffset);
	//check if we're saving a rail or switch to the floor file, and if so get the data we need at this tile
	static char editorGetRailSwitchFloorSaveData(int x, int y);
};
#endif
