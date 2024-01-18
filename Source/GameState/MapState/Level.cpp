#include "Level.h"
#include "GameState/HintState.h"
#include "GameState/MapState/Rail.h"

#define newPlane(owningLevel, indexInOwningLevel) newWithArgs(Plane, owningLevel, indexInOwningLevel)

//////////////////////////////// LevelTypes::Plane::ConnectionSwitch ////////////////////////////////
LevelTypes::Plane::ConnectionSwitch::ConnectionSwitch(short pSwitchId)
: affectedRailByteMaskData()
, switchId(pSwitchId) {
}
LevelTypes::Plane::ConnectionSwitch::~ConnectionSwitch() {}

//////////////////////////////// LevelTypes::Plane::Connection ////////////////////////////////
LevelTypes::Plane::Connection::Connection(Plane* pToPlane, int pRailByteIndex, int pRailTileOffsetByteMask, short pRailId)
: toPlane(pToPlane)
, railByteIndex(pRailByteIndex)
, railTileOffsetByteMask(pRailTileOffsetByteMask)
, railId(pRailId) {
}
LevelTypes::Plane::Connection::~Connection() {
	//don't delete the plane, it's owned by a Level
}

//////////////////////////////// LevelTypes::Plane::Tile ////////////////////////////////
LevelTypes::Plane::Tile::Tile(int pX, int pY)
: x(pX)
, y(pY) {
}
LevelTypes::Plane::Tile::~Tile() {}

