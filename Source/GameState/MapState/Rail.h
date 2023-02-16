#include "Util/Config.h"

#define newRail(x, y, baseHeight, color, initialTileOffset, movementDirection) \
	newWithArgs(Rail, x, y, baseHeight, color, initialTileOffset, movementDirection)
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

		//get the center x of the tile that this segment is on (when raised)
		float tileCenterX();
		//get the center y of the tile that this segment is on (when raised)
		float tileCenterY();
	};

private:
	char baseHeight;
	char color;
	vector<Segment>* segments;
	vector<char> groups;
	char initialTileOffset;
	char maxTileOffset;
	char movementDirection;
public:
	bool editorIsDeleted;

	Rail(
		objCounterParametersComma()
		int x,
		int y,
		char pBaseHeight,
		char pColor,
		char pInitialTileOffset,
		char pMovementDirection);
	virtual ~Rail();

	char getBaseHeight() { return baseHeight; }
	char getColor() { return color; }
	vector<char>& getGroups() { return groups; }
	char getInitialTileOffset() { return initialTileOffset; }
	char getMaxTileOffset() { return maxTileOffset; }
	int getSegmentCount() { return (int)(segments->size()); }
	Segment* getSegment(int i) { return &(*segments)[i]; }
	//get the sprite index based on which direction the center of this end segment extends towards the rest of the rail
	static int endSegmentSpriteHorizontalIndex(int xExtents, int yExtents);
	//get the sprite index based on which other segments this segment extends towards
	static int middleSegmentSpriteHorizontalIndex(int prevX, int prevY, int x, int y, int nextX, int nextY);
	//get the sprite index that extends straight from the previous segment
	static int extentSegmentSpriteHorizontalIndex(int prevX, int prevY, int x, int y);
	//set the color mask for segments of the given rail color based on how much it's been lowered, using a scale from 0 (fully
	//	raised) to 1 (fully lowered)
	static void setSegmentColor(float loweredScale, int railColor);
	//reverse the order of the segments
	void reverseSegments();
	//add this group to the rail if it does not already contain it
	void addGroup(char group);
	//add a segment on this tile to the rail
	void addSegment(int x, int y);
	//render this rail at its position by rendering each segment
	void render(int screenLeftWorldX, int screenTopWorldY, float tileOffset);
	//render the shadow below the rail
	void renderShadow(int screenLeftWorldX, int screenTopWorldY);
	//render groups where the rail would be at 0 offset
	void renderGroups(int screenLeftWorldX, int screenTopWorldY);
private:
	//render the rail segment at its position, clipping it if part of the map is higher than it
	void renderSegment(int screenLeftWorldX, int screenTopWorldY, float tileOffset, int segmentIndex);
public:
	//remove this group from the rail if it contains it
	void editorRemoveGroup(char group);
	//remove the segment on this tile from the rail
	//returns whether we removed a segment
	bool editorRemoveSegment(int x, int y, char pColor, char group);
	//add a segment on this tile to the rail
	//returns whether we added a segment
	bool editorAddSegment(int x, int y, char pColor, char group, char tileHeight);
	//toggle the movement direction for this rail
	void editorToggleMovementDirection();
	//adjust the initial tile offset of this rail if we're clicking on one of its end segments
	void editorAdjustInitialTileOffset(int x, int y, char tileOffset);
	//we're saving this rail to the floor file, get the data we need at this tile
	char editorGetFloorSaveData(int x, int y);
};
class RailState onlyInDebug(: public ObjCounter) {
private:
	static constexpr float tileOffsetPerTick = 3.0f / (float)Config::ticksPerSecond;

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
	//say 1.5 tiles is where the rail goes from below to above the player
	bool isAbovePlayerZ(char z) { return getEffectiveHeight() > (float)z + 1.5f; }
	//check if we need to start/stop moving
	void updateWithPreviousRailState(RailState* prev, int ticksTime);
	//swap the tile offset between 0 and the max tile offset
	void squareToggleOffset();
	//reset the tile offset to 0 so that the rail moves back to its default position
	void moveToDefaultTileOffset();
	//render the rail, possibly with groups
	void render(int screenLeftWorldX, int screenTopWorld);
	//set this rail to the initial tile offset, not moving
	void loadState(float pTileOffset);
	//reset the offset
	void reset();
};
