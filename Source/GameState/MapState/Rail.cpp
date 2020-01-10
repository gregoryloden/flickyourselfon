#include "Rail.h"
#include "GameState/MapState/MapState.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

#ifdef EDITOR
	//Should only be allocated on the stack
	class MutexLocker {
	private:
		mutex* m;
	public:
		MutexLocker(mutex& pM): m(&pM) { pM.lock(); }
		virtual ~MutexLocker() { m->unlock(); }
	};
#endif

//////////////////////////////// Rail::Segment ////////////////////////////////
Rail::Segment::Segment(int pX, int pY, char pMaxTileOffset)
: x(pX)
, y(pY)
//use the bottom end sprite as the default, we'll fix it later
, spriteHorizontalIndex(endSegmentSpriteHorizontalIndex(0, -1))
, maxTileOffset(pMaxTileOffset) {
}
Rail::Segment::~Segment() {}
//get the center x of the tile that this segment is on (when raised)
float Rail::Segment::tileCenterX() {
	return (float)(x * MapState::tileSize) + (float)MapState::tileSize * 0.5f;
}
//get the center y of the tile that this segment is on (when raised)
float Rail::Segment::tileCenterY() {
	return (float)(y * MapState::tileSize) + (float)MapState::tileSize * 0.5f;
}

//////////////////////////////// Rail ////////////////////////////////
Rail::Rail(objCounterParametersComma() int x, int y, char pBaseHeight, char pColor, char pInitialTileOffset)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
baseHeight(pBaseHeight)
, color(pColor)
, segments(new vector<Segment>())
, groups()
, initialTileOffset(pInitialTileOffset)
//give a default tile offset extending to the lowest height
, maxTileOffset(pBaseHeight / 2)
#ifdef EDITOR
	, segmentsMutex()
	, isDeleted(false)
