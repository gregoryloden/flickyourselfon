#include "MapState.h"
#include "Editor/Editor.h"
#include "GameState/DynamicValue.h"
#include "GameState/EntityAnimation.h"
#include "GameState/EntityState.h"
#include "Sprites/SpriteAnimation.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"
#include "Util/FileUtils.h"
#include "Util/Logger.h"
#include "Util/StringUtils.h"

#define newRail(x, y, baseHeight, color, initialTileOffset) \
	newWithArgs(MapState::Rail, x, y, baseHeight, color, initialTileOffset)
#define newSwitch(leftX, topY, color, group) newWithArgs(MapState::Switch, leftX, topY, color, group)
#define newRailState(rail, railIndex) newWithArgs(MapState::RailState, rail, railIndex)
#define newSwitchState(switch0) newWithArgs(MapState::SwitchState, switch0)
#define newRadioWavesState() produceWithoutArgs(MapState::RadioWavesState)

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

//////////////////////////////// MapState::Rail::Segment ////////////////////////////////
MapState::Rail::Segment::Segment(int pX, int pY, char pMaxTileOffset)
: x(pX)
, y(pY)
//use the bottom end sprite as the default, we'll fix it later
, spriteHorizontalIndex(endSegmentSpriteHorizontalIndex(0, -1))
, maxTileOffset(pMaxTileOffset) {
}
MapState::Rail::Segment::~Segment() {}
//get the center x of the tile that this segment is on (when raised)
float MapState::Rail::Segment::tileCenterX() {
	return (float)(x * tileSize) + (float)tileSize * 0.5f;
}
//get the center y of the tile that this segment is on (when raised)
float MapState::Rail::Segment::tileCenterY() {
	return (float)(y * tileSize) + (float)tileSize * 0.5f;
}

