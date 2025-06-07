#include "Switch.h"
#include "Audio/Audio.h"
#include "Editor/Editor.h"
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
, editorIsDeleted(false) {
}
Switch::~Switch() {}
float Switch::getSwitchWavesCenterX() {
	return (float)((leftX + 1) * MapState::tileSize);
}
float Switch::getSwitchWavesCenterY() {
	return (float)(topY * MapState::tileSize + 3);
}
void Switch::getHintRenderBounds(int* outLeftWorldX, int* outTopWorldY, int* outRightWorldX, int* outBottomWorldY) {
	*outLeftWorldX = leftX * MapState::tileSize;
	*outTopWorldY = topY * MapState::tileSize;
	*outRightWorldX = (leftX + 2) * MapState::tileSize;
	*outBottomWorldY = (topY + 2) * MapState::tileSize;
}
void Switch::render(
	int screenLeftWorldX,
	int screenTopWorldY,
	char lastActivatedSwitchColor,
	int lastActivatedSwitchColorFadeInTicksOffset,
	bool isOn)
{
	if (Editor::isActive) {
		if (editorIsDeleted)
			return;
		//always render the activated color for all switches up to sine
		lastActivatedSwitchColor = MapState::sineColor;
		lastActivatedSwitchColorFadeInTicksOffset = MapState::switchesFadeInDuration;
		isOn = true;
	} else {
		//group 0 is the turn-on-all-switches switch, don't render it if we're not in the editor
		if (group == 0)
			return;
	}

	glEnable(GL_BLEND);
	GLint drawLeftX = (GLint)(leftX * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(topY * MapState::tileSize - screenTopWorldY);
	//draw the gray sprite if it's off or fading in
	if (lastActivatedSwitchColor < color
			|| (lastActivatedSwitchColor == color && lastActivatedSwitchColorFadeInTicksOffset <= 0))
		(SpriteRegistry::switches->*SpriteSheet::renderSpriteAtScreenPosition)(0, 0, drawLeftX, drawTopY);
	//draw the color sprite if it's already on or it's fully faded in
	else if (lastActivatedSwitchColor > color
		|| (lastActivatedSwitchColor == color && lastActivatedSwitchColorFadeInTicksOffset >= MapState::switchesFadeInDuration))
	{
		int spriteHorizontalIndex = lastActivatedSwitchColor < color ? 0 : (int)(color * 2 + (isOn ? 1 : 2));
		(SpriteRegistry::switches->*SpriteSheet::renderSpriteAtScreenPosition)(spriteHorizontalIndex, 0, drawLeftX, drawTopY);
	//draw a partially faded light color sprite above the darker color sprite if we're fading in the color
	} else {
		int darkSpriteHorizontalIndex = (int)(color * 2 + 2);
		(SpriteRegistry::switches->*SpriteSheet::renderSpriteAtScreenPosition)(
			darkSpriteHorizontalIndex, 0, drawLeftX, drawTopY);
		float fadeInAlpha =
			MathUtils::fsqr((float)lastActivatedSwitchColorFadeInTicksOffset / (float)MapState::switchesFadeInDuration);
		glColor4f(1.0f, 1.0f, 1.0f, fadeInAlpha);
		int lightSpriteLeftX = (darkSpriteHorizontalIndex - 1) * 12 + 1;
		(SpriteRegistry::switches->*SpriteSheet::renderSpriteSheetRegionAtScreenRegion)(
			lightSpriteLeftX, 1, lightSpriteLeftX + 10, 11, drawLeftX + 1, drawTopY + 1, drawLeftX + 11, drawTopY + 11);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
}
void Switch::renderGroup(int screenLeftWorldX, int screenTopWorldY) {
	if (Editor::isActive && editorIsDeleted)
		return;
	GLint drawLeftX = (GLint)(leftX * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(topY * MapState::tileSize - screenTopWorldY);
	MapState::renderGroupRect(group, drawLeftX + 4, drawTopY + 4, drawLeftX + 8, drawTopY + 8);
}
void Switch::renderHint(int screenLeftWorldX, int screenTopWorldY, float alpha) {
	glEnable(GL_BLEND);
	glColor4f(1.0f, 1.0f, 1.0f, alpha);
	GLint drawLeftX = (GLint)(leftX * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(topY * MapState::tileSize - screenTopWorldY);
	SpriteSheet::renderPreColoredRectangle(
		drawLeftX, drawTopY, drawLeftX + (GLint)MapState::tileSize * 2, drawTopY + (GLint)MapState::tileSize * 2);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void Switch::editorMoveTo(int newLeftX, int newTopY) {
	leftX = newLeftX;
	topY = newTopY;
}
char Switch::editorGetFloorSaveData(int x, int y) {
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
void SwitchState::addConnectedRailState(RailState* railState) {
	connectedRailStates.push_back(railState);
}
void SwitchState::flip(bool moveRailsForward, int flipOffTicksTime) {
	for (RailState* railState : connectedRailStates)
		railState->triggerMovement(moveRailsForward);
	flipOnTicksTime = flipOffTicksTime + MapState::switchFlipDuration;
	(switch0->getColor() == MapState::squareColor ? Audio::railSlideSquareSound : Audio::railSlideSound)->play(0);
}
void SwitchState::updateWithPreviousSwitchState(SwitchState* prev) {
	flipOnTicksTime = prev->flipOnTicksTime;
}
void SwitchState::render(
	int screenLeftWorldX,
	int screenTopWorldY,
	char lastActivatedSwitchColor,
	int lastActivatedSwitchColorFadeInTicksOffset,
	int ticksTime)
{
	switch0->render(
		screenLeftWorldX,
		screenTopWorldY,
		lastActivatedSwitchColor,
		lastActivatedSwitchColorFadeInTicksOffset,
		ticksTime >= flipOnTicksTime);
}
