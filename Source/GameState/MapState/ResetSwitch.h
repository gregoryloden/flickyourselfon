#include "General/General.h"

#define newResetSwitch(centerX, bottomY) newWithArgs(ResetSwitch, centerX, bottomY)
#define newResetSwitchState(resetSwitch) newWithArgs(ResetSwitchState, resetSwitch)

class ResetSwitch onlyInDebug(: public ObjCounter) {
private:
	//Should only be allocated within an object, on the stack, or as a static object
	class Segment {
	public:
		int x;
		int y;
		char color;
		char group;
		int spriteHorizontalIndex;

		Segment(int pX, int pY, char pColor, char pGroup, int pSpriteHorizontalIndex);
		virtual ~Segment();

		//render this reset switch segment
		void render(int screenLeftWorldX, int screenTopWorldY);
		//render the group for this reset switch segment
		void renderGroup(int screenLeftWorldX, int screenTopWorldY);
	};

	int centerX;
	int bottomY;
	vector<Segment> leftSegments;
	vector<Segment> bottomSegments;
	vector<Segment> rightSegments;
	vector<short> affectedRailIds;
	int flipOnTicksTime;
public:
	bool editorIsDeleted;

	ResetSwitch(objCounterParametersComma() int pCenterX, int pBottomY);
	virtual ~ResetSwitch();

	int getCenterX() { return centerX; }
	int getBottomY() { return bottomY; }
	bool hasAnySegments() { return !leftSegments.empty() || !bottomSegments.empty() || !rightSegments.empty(); }
	vector<short>* getAffectedRailIds() { return &affectedRailIds; }
	//add a segment to the segments list in the specified section
	void addSegment(int x, int y, char color, char group, char segmentsSection);
private:
	//add a segment to the given segments list
	void addSegment(int x, int y, char color, char group, vector<Segment>* segments);
public:
	//returns whether the group can be found in any of the segments
	bool hasGroupForColor(char group, char color);
	//get the bounds of the hint to render for this reset switch
	void getHintRenderBounds(int* outLeftWorldX, int* outTopWorldY, int* outRightWorldX, int* outBottomWorldY);
	//render the reset switch body and its segments
	void render(int screenLeftWorldX, int screenTopWorldY, bool isOn);
	//render the groups for the segments
	void renderGroups(int screenLeftWorldX, int screenTopWorldY);
	//render boxes over the tiles of this reset switch
	void renderHint(int screenLeftWorldX, int screenTopWorldY, float alpha);
	//remove a segment from this reset switch if it matches a segment in one of the branches
	//writes the x/y of the tile that used to have a segment, but no longer does, to outEndX/outEndY if a matching segment was
	//	found/deleted
	//returns whether a segment was found/deleted
	bool editorRemoveSegment(int x, int y, char color, char group, int* outFreeX, int* outFreeY);
	//remove any segment matching the given color and group after the corresponding switch was deleted
	//writes the x/y of the tile that used to have a segment, but no longer does, to outEndX/outEndY if a matching segment was
	//	found
	//returns whether a segment was found/deleted
	bool editorRemoveSwitchSegment(char color, char group, int* outEndX, int* outEndY);
private:
	//remove the given segment that we found earlier from the given list
	//writes the x/y of the tile that used to have a segment, but no longer does, to outEndX/outEndY
	void editorRemoveFoundSegment(vector<Segment>* segments, int segmentI, int* outFreeX, int* outFreeY);
public:
	//add a segment to this reset switch if it's new and the space is valid
	bool editorAddSegment(int x, int y, char color, char group);
	//change the group for a segment matching the given color and old group
	void editorRewriteGroup(char color, char oldGroup, char group);
	//we're saving this switch to the floor file, get the data we need at this tile
	char editorGetFloorSaveData(int x, int y);
private:
	//get the save value for this tile if the coordinates match one of the given segments
	char editorGetSegmentFloorSaveData(int x, int y, vector<Segment>& segments);
};
class ResetSwitchState onlyInDebug(: public ObjCounter) {
private:
	ResetSwitch* resetSwitch;
	int flipOffTicksTime;

public:
	ResetSwitchState(objCounterParametersComma() ResetSwitch* pResetSwitch);
	virtual ~ResetSwitchState();

	ResetSwitch* getResetSwitch() { return resetSwitch; }
	//set the time that this reset switch should turn off
	void flip(int flipOnTicksTime);
	//save the time that this reset switch should turn back off
	void updateWithPreviousResetSwitchState(ResetSwitchState* prev);
	//render the reset switch body and its segments
	void render(int screenLeftWorldX, int screenTopWorldY, int ticksTime);
};
