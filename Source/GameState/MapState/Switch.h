#include "General/General.h"

#define newSwitch(leftX, topY, color, group) newWithArgs(Switch, leftX, topY, color, group)
#define newSwitchState(switch0) newWithArgs(SwitchState, switch0)

class RailState;

class Switch onlyInDebug(: public ObjCounter) {
private:
	int leftX;
	int topY;
	char color;
	char group;
public:
	#ifdef EDITOR
		bool editorIsDeleted;
	#endif

	Switch(objCounterParametersComma() int pLeftX, int pTopY, char pColor, char pGroup);
	virtual ~Switch();

	char getColor() { return color; }
	char getGroup() { return group; }
	void render(
		int screenLeftWorldX,
		int screenTopWorldY,
		char lastActivatedSwitchColor,
		int lastActivatedSwitchColorFadeInTicksOffset,
		bool isOn,
		bool showGroup);
	#ifdef EDITOR
		void editorMoveTo(int newLeftX, int newTopY);
		char editorGetFloorSaveData(int x, int y);
	#endif
};
class SwitchState onlyInDebug(: public ObjCounter) {
private:
	Switch* switch0;
	vector<RailState*> connectedRailStates;
	int flipOnTicksTime;

public:
	SwitchState(objCounterParametersComma() Switch* pSwitch0);
	virtual ~SwitchState();

	Switch* getSwitch() { return switch0; }
	void addConnectedRailState(RailState* railState);
	void flip(int flipOffTicksTime);
	void updateWithPreviousSwitchState(SwitchState* prev);
	void render(
		int screenLeftWorldX,
		int screenTopWorldY,
		char lastActivatedSwitchColor,
		int lastActivatedSwitchColorFadeInTicksOffset,
		bool showGroup,
		int ticksTime);
};
