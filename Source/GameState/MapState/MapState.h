#include "GameState/EntityState.h"

#define newMapState() produceWithoutArgs(MapState)

class Holder_RessetSwitchSegmentVector;
class Rail;
class RailState;
class ResetSwitch;
class ResetSwitchState;
class Switch;
class SwitchState;
enum class KickActionType: int;

class MapState: public PooledReferenceCounter {
public:
	class RadioWavesState: public EntityState {
	public:
		static const int interRadioWavesAnimationTicks = 1000;

	private:
		SpriteAnimation* spriteAnimation;
		int spriteAnimationStartTicksTime;

	public:
		RadioWavesState(objCounterParameters());
		virtual ~RadioWavesState();

		static RadioWavesState* produce(objCounterParameters());
		void copyRadioWavesState(RadioWavesState* other);
		virtual void release();
		virtual void setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime);
		void updateWithPreviousRadioWavesState(RadioWavesState* prev, int ticksTime);
		void render(EntityState* camera, int ticksTime);
		virtual void setNextCamera(GameState* nextGameState, int ticksTime) {}
	};

	//map state
	static const int tileCount = 64; // tile = green / 4
	static const int tileDivisor = 256 / tileCount;
	static const int heightCount = 16; // height = blue / 16
	static const int heightDivisor = 256 / heightCount;
	static const int emptySpaceHeight = heightCount - 1;
	static const int highestFloorHeight = heightCount - 2;
	static const int tileSize = 6;
	static const int switchSize = 12;
	static const int switchSideInset = 2;
	static const int switchTopInset = 1;
	static const int switchBottomInset = 2;
	static const char invalidHeight = -1;
	//animations
	static const int firstLevelTileOffsetX = 40;
	static const int firstLevelTileOffsetY = 63;
	static const int radioTowerLeftXOffset = 324 + firstLevelTileOffsetX * tileSize;
	static const int radioTowerTopYOffset = -106 + firstLevelTileOffsetY * tileSize;
	static const int introAnimationBootTileX = 29 + firstLevelTileOffsetX;
	static const int introAnimationBootTileY = 26 + firstLevelTileOffsetY;
	static const int switchesFadeInDuration = 1000;
	static const int switchFlipDuration = 600;
	//tile sections
	static const char tileFloorFirst = 0;
	static const char tileFloorLast = 8;
	static const char tileWallFirst = 9;
	static const char tileWallLast = 14;
	static const char tilePlatformRightFloorFirst = 15;
	static const char tilePlatformRightFloorLast = 18;
	static const char tilePlatformLeftFloorFirst = 19;
	static const char tilePlatformLeftFloorLast = 22;
	static const char tilePlatformTopFloorFirst = 23;
	static const char tilePlatformTopFloorLast = 26;
	static const char tilePlatformTopLeftFloor = 27;
	static const char tilePlatformTopRightFloor = 28;
	static const char tileGroundLeftFloorFirst = 29;
	static const char tileGroundLeftFloorLast = 32;
	static const char tileGroundRightFloorFirst = 33;
	static const char tileGroundRightFloorLast = 36;
	static const char tileBoot = 37;
	static const char tilePuzzleEnd = 38;
	static const char tilePlatformTopGroundLeftFloor = 39;
	static const char tilePlatformTopGroundRightFloor = 40;
	//switch colors
	static const char squareColor = 0;
	static const char triangleColor = 1;
	static const char sawColor = 2;
	static const char sineColor = 3;
	//rail/switch state serialization
	static const short absentRailSwitchId = 0;
	static const short railSwitchIdBitmask = 3 << 12;
	static const short railIdValue = 1 << 12;
	static const short switchIdValue = 2 << 12;
	static const short resetSwitchIdValue = 3 << 12;
	static const short railSwitchIndexBitmask = railIdValue - 1;
	static const int floorIsRailSwitchBitmask = 1;
	static const int floorIsRailSwitchHeadBitmask = 2;
	static const int floorIsRailSwitchAndHeadBitmask = floorIsRailSwitchHeadBitmask | floorIsRailSwitchBitmask;
	static const int floorRailSwitchAndHeadValue = floorIsRailSwitchHeadBitmask | floorIsRailSwitchBitmask;
	static const int floorRailSwitchTailValue = floorIsRailSwitchBitmask;
	static const int floorIsSwitchBitmask = 4;
	static const int floorIsResetSwitchBitmask = 0x20;
	static const int floorIsSwitchAndResetSwitchBitmask = floorIsSwitchBitmask | floorIsResetSwitchBitmask;
	static const char floorRailSwitchColorPostShiftBitmask = 0x3;
	static const char floorRailSwitchGroupPostShiftBitmask = 0x3F;
	static const char floorRailInitialTileOffsetPostShiftBitmask = 0x7;
	static const char floorRailMovementDirectionPostShiftBitmask = 1;
	static const int floorRailSwitchColorDataShift = 3;
	static const int floorRailSwitchGroupDataShift = 2;
	static const int floorRailInitialTileOffsetDataShift = 5;
	static const int floorRailByte2DataShift = 2;
	static const int floorRailHeadValue = floorRailSwitchAndHeadValue;
	static const int floorSwitchHeadValue = floorRailSwitchAndHeadValue | floorIsSwitchBitmask;
	static const int floorResetSwitchHeadValue = floorSwitchHeadValue | floorIsResetSwitchBitmask;
	//other
	static const char* floorFileName;
	static const float smallDistance;
	static const float introAnimationCameraCenterX;
	static const float introAnimationCameraCenterY;
	static const string railOffsetFilePrefix;
	static const string lastActivatedSwitchColorFilePrefix;
	static const string finishedConnectionsTutorialFilePrefix;

