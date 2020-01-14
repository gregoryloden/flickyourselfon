#include "ResetSwitch.h"
#include "GameState/MapState/MapState.h"
#include "GameState/MapState/Rail.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

//////////////////////////////// ResetSwitch::Segment ////////////////////////////////
ResetSwitch::Segment::Segment(int pX, int pY, char pColor, char pGroup, int pSpriteHorizontalIndex)
: x(pX)
, y(pY)
, color(pColor)
, group(pGroup)
, spriteHorizontalIndex(pSpriteHorizontalIndex) {
}
ResetSwitch::Segment::~Segment() {}
//render this reset switch segment
void ResetSwitch::Segment::render(int screenLeftWorldX, int screenTopWorldY, bool showGroup) {
	glEnable(GL_BLEND);
	Rail::setSegmentColor(1.0f, color);
	GLint drawLeftX = (GLint)(x * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(y * MapState::tileSize - screenTopWorldY);
	SpriteRegistry::rails->renderSpriteAtScreenPosition(spriteHorizontalIndex, 0, drawLeftX, drawTopY);
	if (showGroup)
		MapState::renderGroupRect(group, drawLeftX + 2, drawTopY + 2, drawLeftX + 4, drawTopY + 4);
}

//////////////////////////////// ResetSwitch ////////////////////////////////
ResetSwitch::ResetSwitch(objCounterParametersComma() int pCenterX, int pBottomY)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
centerX(pCenterX)
, bottomY(pBottomY)
, leftSegments()
, bottomSegments()
, rightSegments()
#ifdef EDITOR
	, isDeleted(false)
#endif
{
}
ResetSwitch::~ResetSwitch() {}
//render the reset switch
void ResetSwitch::render(int screenLeftWorldX, int screenTopWorldY, bool isOn, bool showGroups) {
	#ifdef EDITOR
		if (isDeleted)
			return;
	#endif

	glEnable(GL_BLEND);
	GLint drawLeftX = (GLint)(centerX * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)((bottomY - 1) * MapState::tileSize - screenTopWorldY);
	SpriteRegistry::resetSwitch->renderSpriteAtScreenPosition(isOn ? 1 : 0, 0, drawLeftX, drawTopY);
	for (Segment& segment : leftSegments)
		segment.render(screenLeftWorldX, screenTopWorldY, showGroups);
	for (Segment& segment : bottomSegments)
		segment.render(screenLeftWorldX, screenTopWorldY, showGroups);
	for (Segment& segment : rightSegments)
		segment.render(screenLeftWorldX, screenTopWorldY, showGroups);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
#ifdef EDITOR
	//we're saving this switch to the floor file, get the data we need at this tile
	char ResetSwitch::getFloorSaveData(int x, int y) {
		if (x == centerX && y == bottomY)
			return MapState::floorResetSwitchHeadValue;
		char leftFloorSaveData = getSegmentFloorSaveData(x, y, leftSegments);
		if (leftFloorSaveData != 0)
			return leftFloorSaveData;
		char bottomFloorSaveData = getSegmentFloorSaveData(x, y, bottomSegments);
		if (bottomFloorSaveData != 0)
			return bottomFloorSaveData;
		return getSegmentFloorSaveData(x, y, rightSegments);
	}
	//get the save value for this tile if the coordinates match one of the given segments
	char ResetSwitch::getSegmentFloorSaveData(int x, int y, vector<Segment>& segments) {
		for (int i = 0; i < (int)segments.size(); i++) {
			Segment& segment = segments[i];
			if (segment.x != x || segment.y != y)
				continue;
			return i == 0
				//segment 0 combines the first and last colors
				? MapState::floorIsRailSwitchBitmask
					| (segment.color << MapState::floorRailSwitchGroupDataShift)
					| (segments.back().color << (MapState::floorRailSwitchGroupDataShift + 2))
				//everything else sets its group
				: MapState::floorIsRailSwitchBitmask | (segment.group << MapState::floorRailSwitchGroupDataShift);
		}
		return 0;
	}
#endif

//////////////////////////////// ResetSwitchState ////////////////////////////////
ResetSwitchState::ResetSwitchState(objCounterParametersComma() ResetSwitch* pResetSwitch)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
resetSwitch(pResetSwitch)
, flipOffTicksTime(0) {
}
ResetSwitchState::~ResetSwitchState() {}
//set the time that this reset switch should turn off
void ResetSwitchState::flip(int flipOnTicksTime) {
	flipOffTicksTime = flipOnTicksTime + MapState::switchFlipDuration;
}
//save the time that this reset switch should turn back off
void ResetSwitchState::updateWithPreviousResetSwitchState(ResetSwitchState* prev) {
	flipOffTicksTime = prev->flipOffTicksTime;
}
//render the reset switch
void ResetSwitchState::render(int screenLeftWorldX, int screenTopWorldY, bool showGroups, int ticksTime) {
	resetSwitch->render(screenLeftWorldX, screenTopWorldY, ticksTime < flipOffTicksTime, showGroups);
}

//////////////////////////////// Holder_RessetSwitchSegmentVector ////////////////////////////////
Holder_RessetSwitchSegmentVector::Holder_RessetSwitchSegmentVector(vector<ResetSwitch::Segment>* pVal)
: val(pVal) {
}
Holder_RessetSwitchSegmentVector::~Holder_RessetSwitchSegmentVector() {
	//don't delete the vector, it's owned by something else
}
