#include "Level.h"
#include "GameState/MapState/MapState.h"
#include "GameState/MapState/Rail.h"
#include "GameState/MapState/Switch.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Logger.h"

#define newPlane(owningLevel, indexInOwningLevel) newWithArgs(Plane, owningLevel, indexInOwningLevel)

//////////////////////////////// LevelTypes::Plane::Tile ////////////////////////////////
LevelTypes::Plane::Tile::Tile(int pX, int pY)
: x(pX)
, y(pY) {
}
LevelTypes::Plane::Tile::~Tile() {}

//////////////////////////////// LevelTypes::Plane::ConnectionSwitch ////////////////////////////////
LevelTypes::Plane::ConnectionSwitch::ConnectionSwitch(Switch* switch0)
: affectedRailByteMaskData()
, hint(Hint::Type::Switch) {
	hint.data.switch0 = switch0;
}
LevelTypes::Plane::ConnectionSwitch::~ConnectionSwitch() {}

//////////////////////////////// LevelTypes::Plane::Connection ////////////////////////////////
LevelTypes::Plane::Connection::Connection(
	Plane* pToPlane, int pRailByteIndex, int pRailTileOffsetByteMask, int pSteps, Rail* rail, Plane* hintPlane)
: toPlane(pToPlane)
, railByteIndex(pRailByteIndex)
, railTileOffsetByteMask(pRailTileOffsetByteMask)
, steps(pSteps)
, hint(Hint::Type::None) {
	if (hintPlane != nullptr) {
		hint.type = Hint::Type::Plane;
		hint.data.plane = hintPlane;
	} else if (rail != nullptr) {
		hint.type = Hint::Type::Rail;
		hint.data.rail = rail;
	} else {
		hint.type = Hint::Type::Plane;
		hint.data.plane = pToPlane;
	}
}
LevelTypes::Plane::Connection::~Connection() {
	//don't delete toPlane, it's owned by a Level
}