#endif
{
	segments->push_back(Segment(x, y, 0));
}
Rail::~Rail() {
	delete segments;
}
//get the sprite index based on which direction this end segment extends towards
int Rail::endSegmentSpriteHorizontalIndex(int xExtents, int yExtents) {
	return yExtents != 0 ? 8 + (1 - yExtents) / 2 : 6 + (1 - xExtents) / 2;
}
//get the sprite index based on which other segments this segment extends towards
int Rail::middleSegmentSpriteHorizontalIndex(int prevX, int prevY, int x, int y, int nextX, int nextY) {
	//vertical rail if they have the same x
	if (prevX == nextX)
		return 0;
	//horizontal rail if they have the same y
	else if (prevY == nextY)
		return 1;
	//each extents is the sum of a 0 and a 1 or -1
	else {
		int xExtentsSum = prevX - x + nextX - x;
		int yExtentsSum = prevY - y + nextY - y;
		return 3 - yExtentsSum + (1 - xExtentsSum) / 2;
	}
}
//get the sprite index that extends straight from the previous segment
int Rail::extentSegmentSpriteHorizontalIndex(int prevX, int prevY, int x, int y) {
	return middleSegmentSpriteHorizontalIndex(prevX, prevY, x, y, x + (x - prevX), y + (y - prevY));
}
//set the color mask for segments of the given rail color
void Rail::setSegmentColor(float colorScale, int railColor) {
	const float nonColorIntensity = 9.0f / 16.0f;
	const float sineColorIntensity = 14.0f / 16.0f;
	float redColor = sineColorIntensity;
	float greenColor = sineColorIntensity;
	float blueColor = sineColorIntensity;
	if (railColor != MapState::sineColor) {
		redColor = railColor == MapState::squareColor ? 1.0f : nonColorIntensity;
		greenColor = railColor == MapState::sawColor ? 1.0f : nonColorIntensity;
		blueColor = railColor == MapState::triangleColor ? 1.0f : nonColorIntensity;
	}
	glColor4f(colorScale * redColor, colorScale * greenColor, colorScale * blueColor, 1.0f);
}
//reverse the order of the segments
void Rail::reverseSegments() {
	vector<Segment>* newSegments = new vector<Segment>();
	for (int i = (int)segments->size() - 1; i >= 0; i--)
		newSegments->push_back((*segments)[i]);
	delete segments;
	segments = newSegments;
}
//add this group to the rail if it does not already contain it
void Rail::addGroup(char group) {
	if (group == 0)
		return;
	for (int i = 0; i < (int)groups.size(); i++) {
		if (groups[i] == group)
			return;
	}
	groups.push_back(group);
}
//add a segment on this tile to the rail
void Rail::addSegment(int x, int y) {
	//if we aren't adding at the end, reverse the list before continuing
	Segment* end = &segments->back();
	if (abs(y - end->y) + abs(x - end->x) != 1)
		reverseSegments();

	//find the tile where the shadow should go
	for (char segmentMaxTileOffset = 0; true; segmentMaxTileOffset++) {
		//in the event we actually went past the bottom of the map, don't try to find the tile height,
		//	this segment does not have an offset and we're done searching
		if (y + segmentMaxTileOffset >= MapState::mapHeight())
			segmentMaxTileOffset = Segment::absentTileOffset;
		else {
			char railGroundHeight = baseHeight - (segmentMaxTileOffset * 2);
			char tileHeight = MapState::getHeight(x, y + segmentMaxTileOffset);
			//keep looking if we see an empty space tile or a non-empty space tile that's too low to be a ground tile at this y
			if (tileHeight == MapState::emptySpaceHeight || tileHeight < railGroundHeight)
				continue;

			//we ended up on a tile above what would be a ground tile for this rail
			//this is an empty space or a tile that the rail hides behind, mark that this segment does not have an offset
			if (tileHeight > railGroundHeight)
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
			maxTileOffset = MathUtils::min(lastEnd->maxTileOffset, maxTileOffset);
			initialTileOffset = MathUtils::min(maxTileOffset, initialTileOffset);
		}
	//we only have 2 segments so the other rail is just the end segment that complements this
	} else
		lastEnd->spriteHorizontalIndex = endSegmentSpriteHorizontalIndex(end->x - lastEnd->x, end->y - lastEnd->y);
}
//render this rail at its position by rendering each segment
void Rail::render(int screenLeftWorldX, int screenTopWorldY, float tileOffset) {
	#ifdef EDITOR
		if (isDeleted)
			return;
		MutexLocker mutexLocker (segmentsMutex);
	#endif

	const float maxRailHeightColorScaleReduction = 3.0f / 8.0f;
	float railHeightColorScale = 1.0f - maxRailHeightColorScaleReduction * MathUtils::fmin(1.0f, tileOffset / 3.0f);
	setSegmentColor(railHeightColorScale, color);
	int lastSegmentIndex = (int)segments->size() - 1;
	for (int i = 1; i < lastSegmentIndex; i++)
		renderSegment(screenLeftWorldX, screenTopWorldY, tileOffset, i);
	setSegmentColor(1.0f, color);
	renderSegment(screenLeftWorldX, screenTopWorldY, 0.0f, 0);
	renderSegment(screenLeftWorldX, screenTopWorldY, 0.0f, lastSegmentIndex);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
//render the shadow below the rail
void Rail::renderShadow(int screenLeftWorldX, int screenTopWorldY) {
	#ifdef EDITOR
		if (isDeleted)
			return;
		MutexLocker mutexLocker (segmentsMutex);
	#endif
	glEnable(GL_BLEND);

	int lastSegmentIndex = (int)segments->size() - 1;
	for (int i = 1; i < lastSegmentIndex; i++) {
		Segment& segment = (*segments)[i];
		//don't render a shadow if this segment hides behind a platform
		if (segment.maxTileOffset > 0)
			SpriteRegistry::rails->renderSpriteAtScreenPosition(
				segment.spriteHorizontalIndex + 10,
				0,
				(GLint)(segment.x * MapState::tileSize - screenLeftWorldX),
				(GLint)((segment.y + (int)segment.maxTileOffset) * MapState::tileSize - screenTopWorldY));
	}
}
//render groups where the rail would be at 0 offset
void Rail::renderGroups(int screenLeftWorldX, int screenTopWorldY) {
	#ifdef EDITOR
		if (isDeleted)
			return;
		MutexLocker mutexLocker (segmentsMutex);
	#endif

	int lastSegmentIndex = (int)segments->size() - 1;
	bool hasGroups = groups.size() > 0;

	for (int i = 0; i <= lastSegmentIndex; i++) {
		Segment& segment = (*segments)[i];
		GLint drawLeftX = (GLint)(segment.x * MapState::tileSize - screenLeftWorldX);
		GLint drawTopY = (GLint)(segment.y * MapState::tileSize - screenTopWorldY);
		MapState::renderGroupRect(
			hasGroups ? groups[i % groups.size()] : 0, drawLeftX + 2, drawTopY + 2, drawLeftX + 4, drawTopY + 4);
	}
}
//render the rail segment at its position, clipping it if part of the map is higher than it
void Rail::renderSegment(int screenLeftWorldX, int screenTopWorldY, float tileOffset, int segmentIndex) {
	Segment& segment = (*segments)[segmentIndex];
	int yPixelOffset = (int)(tileOffset * (float)MapState::tileSize + 0.5f);
	int topWorldY = segment.y * MapState::tileSize + yPixelOffset;
	int bottomTileY = (topWorldY + MapState::tileSize - 1) / MapState::tileSize;
	GLint drawLeftX = (GLint)(segment.x * MapState::tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(topWorldY - screenTopWorldY);
	char topTileHeight = MapState::getHeight(segment.x, topWorldY / MapState::tileSize);
	char bottomTileHeight = MapState::getHeight(segment.x, bottomTileY);
	bool topShadow = false;
	bool bottomShadow = false;

	//this segment is completely visible
	if (bottomTileHeight == MapState::emptySpaceHeight
		|| bottomTileHeight <= baseHeight - (char)((yPixelOffset + MapState::tileSize - 1) / MapState::tileSize) * 2)
	{
		SpriteRegistry::rails->renderSpriteAtScreenPosition(segment.spriteHorizontalIndex, 0, drawLeftX, drawTopY);
		topShadow = topTileHeight % 2 == 1 && topTileHeight != MapState::emptySpaceHeight;
		bottomShadow = bottomTileHeight % 2 == 1 && bottomTileHeight != MapState::emptySpaceHeight;
	//the top part of this segment is visible
	} else if (topTileHeight == MapState::emptySpaceHeight
		|| topTileHeight <= baseHeight - (char)(yPixelOffset / MapState::tileSize) * 2)
	{
		int spriteX = segment.spriteHorizontalIndex * MapState::tileSize;
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
		topShadow = topTileHeight % 2 == 1 && topTileHeight != MapState::emptySpaceHeight;
	}

	if (topShadow || bottomShadow) {
		#ifdef EDITOR
			if (segment.spriteHorizontalIndex > 6)
				return;
		#endif
		int topSpriteHeight = bottomTileY * MapState::tileSize - topWorldY;
		int spriteX = (segment.spriteHorizontalIndex + 16) * MapState::tileSize;
		int spriteTop = topShadow ? 0 : topSpriteHeight;
		int spriteBottom = bottomShadow ? MapState::tileSize : topSpriteHeight;
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
#ifdef EDITOR
	//remove this group from the rail if it contains it
	void Rail::removeGroup(char group) {
		for (int i = 0; i < (int)groups.size(); i++) {
			if (groups[i] == group) {
				groups.erase(groups.begin() + i);
				return;
			}
		}
	}
	//remove the segment on this tile from the rail
	void Rail::removeSegment(int x, int y) {
		MutexLocker mutexLocker (segmentsMutex);
		Segment& end = segments->back();
		if (y == end.y && x == end.x)
			segments->pop_back();
		else
			segments->erase(segments->begin());
		//reset the max tile offset, find the smallest offset among the non-end segments
		maxTileOffset = baseHeight / 2;
		for (int i = 1; i < (int)segments->size() - 1; i++) {
			Segment& segment = (*segments)[i];
			if (segment.maxTileOffset != Segment::absentTileOffset)
				maxTileOffset = MathUtils::min(segment.maxTileOffset, maxTileOffset);
		}
	}
	//adjust the initial tile offset of this rail if we're clicking on one of its end segments
	void Rail::adjustInitialTileOffset(int x, int y, char tileOffset) {
		Segment& start = segments->front();
		Segment& end = segments->back();
		if ((x == start.x && y == start.y) || (x == end.x && y == end.y))
			initialTileOffset = MathUtils::max(0, MathUtils::min(maxTileOffset, initialTileOffset + tileOffset));
	}
	//we're saving this rail to the floor file, get the data we need at this tile
	char Rail::getFloorSaveData(int x, int y) {
		Segment& start = segments->front();
		if (x == start.x && y == start.y)
			return (color << MapState::floorRailSwitchColorDataShift)
				| (initialTileOffset << MapState::floorRailInitialTileOffsetDataShift)
				| MapState::floorRailHeadValue;
		else {
			for (int i = 1; i - 1 < (int)groups.size() && i < (int)segments->size(); i++) {
				Segment& segment = (*segments)[i];
				if (segment.x == x && segment.y == y)
					return groups[i - 1] << MapState::floorRailSwitchGroupDataShift | MapState::floorIsRailSwitchBitmask;
			}
		}
		//no data but still part of this rail
		return MapState::floorRailSwitchTailValue;
	}
#endif

//////////////////////////////// RailState ////////////////////////////////
const float RailState::tileOffsetPerTick = 3.0f / (float)Config::ticksPerSecond;
RailState::RailState(objCounterParametersComma() Rail* pRail, int pRailIndex)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
rail(pRail)
, railIndex(pRailIndex)
, tileOffset((float)pRail->getInitialTileOffset())
, targetTileOffset(0.0f)
, lastUpdateTicksTime(0) {
	targetTileOffset = tileOffset;
}
RailState::~RailState() {
	//don't delete the rail, it's owned by MapState
}
//check if we need to start/stop moving
void RailState::updateWithPreviousRailState(RailState* prev, int ticksTime) {
	targetTileOffset = prev->targetTileOffset;
	if (prev->tileOffset != prev->targetTileOffset) {
		float tileOffsetDiff = tileOffsetPerTick * (float)(ticksTime - prev->lastUpdateTicksTime);
		tileOffset = prev->tileOffset > prev->targetTileOffset
			? MathUtils::fmax(prev->targetTileOffset, prev->tileOffset - tileOffsetDiff)
			: MathUtils::fmin(prev->targetTileOffset, prev->tileOffset + tileOffsetDiff);
	} else
		tileOffset = targetTileOffset;
	lastUpdateTicksTime = ticksTime;

	#ifdef EDITOR
		tileOffset = (float)rail->getInitialTileOffset();
	#endif
}
//swap the tile offset between 0 and the max tile offset
void RailState::squareToggleOffset() {
	targetTileOffset = targetTileOffset == 0.0f ? rail->getMaxTileOffset() : 0.0f;
}
//render the rail, possibly with groups
void RailState::render(int screenLeftWorldX, int screenTopWorldY) {
	rail->render(screenLeftWorldX, screenTopWorldY, tileOffset);
}
//render a shadow below the rail
void RailState::renderShadow(int screenLeftWorldX, int screenTopWorldY) {
	rail->renderShadow(screenLeftWorldX, screenTopWorldY);
}
//render groups where the rail would be at 0 offset
void RailState::renderGroups(int screenLeftWorldX, int screenTopWorldY) {
	rail->renderGroups(screenLeftWorldX, screenTopWorldY);
}
//set this rail to the initial tile offset, not moving
void RailState::loadState(float pTileOffset) {
	tileOffset = pTileOffset;
	targetTileOffset = pTileOffset;
}
//reset the offset
void RailState::reset() {
	loadState((float)rail->getInitialTileOffset());
}
