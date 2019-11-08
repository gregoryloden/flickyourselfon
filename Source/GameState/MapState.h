#include "GameState/EntityState.h"
#ifdef EDITOR
	#include <mutex>
#endif

#define newMapState() produceWithoutArgs(MapState)

class MapState: public PooledReferenceCounter {
public:
	class Rail onlyInDebug(: public ObjCounter) {
	public:
		//Should only be allocated within an object, on the stack, or as a static object
		class Segment {
		public:
			static const char absentTileOffset = -1;

			int x;
			int y;
			int spriteHorizontalIndex;
			char maxTileOffset;

			Segment(int pX, int pY, char pMaxTileOffset);
			virtual ~Segment();
			float tileCenterX();
			float tileCenterY();
		};

	private:
		char baseHeight;
		char color;
		vector<Segment>* segments;
		vector<char> groups;
		char initialTileOffset;
		char maxTileOffset;
	public:
		#ifdef EDITOR
			mutex segmentsMutex;
			bool isDeleted;
		#endif

		Rail(objCounterParametersComma() int x, int y, char pBaseHeight, char pColor, char pInitialTileOffset);
		virtual ~Rail();

		char getBaseHeight() { return baseHeight; }
		char getColor() { return color; }
		vector<char>& getGroups() { return groups; }
		char getInitialTileOffset() { return initialTileOffset; }
		char getMaxTileOffset() { return maxTileOffset; }
		int getSegmentCount() { return (int)(segments->size()); }
		Segment* getSegment(int i) { return &(*segments)[i]; }
		static int endSegmentSpriteHorizontalIndex(int xExtents, int yExtents);
		void reverseSegments();
		void addGroup(char group);
		void addSegment(int x, int y);
		void render(int screenLeftWorldX, int screenTopWorldY, float tileOffset);
		void renderShadow(int screenLeftWorldX, int screenTopWorldY);
		void renderGroups(int screenLeftWorldX, int screenTopWorldY);
	private:
		void renderSegment(int screenLeftWorldX, int screenTopWorldY, float tileOffset, int segmentIndex);
		#ifdef EDITOR
		public:
			void removeGroup(char group);
			void removeSegment(int x, int y);
			void adjustInitialTileOffset(int x, int y, char tileOffset);
			char getFloorSaveData(int x, int y);
		#endif
	};
	class Switch onlyInDebug(: public ObjCounter) {
	private:
		int leftX;
		int topY;
		char color;
		char group;
	#ifdef EDITOR
	public:
		bool isDeleted;
	#endif

	public:
		Switch(objCounterParametersComma() int pLeftX, int pTopY, char pColor, char pGroup);
		virtual ~Switch();

		char getColor() { return color; }
		char getGroup() { return group; }
		void render(
			int screenLeftWorldX,
			int screenTopWorldY,
			char lastActivatedSwitchColor,
			int lastActivatedSwitchColorFadeInTicksOffset,
			bool isOn,
			bool showGroup);
		#ifdef EDITOR
			void moveTo(int newLeftX, int newTopY);
			char getFloorSaveData(int x, int y);
		#endif
	};
	class RailState onlyInDebug(: public ObjCounter) {
	private:
		static const float tileOffsetPerTick;

		Rail* rail;
		int railIndex;
		float tileOffset;
		float targetTileOffset;
		int lastUpdateTicksTime;

	public:
		RailState(objCounterParametersComma() Rail* pRail, int pRailIndex);
		virtual ~RailState();

		Rail* getRail() { return rail; }
		int getRailIndex() { return railIndex; }
		float getTargetTileOffset() { return targetTileOffset; }
		bool canRide() { return tileOffset == 0.0f; }
		float getEffectiveHeight() { return rail->getBaseHeight() - tileOffset * 2; }
		// say 1.5 tiles is where the rail goes from below to above the player
		bool isAbovePlayerZ(char z) { return getEffectiveHeight() > (float)z + 1.5f; }
		void updateWithPreviousRailState(RailState* prev, int ticksTime);
		void squareToggleOffset();
		void render(int screenLeftWorldX, int screenTopWorld);
		void renderShadow(int screenLeftWorldX, int screenTopWorldY);
		void renderGroups(int screenLeftWorldX, int screenTopWorldY);
		void loadState(float pTileOffset);
		void reset();
	};
	class SwitchState onlyInDebug(: public ObjCounter) {
	private:
		Switch* switch0;
		vector<RailState*> connectedRailStates;
		int flipOnTicksTime;

	public:
		SwitchState(objCounterParametersComma() Switch* pSwitch0);
		virtual ~SwitchState();

