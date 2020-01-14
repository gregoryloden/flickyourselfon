#include "Switch.h"
#include "GameState/MapState/MapState.h"
#include "GameState/MapState/Rail.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

//////////////////////////////// Switch ////////////////////////////////
Switch::Switch(objCounterParametersComma() int pLeftX, int pTopY, char pColor, char pGroup)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
leftX(pLeftX)
, topY(pTopY)
, color(pColor)
, group(pGroup)
#ifdef EDITOR
	, isDeleted(false)
#endif
{
}
Switch::~Switch() {}
//render the switch
void Switch::render(
	int screenLeftWorldX,
	int screenTopWorldY,
	char lastActivatedSwitchColor,
	int lastActivatedSwitchColorFadeInTicksOffset,
	bool isOn,
	bool showGroup)
{
	#ifdef EDITOR
		if (isDeleted)
			return;
		//always render the activated color for all switches up to sine
		lastActivatedSwitchColor = MapState::sineColor;
		lastActivatedSwitchColorFadeInTicksOffset = MapState::switchesFadeInDuration;
		isOn = true;
	#else
		//group 0 is the turn-on-all-switches switch, don't render it if we're not in the editor
		if (group == 0)
			return;
	#endif

	glEnable(GL_BLEND);
	GLint drawLeftX = (GLint)(leftX * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(topY * MapState::tileSize - screenTopWorldY);
	//draw the gray sprite if it's off or fading in
	if (lastActivatedSwitchColor < color
			|| (lastActivatedSwitchColor == color && lastActivatedSwitchColorFadeInTicksOffset <= 0))
		SpriteRegistry::switches->renderSpriteAtScreenPosition(0, 0, drawLeftX, drawTopY);
	//draw the color sprite if it's already on or it's fully faded in
	else if (lastActivatedSwitchColor > color
		|| (lastActivatedSwitchColor == color && lastActivatedSwitchColorFadeInTicksOffset >= MapState::switchesFadeInDuration))
	{
		int spriteHorizontalIndex = lastActivatedSwitchColor < color ? 0 : (int)(color * 2 + (isOn ? 1 : 2));
		SpriteRegistry::switches->renderSpriteAtScreenPosition(spriteHorizontalIndex, 0, drawLeftX, drawTopY);
	//draw a partially faded light color sprite above the darker color sprite if we're fading in the color
	} else {
		int darkSpriteHorizontalIndex = (int)(color * 2 + 2);
		SpriteRegistry::switches->renderSpriteAtScreenPosition(darkSpriteHorizontalIndex, 0, drawLeftX, drawTopY);
		float fadeInAlpha =
			MathUtils::fsqr((float)lastActivatedSwitchColorFadeInTicksOffset / (float)MapState::switchesFadeInDuration);
		glColor4f(1.0f, 1.0f, 1.0f, fadeInAlpha);
		int lightSpriteLeftX = (darkSpriteHorizontalIndex - 1) * 12 + 1;
		SpriteRegistry::switches->renderSpriteSheetRegionAtScreenRegion(
			lightSpriteLeftX, 1, lightSpriteLeftX + 10, 11, drawLeftX + 1, drawTopY + 1, drawLeftX + 11, drawTopY + 11);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
	if (showGroup)
		MapState::renderGroupRect(group, drawLeftX + 4, drawTopY + 4, drawLeftX + 8, drawTopY + 8);
}
#ifdef EDITOR
	//update the position of this switch
	void Switch::moveTo(int newLeftX, int newTopY) {
		leftX = newLeftX;
		topY = newTopY;
	}
	//we're saving this switch to the floor file, get the data we need at this tile
	char Switch::getFloorSaveData(int x, int y) {
		//head byte, write our color
		if (x == leftX && y == topY)
			return (color << MapState::floorRailSwitchColorDataShift) | MapState::floorSwitchHeadValue;
		//tail byte, write our number
		else if (x == leftX + 1&& y == topY)
			return (group << MapState::floorRailSwitchGroupDataShift) | MapState::floorIsRailSwitchBitmask;
		//no data but still part of this switch
		else
			return MapState::floorRailSwitchTailValue;
	}
#endif

//////////////////////////////// SwitchState ////////////////////////////////
SwitchState::SwitchState(objCounterParametersComma() Switch* pSwitch0)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
switch0(pSwitch0)
, connectedRailStates()
, flipOnTicksTime(0) {
}
SwitchState::~SwitchState() {
	//don't delete the switch, it's owned by MapState
}
//add a rail state to be affected by this switch state
void SwitchState::addConnectedRailState(RailState* railState) {
	connectedRailStates.push_back(railState);
}
//activate rails of the same group and color because this switch was kicked
void SwitchState::flip(int flipOffTicksTime) {
	char color = switch0->getColor();
	//square wave switch: just toggle the target tile offset of the rails
	if (color == MapState::squareColor) {
		for (RailState* railState : connectedRailStates) {
			railState->squareToggleOffset();
		}
	}
	flipOnTicksTime = flipOffTicksTime + MapState::switchFlipDuration;
}
//save the time that this switch should turn back on
void SwitchState::updateWithPreviousSwitchState(SwitchState* prev) {
	flipOnTicksTime = prev->flipOnTicksTime;
}
//render the switch
void SwitchState::render(
	int screenLeftWorldX,
	int screenTopWorldY,
	char lastActivatedSwitchColor,
	int lastActivatedSwitchColorFadeInTicksOffset,
	bool showGroup,
	int ticksTime)
{
	switch0->render(
		screenLeftWorldX,
		screenTopWorldY,
		lastActivatedSwitchColor,
		lastActivatedSwitchColorFadeInTicksOffset,
		ticksTime >= flipOnTicksTime,
		showGroup);
}