private:
	static char* tiles;
	static char* heights;
	//bits 0-11 indicate the index in the appropriate rail/switch array, bits 13 and 12 indicate a rail (01), a switch (10), or
	//	a reset switch (11)
	static short* railSwitchIds;
	static vector<Rail*> rails;
	static vector<Switch*> switches;
	static vector<ResetSwitch*> resetSwitches;
	static int width;
	static int height;
	static int editorNonTilesHidingState;

	vector<RailState*> railStates;
	vector<RailState*> railStatesByHeight;
	int railsBelowPlayerZ;
	vector<SwitchState*> switchStates;
	vector<ResetSwitchState*> resetSwitchStates;
	char lastActivatedSwitchColor;
	bool finishedConnectionsTutorial;
	int switchesAnimationFadeInStartTicksTime;
	bool shouldPlayRadioTowerAnimation;
	ReferenceCounterHolder<RadioWavesState> radioWavesState;

public:
	MapState(objCounterParameters());
	virtual ~MapState();

	static char getTile(int x, int y) { return tiles[y * width + x]; }
	static char getHeight(int x, int y) { return heights[y * width + x]; }
	static short getRailSwitchId(int x, int y) { return railSwitchIds[y * width + x]; }
	static Rail* getRailFromId(int railId) { return rails[railId & railSwitchIndexBitmask]; }
	static short getIdFromRailIndex(short railIndex) { return railIndex | railIdValue; }
	static short getIdFromSwitchIndex(short switchIndex) { return switchIndex | switchIdValue; }
	static short getIdFromResetSwitchIndex(short resetSwitchIndex) { return resetSwitchIndex | switchIdValue; }
	static int mapWidth() { return width; }
	static int mapHeight() { return height; }
	static bool tileHasRailOrSwitch(int x, int y) { return getRailSwitchId(x, y) != 0; }
	static bool tileHasRail(int x, int y) { return (getRailSwitchId(x, y) & railSwitchIdBitmask) == railIdValue; }
	static bool tileHasSwitch(int x, int y) { return (getRailSwitchId(x, y) & railSwitchIdBitmask) == switchIdValue; }
	static bool tileHasResetSwitch(int x, int y) { return (getRailSwitchId(x, y) & railSwitchIdBitmask) == resetSwitchIdValue; }
	RailState* getRailState(int x, int y) { return railStates[getRailSwitchId(x, y) & railSwitchIndexBitmask]; }
	SwitchState* getSwitchState(int x, int y) { return switchStates[getRailSwitchId(x, y) & railSwitchIndexBitmask]; }
	ResetSwitchState* getResetSwitchState(int x, int y) {
		return resetSwitchStates[getRailSwitchId(x, y) & railSwitchIndexBitmask];
	}
	bool getShouldPlayRadioTowerAnimation() { return shouldPlayRadioTowerAnimation; }
	char getLastActivatedSwitchColor() { return lastActivatedSwitchColor; }
	bool getFinishedConnectionsTutorial() { return finishedConnectionsTutorial; }
	int getRadioWavesAnimationTicksDuration() { return radioWavesState.get()->getAnimationTicksDuration(); }
	static void editorSetTile(int x, int y, char tile) { tiles[y * width + x] = tile; }
	static void editorSetHeight(int x, int y, char height) { heights[y * width + x] = height; }
	static MapState* produce(objCounterParameters());
	virtual void release();
