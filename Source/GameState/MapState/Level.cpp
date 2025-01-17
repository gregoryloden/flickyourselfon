#include "Level.h"
#include "GameState/MapState/MapState.h"
#include "GameState/MapState/Rail.h"
#include "GameState/MapState/Switch.h"
#include "Sprites/SpriteSheet.h"
#if defined(TRACK_HINT_SEARCH_STATS) || defined(LOG_FOUND_HINT_STEPS)
	#include "Util/Logger.h"
#endif

#define newPlane(owningLevel, indexInOwningLevel) newWithArgs(Plane, owningLevel, indexInOwningLevel)

//////////////////////////////// LevelTypes::Plane::Tile ////////////////////////////////
LevelTypes::Plane::Tile::Tile(int pX, int pY)
: x(pX)
, y(pY) {
}
LevelTypes::Plane::Tile::~Tile() {}

//////////////////////////////// LevelTypes::Plane::ConnectionSwitch ////////////////////////////////
LevelTypes::Plane::ConnectionSwitch::ConnectionSwitch(Switch* switch0, int levelN)
: affectedRailByteMaskData()
, hint(Hint::Type::Switch, levelN) {
	hint.data.switch0 = switch0;
}
LevelTypes::Plane::ConnectionSwitch::~ConnectionSwitch() {}

//////////////////////////////// LevelTypes::Plane::Connection ////////////////////////////////
LevelTypes::Plane::Connection::Connection(
	Plane* pToPlane, int pRailByteIndex, int pRailTileOffsetByteMask, Rail* rail, int levelN)