//////////////////////////////// MapState::Rail ////////////////////////////////
MapState::Rail::Rail(objCounterParametersComma() int x, int y, char pBaseHeight, char pColor, char pInitialTileOffset)
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
MapState::Rail::~Rail() {
	delete segments;
}
//get the sprite index based on which direction this end segment extends towards
int MapState::Rail::endSegmentSpriteHorizontalIndex(int xExtents, int yExtents) {
	return yExtents != 0 ? 8 + (1 - yExtents) / 2 : 6 + (1 - xExtents) / 2;
}
//reverse the order of the segments
void MapState::Rail::reverseSegments() {
	vector<Segment>* newSegments = new vector<Segment>();
	for (int i = (int)segments->size() - 1; i >= 0; i--)
		newSegments->push_back((*segments)[i]);
	delete segments;
	segments = newSegments;
}
//add this group to the rail if it does not already contain it
void MapState::Rail::addGroup(char group) {
	if (group == 0)
		return;
	for (int i = 0; i < (int)groups.size(); i++) {
		if (groups[i] == group)
			return;
	}
	groups.push_back(group);
}
//add a segment on this tile to the rail
void MapState::Rail::addSegment(int x, int y) {
	//if we aren't adding at the end, reverse the list before continuing
	Segment* end = &segments->back();
	if (abs(y - end->y) + abs(x - end->x) != 1)
		reverseSegments();

	//find the tile where the shadow should go
	for (char segmentMaxTileOffset = 0; true; segmentMaxTileOffset++) {
		//in the event we actually went past the bottom of the map, don't try to find the tile height,
		//	this segment does not have an offset and we're done searching
		if (y + segmentMaxTileOffset >= height)
			segmentMaxTileOffset = Segment::absentTileOffset;
		else {
			char railGroundHeight = baseHeight - (segmentMaxTileOffset * 2);
			char tileHeight = getHeight(x, y + segmentMaxTileOffset);
			//keep looking if we haven't gone down to the lowest level
			//	and we found an empty space tile or a non-empty-space tile that's too low
			if (tileHeight == emptySpaceHeight || tileHeight < railGroundHeight)
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
		if (secondLastEnd->x == end->x)
			lastEnd->spriteHorizontalIndex = 0;
		else if (secondLastEnd->y == end->y)
			lastEnd->spriteHorizontalIndex = 1;
		else {
			int xExtents = secondLastEnd->x - lastEnd->x + end->x - lastEnd->x;
			int yExtents = secondLastEnd->y - lastEnd->y + end->y - lastEnd->y;
			lastEnd->spriteHorizontalIndex = 3 - yExtents + (1 - xExtents) / 2;
		}
		if (lastEnd->maxTileOffset != Segment::absentTileOffset) {
			maxTileOffset = MathUtils::min(lastEnd->maxTileOffset, maxTileOffset);
			initialTileOffset = MathUtils::min(maxTileOffset, initialTileOffset);
		}
	//we only have 2 segments so the other rail is just the end segment that complements this
	} else
		lastEnd->spriteHorizontalIndex = endSegmentSpriteHorizontalIndex(end->x - lastEnd->x, end->y - lastEnd->y);
}
//render this rail at its position by rendering each segment
void MapState::Rail::render(int screenLeftWorldX, int screenTopWorldY, float tileOffset) {
	#ifdef EDITOR
		if (isDeleted)
			return;
		MutexLocker mutexLocker (segmentsMutex);
	#endif

	const float nonColorIntensity = 9.0f / 16.0f;
	const float sineColorIntensity = 14.0f / 16.0f;
	float redColor = sineColorIntensity;
	float greenColor = sineColorIntensity;
	float blueColor = sineColorIntensity;
	if (color != sineColor) {
		redColor = color == squareColor ? 1.0f : nonColorIntensity;
		greenColor = color == sawColor ? 1.0f : nonColorIntensity;
		blueColor = color == triangleColor ? 1.0f : nonColorIntensity;
	}
	const float maxRailHeightColorScaleReduction = 3.0f / 8.0f;
	float railHeightColorScale = 1.0f - maxRailHeightColorScaleReduction * MathUtils::fmin(1.0f, tileOffset / 3.0f);
	glColor4f(railHeightColorScale * redColor, railHeightColorScale * greenColor, railHeightColorScale * blueColor, 1.0f);
	int lastSegmentIndex = (int)segments->size() - 1;
	for (int i = 1; i < lastSegmentIndex; i++)
		renderSegment(screenLeftWorldX, screenTopWorldY, tileOffset, i);
	glColor4f(redColor, greenColor, blueColor, 1.0f);
	renderSegment(screenLeftWorldX, screenTopWorldY, 0.0f, 0);
	renderSegment(screenLeftWorldX, screenTopWorldY, 0.0f, lastSegmentIndex);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
//render the shadow below the rail
void MapState::Rail::renderShadow(int screenLeftWorldX, int screenTopWorldY) {
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
				(GLint)(segment.x * tileSize - screenLeftWorldX),
				(GLint)((segment.y + (int)segment.maxTileOffset) * tileSize - screenTopWorldY));
	}
}
//render groups where the rail would be at 0 offset
void MapState::Rail::renderGroups(int screenLeftWorldX, int screenTopWorldY) {
	#ifdef EDITOR
		if (isDeleted)
			return;
		MutexLocker mutexLocker (segmentsMutex);
	#endif

	int lastSegmentIndex = (int)segments->size() - 1;
	bool hasGroups = groups.size() > 0;

	for (int i = 0; i <= lastSegmentIndex; i++) {
		Segment& segment = (*segments)[i];
		GLint drawLeftX = (GLint)(segment.x * tileSize - screenLeftWorldX);
		GLint drawTopY = (GLint)(segment.y * tileSize - screenTopWorldY);
		renderGroupRect(
			hasGroups ? groups[i % groups.size()] : 0, drawLeftX + 2, drawTopY + 2, drawLeftX + 4, drawTopY + 4);
	}
}
//render the rail segment at its position, clipping it if part of the map is higher than it
void MapState::Rail::renderSegment(int screenLeftWorldX, int screenTopWorldY, float tileOffset, int segmentIndex) {
	Segment& segment = (*segments)[segmentIndex];
	int yPixelOffset = (int)(tileOffset * (float)tileSize + 0.5f);
	int topWorldY = segment.y * tileSize + yPixelOffset;
	int bottomTileY = (topWorldY + tileSize - 1) / tileSize;
	GLint drawLeftX = (GLint)(segment.x * tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(topWorldY - screenTopWorldY);
	char topTileHeight = getHeight(segment.x, topWorldY / tileSize);
	char bottomTileHeight = getHeight(segment.x, bottomTileY);
	bool topShadow = false;
	bool bottomShadow = false;

	//this segment is completely visible
	if (bottomTileHeight == emptySpaceHeight
		|| bottomTileHeight <= baseHeight - (char)((yPixelOffset + tileSize - 1) / tileSize) * 2)
	{
		SpriteRegistry::rails->renderSpriteAtScreenPosition(segment.spriteHorizontalIndex, 0, drawLeftX, drawTopY);
		topShadow = topTileHeight % 2 == 1 && topTileHeight != emptySpaceHeight;
		bottomShadow = bottomTileHeight % 2 == 1 && bottomTileHeight != emptySpaceHeight;
	//the top part of this segment is visible
	} else if (topTileHeight == emptySpaceHeight || topTileHeight <= baseHeight - (char)(yPixelOffset / tileSize) * 2) {
		int spriteX = segment.spriteHorizontalIndex * tileSize;
		int spriteHeight = bottomTileY * tileSize - topWorldY;
		SpriteRegistry::rails->renderSpriteSheetRegionAtScreenRegion(
			spriteX,
			0,
			spriteX + tileSize,
			spriteHeight,
			drawLeftX,
			drawTopY,
			drawLeftX + (GLint)tileSize,
			drawTopY + (GLint)spriteHeight);
		topShadow = topTileHeight % 2 == 1 && topTileHeight != emptySpaceHeight;
	}

	if (topShadow || bottomShadow) {
		#ifdef EDITOR
			if (segment.spriteHorizontalIndex > 6)
				return;
		#endif
		int topSpriteHeight = bottomTileY * tileSize - topWorldY;
		int spriteX = (segment.spriteHorizontalIndex + 16) * tileSize;
		int spriteTop = topShadow ? 0 : topSpriteHeight;
		int spriteBottom = bottomShadow ? tileSize : topSpriteHeight;
		SpriteRegistry::rails->renderSpriteSheetRegionAtScreenRegion(
			spriteX,
			spriteTop,
			spriteX + tileSize,
			spriteBottom,
			drawLeftX,
			drawTopY + (GLint)spriteTop,
			drawLeftX + (GLint)tileSize,
			drawTopY + (GLint)spriteBottom);
	}
}
#ifdef EDITOR
	//remove this group from the rail if it contains it
	void MapState::Rail::removeGroup(char group) {
		for (int i = 0; i < (int)groups.size(); i++) {
			if (groups[i] == group) {
				groups.erase(groups.begin() + i);
				return;
			}
		}
	}
	//remove the segment on this tile from the rail
	void MapState::Rail::removeSegment(int x, int y) {
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
	void MapState::Rail::adjustInitialTileOffset(int x, int y, char tileOffset) {
		Segment& start = segments->front();
		Segment& end = segments->back();
		if ((x == start.x && y == start.y) || (x == end.x && y == end.y))
			initialTileOffset = MathUtils::max(0, MathUtils::min(maxTileOffset, initialTileOffset + tileOffset));
	}
	//we're saving this rail to the floor file, get the data we need at this tile
	char MapState::Rail::getFloorSaveData(int x, int y) {
		Segment& start = segments->front();
		if (x == start.x && y == start.y)
			return (color << floorRailSwitchColorDataShift)
				| (initialTileOffset << floorRailInitialTileOffsetDataShift)
				| floorRailHeadValue;
		else {
			for (int i = 1; i - 1 < (int)groups.size() && i < (int)segments->size(); i++) {
				Segment& segment = (*segments)[i];
				if (segment.x == x && segment.y == y)
					return groups[i - 1] << floorRailSwitchGroupDataShift | floorIsRailSwitchBitmask;
			}
		}
		//no data but still part of this rail
		return floorIsRailSwitchBitmask;
	}
#endif

//////////////////////////////// MapState::Switch ////////////////////////////////
MapState::Switch::Switch(objCounterParametersComma() int pLeftX, int pTopY, char pColor, char pGroup)
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
MapState::Switch::~Switch() {}
//render the switch
void MapState::Switch::render(
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
		lastActivatedSwitchColor = sineColor;
		lastActivatedSwitchColorFadeInTicksOffset = switchesFadeInDuration;
		isOn = true;
	#else
		//group 0 is the turn-on-all-switches switch, don't render it if we're not in the editor
		if (group == 0)
			return;
	#endif

	glEnable(GL_BLEND);
	GLint drawLeftX = (GLint)(leftX * tileSize - screenLeftWorldX);
	GLint drawTopY = (GLint)(topY * tileSize - screenTopWorldY);
	//draw the gray sprite if it's off or fading in
	if (lastActivatedSwitchColor < color
			|| (lastActivatedSwitchColor == color && lastActivatedSwitchColorFadeInTicksOffset <= 0))
		SpriteRegistry::switches->renderSpriteAtScreenPosition(0, 0, drawLeftX, drawTopY);
	//draw the color sprite if it's already on or it's fully faded in
	else if (lastActivatedSwitchColor > color
		|| (lastActivatedSwitchColor == color && lastActivatedSwitchColorFadeInTicksOffset >= switchesFadeInDuration))
	{
		int spriteHorizontalIndex = lastActivatedSwitchColor < color ? 0 : (int)(color * 2 + (isOn ? 1 : 2));
		SpriteRegistry::switches->renderSpriteAtScreenPosition(spriteHorizontalIndex, 0, drawLeftX, drawTopY);
	//draw a partially faded light color sprite above the darker color sprite if we're fading in the color
	} else {
		int darkSpriteHorizontalIndex = (int)(color * 2 + 2);
		SpriteRegistry::switches->renderSpriteAtScreenPosition(darkSpriteHorizontalIndex, 0, drawLeftX, drawTopY);
		float fadeInAlpha = MathUtils::fsqr((float)lastActivatedSwitchColorFadeInTicksOffset / (float)switchesFadeInDuration);
		glColor4f(1.0f, 1.0f, 1.0f, fadeInAlpha);
		int lightSpriteLeftX = (darkSpriteHorizontalIndex - 1) * 12 + 1;
		SpriteRegistry::switches->renderSpriteSheetRegionAtScreenRegion(
			lightSpriteLeftX, 1, lightSpriteLeftX + 10, 11, drawLeftX + 1, drawTopY + 1, drawLeftX + 11, drawTopY + 11);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
	if (showGroup)
		renderGroupRect(group, drawLeftX + 4, drawTopY + 4, drawLeftX + 8, drawTopY + 8);
}
#ifdef EDITOR
	//update the position of this switch
	void MapState::Switch::moveTo(int newLeftX, int newTopY) {
		leftX = newLeftX;
		topY = newTopY;
	}
	//we're saving this switch to the floor file, get the data we need at this tile
	char MapState::Switch::getFloorSaveData(int x, int y) {
		//head byte, write our color
		if (x == leftX && y == topY)
			return (color << floorRailSwitchColorDataShift) | floorSwitchHeadValue;
		//tail byte, write our number
		else if (x == leftX + 1&& y == topY)
			return (group << floorRailSwitchGroupDataShift) | floorIsRailSwitchBitmask;
		//no data but still part of this switch
		else
			return floorIsRailSwitchBitmask;
	}
#endif

//////////////////////////////// MapState::RailState ////////////////////////////////
const float MapState::RailState::tileOffsetPerTick = 3.0f / (float)Config::ticksPerSecond;
MapState::RailState::RailState(objCounterParametersComma() Rail* pRail, int pRailIndex)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
rail(pRail)
, railIndex(pRailIndex)
, tileOffset((float)pRail->getInitialTileOffset())
, targetTileOffset(0.0f)
, lastUpdateTicksTime(0) {
	targetTileOffset = tileOffset;
}
MapState::RailState::~RailState() {
	//don't delete the rail, it's owned by MapState
}
//check if we need to start/stop moving
void MapState::RailState::updateWithPreviousRailState(RailState* prev, int ticksTime) {
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
void MapState::RailState::squareToggleOffset() {
	targetTileOffset = targetTileOffset == 0.0f ? rail->getMaxTileOffset() : 0.0f;
}
//render the rail, possibly with groups
void MapState::RailState::render(int screenLeftWorldX, int screenTopWorldY) {
	rail->render(screenLeftWorldX, screenTopWorldY, tileOffset);
}
//render a shadow below the rail
void MapState::RailState::renderShadow(int screenLeftWorldX, int screenTopWorldY) {
	rail->renderShadow(screenLeftWorldX, screenTopWorldY);
}
//render groups where the rail would be at 0 offset
void MapState::RailState::renderGroups(int screenLeftWorldX, int screenTopWorldY) {
	rail->renderGroups(screenLeftWorldX, screenTopWorldY);
}
//set this rail to the initial tile offset, not moving
void MapState::RailState::loadState(float pTileOffset) {
	tileOffset = pTileOffset;
	targetTileOffset = pTileOffset;
}
//reset the offset
void MapState::RailState::reset() {
	loadState((float)rail->getInitialTileOffset());
}

//////////////////////////////// MapState::SwitchState ////////////////////////////////
MapState::SwitchState::SwitchState(objCounterParametersComma() Switch* pSwitch0)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
switch0(pSwitch0)
, connectedRailStates()
, flipOnTicksTime(0) {
}
MapState::SwitchState::~SwitchState() {
	//don't delete the switch, it's owned by MapState
}
//add a rail state to be affected by this switch state
void MapState::SwitchState::addConnectedRailState(RailState* railState) {
	connectedRailStates.push_back(railState);
}
//activate rails because this switch was kicked
void MapState::SwitchState::flip(int pFlipOnTicksTime) {
	char color = switch0->getColor();
	//square wave switch: just toggle the target tile offset of the rails
	if (color == squareColor) {
		for (RailState* railState : connectedRailStates) {
			railState->squareToggleOffset();
		}
	}
	flipOnTicksTime = pFlipOnTicksTime;
}
//activate rails if this switch was kicked
void MapState::SwitchState::updateWithPreviousSwitchState(SwitchState* prev) {
	flipOnTicksTime = prev->flipOnTicksTime;
}
//render the switch
void MapState::SwitchState::render(
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

//////////////////////////////// MapState::RadioWavesState ////////////////////////////////
MapState::RadioWavesState::RadioWavesState(objCounterParameters())
: EntityState(objCounterArguments())
, spriteAnimation(nullptr)
, spriteAnimationStartTicksTime(0)
{
	x.set(newCompositeQuarticValue(antennaCenterWorldX(), 0.0f, 0.0f, 0.0f, 0.0f));
	y.set(newCompositeQuarticValue(antennaCenterWorldY(), 0.0f, 0.0f, 0.0f, 0.0f));
}
MapState::RadioWavesState::~RadioWavesState() {
	//don't delete the sprite animation, SpriteRegistry owns it
}
//initialize and return a RadioWavesState
MapState::RadioWavesState* MapState::RadioWavesState::produce(objCounterParameters()) {
	initializeWithNewFromPool(r, MapState::RadioWavesState)
	return r;
}
//copy the other state
void MapState::RadioWavesState::copyRadioWavesState(RadioWavesState* other) {
	copyEntityState(other);
	spriteAnimation = other->spriteAnimation;
	spriteAnimationStartTicksTime = other->spriteAnimationStartTicksTime;
}
pooledReferenceCounterDefineRelease(MapState::RadioWavesState)
//set the animation to the given animation at the given time
void MapState::RadioWavesState::setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime) {
	spriteAnimation = pSpriteAnimation;
	spriteAnimationStartTicksTime = pSpriteAnimationStartTicksTime;
}
//update this radio waves state
void MapState::RadioWavesState::updateWithPreviousRadioWavesState(RadioWavesState* prev, int ticksTime) {
	copyRadioWavesState(prev);
	//if we have an entity animation, update with that instead
	if (prev->entityAnimation.get() != nullptr) {
		if (entityAnimation.get()->update(this, ticksTime))
			return;
		entityAnimation.set(nullptr);
	}
}
//render the radio waves if we've got an animation
void MapState::RadioWavesState::render(EntityState* camera, int ticksTime) {
	if (spriteAnimation == nullptr)
		return;

	float renderCenterX = getRenderCenterScreenX(camera,  ticksTime);
	float renderCenterY = getRenderCenterScreenY(camera,  ticksTime);
	spriteAnimation->renderUsingCenter(renderCenterX, renderCenterY, ticksTime - spriteAnimationStartTicksTime, 0, 0);
}

//////////////////////////////// MapState ////////////////////////////////
const char* MapState::floorFileName = "floor.png";
const float MapState::smallDistance = 1.0f / 256.0f;
const float MapState::introAnimationCameraCenterX = (float)(MapState::tileSize * introAnimationBootTileX) + 4.5f;
const float MapState::introAnimationCameraCenterY = (float)(MapState::tileSize * introAnimationBootTileY) - 4.5f;
char* MapState::tiles = nullptr;
char* MapState::heights = nullptr;
short* MapState::railSwitchIds = nullptr;
vector<MapState::Rail*> MapState::rails;
vector<MapState::Switch*> MapState::switches;
int MapState::width = 1;
int MapState::height = 1;
#ifdef EDITOR
	int MapState::nonTilesHidingState = 1;
#endif
const string MapState::railOffsetFilePrefix = "rail ";
const string MapState::lastActivatedSwitchColorFilePrefix = "lastActivatedSwitchColor ";
MapState::MapState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, railStates()
, railStatesByHeight()
, railsBelowPlayerZ(0)
, switchStates()
, lastActivatedSwitchColor(-1)
, switchesAnimationFadeInStartTicksTime(0)
, shouldPlayRadioTowerAnimation(false)
, radioWavesState(nullptr) {
	for (int i = 0; i < (int)rails.size(); i++)
		railStates.push_back(newRailState(rails[i], i));
	for (Switch* switch0 : switches)
		switchStates.push_back(newSwitchState(switch0));

	//add all the rail states to their appropriate switch states
	vector<SwitchState*> switchStatesByGroup;
	for (SwitchState* switchState : switchStates) {
		int group = (int)switchState->getSwitch()->getGroup();
		while ((int)switchStatesByGroup.size() <= group)
			switchStatesByGroup.push_back(nullptr);
		switchStatesByGroup[group] = switchState;
	}
	for (RailState* railState : railStates) {
		for (char group : railState->getRail()->getGroups())
			switchStatesByGroup[group]->addConnectedRailState(railState);
	}
}
MapState::~MapState() {
	for (RailState* railState : railStates)
		delete railState;
	for (SwitchState* switchState : switchStates)
		delete switchState;
}
//initialize and return a MapState
MapState* MapState::produce(objCounterParameters()) {
	initializeWithNewFromPool(m, MapState)
	m->radioWavesState.set(newRadioWavesState());
	return m;
}
pooledReferenceCounterDefineRelease(MapState)
//release the radio waves state before this is returned to the pool
void MapState::prepareReturnToPool() {
	radioWavesState.set(nullptr);
}
//load the map and extract all the map data from it
void MapState::buildMap() {
	SDL_Surface* floor = FileUtils::loadImage(floorFileName);
	width = floor->w;
	height = floor->h;
	int totalTiles = width * height;
	tiles = new char[totalTiles];
	heights = new char[totalTiles];
	railSwitchIds = new short[totalTiles];

	//these need to be initialized so that we can update them without erasing already-updated ids
	for (int i = 0; i < totalTiles; i++)
		railSwitchIds[i] = 0;

	int redShift = (int)floor->format->Rshift;
	int redMask = (int)floor->format->Rmask;
	int greenShift = (int)floor->format->Gshift;
	int greenMask = (int)floor->format->Gmask;
	int blueShift = (int)floor->format->Bshift;
	int blueMask = (int)floor->format->Bmask;
	int* pixels = static_cast<int*>(floor->pixels);

	//set all tiles and heights before loading rails/switches
	for (int i = 0; i < totalTiles; i++) {
		tiles[i] = (char)(((pixels[i] & greenMask) >> greenShift) / tileDivisor);
		heights[i] = (char)(((pixels[i] & blueMask) >> blueShift) / heightDivisor);
		railSwitchIds[i] = absentRailSwitchId;
	}

	for (int i = 0; i < totalTiles; i++) {
		char railSwitchValue = (char)((pixels[i] & redMask) >> redShift);

		//rail/switch data occupies 2+ red bytes, each byte has bit 0 set (non-switch/rail bytes have bit 0 unset)
		//head bytes that start a rail/switch have bit 1 set, we only build rails/switches when we get to one of these
		if ((railSwitchValue & floorIsRailSwitchAndHeadBitmask) != floorRailSwitchAndHeadValue)
			continue;

		//head byte 1:
		//	bit 1: 1 (indicates head byte)
		//	bit 2 indicates rail (0) vs switch (1)
		//	bits 3-4: color (R/B/G/W)
		//	rail bits 5-7: initial tile offset
		//tail byte 2+:
		//	bit 1: 0 (indicates tail byte)
		//	bits 2-7: group number
		char color = (railSwitchValue >> floorRailSwitchColorDataShift) & floorRailSwitchColorPostShiftBitmask;
		//switches have only 2 bytes, head byte 1 at its top-left tile, and tail byte 2 at its top-right tile
		if ((railSwitchValue & floorIsSwitchBitmask) != 0) {
			char switchByte2 = (char)((pixels[i + 1] & redMask) >> redShift);
			char group = (switchByte2 >> floorRailSwitchGroupDataShift) & floorRailSwitchGroupPostShiftBitmask;
			short newSwitchId = (short)switches.size() | switchIdBitmask;
			//add the switch and set all the ids
			switches.push_back(newSwitch(i % width, i / width, color, group));
			for (int yOffset = 0; yOffset <= 1; yOffset++) {
				for (int xOffset = 0; xOffset <= 1; xOffset++) {
					railSwitchIds[i + xOffset + yOffset * width] = newSwitchId;
				}
			}
		//rails can extend in any direction after this head byte tile
		} else {
			short newRailId = (short)rails.size() | railIdBitmask;
			char initialTileOffset =
				(railSwitchValue >> floorRailInitialTileOffsetDataShift) & floorRailInitialTileOffsetPostShiftBitmask;
			Rail* rail = newRail(i % width, i / width, heights[i], color, initialTileOffset);
			rails.push_back(rail);
			railSwitchIds[i] = newRailId;
			int railI = i;
			//cache shift values so that we can iterate the floor data quicker
			int floorIsRailSwitchAndHeadShiftedBitmask = floorIsRailSwitchAndHeadBitmask << redShift;
			int floorRailSwitchTailShiftedValue = floorIsRailSwitchBitmask << redShift;
			int floorRailGroupShiftedShift = redShift + floorRailSwitchGroupDataShift;
			while (true) {
				if ((pixels[railI + 1] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
						&& railSwitchIds[railI + 1] == 0)
					railI++;
				else if ((pixels[railI - 1] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
						&& railSwitchIds[railI - 1] == 0)
					railI--;
				else if ((pixels[railI + width] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
						&& railSwitchIds[railI + width] == 0)
					railI += width;
				else if ((pixels[railI - width] & floorIsRailSwitchAndHeadShiftedBitmask) == floorRailSwitchTailShiftedValue
						&& railSwitchIds[railI - width] == 0)
					railI -= width;
				else
					break;
				rail->addSegment(railI % width, railI / width);
				rail->addGroup((char)(pixels[railI] >> floorRailGroupShiftedShift) & floorRailSwitchGroupPostShiftBitmask);
				railSwitchIds[railI] = newRailId;
			}
		}
	}

	SDL_FreeSurface(floor);
}
//delete the resources used to handle the map
void MapState::deleteMap() {
	delete[] tiles;
	delete[] heights;
	delete[] railSwitchIds;
	for (Rail* rail : rails)
		delete rail;
	rails.clear();
	for (Switch* switch0 : switches)
		delete switch0;
	switches.clear();
}
//get the world position of the left edge of the screen using the camera as the center of the screen
int MapState::getScreenLeftWorldX(EntityState* camera, int ticksTime) {
	//we convert the camera center to int first because with a position with 0.5 offsets, we render all pixels aligned (because
	//	the screen/player width is odd); once we get to a .0 position, then we render one pixel over
	return (int)(camera->getRenderCenterWorldX(ticksTime)) - Config::gameScreenWidth / 2;
}
//get the world position of the top edge of the screen using the camera as the center of the screen
int MapState::getScreenTopWorldY(EntityState* camera, int ticksTime) {
	//(see getScreenLeftWorldX)
	return (int)(camera->getRenderCenterWorldY(ticksTime)) - Config::gameScreenHeight / 2;
}
//find the switch for the given index and write its top left corner
//does not write map coordinates if it doesn't find any
void MapState::getSwitchMapTopLeft(short switchIndex, int* outMapLeftX, int* outMapTopY) {
	short targetSwitchId = switchIndex | switchIdBitmask;
	for (int mapY = 0; mapY < height; mapY++) {
		for (int mapX = 0; mapX < width; mapX++) {
			if (getRailSwitchId(mapX, mapY) == targetSwitchId) {
				*outMapLeftX = mapX;
				*outMapTopY = mapY;
				return;
			}
		}
	}
}
//get the center x of the radio tower antenna
float MapState::antennaCenterWorldX() {
	return (float)(radioTowerLeftXOffset + SpriteRegistry::radioTower->getSpriteSheetWidth() / 2);
}
//get the center y of the radio tower antenna
float MapState::antennaCenterWorldY() {
	return (float)(radioTowerTopYOffset + 2);
}
//check the height of all the tiles in the row, and return it if they're all the same or -1 if they differ
char MapState::horizontalTilesHeight(int lowMapX, int highMapX, int mapY) {
	char foundHeight = getHeight(lowMapX, mapY);
	for (int mapX = lowMapX + 1; mapX <= highMapX; mapX++) {
		if (getHeight(mapX, mapY) != foundHeight)
			return invalidHeight;
	}
	return foundHeight;
}
//change one of the tiles to be the boot tile
void MapState::setIntroAnimationBootTile(bool showBootTile) {
	//if we're not showing the boot tile, just show a default tile instead of showing the tile from the floor file
	tiles[introAnimationBootTileY * width + introAnimationBootTileX] = showBootTile ? tileBoot : tileFloorFirst;
}
//update the rails and switches
void MapState::updateWithPreviousMapState(MapState* prev, int ticksTime) {
	lastActivatedSwitchColor = prev->lastActivatedSwitchColor;
	shouldPlayRadioTowerAnimation = false;
	switchesAnimationFadeInStartTicksTime = prev->switchesAnimationFadeInStartTicksTime;

	#ifdef EDITOR
		//since the editor can add switches and rails, make sure we update our list to track them
		//we won't connect rail states to switch states since we can't kick switches in the editor
		while (railStates.size() < rails.size())
			railStates.push_back(newRailState(rails[railStates.size()], railStates.size()));
		while (switchStates.size() < switches.size())
			switchStates.push_back(newSwitchState(switches[switchStates.size()]));
	#endif
	for (int i = 0; i < (int)prev->switchStates.size(); i++)
		switchStates[i]->updateWithPreviousSwitchState(prev->switchStates[i]);
	railStatesByHeight.clear();
	for (RailState* otherRailState : prev->railStatesByHeight) {
		RailState* railState = railStates[otherRailState->getRailIndex()];
		railState->updateWithPreviousRailState(otherRailState, ticksTime);
		insertRailByHeight(railState);
	}
	#ifdef EDITOR
		//if we added rail states this update, add them to the height list too
		//we know they're the last set of rails in the list
		//if we added them in a previous state, they'll already be sorted
		while (railStatesByHeight.size() < railStates.size())
			railStatesByHeight.push_back(railStates[railStatesByHeight.size()]);
	#endif

	radioWavesState.get()->updateWithPreviousRadioWavesState(prev->radioWavesState.get(), ticksTime);
}
//via insertion sort, add a rail state to the list of above- or below-player rail states
//compare starting at the end since we expect the rails to mostly be already sorted
void MapState::insertRailByHeight(RailState* railState) {
	float effectiveHeight = railState->getEffectiveHeight();
	//no insertion needed if there are no higher rails
	if (railStatesByHeight.size() == 0 || railStatesByHeight.back()->getEffectiveHeight() <= effectiveHeight) {
		railStatesByHeight.push_back(railState);
		return;
	}
	int insertionIndex = (int)railStatesByHeight.size() - 1;
	railStatesByHeight.push_back(railStatesByHeight.back());
	for (; insertionIndex > 0; insertionIndex--) {
		RailState* other = railStatesByHeight[insertionIndex - 1];
		if (other->getEffectiveHeight() <= effectiveHeight)
			break;
		railStatesByHeight[insertionIndex] = other;
	}
	railStatesByHeight[insertionIndex] = railState;
}
//set that we should flip a switch in the future
void MapState::flipSwitch(short switchId, bool allowRadioTowerAnimation, int ticksTime) {
	SwitchState* switchState = switchStates[switchId & railSwitchIndexBitmask];
	Switch* switch0 = switchState->getSwitch();
	//this is a turn-on-other-switches switch, flip it if we haven't done so already
	if (switch0->getGroup() == 0) {
		if (lastActivatedSwitchColor < switch0->getColor()) {
			lastActivatedSwitchColor = switch0->getColor();
			if (allowRadioTowerAnimation)
				shouldPlayRadioTowerAnimation = true;
		}
	//this is just a regular switch and we've turned on the parent switch, flip it
	} else if (lastActivatedSwitchColor >= switch0->getColor())
		switchState->flip(ticksTime);
}
//begin a radio waves animation
void MapState::startRadioWavesAnimation(int initialTicksDelay, int ticksTime) {
	vector<ReferenceCounterHolder<EntityAnimation::Component>> radioWavesAnimationComponents ({
		newEntityAnimationDelay(initialTicksDelay) COMMA
		newEntityAnimationSetSpriteAnimation(SpriteRegistry::radioWavesAnimation) COMMA
		newEntityAnimationDelay(SpriteRegistry::radioWavesAnimation->getTotalTicksDuration()) COMMA
		newEntityAnimationSetSpriteAnimation(nullptr) COMMA
		newEntityAnimationDelay(RadioWavesState::interRadioWavesAnimationTicks) COMMA
		newEntityAnimationSetSpriteAnimation(SpriteRegistry::radioWavesAnimation) COMMA
		newEntityAnimationDelay(SpriteRegistry::radioWavesAnimation->getTotalTicksDuration()) COMMA
		newEntityAnimationSetSpriteAnimation(nullptr)
	});
	Holder_EntityAnimationComponentVector radioWavesAnimationComponentsHolder (&radioWavesAnimationComponents);
	radioWavesState.get()->beginEntityAnimation(&radioWavesAnimationComponentsHolder, ticksTime);
}
//activate the next switch color and set the start of the animation
void MapState::startSwitchesFadeInAnimation(int ticksTime) {
	shouldPlayRadioTowerAnimation = false;
	switchesAnimationFadeInStartTicksTime = ticksTime;
}
//draw the map
void MapState::render(EntityState* camera, char playerZ, bool showConnections, int ticksTime) {
	glDisable(GL_BLEND);
	//render the map
	//these values are just right so that every tile rendered is at least partially in the window and no tiles are left out
	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	int tileMinX = MathUtils::max(screenLeftWorldX / tileSize, 0);
	int tileMinY = MathUtils::max(screenTopWorldY / tileSize, 0);
	int tileMaxX = MathUtils::min((Config::gameScreenWidth + screenLeftWorldX - 1) / tileSize + 1, width);
	int tileMaxY = MathUtils::min((Config::gameScreenHeight + screenTopWorldY - 1) / tileSize + 1, height);
	#ifdef EDITOR
		char editorSelectedHeight = Editor::getSelectedHeight();
	#endif
	for (int y = tileMinY; y < tileMaxY; y++) {
		for (int x = tileMinX; x < tileMaxX; x++) {
			//consider any tile at the max height to be filler
			int mapIndex = y * width + x;
			char mapHeight = heights[mapIndex];
			if (mapHeight == emptySpaceHeight)
				continue;

			GLint leftX = (GLint)(x * tileSize - screenLeftWorldX);
			GLint topY = (GLint)(y * tileSize - screenTopWorldY);
			SpriteRegistry::tiles->renderSpriteAtScreenPosition((int)(tiles[mapIndex]), 0, leftX, topY);
			#ifdef EDITOR
				if (editorSelectedHeight != -1 && editorSelectedHeight != mapHeight)
					SpriteSheet::renderFilledRectangle(
						0.0f, 0.0f, 0.0f, 0.5f, leftX, topY, leftX + (GLint)tileSize, topY + (GLint)tileSize);
			#endif
		}
	}

	#ifdef EDITOR
		if (showConnections == (nonTilesHidingState % 2 == 1))
			nonTilesHidingState = (nonTilesHidingState + 1) % 6;
		//by default, show the connections
		if (nonTilesHidingState / 2 == 0)
			showConnections = true;
		//on the first press of the show-connections button, hide the connections
		else if (nonTilesHidingState / 2 == 1)
			showConnections = false;
		//on the 2nd press of the show-connections button, hide everything other than the tiles
		else
			return;
	#endif

	//draw rail shadows, rails (that are below the player), and switches
	for (RailState* railState : railStates)
		railState->renderShadow(screenLeftWorldX, screenTopWorldY);
	railsBelowPlayerZ = 0;
	for (RailState* railState : railStatesByHeight) {
		if (railState->isAbovePlayerZ(playerZ))
			break;
		railsBelowPlayerZ++;
		railState->render(screenLeftWorldX, screenTopWorldY);
	}
	for (SwitchState* switchState : switchStates)
		switchState->render(
			screenLeftWorldX,
			screenTopWorldY,
			lastActivatedSwitchColor,
			ticksTime - switchesAnimationFadeInStartTicksTime,
			showConnections,
			ticksTime);

	//draw the radio tower after drawing everything else
	glEnable(GL_BLEND);
	#ifdef EDITOR
		glColor4f(1.0f, 1.0f, 1.0f, 2.0f / 3.0f);
	#endif
	SpriteRegistry::radioTower->renderSpriteAtScreenPosition(
		0, 0, (GLint)(radioTowerLeftXOffset - screenLeftWorldX), (GLint)(radioTowerTopYOffset - screenTopWorldY));
	#ifdef EDITOR
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	#endif

	glColor4f(
		(lastActivatedSwitchColor == MapState::squareColor || lastActivatedSwitchColor == MapState::sineColor) ? 1.0f : 0.0f,
		(lastActivatedSwitchColor == MapState::sawColor || lastActivatedSwitchColor == MapState::sineColor) ? 1.0f : 0.0f,
		(lastActivatedSwitchColor == MapState::triangleColor || lastActivatedSwitchColor == MapState::sineColor) ? 1.0f : 0.0f,
		1.0f);
	radioWavesState.get()->render(camera, ticksTime);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
//draw any rails that are above the player
void MapState::renderRailsAbovePlayer(EntityState* camera, char playerZ, bool showConnections, int ticksTime) {
	#ifdef EDITOR
		//by default, show the connections
		if (nonTilesHidingState / 2 == 0)
			showConnections = true;
		//on the first press of the show-connections button, hide the connections
		else if (nonTilesHidingState / 2 == 1)
			showConnections = false;
		//on the 2nd press of the show-connections button, hide everything other than the tiles
		else
			return;
	#endif

	int screenLeftWorldX = getScreenLeftWorldX(camera, ticksTime);
	int screenTopWorldY = getScreenTopWorldY(camera, ticksTime);
	for (int i = railsBelowPlayerZ; i < (int)railStatesByHeight.size(); i++)
		railStatesByHeight[i]->render(screenLeftWorldX, screenTopWorldY);
	if (showConnections) {
		for (RailState* railState : railStates)
			railState->renderGroups(screenLeftWorldX, screenTopWorldY);
	}
}
//draw a graphic to represent this rail/switch group
void MapState::renderGroupRect(char group, GLint leftX, GLint topY, GLint rightX, GLint bottomY) {
	GLint midX = (leftX + rightX) / 2;
	//bits 0-2
	SpriteSheet::renderFilledRectangle(
		(float)(group & 1), (float)((group & 2) >> 1), (float)((group & 4) >> 2), 1.0f, leftX, topY, midX, bottomY);
	//bits 3-5
	SpriteSheet::renderFilledRectangle(
		(float)((group & 8) >> 3), (float)((group & 16) >> 4), (float)((group & 32) >> 5), 1.0f, midX, topY, rightX, bottomY);
}
//log the colors of the group to the message
void MapState::logGroup(char group, stringstream* message) {
	for (int i = 0; i < 2; i++) {
		if (i > 0)
			*message << " ";
		switch (group % 8) {
			case 0: *message << "black"; break;
			case 1: *message << "red"; break;
			case 2: *message << "green"; break;
			case 3: *message << "yellow"; break;
			case 4: *message << "blue"; break;
			case 5: *message << "magenta"; break;
			case 6: *message << "cyan"; break;
			case 7: *message << "white"; break;
		}
		group /= 8;
	}
}
//log that the switch was kicked
void MapState::logSwitchKick(short switchId) {
	short switchIndex = switchId & railSwitchIndexBitmask;
	stringstream message;
	message << "  switch " << switchIndex << " (";
	logGroup(switches[switchIndex]->getGroup(), &message);
	message << ")";
	Logger::gameplayLogger.logString(message.str());
}
//log that the switch was kicked
void MapState::logRailRide(short railId, int playerX, int playerY) {
	short railIndex = railId & railSwitchIndexBitmask;
	stringstream message;
	message << "  rail " << railIndex << " (";
	bool skipComma = true;
	for (char group : rails[railIndex]->getGroups()) {
		if (skipComma)
			skipComma = false;
		else
			message << ", ";
		logGroup(group, &message);
	}
	message << ")" << " " << playerX << " " << playerY;
	Logger::gameplayLogger.logString(message.str());
}
//save the map state to the file
void MapState::saveState(ofstream& file) {
	if (lastActivatedSwitchColor >= 0)
		file << lastActivatedSwitchColorFilePrefix << (int)lastActivatedSwitchColor << "\n";

	#ifdef EDITOR
		//don't save the rail states if we're saving the floor file
		//also write that we unlocked all the switches
		if (Editor::needsGameStateSave) {
			file << lastActivatedSwitchColorFilePrefix << "100\n";
			return;
		}
	#endif
	for (int i = 0; i < (int)railStates.size(); i++) {
		RailState* railState = railStates[i];
		char targetTileOffset = (char)railState->getTargetTileOffset();
		if (targetTileOffset != railState->getRail()->getInitialTileOffset())
			file << railOffsetFilePrefix << i << ' ' << (int)targetTileOffset << "\n";
	}
}
//try to load state from the line of the file, return whether state was loaded
bool MapState::loadState(string& line) {
	if (StringUtils::startsWith(line, lastActivatedSwitchColorFilePrefix))
		lastActivatedSwitchColor = (char)atoi(line.c_str() + lastActivatedSwitchColorFilePrefix.size());
	else if (StringUtils::startsWith(line, railOffsetFilePrefix)) {
		const char* railIndexString = line.c_str() + railOffsetFilePrefix.size();
		const char* offsetString = strchr(railIndexString, ' ') + 1;
		int railIndex = atoi(railIndexString);
		railStates[railIndex]->loadState((float)atoi(offsetString));
	} else
		return false;
	return true;
}
//we don't have previous rails to update from so sort the rails from our base list
void MapState::sortInitialRails() {
	for (RailState* railState : railStates)
		insertRailByHeight(railState);
}
//reset the rails and switches
void MapState::resetMap() {
	lastActivatedSwitchColor = -1;
	for (RailState* railState : railStates)
		railState->reset();
}
#ifdef EDITOR
	//examine the neighboring tiles and pick an appropriate default tile, but only if we match the expected floor height
	//wall tiles and floor tiles of a different height will be ignored
	void MapState::setAppropriateDefaultFloorTile(int x, int y, char expectedFloorHeight) {
		char height = getHeight(x, y);
		if (height != expectedFloorHeight)
			return;

		int leftX = x - 1;
		int rightX = x + 1;
		char leftHeight = getHeight(leftX, y);
		char rightHeight = getHeight(rightX, y);
		bool leftIsBelow = leftHeight < height || leftHeight == emptySpaceHeight;
		bool leftIsAbove = leftHeight > height && hasFloorTileCreatingShadowForHeight(leftX, y, height);
		bool rightIsBelow = rightHeight < height || rightHeight == emptySpaceHeight;
		bool rightIsAbove = rightHeight > height && hasFloorTileCreatingShadowForHeight(rightX, y, height);

		//this tile is higher than the one above it
		char topHeight = getHeight(x, y - 1);
		if (height > topHeight || topHeight == emptySpaceHeight) {
			if (leftIsAbove)
				setTile(x, y, tilePlatformTopGroundLeftFloor);
			else if (leftIsBelow)
				setTile(x, y, tilePlatformTopLeftFloor);
			else if (rightIsAbove)
				setTile(x, y, tilePlatformTopGroundRightFloor);
			else if (rightIsBelow)
				setTile(x, y, tilePlatformTopRightFloor);
			else
				setTile(x, y, tilePlatformTopFloorFirst);
		//this tile is the same height or lower than the one above it
		} else {
			if (leftIsAbove)
				setTile(x, y, tileGroundLeftFloorFirst);
			else if (leftIsBelow)
				setTile(x, y, tilePlatformLeftFloorFirst);
			else if (rightIsAbove)
				setTile(x, y, tileGroundRightFloorFirst);
			else if (rightIsBelow)
				setTile(x, y, tilePlatformRightFloorFirst);
			else
				setTile(x, y, tileFloorFirst);
		}
	}
	//check to see if there is a floor tile at this x that is effectively "above" an adjacent tile at the given y
	//go up the tiles, and if we find a floor tile with the right height, return true, or if it's too low, return false
	bool MapState::hasFloorTileCreatingShadowForHeight(int x, int y, char height) {
		for (char tileOffset = 1; true; tileOffset++) {
			char heightDiff = getHeight(x, y - (int)tileOffset) - height;
			//too high to match, keep going
			if (heightDiff > tileOffset * 2)
				continue;

			//if it's an exact match then we found the tile above this one, otherwise there is no tile above this one
			return heightDiff == tileOffset * 2;
		}
	}
	//set a switch if there's room, or delete a switch if we can
	void MapState::setSwitch(int leftX, int topY, char color, char group) {
		//a switch occupies a 2x2 square, and must be surrounded by a 1-tile ring of no-swich-or-rail tiles
		if (leftX - 1 < 0 || topY - 1 < 0 || leftX + 3 > width || topY + 3 > height)
			return;

		short newSwitchId = (short)switches.size() | switchIdBitmask;
		Switch* matchedSwitch = nullptr;
		int matchedSwitchX = -1;
		int matchedSwitchY = -1;
		for (int checkY = topY - 1; checkY < topY + 3; checkY++) {
			for (int checkX = leftX - 1; checkX < leftX + 3; checkX++) {
				//no rail or switch here, keep looking
				if (!tileHasRailOrSwitch(checkX, checkY))
					continue;

				//this is a rail, not a switch, we can't delete, move, or place a switch here
				short otherRailSwitchId = getRailSwitchId(checkX, checkY);
				if ((otherRailSwitchId & switchIdBitmask) == 0)
					return;

				//there is a switch here but we won't delete it because it isn't exactly the same as the switch we're placing
				Switch* switch0 = switches[otherRailSwitchId & railSwitchIndexBitmask];
				if (switch0->getColor() != color || switch0->getGroup() != group)
					return;

				int moveDist = abs(checkX - leftX) + abs(checkY - topY);
				//we found this switch already, keep going
				if (matchedSwitch != nullptr)
					continue;
				//we clicked on a switch exactly the same as the one we're placing, mark it as deleted and set newSwitchId to 0
				//	so that we can use the regular switch-placing logic to clear the switch
				else if (moveDist == 0) {
					switch0->isDeleted = true;
					newSwitchId = 0;
					checkY = topY + 3;
					break;
				//we clicked 1 square adjacent to a switch exactly the same as the one we're placing, save the position and keep
				//	going
				} else if (moveDist == 1) {
					matchedSwitch = switch0;
					newSwitchId = otherRailSwitchId;
					matchedSwitchX = checkX;
					matchedSwitchY = checkY;
				//it's the same switch but it's too far, we can't move it or delete it
				} else
					return;
			}
		}

		//we've moving a switch, erase the ID from the old tiles and update the position on the switch
		if (matchedSwitch != nullptr) {
			for (int eraseY = matchedSwitchY; eraseY < matchedSwitchY + 2; eraseY++) {
				for (int eraseX = matchedSwitchX; eraseX < matchedSwitchX + 2; eraseX++) {
					railSwitchIds[eraseY * width + eraseX] = 0;
				}
			}
			matchedSwitch->moveTo(leftX, topY);
		//we setting a new switch, unless we already have a switch exactly like this one
		} else if (newSwitchId != 0) {
			for (int i = 0; i < (int)switches.size(); i++) {
				Switch* switch0 = switches[i];
				if (switch0->getColor() == color && switch0->getGroup() == group && !switch0->isDeleted)
					return;
			}
			switches.push_back(newSwitch(leftX, topY, color, group));
		//we're deleting a switch, remove this group from any matching rails
		} else {
			for (Rail* rail : rails) {
				if (rail->getColor() == color)
					rail->removeGroup(group);
			}
		}

		//go through and set the new switch ID, whether we're adding, moving, or removing a switch
		for (int switchIdY = topY; switchIdY < topY + 2; switchIdY++) {
			for (int switchIdX = leftX; switchIdX < leftX + 2; switchIdX++) {
				railSwitchIds[switchIdY * width + switchIdX] = newSwitchId;
			}
		}
	}
	//set a rail, or delete a rail if we can
	void MapState::setRail(int x, int y, char color, char group) {
		//if no switch has this group (and it's a valid rail group), don't do anything
		if (group > 0) {
			bool foundMatchingSwitch = false;
			for (Switch* switch0 : switches) {
				if (switch0->getGroup() == group && !switch0->isDeleted) {
					foundMatchingSwitch = true;
					break;
				}
			}
			if (!foundMatchingSwitch)
				return;
		}

		short editingRailId = -1;
		Rail* editingRail = nullptr;
		int orthogonalRails = 0;
		int diagonalRails = 0;
		bool clickedOnRail = false;
		for (int checkY = y - 1; checkY <= y + 1; checkY++) {
			for (int checkX = x - 1; checkX <= x + 1; checkX++) {
				//no rail or switch here, keep looking
				if (!tileHasRailOrSwitch(checkX, checkY))
					continue;

				//this is a switch, not a rail, we can't place a new rail here
				short otherRailSwitchId = getRailSwitchId(checkX, checkY);
				if ((otherRailSwitchId & railIdBitmask) == 0)
					return;

				if (editingRailId == -1) {
					editingRailId = otherRailSwitchId;
					editingRail = rails[otherRailSwitchId & railSwitchIndexBitmask];
				//we saw one rail id before and we found another rail id now, we can't place a rail here
				} else if (editingRailId != otherRailSwitchId)
					return;

				if (checkX == x && checkY == y)
					clickedOnRail = true;
				else if (checkX == x || checkY == y)
					orthogonalRails++;
				else
					diagonalRails++;
			}
		}

		//don't make any changes except at the end of a rail, which will have exactly one orthogonal rail and at most one
		//	diagonal rail, or on an empty space/single rail, which will have no adjacent rails
		bool endOfRail = orthogonalRails == 1 && diagonalRails <= 1;
		bool hasAdjacentRail = orthogonalRails + diagonalRails > 0;
		if (!endOfRail && hasAdjacentRail)
			return;

		//if we have a diagonal rail, make sure our orthogonal rail isn't actually connected to a rail on the other side
		if (diagonalRails == 1
				&& ((tileHasRailOrSwitch(x - 1, y) && tileHasRailOrSwitch(x - 2, y))
					|| (tileHasRailOrSwitch(x + 1, y) && tileHasRailOrSwitch(x + 2, y))
					|| (tileHasRailOrSwitch(x, y - 1) && tileHasRailOrSwitch(x, y - 2))
					|| (tileHasRailOrSwitch(x, y + 1) && tileHasRailOrSwitch(x, y + 2))))
			return;

		//don't modify a rail if it doesn't match the color that we selected
		if (editingRail != nullptr && editingRail->getColor() != color)
			return;

		int railSwitchIndex = y * width + x;

		//this is the end of a rail, find the rail and modify it
		if (endOfRail) {
			//delete this end segment of this rail
			if (clickedOnRail) {
				editingRail->removeGroup(group);
				editingRail->removeSegment(x, y);
				railSwitchIds[railSwitchIndex] = 0;
			} else {
				int tileHeight = getHeight(x, y);
				//add a segment to the end of this rail, if the tile is not above the rail or if it's empty
				if (tileHeight <= editingRail->getBaseHeight() || tileHeight == emptySpaceHeight) {
					editingRail->addGroup(group);
					editingRail->addSegment(x, y);
					railSwitchIds[railSwitchIndex] = editingRailId;
				}
			}
		//there are no rails around this square, create a new rail, or remove the last segment of this rail
		} else {
			//mark this rail as deleted
			if (clickedOnRail) {
				editingRail->isDeleted = true;
				railSwitchIds[railSwitchIndex] = 0;
			//add a new rail here
			} else {
				railSwitchIds[railSwitchIndex] = (short)rails.size() | railIdBitmask;
				rails.push_back(newRail(x, y, getHeight(x, y), color, 0));
			}
		}
	}
	//adjust the tile offset of the rail here, if there is one
	void MapState::adjustRailInitialTileOffset(int x, int y, char tileOffset) {
		int railSwitchId = getRailSwitchId(x, y);
		if ((railSwitchId & railIdBitmask) != 0)
			rails[railSwitchId & railSwitchIndexBitmask]->adjustInitialTileOffset(x, y, tileOffset);
	}
	//check if we're saving a rail or switch to the floor file, and if so get the data we need at this tile
	char MapState::getRailSwitchFloorSaveData(int x, int y) {
		int railSwitchId = getRailSwitchId(x, y);
		if ((railSwitchId & railIdBitmask) != 0)
			return rails[railSwitchId & railSwitchIndexBitmask]->getFloorSaveData(x, y);
		else if ((railSwitchId & switchIdBitmask) != 0)
			return switches[railSwitchId & railSwitchIndexBitmask]->getFloorSaveData(x, y);
		else
			return 0;
	}
#endif

//////////////////////////////// Holder_MapStateRail ////////////////////////////////
Holder_MapStateRail::Holder_MapStateRail(MapState::Rail* pVal)
: val(pVal) {
}
Holder_MapStateRail::~Holder_MapStateRail() {
	//don't delete the rail, it's owned by something else
}