		Switch* getSwitch() { return switch0; }
		void addConnectedRailState(RailState* railState);
		void flip(int pFlipOnTicksTime);
		void updateWithPreviousSwitchState(SwitchState* prev);
		void render(
			int screenLeftWorldX,
			int screenTopWorldY,
			char lastActivatedSwitchColor,
			int lastActivatedSwitchColorFadeInTicksOffset,
			bool showGroup,
			int ticksTime);
	};
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
	static const short railIdBitmask = 1 << 12;
	static const short switchIdBitmask = railIdBitmask << 1;
	static const short railSwitchIndexBitmask = railIdBitmask - 1;
	static const int floorIsRailSwitchBitmask = 1;
	static const int floorIsRailSwitchHeadBitmask = 2;
	static const int floorIsRailSwitchAndHeadBitmask = floorIsRailSwitchHeadBitmask | floorIsRailSwitchBitmask;
	static const int floorIsSwitchBitmask = 4;
	static const char floorRailSwitchColorPostShiftBitmask = 0x3;
	static const char floorRailSwitchGroupPostShiftBitmask = 0x3F;
	static const char floorRailInitialTileOffsetPostShiftBitmask = 0x7;
	static const int floorRailSwitchColorDataShift = 3;
	static const int floorRailSwitchGroupDataShift = 2;
	static const int floorRailInitialTileOffsetDataShift = 5;
	static const int floorRailSwitchAndHeadValue = floorIsRailSwitchHeadBitmask | floorIsRailSwitchBitmask;
	static const int floorRailHeadValue = floorRailSwitchAndHeadValue;
	static const int floorSwitchHeadValue = floorIsSwitchBitmask | floorRailSwitchAndHeadValue;
	//other
	static const char* floorFileName;
	static const float smallDistance;
	static const float introAnimationCameraCenterX;
	static const float introAnimationCameraCenterY;
	static const string railOffsetFilePrefix;
	static const string lastActivatedSwitchColorFilePrefix;

private:
	static char* tiles;
	static char* heights;
	//bits 0-11 indicate the index in the appropriate rail/switch array, bit 12 indicates a rail, bit 13 indicates a switch
	static short* railSwitchIds;
	static vector<Rail*> rails;
	static vector<Switch*> switches;
	static int width;
	static int height;
	#ifdef EDITOR
		static int nonTilesHidingState;
	#endif

	vector<RailState*> railStates;
	vector<RailState*> railStatesByHeight;
	int railsBelowPlayerZ;
	vector<SwitchState*> switchStates;
	char lastActivatedSwitchColor;
	int switchesAnimationFadeInStartTicksTime;
	bool shouldPlayRadioTowerAnimation;
	ReferenceCounterHolder<RadioWavesState> radioWavesState;

public:
	MapState(objCounterParameters());
	virtual ~MapState();

	static char getTile(int x, int y) { return tiles[y * width + x]; }
	static char getHeight(int x, int y) { return heights[y * width + x]; }
	static short getRailSwitchId(int x, int y) { return railSwitchIds[y * width + x]; }
	static Rail* getRailByIndex(int railIndex) { return rails[railIndex]; }
	static short getIdFromSwitchIndex(short switchIndex) { return switchIndex | switchIdBitmask; }
	static int mapWidth() { return width; }
	static int mapHeight() { return height; }
	static bool tileHasRailOrSwitch(int x, int y) { return getRailSwitchId(x, y) != 0; }
	static bool tileHasRail(int x, int y) { return (getRailSwitchId(x, y) & railIdBitmask) != 0; }
	static bool tileHasSwitch(int x, int y) { return (getRailSwitchId(x, y) & switchIdBitmask) != 0; }
	RailState* getRailState(int x, int y) { return railStates[getRailSwitchId(x, y) & railSwitchIndexBitmask]; }
	bool getShouldPlayRadioTowerAnimation() { return shouldPlayRadioTowerAnimation; }
	int getRadioWavesAnimationTicksDuration() { return radioWavesState.get()->getAnimationTicksDuration(); }
	#ifdef EDITOR
		static void setTile(int x, int y, char tile) { tiles[y * width + x] = tile; }
		static void setHeight(int x, int y, char height) { heights[y * width + x] = height; }
	#endif
	static MapState* produce(objCounterParameters());
	virtual void release();
protected:
	virtual void prepareReturnToPool();
public:
	static void buildMap();
	static void deleteMap();
	static int getScreenLeftWorldX(EntityState* camera, int ticksTime);
	static int getScreenTopWorldY(EntityState* camera, int ticksTime);
	static void getSwitchMapTopLeft(short switchIndex, int* outMapLeftX, int* outMapTopY);
	static float antennaCenterWorldX();
	static float antennaCenterWorldY();
	static char horizontalTilesHeight(int lowMapX, int highMapX, int mapY);
	static void setIntroAnimationBootTile(bool showBootTile);
	void updateWithPreviousMapState(MapState* prev, int ticksTime);
private:
	void insertRailByHeight(RailState* railState);
public:
	void flipSwitch(short switchId, bool allowRadioTowerAnimation, int ticksTime);
	void startRadioWavesAnimation(int initialTicksDelay, int ticksTime);
	void startSwitchesFadeInAnimation(int ticksTime);
	void render(EntityState* camera, char playerZ, bool showConnections, int ticksTime);
	void renderRailsAbovePlayer(EntityState* camera, char playerZ, bool showConnections, int ticksTime);
	static void renderGroupRect(char group, GLint leftX, GLint topY, GLint rightX, GLint bottomY);
	static void logGroup(char group, stringstream* message);
	static void logSwitchKick(short switchId);
	static void logRailRide(short railId, int playerX, int playerY);
	void saveState(ofstream& file);
	bool loadState(string& line);
	void sortInitialRails();
	void resetMap();
	#ifdef EDITOR
		static void setAppropriateDefaultFloorTile(int x, int y, char expectedFloorHeight);
		static bool hasFloorTileCreatingShadowForHeight(int x, int y, char height);
		static void setSwitch(int leftX, int topY, char color, char group);
		static void setRail(int x, int y, char color, char group);
		static void adjustRailInitialTileOffset(int x, int y, char tileOffset);
		static char getRailSwitchFloorSaveData(int x, int y);
	#endif
};
//Should only be allocated within an object, on the stack, or as a static object
class Holder_MapStateRail {
public:
	MapState::Rail* val;

	Holder_MapStateRail(MapState::Rail* pVal);
	virtual ~Holder_MapStateRail();
};
