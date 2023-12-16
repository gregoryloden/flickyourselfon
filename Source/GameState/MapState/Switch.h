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
	bool editorIsDeleted;

	Switch(objCounterParametersComma() int pLeftX, int pTopY, char pColor, char pGroup);
	virtual ~Switch();

	char getColor() { return color; }
	char getGroup() { return group; }
	//get the map center x that radio waves should originate from
	float getSwitchWavesCenterX();
	//get the map center y that radio waves should originate from
	float getSwitchWavesCenterY();
	//render the switch
	void render(
		int screenLeftWorldX,
		int screenTopWorldY,
		char lastActivatedSwitchColor,
		int lastActivatedSwitchColorFadeInTicksOffset,
		bool isOn);
	//render the group without rendering the switch
	void renderGroup(int screenLeftWorldX, int screenTopWorldY);
	//update the position of this switch
	void editorMoveTo(int newLeftX, int newTopY);
	//we're saving this switch to the floor file, get the data we need at this tile
	char editorGetFloorSaveData(int x, int y);
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
	vector<RailState*>* getConnectedRailStates() { return &connectedRailStates; }
	//add a rail state to be affected by this switch state
	void addConnectedRailState(RailState* railState);
	//activate rails of the same group and color because this switch was kicked
	void flip(bool moveRailsForward, int flipOffTicksTime);
	//save the time that this switch should turn back on
	void updateWithPreviousSwitchState(SwitchState* prev);
	//render the switch
	void render(
		int screenLeftWorldX,
		int screenTopWorldY,
		char lastActivatedSwitchColor,
		int lastActivatedSwitchColorFadeInTicksOffset,
		int ticksTime);
};
