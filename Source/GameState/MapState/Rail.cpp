#include "Rail.h"
#include "Editor/Editor.h"
#include "GameState/MapState/MapState.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"

//////////////////////////////// Rail::Segment ////////////////////////////////
Rail::Segment::Segment(int pX, int pY, char pMaxTileOffset)
: x(pX)
, y(pY)
//use the bottom end sprite as the default, we'll fix it later
, spriteHorizontalIndex(endSegmentSpriteHorizontalIndex(0, -1))
, maxTileOffset(pMaxTileOffset) {
}
Rail::Segment::~Segment() {}
float Rail::Segment::tileCenterX() {
	return ((float)x + 0.5f) * (float)MapState::tileSize;
}
float Rail::Segment::tileCenterY() {
	return ((float)y + 0.5f) * (float)MapState::tileSize;
}
void Rail::Segment::render(int screenLeftWorldX, int screenTopWorldY, float tileOffset, char baseHeight) {
	int yPixelOffset = (int)(tileOffset * (float)MapState::tileSize + 0.5f);
	int topWorldY = y * MapState::tileSize + yPixelOffset;
	int bottomTileY = (topWorldY + MapState::tileSize - 1) / MapState::tileSize;
	GLint drawLeftX = (GLint)(x * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(topWorldY - screenTopWorldY);
	char topTileHeight = MapState::getHeight(x, topWorldY / MapState::tileSize);
	char bottomTileHeight = MapState::getHeight(x, bottomTileY);
	bool topBorder = false;
	bool bottomBorder = false;

	//this segment is completely visible
	if (bottomTileHeight == MapState::emptySpaceHeight
		|| bottomTileHeight <= baseHeight - (char)((yPixelOffset + MapState::tileSize - 1) / MapState::tileSize) * 2)
	{
		SpriteRegistry::rails->renderSpriteAtScreenPosition(spriteHorizontalIndex, 0, drawLeftX, drawTopY);
		topBorder = topTileHeight % 2 == 1 && topTileHeight != MapState::emptySpaceHeight;
		bottomBorder = bottomTileHeight % 2 == 1 && bottomTileHeight != MapState::emptySpaceHeight;
	//the top part of this segment is visible
	} else if (topTileHeight == MapState::emptySpaceHeight
		|| topTileHeight <= baseHeight - (char)(yPixelOffset / MapState::tileSize) * 2)
	{
		int spriteX = spriteHorizontalIndex * MapState::tileSize;
		int spriteHeight = bottomTileY * MapState::tileSize - topWorldY;
		SpriteRegistry::rails->renderSpriteSheetRegionAtScreenRegion(
			spriteX,
			0,
			spriteX + MapState::tileSize,
			spriteHeight,
			drawLeftX,
			drawTopY,
			drawLeftX + (GLint)MapState::tileSize,
			drawTopY + (GLint)spriteHeight);
		topBorder = topTileHeight % 2 == 1 && topTileHeight != MapState::emptySpaceHeight;
	}

	if ((topBorder || bottomBorder) && spriteHorizontalIndex < spriteHorizontalIndexEndFirst) {
		int topSpriteHeight = bottomTileY * MapState::tileSize - topWorldY;
		int spriteX = (spriteHorizontalIndex + spriteHorizontalIndexBorderFirst) * MapState::tileSize;
		int spriteTop = topBorder ? 0 : topSpriteHeight;
		int spriteBottom = bottomBorder ? MapState::tileSize : topSpriteHeight;
		SpriteRegistry::rails->renderSpriteSheetRegionAtScreenRegion(
			spriteX,
			spriteTop,
			spriteX + MapState::tileSize,
			spriteBottom,
			drawLeftX,
			drawTopY + (GLint)spriteTop,
			drawLeftX + (GLint)MapState::tileSize,
			drawTopY + (GLint)spriteBottom);
	}
}

//////////////////////////////// Rail ////////////////////////////////
Rail::Rail(
	objCounterParametersComma()
	int x,
	int y,
	char pBaseHeight,
	char pColor,
	char pInitialTileOffset,
	char pInitialMovementDirection,
	char pMovementMagnitude)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
baseHeight(pBaseHeight)
, color(pColor)
, segments(new vector<Segment>())
, groups()
, initialTileOffset(pInitialTileOffset)
//give a default tile offset extending to the lowest height, we'll correct it as we add segments
, maxTileOffset(pBaseHeight / 2)
, initialMovementDirection(pInitialMovementDirection)
, movementMagnitude(pMovementMagnitude)
, renderLeftTileX(0)
, renderTopTileY(0)
, renderRightTileX(MapState::getMapWidth())
, renderBottomTileY(MapState::getMapHeight())
, renderHintBottomTileY(MapState::getMapHeight())
, editorIsDeleted(false) {
	segments->push_back(Segment(x, y, 0));
	if (pColor == MapState::sineColor)
		maxTileOffset = 4;
}
Rail::~Rail() {
	delete segments;
}
int Rail::endSegmentSpriteHorizontalIndex(int xExtents, int yExtents) {
	return yExtents != 0
		? Segment::spriteHorizontalIndexEndVerticalFirst + (1 - yExtents) / 2
		: Segment::spriteHorizontalIndexEndHorizontalFirst + (1 - xExtents) / 2;
}
int Rail::middleSegmentSpriteHorizontalIndex(int prevX, int prevY, int x, int y, int nextX, int nextY) {
	//vertical rail if they have the same x
	if (prevX == nextX)
		return Segment::spriteHorizontalIndexVertical;
	//horizontal rail if they have the same y
	else if (prevY == nextY)
		return Segment::spriteHorizontalIndexHorizontal;
	//each extents is the sum of a 0 and a 1 or -1
	else {
		int xExtentsSum = prevX - x + nextX - x;
		int yExtentsSum = prevY - y + nextY - y;
		return 3 - yExtentsSum + (1 - xExtentsSum) / 2;
	}
}
int Rail::extentSegmentSpriteHorizontalIndex(int prevX, int prevY, int x, int y) {
	return middleSegmentSpriteHorizontalIndex(prevX, prevY, x, y, x + (x - prevX), y + (y - prevY));
}
void Rail::setSegmentColor(int railColor, float saturation, float alpha) {
	static constexpr float sineColorIntensity = 14.0f / 16.0f;
	float redColor = sineColorIntensity;
	float greenColor = sineColorIntensity;
	float blueColor = sineColorIntensity;
	if (railColor != MapState::sineColor) {
		static constexpr float nonColorIntensity = 9.0f / 16.0f;
		redColor = railColor == MapState::squareColor ? 1.0f : nonColorIntensity;
		greenColor = railColor == MapState::sawColor ? 1.0f : nonColorIntensity;
		blueColor = railColor == MapState::triangleColor ? 1.0f : nonColorIntensity;
	}
	static constexpr float desaturatedColorIntensity = 10.0f / 16.0f;
	float desaturatedAdd = (1.0f - saturation) * desaturatedColorIntensity;
	glColor4f(
		redColor * saturation + desaturatedAdd,
		greenColor * saturation + desaturatedAdd,
		blueColor * saturation + desaturatedAdd,
		alpha);
}
void Rail::reverseSegments() {
	vector<Segment>* newSegments = new vector<Segment>();
	for (int i = (int)segments->size() - 1; i >= 0; i--)
		newSegments->push_back((*segments)[i]);
	delete segments;
	segments = newSegments;
}
void Rail::addGroup(char group) {
	if (group == 0)
		return;
	for (int i = 0; i < (int)groups.size(); i++) {
		if (groups[i] == group)
			return;
	}
	groups.push_back(group);
}
void Rail::addSegment(int x, int y) {
	//if we aren't adding at the end, reverse the list before continuing
	Segment* end = &segments->back();
	if (abs(y - end->y) + abs(x - end->x) != 1)
		reverseSegments();

	//find the tile where the shadow should go
	for (char segmentMaxTileOffset = 0; true; segmentMaxTileOffset++) {
		//in the event we actually went past the bottom of the map, don't try to find the tile height,
		//	this segment does not have an offset and we're done searching
		if (y + segmentMaxTileOffset >= MapState::getMapHeight())
			segmentMaxTileOffset = Segment::absentTileOffset;
		else {
			char railGroundHeight = baseHeight - (segmentMaxTileOffset * 2);
			char tileHeight = MapState::getHeight(x, y + segmentMaxTileOffset);
			//keep looking if we see an empty space tile
			if (tileHeight == MapState::emptySpaceHeight) {
				//but only if we haven't yet reached the theoretical-height-0 tile
				//in that case, this segment does not have an offset
				if (railGroundHeight == 0)
					segmentMaxTileOffset = Segment::absentTileOffset;
				else
					continue;
			//keep looking if we see a non-empty-space tile that's too low to be a ground tile at this y
			} else if (tileHeight < railGroundHeight)
				continue;
			//we ended up on a tile that is above what would be a ground tile for this rail, which means that that the rail
			//	hides behind it- mark that this segment does not have an offset
			else if (tileHeight > railGroundHeight)
				segmentMaxTileOffset = Segment::absentTileOffset;
		}
		segments->push_back(Segment(x, y, segmentMaxTileOffset));
		break;
	}

	//our newest segment is an end segment, figure out which way it should face
	Segment* lastEnd = &(*segments)[segments->size() - 2];
	end = &segments->back();
	end->spriteHorizontalIndex = endSegmentSpriteHorizontalIndex(lastEnd->x - end->x, lastEnd->y - end->y);

	//our last end segment is now a middle rail, find its sprite and account for its max height offset
	if (segments->size() >= 3) {
		Segment* secondLastEnd = &(*segments)[segments->size() - 3];
		lastEnd->spriteHorizontalIndex =
			middleSegmentSpriteHorizontalIndex(secondLastEnd->x, secondLastEnd->y, lastEnd->x, lastEnd->y, end->x, end->y);
		if (lastEnd->maxTileOffset != Segment::absentTileOffset) {
			if (color != MapState::sineColor)
				maxTileOffset = MathUtils::min(lastEnd->maxTileOffset, maxTileOffset);
			initialTileOffset = MathUtils::min(maxTileOffset, initialTileOffset);
		}
	//we only have 2 segments so the other rail is just the end segment that complements this
	} else
		lastEnd->spriteHorizontalIndex = endSegmentSpriteHorizontalIndex(end->x - lastEnd->x, end->y - lastEnd->y);
}
void Rail::assignRenderBox() {
	renderLeftTileX = MapState::getMapWidth();
	renderTopTileY = MapState::getMapHeight();
	renderRightTileX = 0;
	renderHintBottomTileY = 0;
	char renderMaxTileOffset = maxTileOffset;
	for (Segment& segment : *segments) {
		//extend the render box
		renderLeftTileX = MathUtils::min(segment.x, renderLeftTileX);
		renderTopTileY = MathUtils::min(segment.y, renderTopTileY);
		renderRightTileX = MathUtils::max(segment.x + 1, renderRightTileX);
		renderHintBottomTileY = MathUtils::max(segment.y + 1, renderHintBottomTileY);
		if (segment.maxTileOffset != Segment::absentTileOffset)
			renderMaxTileOffset = MathUtils::max(segment.maxTileOffset, renderMaxTileOffset);
	}
	renderBottomTileY = renderHintBottomTileY + renderMaxTileOffset;
}
bool Rail::triggerMovement(char movementDirection, char* inOutTileOffset) {
	switch (color) {
		//square wave rail: swap the tile offset between 0 and the max tile offset
		case MapState::squareColor:
			*inOutTileOffset = *inOutTileOffset == 0 ? maxTileOffset : 0;
			return false;
		//triangle wave rail: move the rail movementMagnitude tiles in its current movement direction, possibly bouncing and
		//	reversing direction
		case MapState::triangleColor: {
			char newTileOffset = *inOutTileOffset + movementMagnitude * movementDirection;
			if (newTileOffset < 0)
				*inOutTileOffset = -newTileOffset;
			else if (newTileOffset > maxTileOffset)
				*inOutTileOffset = maxTileOffset * 2 - newTileOffset;
			else {
				*inOutTileOffset = newTileOffset;
				return false;
			}
			return true;
		}
		//saw wave rail: move the rail movementMagnitude tiles up, possibly sending it to the bottom
		case MapState::sawColor: {
			char newTileOffset = *inOutTileOffset + movementMagnitude * movementDirection;
			if (newTileOffset < 0)
				*inOutTileOffset = newTileOffset + maxTileOffset;
			else if (newTileOffset >= maxTileOffset)
				*inOutTileOffset = newTileOffset - maxTileOffset;
			else {
				*inOutTileOffset = newTileOffset;
				return false;
			}
			return true;
		}
		//sine wave rail: move the rail to the next of the 3 static sine wave positions for this movement direction
		//higher movement magnitudes will move more than one position at a time
		case MapState::sineColor: {
			//pick the next offset for the rail, based on knowing [movement magnitude][movement direction][tile offset]
			static constexpr char sineWaveNextOffset[3][3][5] = {
				//movement magnitude index is (movement magnitude - 1)
				//movement direction index is (movement direction + 1)
				//tile offset index is (tile offset)
				//certain tile offset inputs will be unused, but put values for them anyways to advance to the next valid tile
				//	offset
				//bit flipped tile offset values are negative, for those values the real target is ~N (after a bounce)
				{ { ~1, 0, 1, 1, 3 }, { 0, 0, 0, 0, 0 }, { 1, 3, 3, 4, ~3 } },
				{ { ~3, 0, 0, 0, 3 }, { 0, 0, 0, 0, 0 }, { 3, 3, 3, ~3, ~3 } },
				{ { ~4, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 4, 4, 4, 4, ~0 } },
			};
			char newTileOffset = sineWaveNextOffset[movementMagnitude - 1][movementDirection + 1][*inOutTileOffset];
			//bit-flipped values need to be restored, and result in a bounce
			if (newTileOffset < 0)
				*inOutTileOffset = ~newTileOffset;
			else {
				*inOutTileOffset = newTileOffset;
				return false;
			}
			return true;
		}
		default:
			return false;
	}
}
bool Rail::canRender(int screenLeftTileX, int screenTopTileY, int screenRightTileX, int screenBottomTileY) {
	return renderLeftTileX < screenRightTileX
		&& renderRightTileX > screenLeftTileX
		&& renderTopTileY < screenBottomTileY
		&& renderBottomTileY > screenTopTileY;
}
void Rail::getHintRenderBounds(int* outLeftWorldX, int* outTopWorldY, int* outRightWorldX, int* outBottomWorldY) {
	*outLeftWorldX = renderLeftTileX * MapState::tileSize;
	*outTopWorldY = renderTopTileY * MapState::tileSize;
	*outRightWorldX = renderRightTileX * MapState::tileSize;
	*outBottomWorldY = renderBottomTileY * MapState::tileSize;
}
void Rail::renderShadow(int screenLeftWorldX, int screenTopWorldY) {
	if (Editor::isActive && editorIsDeleted)
		return;
	glEnable(GL_BLEND);

	int lastSegmentIndex = (int)segments->size() - 1;
	for (int i = 1; i < lastSegmentIndex; i++) {
		Segment& segment = (*segments)[i];
		//don't render a shadow if this segment hides behind a platform or can't be lowered
		if (segment.maxTileOffset > 0)
			SpriteRegistry::rails->renderSpriteAtScreenPosition(
				segment.spriteHorizontalIndex + Segment::spriteHorizontalIndexShadowFirst,
				0,
				(GLint)(segment.x * MapState::tileSize - screenLeftWorldX),
				(GLint)((segment.y + (int)segment.maxTileOffset) * MapState::tileSize - screenTopWorldY));
	}
}
void Rail::renderGroups(int screenLeftWorldX, int screenTopWorldY) {
	if (Editor::isActive && editorIsDeleted)
		return;
	for (int i = 0; i < (int)segments->size(); i++) {
		Segment& segment = (*segments)[i];
		GLint drawLeftX = (GLint)(segment.x * MapState::tileSize - screenLeftWorldX);
		GLint drawTopY = (GLint)(segment.y * MapState::tileSize - screenTopWorldY);
		MapState::renderGroupRect(groups[i % groups.size()], drawLeftX + 2, drawTopY + 2, drawLeftX + 4, drawTopY + 4);
	}
}
void Rail::renderHint(int screenLeftWorldX, int screenTopWorldY, float alpha) {
	glEnable(GL_BLEND);
	glColor4f(1.0f, 1.0f, 1.0f, alpha);
	for (Segment& segment : *segments) {
		GLint leftX = (GLint)(segment.x * MapState::tileSize - screenLeftWorldX);
		GLint topY = (GLint)(segment.y * MapState::tileSize - screenTopWorldY);
		SpriteSheet::renderPreColoredRectangle(
			leftX, topY, leftX + (GLint)MapState::tileSize, topY + (GLint)MapState::tileSize);
	}
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
bool Rail::editorRemoveGroup(char group) {
	for (int i = 0; i < (int)groups.size(); i++) {
		if (groups[i] == group) {
			groups.erase(groups.begin() + i);
			return true;
		}
	}
	return false;
}
bool Rail::editorRemoveSegment(int x, int y, char pColor, char group) {
	//colors usually have to match, unless all the groups are 0
	if (pColor != color) {
		//if all groups are 0, change the color
		if (groups.empty() && group == 0)
			color = pColor;
		//either way, don't modify the rail in any other way
		return false;
	}
	//can only remove a segment from the start or end of the rail
	Segment& start = segments->front();
	Segment& end = segments->back();
	if (y == start.y && x == start.x)
		segments->erase(segments->begin());
	else if (y == end.y && x == end.x)
		segments->pop_back();
	//if we clicked a middle segment, we can add or remove a group
	else {
		//if the group isn't 0, try to remove it, and if it's not present, add it
		if (group != 0 && !editorRemoveGroup(group))
			groups.push_back(group);
		//either way, don't modify the rail in any other way
		return false;
	}
	editorRemoveGroup(group);
	//reset the max tile offset, find the smallest offset among the non-end segments
	if (color != MapState::sineColor) {
		maxTileOffset = baseHeight / 2;
		for (int i = 1; i < (int)segments->size() - 1; i++) {
			Segment& segment = (*segments)[i];
			if (segment.maxTileOffset != Segment::absentTileOffset)
				maxTileOffset = MathUtils::min(segment.maxTileOffset, maxTileOffset);
		}
	}
	if (segments->size() == 0)
		editorIsDeleted = true;
	else
		assignRenderBox();
	return true;
}
bool Rail::editorAddSegment(int x, int y, char pColor, char group, char tileHeight) {
	//don't add a segment if it's on on a non-empty-space tile above the rail
	if (tileHeight > baseHeight && tileHeight != MapState::emptySpaceHeight)
		return false;
	//make sure the colors match
	if (pColor != color)
		return false;
	//make sure the new rail touches exactly one end segment
	int checkMinIndex = 0;
	int checkMaxIndex = segments->size();
	Segment& start = segments->front();
	Segment& end = segments->back();
	if (abs(x - start.x) + abs(y - start.y) == 1)
		checkMinIndex += 2;
	else if (abs(x - end.x) + abs(y - end.y) == 1)
		checkMaxIndex -= 2;
	else
		return false;
	//make sure the new rail doesn't go near any other segments other than the near end of the rail
	for (int checkIndex = checkMinIndex; checkIndex < checkMaxIndex; checkIndex++) {
		Segment& checkSegment = (*segments)[checkIndex];
		if (abs(x - checkSegment.x) <= 1 && abs(y - checkSegment.y) <= 1)
			return false;
	}
	addGroup(group);
	addSegment(x, y);
	assignRenderBox();
	return true;
}
void Rail::editorAdjustMovementMagnitude(int x, int y, char magnitudeAdd) {
	Segment& start = segments->front();
	Segment& end = segments->back();
	if ((x == start.x && y == start.y) || (x == end.x && y == end.y))
		movementMagnitude = MathUtils::max(1, MathUtils::min(maxTileOffset, movementMagnitude + magnitudeAdd));
	if (color == MapState::sineColor)
		movementMagnitude = MathUtils::min(3, movementMagnitude);
}
void Rail::editorToggleMovementDirection(int x, int y) {
	Segment& start = segments->front();
	Segment& end = segments->back();
	if (((x == start.x && y == start.y) || (x == end.x && y == end.y)) && color != MapState::sawColor)
		initialMovementDirection = -initialMovementDirection;
}
void Rail::editorAdjustInitialTileOffset(int x, int y, char tileOffset) {
	Segment& start = segments->front();
	Segment& end = segments->back();
	if ((x == start.x && y == start.y) || (x == end.x && y == end.y))
		initialTileOffset = MathUtils::max(0, MathUtils::min(maxTileOffset, initialTileOffset + tileOffset));
}
char Rail::editorGetFloorSaveData(int x, int y) {
	Segment& start = segments->front();
	if (x == start.x && y == start.y)
		return (color << MapState::floorRailSwitchColorDataShift)
			| (initialTileOffset << MapState::floorRailInitialTileOffsetDataShift)
			| MapState::floorRailHeadValue;
	Segment& second = (*segments)[1];
	if (x == second.x && y == second.y) {
		char unshiftedData = ((initialMovementDirection + 1) / 2) | movementMagnitude << 1;
		return unshiftedData << MapState::floorRailByte2DataShift | MapState::floorIsRailSwitchBitmask;
	}
	for (int i = 2; i < (int)segments->size() && i - 2 < (int)groups.size(); i++) {
		Segment& segment = (*segments)[i];
		if (segment.x == x && segment.y == y)
			return groups[i - 2] << MapState::floorRailSwitchGroupDataShift | MapState::floorIsRailSwitchBitmask;
	}
	//no data but still part of this rail
	return MapState::floorRailSwitchTailValue;
}

//////////////////////////////// RailState ////////////////////////////////
RailState::RailState(objCounterParametersComma() Rail* pRail)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
rail(pRail)
, tileOffset((float)pRail->getInitialTileOffset())
, targetTileOffset(pRail->getInitialTileOffset())
, currentMovementDirection(pRail->getInitialMovementDirection())
, effectiveHeight((float)(pRail->getBaseHeight() - 2 * pRail->getInitialTileOffset()))
, bouncesRemaining(0)
, nextMovementDirection(pRail->getInitialMovementDirection())
, distancePerMovement(pRail->getColor() == MapState::squareColor ? pRail->getMaxTileOffset() : pRail->getMovementMagnitude())
, segmentsAbovePlayer()
, lastUpdateTicksTime(0) {
}
RailState::~RailState() {
	//don't delete the rail, it's owned by MapState
}
void RailState::updateWithPreviousRailState(RailState* prev, int ticksTime) {
	targetTileOffset = prev->targetTileOffset;
	lastUpdateTicksTime = ticksTime;
	if (Editor::isActive) {
		targetTileOffset = rail->getInitialTileOffset();
		tileOffset = (float)targetTileOffset;
		effectiveHeight = rail->getBaseHeight() - 2 * tileOffset;
		nextMovementDirection = rail->getInitialMovementDirection();
		return;
	}

	currentMovementDirection = prev->currentMovementDirection;
	bouncesRemaining = prev->bouncesRemaining;
	nextMovementDirection = prev->nextMovementDirection;
	float tileOffsetDiff = distancePerMovement * (ticksTime - prev->lastUpdateTicksTime) / fullMovementDurationTicks;
	//adjust speed/distance depending on the rail
	switch (rail->getColor()) {
		case MapState::squareColor:
			tileOffsetDiff *= 2.0f;
			break;
		case MapState::sawColor:
			if ((bouncesRemaining & 1) == 1)
				tileOffsetDiff *= 8.0f;
			break;
		case MapState::sineColor:
			updateSineRailTileOffset(prev, ticksTime);
			effectiveHeight = rail->getBaseHeight() - 2 * tileOffset;
			return;
		case MapState::triangleColor:
		default:
			break;
	}
	if (bouncesRemaining != 0) {
		//advance the rail, bouncing it if applicable
		tileOffset =
			prev->tileOffset + tileOffsetDiff * (bouncesRemaining > 0 ? currentMovementDirection : -currentMovementDirection);
		if (tileOffset < 0)
			tileOffset = MathUtils::fmin(-tileOffset, targetTileOffset);
		else if (tileOffset > rail->getMaxTileOffset())
			tileOffset = MathUtils::fmax(rail->getMaxTileOffset() * 2 - tileOffset, targetTileOffset);
		else {
			effectiveHeight = rail->getBaseHeight() - 2 * tileOffset;
			return;
		}
		currentMovementDirection = -currentMovementDirection;
		bouncesRemaining -= (bouncesRemaining > 0 ? 1 : -1);
	} else if (prev->tileOffset != targetTileOffset)
		tileOffset = prev->tileOffset > targetTileOffset
			? MathUtils::fmax(targetTileOffset, prev->tileOffset - tileOffsetDiff)
			: MathUtils::fmin(targetTileOffset, prev->tileOffset + tileOffsetDiff);
	else
		tileOffset = targetTileOffset;
	effectiveHeight = rail->getBaseHeight() - 2 * tileOffset;
}
void RailState::updateSineRailTileOffset(RailState* prev, int ticksTime) {
	//find the current "angle" for the rail
	float tileOffsetAngle = acosf(1.0f - prev->tileOffset / rail->getMaxTileOffset() * 2.0f);
	if (currentMovementDirection < 0)
		tileOffsetAngle = -tileOffsetAngle;
	float tileOffsetAngleDiff =
		distancePerMovement * MathUtils::piOver3 * (ticksTime - prev->lastUpdateTicksTime) / fullMovementDurationTicks;
	//find the next "angle" for the rail
	float nextTileOffsetAngle;
	if (bouncesRemaining > 0) {
		nextTileOffsetAngle = tileOffsetAngle + tileOffsetAngleDiff;
		if (nextTileOffsetAngle > MathUtils::pi || (nextTileOffsetAngle > 0 && tileOffsetAngle <= 0)) {
			currentMovementDirection = -currentMovementDirection;
			bouncesRemaining--;
		}
	} else if (bouncesRemaining < 0) {
		nextTileOffsetAngle = tileOffsetAngle - tileOffsetAngleDiff;
		if (nextTileOffsetAngle < -MathUtils::pi || (nextTileOffsetAngle < 0 && tileOffsetAngle >= 0)) {
			currentMovementDirection = -currentMovementDirection;
			bouncesRemaining++;
		}
	} else if (prev->tileOffset != targetTileOffset) {
		if (tileOffsetAngle < 0)
			tileOffsetAngle = -tileOffsetAngle;
		float targetTileOffsetAngle = acosf(1.0f - (float)targetTileOffset / rail->getMaxTileOffset() * 2.0f);
		if (abs(tileOffsetAngle - targetTileOffsetAngle) <= tileOffsetAngleDiff) {
			tileOffset = targetTileOffset;
			return;
		} else
			nextTileOffsetAngle = tileOffsetAngle < targetTileOffsetAngle
				? tileOffsetAngle + tileOffsetAngleDiff
				: tileOffsetAngle - tileOffsetAngleDiff;
	} else {
		tileOffset = targetTileOffset;
		return;
	}
	//convert the "angle" back to a tile offset
	tileOffset = (1.0f - cosf(nextTileOffsetAngle)) / 2.0f * rail->getMaxTileOffset();
}
void RailState::triggerMovement(bool moveForward) {
	if (rail->triggerMovement(moveForward ? nextMovementDirection : -nextMovementDirection, &targetTileOffset)) {
		if (rail->getColor() == MapState::sawColor)
			bouncesRemaining += (moveForward ? 2 : -2);
		else {
			nextMovementDirection = -nextMovementDirection;
			bouncesRemaining += (moveForward ? 1 : -1);
		}
	}
}
void RailState::renderBelowPlayer(int screenLeftWorldX, int screenTopWorldY, float playerWorldGroundY) {
	if (Editor::isActive && rail->editorIsDeleted)
		return;

	setSegmentColor();
	int lastSegmentIndex = rail->getSegmentCount() - 1;
	char baseHeight = rail->getBaseHeight();
	float groundYOffset = baseHeight / 2 + 0.5f;
	for (int i = 1; i < lastSegmentIndex; i++) {
		Rail::Segment* segment = rail->getSegment(i);
		float segmentWorldGroundY = (segment->y + groundYOffset) * MapState::tileSize;
		if (segmentWorldGroundY <= playerWorldGroundY)
			segment->render(screenLeftWorldX, screenTopWorldY, tileOffset, baseHeight);
		else
			segmentsAbovePlayer.push_back(segment);
	}
	Rail::setSegmentColor(rail->getColor(), 1.0f, canRide() ? 1.0f : nearlyRaisedAlphaIntensity);
	rail->getSegment(0)->render(screenLeftWorldX, screenTopWorldY, 0.0f, baseHeight);
	rail->getSegment(lastSegmentIndex)->render(screenLeftWorldX, screenTopWorldY, 0.0f, baseHeight);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void RailState::renderAbovePlayer(int screenLeftWorldX, int screenTopWorldY) {
	if (Editor::isActive && rail->editorIsDeleted)
		return;

	setSegmentColor();
	char baseHeight = rail->getBaseHeight();
	for (Rail::Segment* segment : segmentsAbovePlayer)
		segment->render(screenLeftWorldX, screenTopWorldY, tileOffset, baseHeight);
	segmentsAbovePlayer.clear();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void RailState::setSegmentColor() {
	static constexpr float loweredAlphaIntensity = 0.5f;
	static constexpr float loweredAlphaIntensityAdd = loweredAlphaIntensity - nearlyRaisedAlphaIntensity;
	float loweredScale = rail->getMaxTileOffset() > 0 ? tileOffset / rail->getMaxTileOffset() : 0.0f;
	Rail::setSegmentColor(
		rail->getColor(),
		1.0f - loweredScale,
		loweredScale == 0 ? 1.0f : nearlyRaisedAlphaIntensity + loweredAlphaIntensityAdd * loweredScale);
}
void RailState::renderMovementDirections(int screenLeftWorldX, int screenTopWorldY) {
	if ((Editor::isActive && rail->editorIsDeleted) || rail->getColor() == MapState::squareColor)
		return;

	Rail::Segment* endSegments[] = { rail->getSegment(0), rail->getSegment(rail->getSegmentCount() - 1) };
	for (Rail::Segment* segment : endSegments) {
		char movementMagnitude = rail->getMovementMagnitude();
		GLint centerX = (GLint)(segment->x * MapState::tileSize - screenLeftWorldX + MapState::halfTileSize);
		GLint baseTopY = (GLint)(segment->y * MapState::tileSize - screenTopWorldY + MapState::halfTileSize);
		//position the arrows so that one arrow has its wide end going through the middle of the group
		//this happens normally on even magnitudes, and with a bias in the movement direction for odd magnitudes
		static constexpr int arrowSize = 3;
		baseTopY -= (movementMagnitude + (1 - nextMovementDirection) / 2) / 2 * arrowSize;
		for (char i = 0; i < movementMagnitude; i++) {
			GLint topY = baseTopY + i * arrowSize;
			for (int j = 1; j <= arrowSize; j++) {
				static constexpr GLfloat movementDirectionColor = 0.75f;
				GLint arrowTopY = topY + (nextMovementDirection < 0 ? j - 1 : arrowSize - j);
				SpriteSheet::renderFilledRectangle(
					movementDirectionColor,
					movementDirectionColor,
					movementDirectionColor,
					1.0f,
					centerX - j,
					arrowTopY,
					centerX + j,
					arrowTopY + 1);
			}
		}
	}
}
void RailState::loadState(char pTileOffset, char pNextMovementDirection, bool animateMovement) {
	if (!animateMovement)
		tileOffset = pTileOffset;
	targetTileOffset = pTileOffset;
	bouncesRemaining = 0;
	currentMovementDirection = pNextMovementDirection;
	nextMovementDirection = pNextMovementDirection;
}
void RailState::reset(bool animateMovement) {
	loadState(rail->getInitialTileOffset(), rail->getInitialMovementDirection(), animateMovement);
}
