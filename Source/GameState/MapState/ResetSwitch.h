#include "General/General.h"

#define newResetSwitch(centerX, bottomY) newWithArgs(ResetSwitch, centerX, bottomY)
#define newResetSwitchState(resetSwitch) newWithArgs(ResetSwitchState, resetSwitch)

class ResetSwitch onlyInDebug(: public ObjCounter) {
public:
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

private:
	int centerX;
	int bottomY;
public:
	vector<Segment> leftSegments;
	vector<Segment> bottomSegments;
	vector<Segment> rightSegments;
private:
	int flipOnTicksTime;
public:
	#ifdef EDITOR
		bool isDeleted;
	#endif

	ResetSwitch(objCounterParametersComma() int pCenterX, int pBottomY);
	virtual ~ResetSwitch();

	int getCenterX() { return centerX; }
	int getBottomY() { return bottomY; }
	bool hasGroupForColor(char group, char color);
	void render(int screenLeftWorldX, int screenTopWorldY, bool isOn, bool showGroups);
	#ifdef EDITOR
		public:
			char getFloorSaveData(int x, int y);
		private:
			char getSegmentFloorSaveData(int x, int y, vector<Segment>& segments);
	#endif
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
//Should only be allocated within an object, on the stack, or as a static object
class Holder_RessetSwitchSegmentVector {
public:
	vector<ResetSwitch::Segment>* val;

	Holder_RessetSwitchSegmentVector(vector<ResetSwitch::Segment>* pVal);
	virtual ~Holder_RessetSwitchSegmentVector();
};
