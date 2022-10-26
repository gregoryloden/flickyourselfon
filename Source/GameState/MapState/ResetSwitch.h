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
	void addSegment(int x, int y, char color, char group, char segmentsSection);
private:
	void addSegment(int x, int y, char color, char group, vector<Segment>* segments);
public:
	bool hasGroupForColor(char group, char color);
	void resetMatchingRails(vector<RailState*>* railStates);
	void render(int screenLeftWorldX, int screenTopWorldY, bool isOn, bool showGroups);
	bool editorRemoveSegment(int x, int y, char color, char group);
	bool editorAddSegment(int x, int y, char color, char group);
	char editorGetFloorSaveData(int x, int y);
private:
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
	void flip(int flipOnTicksTime);
	void updateWithPreviousResetSwitchState(ResetSwitchState* prev);
	void render(int screenLeftWorldX, int screenTopWorldY, bool showGroups, int ticksTime);
};
