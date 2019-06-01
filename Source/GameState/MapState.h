#include "GameState/EntityState.h"

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
		vector<char>& getGroups() { return groups; }
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
	#ifdef EDITOR
	public:
		bool isDeleted;
	#endif

	public:
		Switch(objCounterParametersComma() int pLeftX, int pTopY, char pColor, char pGroup);
		~Switch();

		char getColor() { return color; }
		char getGroup() { return group; }
		void render(
			int screenLeftWorldX,
			int screenTopWorldY,
			char lastActivatedSwitchColor,
			int lastActivatedSwitchColorFadeInTicksOffset,
			bool isOn);
		#ifdef EDITOR
			char getFloorSaveData(int x, int y);
		#endif
	};
	class RailState onlyInDebug(: public ObjCounter) {
	private:
		static const float tileOffsetPerTick;

		Rail* rail;
		float tileOffset;
		float targetTileOffset;
		int lastUpdateTicksTime;

	public:
		RailState(objCounterParametersComma() Rail* pRail);
		~RailState();

		Rail* getRail() { return rail; }
		float getTargetTileOffset() { return targetTileOffset; }
		bool canRide() { return tileOffset == 0.0f; }
		// say the last 1/2 tile of offset is below the player
		bool isAbovePlayerZ(char z) { return rail->getBaseHeight() - (char)(tileOffset + 0.5f) * 2 > z; }
		void updateWithPreviousRailState(RailState* prev, int ticksTime);
		void squareToggleOffset();
		void render(int screenLeftWorldX, int screenTopWorldY, bool renderShadow);
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
		~SwitchState();

		Switch* getSwitch() { return switch0; }
		void addConnectedRailState(RailState* railState);
		void flip(int pFlipOnTicksTime);
		void updateWithPreviousSwitchState(SwitchState* prev);
		void render(
			int screenLeftWorldX,
			int screenTopWorldY,
			char lastActivatedSwitchColor,
			int lastActivatedSwitchColorFadeInTicksOffset,
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
		~RadioWavesState();

		static RadioWavesState* produce(objCounterParameters());
		void copyRadioWavesState(RadioWavesState* other);
		virtual void release();
		virtual void setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime);
		void updateWithPreviousRadioWavesState(RadioWavesState* prev, int ticksTime);
		void render(EntityState* camera, int ticksTime);
		virtual void setNextCamera(GameState* nextGameState, int ticksTime) {}
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
	static const int switchesFadeInDuration = 1000;
	static const char squareColor = 0;
	static const char triangleColor = 1;
	static const char sawColor = 2;
	static const char sineColor = 3;
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

	vector<RailState*> railStates;
	vector<SwitchState*> switchStates;
	short switchToFlipId;
	int switchFlipOffTicksTime;
	int switchFlipOnTicksTime;
	char lastActivatedSwitchColor;
	int switchesAnimationFadeInStartTicksTime;
	bool shouldPlayRadioTowerAnimation;
	ReferenceCounterHolder<RadioWavesState> radioWavesState;

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
	static float antennaCenterWorldX();
	static float antennaCenterWorldY();
	static char horizontalTilesHeight(int lowMapX, int highMapX, int mapY);
	static void setIntroAnimationBootTile(bool showBootTile);
	void updateWithPreviousMapState(MapState* prev, int ticksTime);
	void setSwitchToFlip(short pSwitchToFlipId, int pSwitchFlipOffTicksTime, int pSwitchFlipOnTicksTime);
	void startRadioWavesAnimation(int initialTicksDelay, int ticksTime);
	void startSwitchesFadeInAnimation(int ticksTime);
	void render(EntityState* camera, char playerZ, int ticksTime);
	void renderRailsAbovePlayer(EntityState* camera, char playerZ, int ticksTime);
	void saveState(ofstream& file);
	bool loadState(string& line);
	void resetMap();
	#ifdef EDITOR
		static void setSwitch(int leftX, int topY, char color, char group);
		static void setRail(int x, int y, char color, char group);
		static void adjustRailInitialTileOffset(int x, int y, char tileOffset);
		static char getRailSwitchFloorSaveData(int x, int y);
	#endif
};
