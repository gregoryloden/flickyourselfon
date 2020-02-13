#include "General/General.h"
#ifdef EDITOR
	#include <mutex>
#endif

#define newRail(x, y, baseHeight, color, initialTileOffset) newWithArgs(Rail, x, y, baseHeight, color, initialTileOffset)
#define newRailState(rail, railIndex) newWithArgs(RailState, rail, railIndex)

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
	static int middleSegmentSpriteHorizontalIndex(int prevX, int prevY, int x, int y, int nextX, int nextY);
	static int extentSegmentSpriteHorizontalIndex(int prevX, int prevY, int x, int y);
	static void setSegmentColor(float loweredScale, int railColor);
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
	void moveToDefaultTileOffset();
	void render(int screenLeftWorldX, int screenTopWorld);
	void renderShadow(int screenLeftWorldX, int screenTopWorldY);
	void renderGroups(int screenLeftWorldX, int screenTopWorldY);
	void loadState(float pTileOffset);
	void reset();
};