//////////////////////////////// LevelTypes::Plane ////////////////////////////////
LevelTypes::Plane::Plane(objCounterParametersComma() Level* pOwningLevel, int pIndexInOwningLevel)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
owningLevel(pOwningLevel)
, indexInOwningLevel(pIndexInOwningLevel)
, tiles()
, connectionSwitches()
, connections()
, hasAction(false)
, renderLeftTileX(MapState::getMapWidth())
, renderTopTileY(MapState::getMapHeight())
, renderRightTileX(0)
, renderBottomTileY(0) {
}
LevelTypes::Plane::~Plane() {
	//don't delete owningLevel, it owns and deletes this
}
void LevelTypes::Plane::addTile(int x, int y) {
	tiles.push_back(Tile(x, y));
	renderLeftTileX = MathUtils::min(renderLeftTileX, x);
	renderTopTileY = MathUtils::min(renderTopTileY, y);
	renderRightTileX = MathUtils::max(renderRightTileX, x + 1);
	renderBottomTileY = MathUtils::max(renderBottomTileY, y + 1);
}
int LevelTypes::Plane::addConnectionSwitch(Switch* switch0) {
	connectionSwitches.push_back(ConnectionSwitch(switch0));
	hasAction = true;
	return (int)connectionSwitches.size() - 1;
}
void LevelTypes::Plane::addPlaneConnection(Plane* toPlane, int steps, Plane* hintPlane) {
	//add a plane-plane connection to a plane if we don't already have one
	for (Connection& connection : connections) {
		if (connection.toPlane == toPlane && connection.hint.type == Hint::Type::Plane)
			return;
	}
	connections.push_back(Connection(toPlane, Level::absentRailByteIndex, 0, steps, nullptr, hintPlane));
}
void LevelTypes::Plane::addRailConnection(
	Plane* toPlane, RailByteMaskData* railByteMaskData, int steps, Rail* rail, Plane* hintPlane)
{
	if (toPlane->owningLevel != owningLevel)
		toPlane = owningLevel->getVictoryPlane();
	connections.push_back(
		Connection(
			toPlane,
			railByteMaskData->railByteIndex,
			Level::baseRailTileOffsetByteMask << railByteMaskData->railBitShift,
			steps,
			rail,
			hintPlane));
}
void LevelTypes::Plane::addReverseRailConnection(Plane* fromPlane, Plane* toPlane, int steps, Rail* rail, Plane* hintPlane) {
	//add a rail connection to a plane that is already connected to the from-plane (usually this plane) by the given rail
	for (Connection& connection : toPlane->connections) {
		if (connection.toPlane != fromPlane || connection.hint.type != Hint::Type::Rail || connection.hint.data.rail != rail)
			continue;
		connections.push_back(
			Connection(toPlane, connection.railByteIndex, connection.railTileOffsetByteMask, steps, rail, hintPlane));
	}
}
Hint* LevelTypes::Plane::pursueSolution(HintState::PotentialLevelState* currentState) {
	unsigned int bucket = currentState->railByteMasksHash % Level::PotentialLevelStatesByBucket::bucketSize;
	//check connections
	for (Connection& connection : connections) {
		#ifdef TRACK_HINT_SEARCH_STATS
			Level::hintSearchActionsChecked++;
		#endif
		//skip any connections with rails that are lowered
		if (connection.railByteIndex >= 0
				&& (currentState->railByteMasks[connection.railByteIndex] & connection.railTileOffsetByteMask) != 0)
			continue;

		//make sure we haven't seen this state before
		vector<HintState::PotentialLevelState*>& potentialLevelStates =
			Level::potentialLevelStatesByBucketByPlane[connection.toPlane->indexInOwningLevel].buckets[bucket];
		HintState::PotentialLevelState* nextPotentialLevelState =
			currentState->addNewState(potentialLevelStates, Level::currentPotentialLevelStateSteps + connection.steps);
		if (nextPotentialLevelState == nullptr)
			continue;

		//fill out the new PotentialLevelState
		nextPotentialLevelState->plane = connection.toPlane;
		nextPotentialLevelState->hint = &connection.hint;
		#ifdef TRACK_HINT_SEARCH_STATS
			Level::hintSearchUniqueStates++;
		#endif

		//if it goes to the victory plane, return the hint for the first transition
		if (connection.toPlane == Level::cachedHintSearchVictoryPlane) {
			#ifdef LOG_FOUND_HINT_STEPS
				logSteps(nextPotentialLevelState);
			#endif
			#ifdef TRACK_HINT_SEARCH_STATS
				Level::foundHintSearchTotalSteps = nextPotentialLevelState->steps;
			#endif
			return nextPotentialLevelState->getHint();
		}
		//otherwise, track it
		Level::getNextPotentialLevelStatesForSteps(nextPotentialLevelState->steps)->push_back(nextPotentialLevelState);
	}

	//check switches
	for (ConnectionSwitch& connectionSwitch : connectionSwitches) {
		#ifdef TRACK_HINT_SEARCH_STATS
			Level::hintSearchActionsChecked++;
		#endif
		//first, reset the draft rail byte masks
		for (int i = HintState::PotentialLevelState::currentRailByteMaskCount - 1; i >= 0; i--)
			HintState::PotentialLevelState::draftState.railByteMasks[i] = currentState->railByteMasks[i];
		//then, go through and modify the byte mask for each affected rail
		char lastTileOffset;
		for (RailByteMaskData* railByteMaskData : connectionSwitch.affectedRailByteMaskData) {
			unsigned int* railByteMask =
				&HintState::PotentialLevelState::draftState.railByteMasks[railByteMaskData->railByteIndex];
			char shiftedRailState = (char)(*railByteMask >> railByteMaskData->railBitShift);
			char movementDirectionBit = shiftedRailState & (char)Level::baseRailMovementDirectionByteMask;
			char tileOffset = (lastTileOffset = shiftedRailState & (char)Level::baseRailTileOffsetByteMask);
			char movementDirection = (movementDirectionBit >> Level::railTileOffsetByteMaskBitCount) * 2 - 1;
			char resultRailState =
				railByteMaskData->rail->triggerMovement(movementDirection, &tileOffset)
						&& railByteMaskData->rail->getColor() != MapState::sawColor
					? tileOffset | (movementDirectionBit ^ Level::baseRailMovementDirectionByteMask)
					: tileOffset | movementDirectionBit;
			*railByteMask =
				(*railByteMask & railByteMaskData->inverseRailByteMask)
					| ((unsigned int)resultRailState << railByteMaskData->railBitShift);
		}
		HintState::PotentialLevelState::draftState.setHash();
		bucket = HintState::PotentialLevelState::draftState.railByteMasksHash % Level::PotentialLevelStatesByBucket::bucketSize;

		//make sure we haven't seen this state before
		vector<HintState::PotentialLevelState*>& potentialLevelStates =
			Level::potentialLevelStatesByBucketByPlane[indexInOwningLevel].buckets[bucket];
		HintState::PotentialLevelState* nextPotentialLevelState = HintState::PotentialLevelState::draftState.addNewState(
			potentialLevelStates, Level::currentPotentialLevelStateSteps + 1);
		if (nextPotentialLevelState == nullptr)
			continue;
		//also don't bother with any states that lower the only rail of a single-rail switch
		if (connectionSwitch.affectedRailByteMaskData.size() == 1 && lastTileOffset == 0) {
			potentialLevelStates.pop_back();
			nextPotentialLevelState->release();
			continue;
		}

		//fill out the new PotentialLevelState and track it
		nextPotentialLevelState->priorState = currentState;
		nextPotentialLevelState->plane = this;
		nextPotentialLevelState->hint = &connectionSwitch.hint;
		#ifdef TRACK_HINT_SEARCH_STATS
			Level::hintSearchUniqueStates++;
		#endif
		Level::getNextPotentialLevelStatesForSteps(nextPotentialLevelState->steps)->push_back(nextPotentialLevelState);
	}

	return nullptr;
}
void LevelTypes::Plane::getHintRenderBounds(int* outLeftWorldX, int* outTopWorldY, int* outRightWorldX, int* outBottomWorldY) {
	*outLeftWorldX = renderLeftTileX * MapState::tileSize;
	*outTopWorldY = renderTopTileY * MapState::tileSize;
	*outRightWorldX = renderRightTileX * MapState::tileSize;
	*outBottomWorldY = renderBottomTileY * MapState::tileSize;
}
void LevelTypes::Plane::renderHint(int screenLeftWorldX, int screenTopWorldY, float alpha) {
	glEnable(GL_BLEND);
	glColor4f(1.0f, 1.0f, 1.0f, alpha);
	for (Tile& tile : tiles) {
		GLint leftX = (GLint)(tile.x * MapState::tileSize - screenLeftWorldX);
		GLint topY = (GLint)(tile.y * MapState::tileSize - screenTopWorldY);
		SpriteSheet::renderPreColoredRectangle(
			leftX, topY, leftX + (GLint)MapState::tileSize, topY + (GLint)MapState::tileSize);
	}
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void LevelTypes::Plane::countSwitches(int outSwitchCounts[4], int* outSingleUseSwitches) {
	for (ConnectionSwitch& connectionSwitch : connectionSwitches) {
		outSwitchCounts[connectionSwitch.hint.data.switch0->getColor()]++;
		if (connectionSwitch.affectedRailByteMaskData.size() == 1)
			(*outSingleUseSwitches)++;
	}
}
#ifdef LOG_FOUND_HINT_STEPS
	void LevelTypes::Plane::logSteps(HintState::PotentialLevelState* hintState) {
		stringstream stepsMessage;
		if (hintState->priorState == nullptr) {
			stepsMessage << "start at plane " << hintState->plane->getIndexInOwningLevel();
			logRailByteMasks(stepsMessage, hintState->railByteMasks);
		} else {
			logSteps(hintState->priorState);
			stepsMessage << " -> ";
			logHint(stepsMessage, hintState->hint, hintState->railByteMasks);
			stepsMessage << " -> plane " << hintState->plane->getIndexInOwningLevel();
		}
		Logger::debugLogger.logString(stepsMessage.str());
	}
	void LevelTypes::Plane::logHint(stringstream& stepsMessage, Hint* hint, unsigned int* railByteMasks) {
		if (hint->type == Hint::Type::Plane)
			stepsMessage << "plane " << hint->data.plane->getIndexInOwningLevel();
		else if (hint->type == Hint::Type::Rail) {
			Rail::Segment* segment = hint->data.rail->getSegment(0);
			stepsMessage << "rail " << MapState::getRailSwitchId(segment->x, segment->y);
			MapState::logRailDescriptor(hint->data.rail, &stepsMessage);
		} else if (hint->type == Hint::Type::Switch) {
			Switch* switch0 = hint->data.switch0;
			stepsMessage << "switch " << MapState::getRailSwitchId(switch0->getLeftX(), switch0->getTopY());
			logRailByteMasks(stepsMessage, railByteMasks);
			MapState::logSwitchDescriptor(hint->data.switch0, &stepsMessage);
		} else if (hint->type == Hint::Type::None)
			stepsMessage << "none";
		else if (hint->type == Hint::Type::UndoReset)
			stepsMessage << "undo/reset";
		else
			stepsMessage << "[unknown]";
	}
	void LevelTypes::Plane::logRailByteMasks(stringstream& stepsMessage, unsigned int* railByteMasks) {
		for (int i = 0; i < HintState::PotentialLevelState::currentRailByteMaskCount; i++)
			stepsMessage << std::hex << std::uppercase << "  " << railByteMasks[i] << std::dec;
	}
#endif

//////////////////////////////// LevelTypes::RailByteMaskData ////////////////////////////////
LevelTypes::RailByteMaskData::RailByteMaskData(short pRailId, int pRailByteIndex, int pRailBitShift, Rail* pRail)
: railId(pRailId)
, railByteIndex(pRailByteIndex)
, railBitShift(pRailBitShift)
, rail(pRail)
, inverseRailByteMask(~(Level::baseRailByteMask << pRailBitShift)) {
}
LevelTypes::RailByteMaskData::~RailByteMaskData() {}
using namespace LevelTypes;

//////////////////////////////// Level::PotentialLevelStatesByBucket ////////////////////////////////
Level::PotentialLevelStatesByBucket::PotentialLevelStatesByBucket()
: buckets() {
}
Level::PotentialLevelStatesByBucket::~PotentialLevelStatesByBucket() {}

//////////////////////////////// Level ////////////////////////////////
vector<Level::PotentialLevelStatesByBucket> Level::potentialLevelStatesByBucketByPlane;
vector<HintState::PotentialLevelState*> Level::replacedPotentialLevelStates;
int Level::currentPotentialLevelStateSteps = 0;
int Level::maxPotentialLevelStateSteps = -1;
vector<deque<HintState::PotentialLevelState*>*> Level::nextPotentialLevelStatesBySteps;
Plane* Level::cachedHintSearchVictoryPlane = nullptr;
#ifdef TRACK_HINT_SEARCH_STATS
	int Level::hintSearchActionsChecked = 0;
	int Level::hintSearchUniqueStates = 0;
	int Level::hintSearchComparisonsPerformed = 0;
	int Level::foundHintSearchTotalHintSteps = 0;
	int Level::foundHintSearchTotalSteps = 0;
#endif
Level::Level(objCounterParametersComma() int pLevelN, int pStartTile)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
levelN(pLevelN)
, startTile(pStartTile)
, planes()
, allRailByteMaskData()
, railByteMaskBitsTracked(0)
, victoryPlane(nullptr)
, minimumRailColor(0)
, radioTowerHint(Hint::Type::Switch) {
}
Level::~Level() {
	for (Plane* plane : planes)
		delete plane;
	//don't delete victoryPlane, it was included in planes
}
Plane* Level::addNewPlane() {
	Plane* plane = newPlane(this, (int)planes.size());
	planes.push_back(plane);
	return plane;
}
void Level::addVictoryPlane() {
	victoryPlane = addNewPlane();
	victoryPlane->setHasAction();
}
int Level::trackNextRail(short railId, Rail* rail) {
	minimumRailColor = MathUtils::max(rail->getColor(), minimumRailColor);
	int railByteIndex = railByteMaskBitsTracked / 32;
	int railBitShift = railByteMaskBitsTracked % 32;
	//make sure there are enough bits to fit the new mask
	if (railBitShift + railByteMaskBitCount > 32) {
		railByteIndex++;
		railBitShift = 0;
		railByteMaskBitsTracked = (railByteMaskBitsTracked / 32 + 1) * 32 + railByteMaskBitCount;
	} else
		railByteMaskBitsTracked += railByteMaskBitCount;
	allRailByteMaskData.push_back(RailByteMaskData(railId, railByteIndex, railBitShift, rail));
	return (int)allRailByteMaskData.size() - 1;
}
void Level::setupPotentialLevelStateHelpers(vector<Level*>& allLevels) {
	for (Level* level : allLevels) {
		//add one PotentialLevelStatesByBucket per plane, which includes the victory plane
		while (potentialLevelStatesByBucketByPlane.size() < level->planes.size())
			potentialLevelStatesByBucketByPlane.push_back(PotentialLevelStatesByBucket());
		HintState::PotentialLevelState::maxRailByteMaskCount =
			MathUtils::max(HintState::PotentialLevelState::maxRailByteMaskCount, level->getRailByteMaskCount());
	}
	//just once, fix the draft state byte list
	delete[] HintState::PotentialLevelState::draftState.railByteMasks;
	HintState::PotentialLevelState::draftState.railByteMasks =
		new unsigned int[HintState::PotentialLevelState::maxRailByteMaskCount];
}
void Level::deleteHelpers() {
	potentialLevelStatesByBucketByPlane.clear();
	for (deque<HintState::PotentialLevelState*>* nextPotentialLevelStates : nextPotentialLevelStatesBySteps)
		delete nextPotentialLevelStates;
	nextPotentialLevelStatesBySteps.clear();
}
void Level::preAllocatePotentialLevelStates() {
	generateHint(
		planes[0],
		[](short railId, Rail* rail, char* outMovementDirection, char* outTileOffset) {
			*outMovementDirection = rail->getInitialMovementDirection();
			*outTileOffset = rail->getInitialTileOffset();
		},
		minimumRailColor);
}
Hint* Level::generateHint(
	LevelTypes::Plane* currentPlane,
	function<void(short railId, Rail* rail, char* outMovementDirection, char* outTileOffset)> getRailState,
	char lastActivatedSwitchColor)
{
	if (lastActivatedSwitchColor < minimumRailColor)
		return &radioTowerHint;
	else if (victoryPlane == nullptr)
		return &Hint::none;

	//save the victory plane so Plane can use it
	cachedHintSearchVictoryPlane = victoryPlane;

	//setup the base potential level state
	HintState::PotentialLevelState::currentRailByteMaskCount = getRailByteMaskCount();
	for (int i = 0; i < HintState::PotentialLevelState::currentRailByteMaskCount; i++)
		HintState::PotentialLevelState::draftState.railByteMasks[i] = 0;
	for (LevelTypes::RailByteMaskData& railByteMaskData : allRailByteMaskData) {
		char movementDirection, tileOffset;
		getRailState(railByteMaskData.railId, railByteMaskData.rail, &movementDirection, &tileOffset);
		char movementDirectionBit = ((movementDirection + 1) / 2) << Level::railTileOffsetByteMaskBitCount;
		HintState::PotentialLevelState::draftState.railByteMasks[railByteMaskData.railByteIndex] |=
			(unsigned int)(movementDirectionBit | tileOffset) << railByteMaskData.railBitShift;
	}
	HintState::PotentialLevelState::draftState.setHash();

	//load it into the potential level state structures
	HintState::PotentialLevelState* baseLevelState = HintState::PotentialLevelState::draftState.addNewState(
		potentialLevelStatesByBucketByPlane[currentPlane->getIndexInOwningLevel()]
			.buckets[HintState::PotentialLevelState::draftState.railByteMasksHash % PotentialLevelStatesByBucket::bucketSize],
		0);
	baseLevelState->priorState = nullptr;
	baseLevelState->plane = currentPlane;
	baseLevelState->hint = nullptr;
	maxPotentialLevelStateSteps = -1;
	getNextPotentialLevelStatesForSteps(0)->push_back(baseLevelState);

	//go through all states and see if there's anything we could do to get closer to the victory plane
	Hint* result = nullptr;
	#ifdef TRACK_HINT_SEARCH_STATS
		hintSearchActionsChecked = 0;
		hintSearchUniqueStates = 0;
		hintSearchComparisonsPerformed = 0;
		foundHintSearchTotalHintSteps = 0;
		stringstream beginHintSearchMessage;
		beginHintSearchMessage << "begin level " << levelN << " hint search";
		Logger::debugLogger.logString(beginHintSearchMessage.str());
		int timeBeforeSearch = SDL_GetTicks();
	#endif
	for (currentPotentialLevelStateSteps = 0;
		currentPotentialLevelStateSteps < (int)nextPotentialLevelStatesBySteps.size();
		currentPotentialLevelStateSteps++)
	{
		deque<HintState::PotentialLevelState*>* nextPotentialLevelStates =
			nextPotentialLevelStatesBySteps[currentPotentialLevelStateSteps];
		while (!nextPotentialLevelStates->empty()) {
			HintState::PotentialLevelState* potentialLevelState = nextPotentialLevelStates->front();
			nextPotentialLevelStates->pop_front();
			//skip any states that were replaced with shorter routes
			if (potentialLevelState->steps == -1)
				continue;
			result = potentialLevelState->plane->pursueSolution(potentialLevelState);
			if (result != nullptr) {
				currentPotentialLevelStateSteps = (int)nextPotentialLevelStatesBySteps.size();
				break;
			}
		}
	}

	//cleanup
	#ifdef TRACK_HINT_SEARCH_STATS
		int timeAfterSearchBeforeCleanup = SDL_GetTicks();
	#endif
	for (int i = 0; i <= maxPotentialLevelStateSteps; i++)
		nextPotentialLevelStatesBySteps[i]->clear();
	//only clear as many plane buckets as we used
	for (int i = 0; i < (int)planes.size(); i++) {
		for (vector<HintState::PotentialLevelState*>& potentialLevelStates : potentialLevelStatesByBucketByPlane[i].buckets) {
			//these were all retained once before by PotentialLevelState::addNewState, so release them here
			for (HintState::PotentialLevelState* potentialLevelState : potentialLevelStates)
				potentialLevelState->release();
			potentialLevelStates.clear();
		}
	}
	//release all PotentialLevelStates that were taken out of potentialLevelStatesByBucketByPlane, but couldn't be released
	//	because they were still in nextPotentialLevelStatesBySteps
	for (HintState::PotentialLevelState* potentialLevelState : replacedPotentialLevelStates)
		potentialLevelState->release();
	replacedPotentialLevelStates.clear();

	#ifdef TRACK_HINT_SEARCH_STATS
		int timeAfterCleanup = SDL_GetTicks();
		stringstream hintSearchPerformanceMessage;
		hintSearchPerformanceMessage
			<< "level " << levelN
			<< " (" << planes.size() << "p, " << allRailByteMaskData.size() << "r, c" << (int)minimumRailColor << ")"
			<< "  actionsChecked " << hintSearchActionsChecked
			<< "  uniqueStates " << hintSearchUniqueStates
			<< "  comparisonsPerformed " << hintSearchComparisonsPerformed
			<< "  searchTime " << (timeAfterSearchBeforeCleanup - timeBeforeSearch)
			<< "  cleanupTime " << (timeAfterCleanup - timeAfterSearchBeforeCleanup)
			<< "  found solution? " << (result != nullptr ? "true" : "false")
			<< "  steps " << foundHintSearchTotalSteps << "(" << foundHintSearchTotalHintSteps << ")";
		Logger::debugLogger.logString(hintSearchPerformanceMessage.str());
	#endif

	return result != nullptr ? result : &Hint::undoReset;
}
deque<HintState::PotentialLevelState*>* Level::getNextPotentialLevelStatesForSteps(int nextPotentialLevelStateSteps) {
	while (maxPotentialLevelStateSteps < nextPotentialLevelStateSteps) {
		maxPotentialLevelStateSteps++;
		if ((int)nextPotentialLevelStatesBySteps.size() == maxPotentialLevelStateSteps)
			nextPotentialLevelStatesBySteps.push_back(new deque<HintState::PotentialLevelState*>());
	}
	return nextPotentialLevelStatesBySteps[nextPotentialLevelStateSteps];
}
void Level::logStats() {
	int switchCounts[4] = {};
	int singleUseSwitches = 0;
	for (Plane* plane : planes)
		plane->countSwitches(switchCounts, &singleUseSwitches);
	int totalSwitches = 0;
	for (int switchCount : switchCounts)
		totalSwitches += switchCount;
	stringstream message;
	message
		<< "level " << levelN << " detected: "
		<< planes.size() << " planes, "
		<< allRailByteMaskData.size() << " rails, "
		<< totalSwitches << " switches (";
	for (int i = 0; i < 4; i++)
		message << switchCounts[i] << (i == 3 ? ", " : " ");
	message << singleUseSwitches << " single-use)";
	Logger::debugLogger.logString(message.str());
}
