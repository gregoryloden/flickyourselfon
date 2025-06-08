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
	Rail::setSegmentColor(color, 1.0f, 1.0f);
	GLint drawLeftX = (GLint)(x * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(y * MapState::tileSize - screenTopWorldY);
	(SpriteRegistry::rails->*SpriteSheet::renderSpriteAtScreenPosition)(spriteHorizontalIndex, 0, drawLeftX, drawTopY);
}
void ResetSwitch::Segment::renderGroup(int screenLeftWorldX, int screenTopWorldY) {
	if (group == 0)
		return;
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
void ResetSwitch::getHintRenderBounds(int* outLeftWorldX, int* outTopWorldY, int* outRightWorldX, int* outBottomWorldY) {
	*outLeftWorldX = centerX * MapState::tileSize;
	*outTopWorldY = (bottomY - 1) * MapState::tileSize;
	*outRightWorldX = (centerX + 1) * MapState::tileSize;
	*outBottomWorldY = (bottomY + 1) * MapState::tileSize;
}
void ResetSwitch::render(int screenLeftWorldX, int screenTopWorldY, bool isOn) {
	if (Editor::isActive && editorIsDeleted)
		return;

	GLint drawLeftX = (GLint)(centerX * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)((bottomY - 1) * MapState::tileSize - screenTopWorldY);
	(SpriteRegistry::resetSwitch->*SpriteSheet::renderSpriteAtScreenPosition)(isOn ? 1 : 0, 0, drawLeftX, drawTopY);
	for (Segment& segment : leftSegments)
		segment.render(screenLeftWorldX, screenTopWorldY);
	for (Segment& segment : bottomSegments)
		segment.render(screenLeftWorldX, screenTopWorldY);
	for (Segment& segment : rightSegments)
		segment.render(screenLeftWorldX, screenTopWorldY);
	(SpriteRegistry::rails->*SpriteSheet::setSpriteColor)(1.0f, 1.0f, 1.0f, 1.0f);
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
void ResetSwitch::renderHint(int screenLeftWorldX, int screenTopWorldY, float alpha) {
	SpriteSheet::setRectangleColor(1.0f, 1.0f, 1.0f, alpha);
	GLint drawLeftX = (GLint)(centerX * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)((bottomY - 1) * MapState::tileSize - screenTopWorldY);
	SpriteSheet::renderPreColoredRectangle(
		drawLeftX, drawTopY, drawLeftX + (GLint)MapState::tileSize, drawTopY + (GLint)MapState::tileSize * 2);
	SpriteSheet::setRectangleColor(1.0f, 1.0f, 1.0f, 1.0f);
}
bool ResetSwitch::editorRemoveSegment(int x, int y, char color, char group, int* outFreeX, int* outFreeY) {
	vector<Segment>* allSegments[] { &leftSegments, &bottomSegments, &rightSegments };
	for (vector<Segment>* segments : allSegments) {
		for (int i = 0; i < (int)segments->size(); i++) {
			Segment& segment = (*segments)[i];
			if (segment.x == x && segment.y == y && segment.color == color && segment.group == group) {
				//don't delete group 0 if there's a segment of the same color after it
				if (group == 0 && i + 1 < (int)segments->size() && (*segments)[i + 1].color == color)
					return false;
				editorRemoveFoundSegment(segments, i, outFreeX, outFreeY);
				return true;
			}
		}
	}
	return false;
}
bool ResetSwitch::editorRemoveSwitchSegment(char color, char group, int* outFreeX, int* outFreeY) {
	if (group == 0)
		return false;
	vector<Segment>* allSegments[] { &leftSegments, &bottomSegments, &rightSegments };
	for (vector<Segment>* segments : allSegments) {
		for (int i = 1; i < (int)segments->size(); i++) {
			Segment& segment = (*segments)[i];
			if (segment.color != color || segment.group != group)
				continue;
			//we found a matching color + group, delete it
			editorRemoveFoundSegment(segments, i, outFreeX, outFreeY);
			return true;
		}
	}
	return false;
}
void ResetSwitch::editorRemoveFoundSegment(vector<Segment>* segments, int segmentI, int* outFreeX, int* outFreeY) {
	//shift only the colors and groups back to preserve the positions + sprite indices
	for (segmentI++; segmentI < (int)segments->size(); segmentI++) {
		Segment& deleteSegment = (*segments)[segmentI - 1];
		Segment& remainingSegment = (*segments)[segmentI];
		deleteSegment.color = remainingSegment.color;
		deleteSegment.group = remainingSegment.group;
	}
	*outFreeX = segments->back().x;
	*outFreeY = segments->back().y;
	segments->erase(segments->end() - 1);
}
bool ResetSwitch::editorAddSegment(int x, int y, char color, char group) {
	//make sure this color/group combination doesn't already exist, except for group 0
	vector<Segment>* allSegments[] { &leftSegments, &bottomSegments, &rightSegments };
	for (vector<Segment>* segments : allSegments) {
		for (Segment& segment : *segments) {
			if (segment.color == color && segment.group == group && group != 0)
				return false;
		}
	}
	bool newColor = true;
	bool insertingGroup = false;
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
		//not a new color, we can definitely add it
		if (!newColor)
			;
		//if it's the same as the first color, we'll insert it at the end of the first color
		else if (color == segmentsToAddTo->front().color) {
			insertingGroup = true;
			//and it's actually not a new color
			newColor = false;
		//we can't add a third color
		} else if (segmentsToAddTo->front().color != segmentsToAddTo->back().color)
			return false;
	}
	//adding a new color and group 0 must be paired with each other
	if (newColor != (group == 0))
		return false;

	//if we get here, we can add this segment
	addSegment(x, y, color, group, segmentsToAddTo);
	//if we needed to insert it, shift the other segment group and colors back
	if (insertingGroup) {
		for (int i = (int)segmentsToAddTo->size() - 1; i >= 1; i--) {
			Segment& shiftFromSegment = (*segmentsToAddTo)[i - 1];
			Segment& shiftToSegment = (*segmentsToAddTo)[i];
			shiftToSegment.color = shiftFromSegment.color;
			shiftToSegment.group = shiftFromSegment.group;
			//the old group 0 segment for the second color is where the new segment goes
			if (shiftFromSegment.group == 0) {
				shiftFromSegment.color = color;
				shiftFromSegment.group = group;
				break;
			}
		}
	}
	return true;
}
void ResetSwitch::editorRewriteGroup(char color, char oldGroup, char group) {
	vector<Segment>* allSegments[] { &leftSegments, &bottomSegments, &rightSegments };
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
	vector<Segment>* allSegments[] { &leftSegments, &bottomSegments, &rightSegments };
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
