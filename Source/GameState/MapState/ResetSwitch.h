#include "General/General.h"

#define newResetSwitch(centerX, bottomY) newWithArgs(ResetSwitch, centerX, bottomY)
#define newResetSwitchState(resetSwitch) newWithArgs(ResetSwitchState, resetSwitch)

class RailState;

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
		void render(int screenLeftWorldX, int screenTopWorldY, bool showGroup);
	};

	int centerX;
	int bottomY;
	vector<Segment> leftSegments;
	vector<Segment> bottomSegments;
	vector<Segment> rightSegments;
	int flipOnTicksTime;
public:
	bool editorIsDeleted;

	ResetSwitch(objCounterParametersComma() int pCenterX, int pBottomY);
	virtual ~ResetSwitch();

	int getCenterX() { return centerX; }
	int getBottomY() { return bottomY; }
	bool hasAnySegments() { return !leftSegments.empty() || !bottomSegments.empty() || !rightSegments.empty(); }
	//add a segment to the segments list in the specified section
	void addSegment(int x, int y, char color, char group, char segmentsSection);
private:
	//add a segment to the given segments list
	void addSegment(int x, int y, char color, char group, vector<Segment>* segments);
public:
	//returns whether the group can be found in any of the segments
	bool hasGroupForColor(char group, char color);
	//reset any rails that match the segments
	void resetMatchingRails(vector<RailState*>* railStates);
	//render the reset switch body and its segments
	void render(int screenLeftWorldX, int screenTopWorldY, bool isOn, bool showGroups);
	//remove a segment from this reset switch if it matches the end segment of one of the branches
	bool editorRemoveEndSegment(int x, int y, char color, char group);
	//remove any segment matching the given color and group after the corresponding switch was deleted
	void editorRemoveSwitchSegment(char color, char group);
	//add a segment to this reset switch if it's new and the space is valid
	bool editorAddSegment(int x, int y, char color, char group);
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
	void render(int screenLeftWorldX, int screenTopWorldY, bool showGroups, int ticksTime);
};
