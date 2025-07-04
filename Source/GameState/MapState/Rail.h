#include "Util/Config.h"

#define newRail(x, y, baseHeight, color, initialTileOffset, initialMovementDirection, movementMagnitude) \
	newWithArgs(Rail, x, y, baseHeight, color, initialTileOffset, initialMovementDirection, movementMagnitude)
#define newRailState(rail) newWithArgs(RailState, rail)

class Rail onlyInDebug(: public ObjCounter) {
public:
	//Should only be allocated within an object, on the stack, or as a static object
	class Segment {
	public:
		static constexpr int spriteHorizontalIndexVertical = 0;
		static constexpr int spriteHorizontalIndexHorizontal = 1;
		static constexpr int spriteHorizontalIndexEndHorizontalFirst = 6;
		static constexpr int spriteHorizontalIndexEndVerticalFirst = 8;
		static constexpr int spriteHorizontalIndexEndFirst = spriteHorizontalIndexEndHorizontalFirst;
		static constexpr int spriteHorizontalIndexShadowFirst = 10;
		static constexpr int spriteHorizontalIndexBorderFirst = 16;
		static constexpr char absentTileOffset = -1;

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
	vector<Segment> segments;
	vector<char> groups;
	char initialTileOffset;
	char maxTileOffset;
	char initialMovementDirection;
	char movementMagnitude;
	int renderLeftTileX;
	int renderTopTileY;
	int renderRightTileX;
	int renderBottomTileY;
	int renderHintBottomTileY;
public:
	bool editorIsDeleted;

	Rail(
		objCounterParametersComma()
		int x,
		int y,
		char pBaseHeight,
		char pColor,
		char pInitialTileOffset,
		char pInitialMovementDirection,
		char pMovementMagnitude);
	virtual ~Rail();

	char getBaseHeight() { return baseHeight; }
	char getColor() { return color; }
	vector<char>& getGroups() { return groups; }
	char getInitialTileOffset() { return initialTileOffset; }
	char getMaxTileOffset() { return maxTileOffset; }
	char getInitialMovementDirection() { return initialMovementDirection; }
	char getMovementMagnitude() { return movementMagnitude; }
	int getSegmentCount() { return (int)segments.size(); }
	Segment* getSegment(int i) { return &segments[i]; }
	//get the sprite index based on which direction the center of this end segment extends towards the rest of the rail
	static int endSegmentSpriteHorizontalIndex(int xExtents, int yExtents);
	//get the sprite index based on which other segments this segment extends towards
	static int middleSegmentSpriteHorizontalIndex(int prevX, int prevY, int x, int y, int nextX, int nextY);
	//get the sprite index that extends straight from the previous segment
	static int extentSegmentSpriteHorizontalIndex(int prevX, int prevY, int x, int y);
	//set the color mask for segments of the given rail color based on the given saturation (1 for raised rails at full
	//	saturation, 0 for lowered rails at 0 saturation), with the given alpha
	static void setSegmentColor(char railColor, float saturation, float alpha);
	//add this group to the rail if it does not already contain it
	void addGroup(char group);
	//add a segment on this tile to the rail
	void addSegment(int x, int y);
	//go through all the segments and extend the render box to accomodate every tile it could render to
	void assignRenderBox();
	//the switch connected to this rail was kicked, move this rail accordingly
	//returns whether a bounce occurred
	bool triggerMovement(char movementDirection, char* inOutTileOffset);
	//returns whether this rail can render on screen, given the (open-ended) tile coordinates that will be drawn
	bool canRender(int screenLeftTileX, int screenTopTileY, int screenRightTileX, int screenBottomTileY);
	//get the bounds of the hint to render for this rail
	void getHintRenderBounds(int* outLeftWorldX, int* outTopWorldY, int* outRightWorldX, int* outBottomWorldY);
	//render the shadow below the rail
	void renderShadow(int screenLeftWorldX, int screenTopWorldY);
	//render groups where the rail would be at 0 offset
	void renderGroups(int screenLeftWorldX, int screenTopWorldY);
	//render boxes over the tiles of this rail
	void renderHint(int screenLeftWorldX, int screenTopWorldY, float alpha);
	//remove this group from the rail if it contains it
	//returns whether we removed a group
	bool editorRemoveGroup(char group);
	//remove the segment on this tile from the rail
	//returns whether we removed a segment
	bool editorRemoveSegment(int x, int y, char pColor, char group);
	//add a segment on this tile to the rail
	//returns whether we added a segment
	bool editorAddSegment(int x, int y, char pColor, char group, char tileHeight);
	//adjust the movement magnitude of this rail if we're clicking on one of its end segments
	void editorAdjustMovementMagnitude(int x, int y, char magnitudeAdd);
	//toggle the movement direction for this rail if we're clicking on one of its end segments
	void editorToggleMovementDirection(int x, int y);
	//adjust the initial tile offset of this rail if we're clicking on one of its end segments
	void editorAdjustInitialTileOffset(int x, int y, char tileOffset);
	//we're saving this rail to the floor file, get the data we need at this tile
	char editorGetFloorSaveData(int x, int y);
};
class RailState onlyInDebug(: public ObjCounter) {
private:
	static constexpr float fullMovementDurationTicks = 0.75f * Config::ticksPerSecond;
	static constexpr float nearlyRaisedAlphaIntensity = 11.0f / 16.0f;

	Rail* rail;
	float tileOffset;
	char targetTileOffset;
	char currentMovementDirection;
	float effectiveHeight;
	int bouncesRemaining;
	char nextMovementDirection;
	float distancePerMovement;
	vector<Rail::Segment*> segmentsAbovePlayer;
	int lastUpdateTicksTime;

public:
	RailState(objCounterParametersComma() Rail* pRail);
	virtual ~RailState();

	Rail* getRail() { return rail; }
	char getTargetTileOffset() { return targetTileOffset; }
	char getNextMovementDirection() { return nextMovementDirection; }
	bool isInDefaultState() {
		return targetTileOffset == rail->getInitialTileOffset() && nextMovementDirection == rail->getInitialMovementDirection();
	}
	bool canRide() { return tileOffset == 0.0f; }
	static bool effectiveHeightsAreAscending(RailState* a, RailState* b) { return a->effectiveHeight < b->effectiveHeight; }
	//check if we need to start/stop moving
	void updateWithPreviousRailState(RailState* prev, int ticksTime);
	//update the position of a sine wave rail
	void updateSineRailTileOffset(RailState* prev, int ticksTime);
	//the switch connected to this rail was kicked, move this rail accordingly
	void triggerMovement(bool moveForward);
	//render the rail behind the player by rendering each segment, and save which segments are above the player
	void renderBelowPlayer(int screenLeftWorldX, int screenTopWorldY, float playerWorldGroundY);
	//render the rail in front of the player using the list of saved segments
	void renderAbovePlayer(int screenLeftWorldX, int screenTopWorldY);
	//set the segment color for this rail based on the raised/lowered state
	void setSegmentColor();
	//render the movement direction over the ends of the rail
	void renderMovementDirections(int screenLeftWorldX, int screenTopWorldY);
	//get the segment that should be used to write this rail's state
	Rail::Segment* getSaveSegment();
	//set this rail to the initial tile offset and movement direction, with or without a moving animation
	//returns whether this changes the state of the rail
	bool loadState(char pTileOffset, char pNextMovementDirection, bool animateMovement);
	//reset this rail to its default state, with or without a moving animation
	//returns whether this changes the state of the rail
	bool reset(bool animateMovement);
};
