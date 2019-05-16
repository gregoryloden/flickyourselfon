#include "Util/PooledReferenceCounter.h"

#define newMapState() produceWithoutArgs(MapState)

class EntityState;

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
			~Segment();
		};

	private:
		char baseHeight;
		char color;
		vector<Segment>* segments;
		vector<char> groups;
		char initialTileOffset;
		char maxTileOffset;
		#ifdef EDITOR
			char groupIndexToRender;
		public:
			bool isDeleted;
		#endif

	public:
		Rail(objCounterParametersComma() int x, int y, char pBaseHeight, char pColor, char pInitialTileOffset);
		~Rail();

		char getBaseHeight() { return baseHeight; }
		char getColor() { return color; }
		char getInitialTileOffset() { return initialTileOffset; }
		char getMaxTileOffset() { return maxTileOffset; }
		int getSegmentCount() { return (int)(segments->size()); }
		Segment* getSegment(int i) { return &(*segments)[i]; }
		static int endSegmentSpriteHorizontalIndex(int xExtents, int yExtents);
		void reverseSegments();
		void addGroup(char group);
		void addSegment(int x, int y);
		void render(int screenLeftWorldX, int screenTopWorldY, float tileOffset, bool renderShadow);
		void renderSegment(int screenLeftWorldX, int screenTopWorldY, float tileOffset, Rail::Segment& segment);
		#ifdef EDITOR
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
		vector<Rail*> rails;
	#ifdef EDITOR
	public:
		bool isDeleted;
	#endif

	public:
		Switch(objCounterParametersComma() int pLeftX, int pTopY, char pColor, char pGroup);
		~Switch();

		char getColor() { return color; }
		char getGroup() { return group; }
		void render(int screenLeftWorldX, int screenTopWorldY, bool isOn);
		#ifdef EDITOR
			char getFloorSaveData(int x, int y);
		#endif
	};
	class RailState onlyInDebug(: public ObjCounter) {
	private:
		Rail* rail;
		float tileOffset;

	public:
		RailState(objCounterParametersComma() Rail* pRail);
		~RailState();

		// say the last 1/2 tile of offset is below the player
		Rail* getRail() { return rail; }
		bool canRide() { return tileOffset == 0.0f; }
		bool isAbovePlayerZ(char z) { return rail->getBaseHeight() - (char)(tileOffset + 0.5f) * 2 > z; }
		void updateWithPreviousRailState(RailState* prev, int ticksTime);
		void render(int screenLeftWorldX, int screenTopWorldY, bool renderShadow);
	};
	class SwitchState onlyInDebug(: public ObjCounter) {
	private:
		Switch* switch0;
		bool isOn;

	public:
		SwitchState(objCounterParametersComma() Switch* pSwitch0);
		~SwitchState();

		void flip();
		void updateWithPreviousSwitchState(SwitchState* prev, int ticksTime);
		void render(int screenLeftWorldX, int screenTopWorldY);
	};

	static const int tileCount = 64; // tile = green / 4
	static const int tileDivisor = 256 / tileCount;
	static const int heightCount = 16; // height = blue / 16
	static const int heightDivisor = 256 / heightCount;
	static const int emptySpaceHeight = heightCount - 1;
	static const int tileSize = 6;
	static const int switchSize = 12;
	static const char invalidHeight = -1;
	static const int radioTowerLeftXOffset = 324;
	static const int radioTowerTopYOffset = -106;
	static const char introAnimationBootTile = 37;
	static const int introAnimationBootTileX = 29;
	static const int introAnimationBootTileY = 26;
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
	static const char* floorFileName;
	static const float smallDistance;
	static const float introAnimationCameraCenterX;
	static const float introAnimationCameraCenterY;

private:
	static char* tiles;
	static char* heights;
	//bits 0-11 indicate the index in the appropriate rail/switch array, bit 12 indicates a rail, bit 13 indicates a switch
	static short* railSwitchIds;
	static vector<Rail*> rails;
	static vector<Switch*> switches;
	static int width;
	static int height;

	vector<RailState*> railStates;
	vector<SwitchState*> switchStates;
	short switchToFlipId;
	int switchToFlipTicksTime;

public:
	MapState(objCounterParameters());
	~MapState();

	static char getTile(int x, int y) { return tiles[y * width + x]; }
	static char getHeight(int x, int y) { return heights[y * width + x]; }
	static short getRailSwitchId(int x, int y) { return railSwitchIds[y * width + x]; }
	static int mapWidth() { return width; }
	static int mapHeight() { return height; }
	static bool tileHasRailOrSwitch(int x, int y) { return getRailSwitchId(x, y) != 0; }
	static bool tileHasRail(int x, int y) { return (getRailSwitchId(x, y) & railIdBitmask) != 0; }
	static bool tileHasSwitch(int x, int y) { return (getRailSwitchId(x, y) & switchIdBitmask) != 0; }
	RailState* getRailState(int x, int y) { return railStates[getRailSwitchId(x, y) & railSwitchIndexBitmask]; }
	#ifdef EDITOR
		static void setTile(int x, int y, char tile) { tiles[y * width + x] = tile; }
		static void setHeight(int x, int y, char height) { heights[y * width + x] = height; }
	#endif
	static MapState* produce(objCounterParameters());
	virtual void release();
	static void buildMap();
	static void deleteMap();
	static int getScreenLeftWorldX(EntityState* camera, int ticksTime);
	static int getScreenTopWorldY(EntityState* camera, int ticksTime);
	static char horizontalTilesHeight(int lowMapX, int highMapX, int mapY);
	static void setIntroAnimationBootTile(bool startingAnimation);
	void updateWithPreviousMapState(MapState* prev, int ticksTime);
	void setSwitchToFlip(short pSwitchToFlipId, int pSwitchToFlipTicksTime);
	void render(EntityState* camera, char playerZ, int ticksTime);
	void renderRailsAbovePlayer(EntityState* camera, char playerZ, int ticksTime);
	#ifdef EDITOR
		static void setSwitch(int leftX, int topY, char color, char group);
		static void setRail(int x, int y, char color, char group);
		static void adjustRailInitialTileOffset(int x, int y, char tileOffset);
		static char getRailSwitchFloorSaveData(int x, int y);
	#endif
};
