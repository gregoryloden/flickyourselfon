#include "Util/Config.h"

#define newRail(x, y, baseHeight, color, initialTileOffset, movementDirection, movementMagnitude) \
	newWithArgs(Rail, x, y, baseHeight, color, initialTileOffset, movementDirection, movementMagnitude)
#define newRailState(rail) newWithArgs(RailState, rail)

class Rail onlyInDebug(: public ObjCounter) {
public:
	//Should only be allocated within an object, on the stack, or as a static object
	class Segment {
	public:
		static const int spriteHorizontalIndexEndHorizontalFirst = 6;
		static const int spriteHorizontalIndexEndVerticalFirst = 8;
		static const int spriteHorizontalIndexEndFirst = spriteHorizontalIndexEndHorizontalFirst;
		static const int spriteHorizontalIndexShadowFirst = 10;
		static const int spriteHorizontalIndexBorderFirst = 16;
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
		//render the rail segment at its position, clipping it if part of the map is higher than it
		void render(int screenLeftWorldX, int screenTopWorldY, float tileOffset, char baseHeight);
	};

private:
	char baseHeight;
	char color;
	vector<Segment>* segments;
	vector<char> groups;
	char initialTileOffset;
	char maxTileOffset;
	char movementDirection;
	char movementMagnitude;
public:
	bool editorIsDeleted;

	Rail(
		objCounterParametersComma()
		int x,
		int y,
		char pBaseHeight,
		char pColor,
		char pInitialTileOffset,
		char pMovementDirection,
		char pMovementMagnitude);
	virtual ~Rail();

	char getBaseHeight() { return baseHeight; }
	char getColor() { return color; }
	vector<char>& getGroups() { return groups; }
	char getInitialTileOffset() { return initialTileOffset; }
	char getMaxTileOffset() { return maxTileOffset; }
	char getMovementDirection() { return movementDirection; }
	char getMovementMagnitude() { return movementMagnitude; }
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
	//render the shadow below the rail
	void renderShadow(int screenLeftWorldX, int screenTopWorldY);
	//render groups where the rail would be at 0 offset
	void renderGroups(int screenLeftWorldX, int screenTopWorldY);
	//remove this group from the rail if it contains it
	void editorRemoveGroup(char group);
	//remove the segment on this tile from the rail
	//returns whether we removed a segment
	bool editorRemoveSegment(int x, int y, char pColor, char group);
	//add a segment on this tile to the rail
	//returns whether we added a segment
	bool editorAddSegment(int x, int y, char pColor, char group, char tileHeight);
	//adjust the movement magnitude of this rail if we're clicking on one of its end segments
	void editorAdjustMovementMagnitude(int x, int y, char magnitudeAdd);
	//toggle the movement direction for this rail
	void editorToggleMovementDirection();
	//adjust the initial tile offset of this rail if we're clicking on one of its end segments
	void editorAdjustInitialTileOffset(int x, int y, char tileOffset);
	//we're saving this rail to the floor file, get the data we need at this tile
	char editorGetFloorSaveData(int x, int y);
};
class RailState onlyInDebug(: public ObjCounter) {
private:
	static constexpr float fullMovementDurationTicks = 0.75f * Config::ticksPerSecond;

	Rail* rail;
	float tileOffset;
	float targetTileOffset;
	float currentMovementDirection;
	int bouncesRemaining;
	float nextMovementDirection;
	float distancePerMovement;
	vector<Rail::Segment*> segmentsAbovePlayer;
	int lastUpdateTicksTime;

public:
	RailState(objCounterParametersComma() Rail* pRail);
	virtual ~RailState();

	Rail* getRail() { return rail; }
	float getTargetTileOffset() { return targetTileOffset; }
	bool canRide() { return tileOffset == 0.0f; }
	//check if we need to start/stop moving
	void updateWithPreviousRailState(RailState* prev, int ticksTime);
	//the switch connected to this rail was kicked, move this rail accordingly
	void triggerMovement();
	//reset the tile offset to 0 and reset the movement direction so that the rail moves back to its default position
	void moveToDefaultState();
	//render the rail behind the player by rendering each segment, and save which segments are above the player
	void renderBelowPlayer(int screenLeftWorldX, int screenTopWorld, float playerWorldGroundY);
	//render the rail in front of the player using the list of saved segments
	void renderAbovePlayer(int screenLeftWorldX, int screenTopWorld);
	//set the segment color for this rail based on the raised/lowered state
	void setSegmentColor();
	//render the movement direction over the ends of the rail
	void renderMovementDirections(int screenLeftWorldX, int screenTopWorldY);
	//set this rail to the initial tile offset, not moving
	void loadState(float pTileOffset);
	//reset the offset
	void reset();
};