//////////////////////////////// LevelTypes::Plane ////////////////////////////////
LevelTypes::Plane::Plane(objCounterParametersComma() Level* pOwningLevel, int pIndexInOwningLevel)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
owningLevel(pOwningLevel)
, indexInOwningLevel(pIndexInOwningLevel)
, tiles()
, connectionSwitches()
, connections() {
}
LevelTypes::Plane::~Plane() {
	//don't delete owningLevel, it owns and deletes this
}
int LevelTypes::Plane::addConnectionSwitch(short switchId) {
	connectionSwitches.push_back(ConnectionSwitch(switchId));
	return (int)connectionSwitches.size() - 1;
}
bool LevelTypes::Plane::addConnection(Plane* toPlane, bool isRail, short railId) {
	//don't connect to a plane twice
	for (Connection& connection : connections) {
		if (connection.toPlane == toPlane)
			return true;
	}
	//add a climb/fall connection
	if (!isRail) {
		connections.push_back(Connection(toPlane, Level::absentRailByteIndex, 0, railId));
		return true;
	}
	//add a rail connection to a plane that is already connected to this plane
	for (Connection& connection : toPlane->connections) {
		if (connection.toPlane == this) {
			connections.push_back(Connection(toPlane, connection.railByteIndex, connection.railTileOffsetByteMask, railId));
			return true;
		}
	}
	return false;
}
void LevelTypes::Plane::addRailConnection(Plane* toPlane, RailByteMaskData* railByteMaskData, short railId) {
	connections.push_back(
		Connection(
			toPlane,
			railByteMaskData->railByteIndex,
			Level::baseRailTileOffsetByteMask << railByteMaskData->railBitShift,
			railId));
}
void LevelTypes::Plane::writeVictoryPlaneIndex(Plane* victoryPlane, int pIndexInOwningLevel) {
	indexInOwningLevel = 0;
	victoryPlane->indexInOwningLevel = pIndexInOwningLevel;
}
HintState* LevelTypes::Plane::pursueSolution(HintStateTypes::PotentialLevelState* currentState) {
	unsigned int bucket = currentState->railByteMasksHash % Level::PotentialLevelStatesByBucket::bucketSize;
	//check connections
	for (Connection& connection : connections) {
		#ifdef DEBUG
			Level::hintSearchActionsChecked++;
		#endif
		//make sure that this is a climb/fall, or that the rail is raised
		if (connection.railByteIndex >= 0
				&& (currentState->railByteMasks[connection.railByteIndex] & connection.railTileOffsetByteMask) != 0)
			continue;

		//make sure we haven't seen this state before
		vector<HintStateTypes::PotentialLevelState*>& potentialLevelStates =
			Level::potentialLevelStatesByBucketByPlane[connection.toPlane->indexInOwningLevel].buckets[bucket];
		if (!currentState->isNewState(potentialLevelStates))
			continue;

		//build out the new PotentialLevelState
		HintStateTypes::PotentialLevelState* nextPotentialLevelState =
			newHintStatePotentialLevelState(currentState, connection.toPlane, currentState);
		if (connection.railByteIndex < 0) {
			nextPotentialLevelState->type = HintStateTypes::Type::Plane;
			nextPotentialLevelState->data.plane = connection.toPlane;
		} else {
			nextPotentialLevelState->type = HintStateTypes::Type::Rail;
			nextPotentialLevelState->data.railId = connection.railId;
		}
		#ifdef DEBUG
			Level::hintSearchUniqueStates++;
		#endif

		//if it goes to the victory plane, return the hint for the first transition
		if (connection.toPlane == Level::cachedHintSearchVictoryPlane) {
			//make sure the new state is returned to the pool
			ReferenceCounterHolder<HintStateTypes::PotentialLevelState> nextPotentialLevelStateHolder (nextPotentialLevelState);
			return nextPotentialLevelState->getHint();
		}
		//otherwise, track it
		potentialLevelStates.push_back(nextPotentialLevelState);
		Level::nextPotentialLevelStates.push_back(nextPotentialLevelState);
	}

	//check switches
	for (ConnectionSwitch& connectionSwitch : connectionSwitches) {
		#ifdef DEBUG
			Level::hintSearchActionsChecked++;
		#endif
		//first, reset the draft rail byte masks
		for (int i = currentState->railByteMaskCount - 1; i >= 0; i--)
			HintStateTypes::PotentialLevelState::draftState.railByteMasks[i] = currentState->railByteMasks[i];
		//then, go through and modify the byte mask for each affected rail
		for (RailByteMaskData* railByteMaskData : connectionSwitch.affectedRailByteMaskData) {
			unsigned int* railByteMask =
				&HintStateTypes::PotentialLevelState::draftState.railByteMasks[railByteMaskData->railByteIndex];
			char shiftedRailState = (char)(*railByteMask >> railByteMaskData->railBitShift);
			char movementDirectionBit = shiftedRailState & (char)Level::baseRailMovementDirectionByteMask;
			char tileOffset = shiftedRailState & (char)Level::baseRailTileOffsetByteMask;
			char movementDirection = (movementDirectionBit >> Level::railTileOffsetByteMaskBitCount) * 2 - 1;
			char resultRailState = railByteMaskData->rail->triggerMovement(movementDirection, &tileOffset)
				? tileOffset | (movementDirectionBit ^ Level::baseRailMovementDirectionByteMask)
				: tileOffset | movementDirectionBit;
			*railByteMask =
				(*railByteMask & railByteMaskData->inverseRailByteMask)
					| ((unsigned int)resultRailState << railByteMaskData->railBitShift);
		}
		HintStateTypes::PotentialLevelState::draftState.setHash();
		bucket =
			HintStateTypes::PotentialLevelState::draftState.railByteMasksHash % Level::PotentialLevelStatesByBucket::bucketSize;

		//make sure we haven't seen this state before
		vector<HintStateTypes::PotentialLevelState*>& potentialLevelStates =
			Level::potentialLevelStatesByBucketByPlane[indexInOwningLevel].buckets[bucket];
		if (!HintStateTypes::PotentialLevelState::draftState.isNewState(potentialLevelStates))
			continue;

		//build out the new PotentialLevelState and track it
		HintStateTypes::PotentialLevelState* nextPotentialLevelState =
			newHintStatePotentialLevelState(currentState, this, &HintStateTypes::PotentialLevelState::draftState);
		nextPotentialLevelState->type = HintStateTypes::Type::Switch;
		nextPotentialLevelState->data.switchId = connectionSwitch.switchId;
		#ifdef DEBUG
			Level::hintSearchUniqueStates++;
		#endif
		potentialLevelStates.push_back(nextPotentialLevelState);
		Level::nextPotentialLevelStates.push_back(nextPotentialLevelState);
	}

	return nullptr;
}

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
deque<HintStateTypes::PotentialLevelState*> Level::nextPotentialLevelStates;
Plane* Level::cachedHintSearchVictoryPlane = nullptr;
#ifdef DEBUG
	int Level::hintSearchActionsChecked = 0;
	int Level::hintSearchUniqueStates = 0;
	int Level::hintSearchComparisonsPerformed = 0;
