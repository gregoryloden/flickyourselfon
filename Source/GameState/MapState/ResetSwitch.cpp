#include "ResetSwitch.h"
#include "Editor/Editor.h"
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
void ResetSwitch::Segment::render(int screenLeftWorldX, int screenTopWorldY) {
	glEnable(GL_BLEND);
	Rail::setSegmentColor(0.0f, color);
	GLint drawLeftX = (GLint)(x * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(y * MapState::tileSize - screenTopWorldY);
	SpriteRegistry::rails->renderSpriteAtScreenPosition(spriteHorizontalIndex, 0, drawLeftX, drawTopY);
}
void ResetSwitch::Segment::renderGroup(int screenLeftWorldX, int screenTopWorldY) {
	GLint drawLeftX = (GLint)(x * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(y * MapState::tileSize - screenTopWorldY);
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
, affectedRailIds()
, flipOnTicksTime(0)
, editorIsDeleted(false) {
}
ResetSwitch::~ResetSwitch() {}
void ResetSwitch::addSegment(int x, int y, char color, char group, char segmentsSection) {
	vector<Segment>* segments =
		segmentsSection < 0 ? &leftSegments :
		segmentsSection == 0 ? &bottomSegments :
		&rightSegments;
	addSegment(x, y, color, group, segments);
}
void ResetSwitch::addSegment(int x, int y, char color, char group, vector<Segment>* segments) {
	int spriteHorizontalIndex;
	if (segments->empty())
		spriteHorizontalIndex = Rail::extentSegmentSpriteHorizontalIndex(centerX, bottomY, x, y);
	else {
		Segment& last = segments->back();
		spriteHorizontalIndex = Rail::extentSegmentSpriteHorizontalIndex(last.x, last.y, x, y);
		if (segments->size() < 2)
			last.spriteHorizontalIndex = Rail::middleSegmentSpriteHorizontalIndex(centerX, bottomY, last.x, last.y, x, y);
		else {
			Segment& secondLast = (*segments)[segments->size() - 2];
			last.spriteHorizontalIndex =
				Rail::middleSegmentSpriteHorizontalIndex(secondLast.x, secondLast.y, last.x, last.y, x, y);
		}
	}
	segments->push_back(Segment(x, y, color, group, spriteHorizontalIndex));
}
bool ResetSwitch::hasGroupForColor(char group, char color) {
	for (Segment& segment : leftSegments) {
		if (segment.group == group && segment.color == color)
			return true;
	}
	for (Segment& segment : bottomSegments) {
		if (segment.group == group && segment.color == color)
			return true;
	}
	for (Segment& segment : rightSegments) {
		if (segment.group == group && segment.color == color)
			return true;
	}
	return false;
}
void ResetSwitch::render(int screenLeftWorldX, int screenTopWorldY, bool isOn) {
	if (Editor::isActive && editorIsDeleted)
		return;

	glEnable(GL_BLEND);
	GLint drawLeftX = (GLint)(centerX * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)((bottomY - 1) * MapState::tileSize - screenTopWorldY);
	SpriteRegistry::resetSwitch->renderSpriteAtScreenPosition(isOn ? 1 : 0, 0, drawLeftX, drawTopY);
	for (Segment& segment : leftSegments)
		segment.render(screenLeftWorldX, screenTopWorldY);
	for (Segment& segment : bottomSegments)
		segment.render(screenLeftWorldX, screenTopWorldY);
	for (Segment& segment : rightSegments)
		segment.render(screenLeftWorldX, screenTopWorldY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void ResetSwitch::renderGroups(int screenLeftWorldX, int screenTopWorldY) {
	if (Editor::isActive && editorIsDeleted)
		return;

	for (Segment& segment : leftSegments)
		segment.renderGroup(screenLeftWorldX, screenTopWorldY);
	for (Segment& segment : bottomSegments)
		segment.renderGroup(screenLeftWorldX, screenTopWorldY);
	for (Segment& segment : rightSegments)
		segment.renderGroup(screenLeftWorldX, screenTopWorldY);
}
bool ResetSwitch::editorRemoveEndSegment(int x, int y, char color, char group) {
	vector<Segment>* allSegments[3] { &leftSegments, &bottomSegments, &rightSegments };
	for (vector<Segment>* segments : allSegments) {
		if (segments->empty())
			continue;
		Segment& lastSegment = segments->back();
		if (lastSegment.x == x && lastSegment.y == y && lastSegment.color == color && lastSegment.group == group) {
			segments->pop_back();
			return true;
		}
	}
	return false;
}
void ResetSwitch::editorRemoveSwitchSegment(char color, char group) {
	if (group == 0)
		return;
	vector<Segment>* allSegments[3] { &leftSegments, &bottomSegments, &rightSegments };
	for (vector<Segment>* segments : allSegments) {
		if (segments->empty())
			continue;
		for (int i = 1; i < (int)segments->size(); i++) {
			Segment& segment = (*segments)[i];
			if (segment.color != color || segment.group != group)
				continue;
			//we found a matching color + group, mark it for removal
			int removeCount = 1;
			i++;
			//if this is the only segment of the color, mark group 0 for removal too
			if ((*segments)[i - 2].group == 0 && (i == segments->size() || (*segments)[i].color != color))
				removeCount++;
			//shift the colors and groups back to preserve the positions + sprite indices
			for (; i < (int)segments->size(); i++) {
				Segment& deleteSegment = (*segments)[i - removeCount];
				Segment& remainingSegment = (*segments)[i];
				deleteSegment.color = remainingSegment.color;
				deleteSegment.group = remainingSegment.group;
			}
			segments->erase(segments->end() - removeCount, segments->end());
			return;
		}
	}
}
bool ResetSwitch::editorAddSegment(int x, int y, char color, char group) {
	//make sure this color/group combination doesn't already exist, except for group 0
	vector<Segment>* allSegments[3] { &leftSegments, &bottomSegments, &rightSegments };
	for (vector<Segment>* segments : allSegments) {
		for (Segment& segment : *segments) {
			if (segment.color == color && segment.group == group && group != 0)
				return false;
		}
	}
	bool newColor = true;
	vector<Segment>* segmentsToAddTo = nullptr;
	//if this position is adjacent to the reset switch bottom, we can definitely place a segment here
	if (x == centerX - 1 && y == bottomY)
		segmentsToAddTo = &leftSegments;
	else if (x == centerX && y == bottomY + 1)
		segmentsToAddTo = &bottomSegments;
	else if (x == centerX + 1 && y == bottomY)
		segmentsToAddTo = &rightSegments;
	//otherwise, make sure this new segment will be adjacent to the end of one of the segment lists, and that it isn't near any
	//	other segments
	else {
		//don't place a segment near the top of the reset switch body
		if (abs(x - centerX) <= 1 && y >= bottomY - 2 && y <= bottomY - 1)
			return false;
		for (vector<Segment>* segments : allSegments) {
			//this segment list is empty, we already know we aren't placing it on this side because otherwise we would have
			//	caught it in one of the above conditionals
			if (segments->empty())
				continue;
			Segment& endSegment = segments->back();
			int checkMaxIndex = segments->size();
			//it's correct to place a segment adjacent to the last segment, if we don't already have a segments list to add to
			if (abs(x - endSegment.x) + abs(y - endSegment.y) == 1 && segmentsToAddTo == nullptr) {
				segmentsToAddTo = segments;
				checkMaxIndex -= 2;
			}
			//make sure this isn't near any other segments
			for (int i = 0; i < checkMaxIndex; i++) {
				Segment& segment = (*segments)[i];
				if (abs(x - segment.x) <= 1 && abs(y - segment.y) <= 1)
					return false;
			}
		}
		if (segmentsToAddTo == nullptr)
			return false;
		newColor = color != segmentsToAddTo->back().color;
		//can't add a third color
		if (newColor && segmentsToAddTo->front().color != segmentsToAddTo->back().color)
			return false;
	}
	//adding a new color and group 0 must be paired with each other
	if (newColor != (group == 0))
		return false;

	//if we get here, we can add this segment
	addSegment(x, y, color, group, segmentsToAddTo);
	return true;
}
void ResetSwitch::editorRewriteGroup(char color, char oldGroup, char group) {
	vector<Segment>* allSegments[3] { &leftSegments, &bottomSegments, &rightSegments };
	for (vector<Segment>* segments : allSegments) {
		for (Segment& segment : *segments) {
			if (segment.color == color && segment.group == oldGroup)
				segment.group = group;
		}
	}
}
char ResetSwitch::editorGetFloorSaveData(int x, int y) {
	if (x == centerX && y == bottomY)
		return MapState::floorResetSwitchHeadValue;
	vector<Segment>* allSegments[3] { &leftSegments, &bottomSegments, &rightSegments };
	for (vector<Segment>* segments : allSegments) {
		char floorSaveData = editorGetSegmentFloorSaveData(x, y, *segments);
		if (floorSaveData != 0)
			return floorSaveData;
	}
	return 0;
}
char ResetSwitch::editorGetSegmentFloorSaveData(int x, int y, vector<Segment>& segments) {
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

//////////////////////////////// ResetSwitchState ////////////////////////////////
ResetSwitchState::ResetSwitchState(objCounterParametersComma() ResetSwitch* pResetSwitch)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
resetSwitch(pResetSwitch)
, flipOffTicksTime(0) {
}
ResetSwitchState::~ResetSwitchState() {}
void ResetSwitchState::flip(int flipOnTicksTime) {
	flipOffTicksTime = flipOnTicksTime + MapState::switchFlipDuration;
}
void ResetSwitchState::updateWithPreviousResetSwitchState(ResetSwitchState* prev) {
	flipOffTicksTime = prev->flipOffTicksTime;
}
void ResetSwitchState::render(int screenLeftWorldX, int screenTopWorldY, int ticksTime) {
	resetSwitch->render(screenLeftWorldX, screenTopWorldY, ticksTime < flipOffTicksTime);
}