protected:
	virtual void prepareReturnToPool();
public:
	static void buildMap();
private:
	static vector<int> parseRail(int* pixels, int redShift, int segmentIndex, int railSwitchId);
	static void addResetSwitchSegments(
		int* pixels,
		int redShift,
		int resetSwitchX,
		int resetSwitchBottomY,
		int firstSegmentIndex,
		int resetSwitchId,
		Holder_RessetSwitchSegmentVector* segmentsHolder);
public:
	static void deleteMap();
	static int getScreenLeftWorldX(EntityState* camera, int ticksTime);
	static int getScreenTopWorldY(EntityState* camera, int ticksTime);
	#ifdef DEBUG
		static void getSwitchMapTopLeft(short switchIndex, int* outMapLeftX, int* outMapTopY);
		static void getResetSwitchMapTopCenter(short resetSwitchIndex, int* outMapCenterX, int* outMapTopY);
	#endif
	static float antennaCenterWorldX();
	static float antennaCenterWorldY();
	static bool tileHasResetSwitchBody(int x, int y);
	KickActionType getSwitchKickActionType(short switchId);
	static char horizontalTilesHeight(int lowMapX, int highMapX, int mapY);
	static void setIntroAnimationBootTile(bool showBootTile);
	void updateWithPreviousMapState(MapState* prev, int ticksTime);
private:
	void insertRailByHeight(RailState* railState);
public:
	void flipSwitch(short switchId, bool allowRadioTowerAnimation, int ticksTime);
	void flipResetSwitch(short resetSwitchId, int ticksTime);
	void startRadioWavesAnimation(int initialTicksDelay, int ticksTime);
	void startSwitchesFadeInAnimation(int ticksTime);
	void resetMatchingRails(Holder_RessetSwitchSegmentVector* segmentsHolder);
	void render(EntityState* camera, char playerZ, bool showConnections, int ticksTime);
	void renderAbovePlayer(EntityState* camera, bool showConnections, int ticksTime);
	bool renderGroupsForRailsToReset(EntityState* camera, short resetSwitchId, int ticksTime);
	static void renderGroupRect(char group, GLint leftX, GLint topY, GLint rightX, GLint bottomY);
	static void logGroup(char group, stringstream* message);
	static void logSwitchKick(short switchId);
	static void logResetSwitchKick(short resetSwitchId);
	static void logRailRide(short railId, int playerX, int playerY);
	void saveState(ofstream& file);
	bool loadState(string& line);
	void sortInitialRails();
	void resetMap();
	static void editorSetAppropriateDefaultFloorTile(int x, int y, char expectedFloorHeight);
	static bool editorHasFloorTileCreatingShadowForHeight(int x, int y, char height);
	static void editorSetSwitch(int leftX, int topY, char color, char group);
	static void editorSetRail(int x, int y, char color, char group);
	static bool editorUpdateResetSwitchGroups(
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
		Holder_RessetSwitchSegmentVector* segmentsHolder);
	static void editorSetResetSwitch(int x, int bottomY);
	static void editorAdjustRailInitialTileOffset(int x, int y, char tileOffset);
	static char editorGetRailSwitchFloorSaveData(int x, int y);
};