#endif
Level::Level(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
planes()
, allRailByteMaskData()
, railByteMaskBitsTracked(0)
, victoryPlane(nullptr)
, minimumRailColor(0)
, radioTowerSwitchId(0) {
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
void Level::buildPotentialLevelStatesByBucketByPlane(vector<Level*>& allLevels) {
	int maxPlaneCount = 0;
	for (Level* level : allLevels)
		maxPlaneCount = MathUtils::max(maxPlaneCount, (int)level->planes.size());
	//include one extra for the victory plane
	for (int i = 0; i <= maxPlaneCount; i++)
		potentialLevelStatesByBucketByPlane.push_back(PotentialLevelStatesByBucket());
}
HintState* Level::generateHint(
	LevelTypes::Plane* currentPlane,
	function<void(short railId, char* outMovementDirection, char* outTileOffset)> getRailState,
	char lastActivatedSwitchColor)
{
	if (lastActivatedSwitchColor < minimumRailColor) {
		HintStateTypes::Data data;
		data.switchId = radioTowerSwitchId;
		return newHintState(HintStateTypes::Type::Switch, data);
	} else if (victoryPlane == nullptr)
		return newHintState(HintStateTypes::Type::None, {});

	//setup the base potential level state
	HintStateTypes::PotentialLevelState::draftState.resizeRailByteMasks(getRailByteCount());
	for (int i = getRailByteCount() - 1; i >= 0; i--)
		HintStateTypes::PotentialLevelState::draftState.railByteMasks[i] = 0;
	for (LevelTypes::RailByteMaskData& railByteMaskData : allRailByteMaskData) {
		char movementDirection, tileOffset;
		getRailState(railByteMaskData.railId, &movementDirection, &tileOffset);
		char movementDirectionBit = ((movementDirection + 1) / 2) << Level::railTileOffsetByteMaskBitCount;
		HintStateTypes::PotentialLevelState::draftState.railByteMasks[railByteMaskData.railByteIndex] |=
			(unsigned int)(movementDirectionBit | tileOffset) << railByteMaskData.railBitShift;
	}
	HintStateTypes::PotentialLevelState::draftState.setHash();
	HintStateTypes::PotentialLevelState* baseLevelState =
		newHintStatePotentialLevelState(nullptr, currentPlane, &HintStateTypes::PotentialLevelState::draftState);

	//setup the potential level state structures
	potentialLevelStatesByBucketByPlane[baseLevelState->plane->getIndexInOwningLevel()]
		.buckets[baseLevelState->railByteMasksHash % PotentialLevelStatesByBucket::bucketSize]
		.push_back(baseLevelState);
	nextPotentialLevelStates.push_back(baseLevelState);
	cachedHintSearchVictoryPlane = victoryPlane;
	//make sure to distinguish our plane 0 from the victory plane, which is always plane 0 of the next level
	planes[0]->writeVictoryPlaneIndex(victoryPlane, (int)planes.size());

	//go through all states and see if there's anything we could do to get closer to the victory plane
	HintState* result = nullptr;
	#ifdef DEBUG
		hintSearchActionsChecked = 0;
		hintSearchUniqueStates = 0;
		hintSearchComparisonsPerformed = 0;
		int timeBeforeSearch = SDL_GetTicks();
	#endif
	while (!nextPotentialLevelStates.empty()) {
		HintStateTypes::PotentialLevelState* potentialLevelState = nextPotentialLevelStates.front();
		nextPotentialLevelStates.pop_front();
		result = potentialLevelState->plane->pursueSolution(potentialLevelState);
		if (result != nullptr)
			break;
	}

	//cleanup
	#ifdef DEBUG
		int timeAfterSearchBeforeCleanup = SDL_GetTicks();
	#endif
	nextPotentialLevelStates.clear();
	//only clear as many plane buckets as we used
	for (int i = 0; i < (int)planes.size(); i++) {
		for (vector<HintStateTypes::PotentialLevelState*>& potentialLevelStates
				: potentialLevelStatesByBucketByPlane[i].buckets)
		{
			//rather than doing the most correct thing, and storing ReferenceCounterHolders in PotentialLevelStatesByBucket, we
			//	only store pointers, so retain-and-release each one here
			for (HintStateTypes::PotentialLevelState* potentialLevelState : potentialLevelStates)
				ReferenceCounterHolder<HintStateTypes::PotentialLevelState> potendialLevelStateHolder (potentialLevelState);
			potentialLevelStates.clear();
		}
	}

	#ifdef DEBUG
		int timeAfterCleanup = SDL_GetTicks();
		stringstream hintSearchPerformanceMessage;
		hintSearchPerformanceMessage
			<< "actionsChecked " << hintSearchActionsChecked
			<< "  uniqueStates " << hintSearchUniqueStates
			<< "  comparisonsPerformed " << hintSearchComparisonsPerformed
			<< "  searchTime " << (timeAfterSearchBeforeCleanup - timeBeforeSearch)
			<< "  cleanupTime " << (timeAfterCleanup - timeAfterSearchBeforeCleanup)
			<< "  found solution? " << (result != nullptr ? "true" : "false");
		Logger::debugLogger.logString(hintSearchPerformanceMessage.str());
	#endif

	return result != nullptr ? result : newHintState(HintStateTypes::Type::None, {});
}