: toPlane(pToPlane)
, railByteIndex(pRailByteIndex)
, railTileOffsetByteMask(pRailTileOffsetByteMask)
, hint(Hint::Type::None, levelN) {
	if (rail != nullptr) {
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
	connectionSwitches.push_back(ConnectionSwitch(switch0, owningLevel->getLevelN()));
	return (int)connectionSwitches.size() - 1;
}
bool LevelTypes::Plane::addConnection(Plane* toPlane, Rail* rail) {
	//add a climb/fall connection
	if (rail == nullptr) {
		//don't add climb/fall connections to a plane twice
		for (Connection& connection : connections) {
			if (connection.toPlane == toPlane)
				return true;
		}
		connections.push_back(Connection(toPlane, Level::absentRailByteIndex, 0, nullptr, owningLevel->getLevelN()));
		return true;
	}
	//add a rail connection to a plane that is already connected to this plane
	for (Connection& connection : toPlane->connections) {
		if (connection.toPlane != this || connection.hint.data.rail != rail)
			continue;
		connections.push_back(
			Connection(toPlane, connection.railByteIndex, connection.railTileOffsetByteMask, rail, owningLevel->getLevelN()));
		return true;
	}
	return false;
}
void LevelTypes::Plane::addRailConnection(Plane* toPlane, RailByteMaskData* railByteMaskData, Rail* rail) {
	connections.push_back(
		Connection(
			toPlane,
			railByteMaskData->railByteIndex,
			Level::baseRailTileOffsetByteMask << railByteMaskData->railBitShift,
			rail,
			owningLevel->getLevelN()));
}
void LevelTypes::Plane::writeVictoryPlaneIndex(Plane* victoryPlane, int pIndexInOwningLevel) {
	indexInOwningLevel = 0;
	victoryPlane->indexInOwningLevel = pIndexInOwningLevel;
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
		if (!currentState->isNewState(potentialLevelStates))
			continue;

		//build out the new PotentialLevelState
		HintState::PotentialLevelState* nextPotentialLevelState =
			newHintStatePotentialLevelState(currentState, connection.toPlane, currentState, &connection.hint);
		#ifdef TRACK_HINT_SEARCH_STATS
			Level::hintSearchUniqueStates++;
		#endif

		//if it goes to the victory plane, return the hint for the first transition
		if (connection.toPlane == Level::cachedHintSearchVictoryPlane) {
			//make sure the new state is returned to the pool
			ReferenceCounterHolder<HintState::PotentialLevelState> nextPotentialLevelStateHolder (nextPotentialLevelState);
			#ifdef LOG_FOUND_HINT_STEPS
				logSteps(nextPotentialLevelState);
			#endif
			return nextPotentialLevelState->getHint();
		}
		//otherwise, track it
		potentialLevelStates.push_back(nextPotentialLevelState);
		Level::nextPotentialLevelStates.push_back(nextPotentialLevelState);
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
		for (RailByteMaskData* railByteMaskData : connectionSwitch.affectedRailByteMaskData) {
			unsigned int* railByteMask =
				&HintState::PotentialLevelState::draftState.railByteMasks[railByteMaskData->railByteIndex];
			char shiftedRailState = (char)(*railByteMask >> railByteMaskData->railBitShift);
			char movementDirectionBit = shiftedRailState & (char)Level::baseRailMovementDirectionByteMask;
			char tileOffset = shiftedRailState & (char)Level::baseRailTileOffsetByteMask;
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
		if (!HintState::PotentialLevelState::draftState.isNewState(potentialLevelStates))
			continue;

		//build out the new PotentialLevelState and track it
		HintState::PotentialLevelState* nextPotentialLevelState = newHintStatePotentialLevelState(
			currentState, this, &HintState::PotentialLevelState::draftState, &connectionSwitch.hint);
		#ifdef TRACK_HINT_SEARCH_STATS
			Level::hintSearchUniqueStates++;
		#endif
		potentialLevelStates.push_back(nextPotentialLevelState);
		Level::nextPotentialLevelStates.push_back(nextPotentialLevelState);
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
deque<HintState::PotentialLevelState*> Level::nextPotentialLevelStates;
Plane* Level::cachedHintSearchVictoryPlane = nullptr;
#ifdef TRACK_HINT_SEARCH_STATS
	int Level::hintSearchActionsChecked = 0;
	int Level::hintSearchUniqueStates = 0;
	int Level::hintSearchComparisonsPerformed = 0;
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
, radioTowerHint(Hint::Type::Switch, pLevelN) {
}
Level::~Level() {
	for (Plane* plane : planes)
		delete plane;
	//don't delete victoryPlane, it's owned by another Level
}
Plane* Level::addNewPlane() {
	Plane* plane = newPlane(this, (int)planes.size());
	planes.push_back(plane);
	return plane;
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
		//add one PotentialLevelStatesByBucket per plane, plus one extra for the victory plane
		while (potentialLevelStatesByBucketByPlane.size() <= level->planes.size())
			potentialLevelStatesByBucketByPlane.push_back(PotentialLevelStatesByBucket());
		HintState::PotentialLevelState::maxRailByteMaskCount =
			MathUtils::max(HintState::PotentialLevelState::maxRailByteMaskCount, level->getRailByteMaskCount());
	}
	//just once, fix the draft state byte list
	delete[] HintState::PotentialLevelState::draftState.railByteMasks;
	HintState::PotentialLevelState::draftState.railByteMasks =
		new unsigned int[HintState::PotentialLevelState::maxRailByteMaskCount];
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

	//prepare the victory plane
	cachedHintSearchVictoryPlane = victoryPlane;
	//make sure to distinguish our plane 0 from the victory plane, which is always plane 0 of the next level
	planes[0]->writeVictoryPlaneIndex(victoryPlane, (int)planes.size());

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
	HintState::PotentialLevelState* baseLevelState =
		newHintStatePotentialLevelState(nullptr, currentPlane, &HintState::PotentialLevelState::draftState, nullptr);
	potentialLevelStatesByBucketByPlane[currentPlane->getIndexInOwningLevel()]
		.buckets[baseLevelState->railByteMasksHash % PotentialLevelStatesByBucket::bucketSize]
		.push_back(baseLevelState);
	nextPotentialLevelStates.push_back(baseLevelState);

	//go through all states and see if there's anything we could do to get closer to the victory plane
	Hint* result = nullptr;
	#ifdef TRACK_HINT_SEARCH_STATS
		hintSearchActionsChecked = 0;
		hintSearchUniqueStates = 0;
		hintSearchComparisonsPerformed = 0;
		foundHintSearchTotalSteps = 0;
		int timeBeforeSearch = SDL_GetTicks();
	#endif
	while (!nextPotentialLevelStates.empty()) {
		HintState::PotentialLevelState* potentialLevelState = nextPotentialLevelStates.front();
		nextPotentialLevelStates.pop_front();
		result = potentialLevelState->plane->pursueSolution(potentialLevelState);
		if (result != nullptr)
			break;
	}

	//cleanup
	#ifdef TRACK_HINT_SEARCH_STATS
		int timeAfterSearchBeforeCleanup = SDL_GetTicks();
	#endif
	nextPotentialLevelStates.clear();
	//only clear as many plane buckets as we used
	for (int i = 0; i < (int)planes.size(); i++) {
		for (vector<HintState::PotentialLevelState*>& potentialLevelStates : potentialLevelStatesByBucketByPlane[i].buckets) {
			//rather than doing the most correct thing, and storing ReferenceCounterHolders in PotentialLevelStatesByBucket, we
			//	only store pointers, so retain-and-release each one here
			for (HintState::PotentialLevelState* potentialLevelState : potentialLevelStates)
				ReferenceCounterHolder<HintState::PotentialLevelState> potendialLevelStateHolder (potentialLevelState);
			potentialLevelStates.clear();
		}
	}

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
			<< "  steps " << foundHintSearchTotalSteps;
		Logger::debugLogger.logString(hintSearchPerformanceMessage.str());
	#endif

	return result != nullptr ? result : &Hint::undoReset;
}
