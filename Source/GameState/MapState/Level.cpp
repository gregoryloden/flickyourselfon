#include "Level.h"
#include "GameState/MapState/MapState.h"
#include "GameState/MapState/Rail.h"
#include "GameState/MapState/Switch.h"
#include "GameState/MapState/ResetSwitch.h"
#include "Sprites/SpriteSheet.h"
#ifdef RENDER_PLANE_IDS
	#include "Sprites/Text.h"
#endif
#ifdef TEST_SOLUTIONS
	#include "Util/FileUtils.h"
#endif
#include "Util/Logger.h"
#ifdef TEST_SOLUTIONS
	#include "Util/StringUtils.h"
#endif
#include "Util/VectorUtils.h"

#define newPlane(owningLevel, indexInOwningLevel) newWithArgs(Plane, owningLevel, indexInOwningLevel)

//////////////////////////////// LevelTypes::RailByteMaskData::ByteMask ////////////////////////////////
LevelTypes::RailByteMaskData::ByteMask::ByteMask(BitsLocation pLocation, int nBits)
: location(pLocation)
, byteMask(((1 << nBits) - 1) << pLocation.data.bitShift) {
}

//////////////////////////////// LevelTypes::RailByteMaskData ////////////////////////////////
LevelTypes::RailByteMaskData::RailByteMaskData(Rail* pRail, short pRailId, ByteMask pRailBits)
: rail(pRail)
, railId(pRailId)
, cachedRailColor(pRail->getColor())
, railBits(pRailBits.location)
, inverseRailByteMask(~pRailBits.byteMask) {
}
LevelTypes::RailByteMaskData::~RailByteMaskData() {}

//////////////////////////////// LevelTypes::Plane::Tile ////////////////////////////////
LevelTypes::Plane::Tile::Tile(int pX, int pY)
: x(pX)
, y(pY) {
}
LevelTypes::Plane::Tile::~Tile() {}

////////////////////////////////
// LevelTypes::Plane::ConnectionSwitch::ConclusionsData::IsolatedArea
////////////////////////////////
LevelTypes::Plane::ConnectionSwitch::ConclusionsData::IsolatedArea::IsolatedArea(RailByteMaskData::ByteMask pMiniPuzzleBit)
: otherGoalSwitchCanKickBits()
, miniPuzzleBit(pMiniPuzzleBit) {
}

//////////////////////////////// LevelTypes::Plane::ConnectionSwitch ////////////////////////////////
LevelTypes::Plane::ConnectionSwitch::ConnectionSwitch(Switch* switch0)
: affectedRailByteMaskData()
, hint(switch0)
, canKickBit(Level::absentBits)
, isSingleUse(true)
, isMilestone(false)
, conclusionsType(ConclusionsType::None)
, conclusionsData() {
}
LevelTypes::Plane::ConnectionSwitch::ConnectionSwitch(const ConnectionSwitch& other)
: affectedRailByteMaskData(other.affectedRailByteMaskData)
, hint(other.hint)
, canKickBit(other.canKickBit)
, isSingleUse(other.isSingleUse)
, isMilestone(other.isMilestone)
, conclusionsType(other.conclusionsType)
, conclusionsData() {
	switch (conclusionsType) {
		case ConclusionsType::MiniPuzzle:
			new(&conclusionsData.miniPuzzle) ConclusionsData::MiniPuzzle(other.conclusionsData.miniPuzzle);
			break;
		case ConclusionsType::IsolatedArea:
			new(&conclusionsData.isolatedArea) ConclusionsData::IsolatedArea(other.conclusionsData.isolatedArea);
			break;
		case ConclusionsType::DeadRail:
			new(&conclusionsData.deadRail) ConclusionsData::DeadRail(other.conclusionsData.deadRail);
			break;
	}
}
LevelTypes::Plane::ConnectionSwitch::~ConnectionSwitch() {
	switch (conclusionsType) {
		case ConclusionsType::MiniPuzzle: conclusionsData.miniPuzzle.~MiniPuzzle(); break;
		case ConclusionsType::IsolatedArea: conclusionsData.isolatedArea.~IsolatedArea(); break;
		case ConclusionsType::DeadRail: conclusionsData.deadRail.~DeadRail(); break;
	}
}
void LevelTypes::Plane::ConnectionSwitch::writeTileOffsetByteMasks(vector<unsigned int>& railByteMasks) {
	for (RailByteMaskData* railByteMaskData : affectedRailByteMaskData)
		railByteMasks[railByteMaskData->railBits.data.byteIndex] |=
			Level::baseRailTileOffsetByteMask << railByteMaskData->railBits.data.bitShift;
}
void LevelTypes::Plane::ConnectionSwitch::setMiniPuzzle(
	RailByteMaskData::ByteMask miniPuzzleBit, vector<DetailedRail*>& miniPuzzleRails)
{
	new(&conclusionsData.miniPuzzle) ConclusionsData::MiniPuzzle();
	conclusionsType = ConclusionsType::MiniPuzzle;
	canKickBit = miniPuzzleBit;
	for (DetailedRail* miniPuzzleRail : miniPuzzleRails) {
		if (!VectorUtils::includes(affectedRailByteMaskData, miniPuzzleRail->railByteMaskData))
			conclusionsData.miniPuzzle.otherRailBits.push_back(miniPuzzleRail->railByteMaskData->railBits);
	}
}
void LevelTypes::Plane::ConnectionSwitch::setIsolatedArea(
	vector<DetailedConnectionSwitch*>& isolatedAreaSwitches, RailByteMaskData::ByteMask miniPuzzleBit)
{
	new(&conclusionsData.isolatedArea) ConclusionsData::IsolatedArea(miniPuzzleBit);
	conclusionsType = ConclusionsType::IsolatedArea;
	for (DetailedConnectionSwitch* isolatedAreaSwitch : isolatedAreaSwitches) {
		if (isolatedAreaSwitch->connectionSwitch != this)
			conclusionsData.isolatedArea.otherGoalSwitchCanKickBits.push_back(
				isolatedAreaSwitch->connectionSwitch->canKickBit.location);
	}
}
void LevelTypes::Plane::ConnectionSwitch::setDeadRail(
	RailByteMaskData::ByteMask deadRailBit,
	vector<bool>& isDeadRail,
	vector<DetailedConnectionSwitch*>& deadRailCompletedSwitches,
	RailByteMaskData::BitsLocation alwaysOffBitLocation)
{
	new(&conclusionsData.deadRail) ConclusionsData::DeadRail();
	conclusionsType = ConclusionsType::DeadRail;
	canKickBit = deadRailBit;
	//go through and combine live rails and dead rails into one list the size of the switch's affected rails
	for (int i = 0; i < (int)affectedRailByteMaskData.size(); i++) {
		RailByteMaskData::BitsLocation::Data railBitsLocation = affectedRailByteMaskData[i]->railBits.data;
		//live rails will use an absent bits location, indicating that we should check that the rail at the same index is live
		if (!isDeadRail[i])
			conclusionsData.deadRail.completedSwitches.push_back(Level::absentBits.location);
		//dead rails will use a bits location pointing to an arbitrary completed-switch bit to check
		else if (!deadRailCompletedSwitches.empty()) {
			conclusionsData.deadRail.completedSwitches.push_back(
				deadRailCompletedSwitches.back()->connectionSwitch->canKickBit.location);
			deadRailCompletedSwitches.pop_back();
		//in the event that there are fewer completed switches than dead rails, due to multiple rails being used for one switch,
		//	we can just use the always-off bit which will be interpreted as a completed switch
		} else
			conclusionsData.deadRail.completedSwitches.push_back(alwaysOffBitLocation);
	}
	//in the event that there are more completed switches than dead rails, we can add all the remaining switches past the end of
	//	the list
	for (DetailedConnectionSwitch* deadRailCompletedSwitch : deadRailCompletedSwitches)
		conclusionsData.deadRail.completedSwitches.push_back(deadRailCompletedSwitch->connectionSwitch->canKickBit.location);
	//we don't need to clear deadRailCompletedSwitches, it's ok to leave it partially depleted
}

//////////////////////////////// LevelTypes::Plane::Connection ////////////////////////////////
LevelTypes::Plane::Connection::Connection(Plane* pToPlane, RailByteMaskData::BitsLocation pRailBits, int pSteps, Hint& pHint)
: toPlane(pToPlane)
, railBits(pRailBits)
, railTileOffsetByteMask(Level::baseRailTileOffsetByteMask << pRailBits.data.bitShift)
, steps(pSteps)
, hint(pHint) {
}
LevelTypes::Plane::Connection::~Connection() {
	//don't delete toPlane, it's owned by a Level
}

//////////////////////////////// LevelTypes::Plane::DetailedConnection ////////////////////////////////
bool LevelTypes::Plane::DetailedConnection::requiresSwitchesOnPlane(DetailedPlane* destination) {
	//plane connections and rail connections without groups can't require switches, and rails that start raised don't apply
	if (switchRail == nullptr || switchRail->railByteMaskData->rail->getInitialTileOffset() == 0)
		return false;
	//if this rail is affected by any switch outside of the plane, then it doesn't require switches on the plane
	for (DetailedConnectionSwitch* affectingSwitch : switchRail->affectingSwitches) {
		if (affectingSwitch->owningPlane != destination)
			return false;
	}
	//if the rail starts out lowered, and is only affected by switches in the plane, then using this connection requires
	//	reaching the plane first
	return true;
}
void LevelTypes::Plane::DetailedConnection::tryAddMilestoneSwitch(vector<DetailedPlane*>& outDestinationPlanes) {
	//skip plane-plane connections, always-raised rails, and rails affected by more than one switch
	if (switchRail == nullptr || switchRail->affectingSwitches.size() != 1)
		return;

	//all connections at this point are rails with one switch
	//if we already found this switch as a milestone, we don't need to mark or track it again
	DetailedConnectionSwitch* matchingDetailedConnectionSwitch = switchRail->affectingSwitches[0];
	ConnectionSwitch* matchingConnectionSwitch = matchingDetailedConnectionSwitch->connectionSwitch;
	if (matchingConnectionSwitch->isMilestone)
		return;
	//if it's single-use, it's a milestone
	DetailedPlane* matchingDetailedPlane = matchingDetailedConnectionSwitch->owningPlane;
	Plane* matchingPlane = matchingDetailedPlane->plane;
	if (matchingConnectionSwitch->isSingleUse) {
		#ifdef LOG_FOUND_PLANE_CONCLUSIONS
			stringstream newMilestoneMessage;
			newMilestoneMessage << "level " << matchingPlane->owningLevel->getLevelN() << " milestone:";
			MapState::logSwitchDescriptor(matchingConnectionSwitch->hint.data.switch0, &newMilestoneMessage);
			Logger::debugLogger.logString(newMilestoneMessage.str());
		#endif
		matchingConnectionSwitch->isMilestone = true;
		//if we haven't done so already, mark the switch's plane as a milestone destination by having it track a bit for
		//	whether it's been visited or not
		if (matchingPlane->milestoneIsNewBit.location.data.byteIndex == Level::absentRailByteIndex)
			matchingPlane->milestoneIsNewBit = matchingPlane->owningLevel->trackRailByteMaskBits(1);
	}
	//if the rail starts out lowered, track the switch's plane as a destination plane, if it isn't already tracked
	if (switchRail->railByteMaskData->rail->getInitialTileOffset() != 0
		&& !VectorUtils::includes(outDestinationPlanes, matchingDetailedPlane))
	{
		#ifdef LOG_FOUND_PLANE_CONCLUSIONS
			stringstream destinationPlaneMessage;
			destinationPlaneMessage << "level " << matchingPlane->owningLevel->getLevelN()
				<< " destination plane " << matchingPlane->indexInOwningLevel << " with switches:";
			for (ConnectionSwitch& connectionSwitch : matchingPlane->connectionSwitches)
				MapState::logSwitchDescriptor(connectionSwitch.hint.data.switch0, &destinationPlaneMessage);
			Logger::debugLogger.logString(destinationPlaneMessage.str());
		#endif
		outDestinationPlanes.push_back(matchingDetailedPlane);
	}
}

//////////////////////////////// LevelTypes::Plane::DetailedLevel ////////////////////////////////
LevelTypes::Plane::DetailedLevel::DetailedLevel(Level* pLevel, vector<Plane*>& levelPlanes)
: level(pLevel)
, planes(levelPlanes.size())
, rails((size_t)level->getRailByteMaskCount())
, allConnectionSwitches()
, victoryPlane(nullptr) {
	//copy the basic structure
	for (int i = 0; i < (int)levelPlanes.size(); i++) {
		Plane* plane = levelPlanes[i];
		DetailedPlane& detailedPlane = planes[i];
		detailedPlane.plane = plane;
		for (Connection& connection : plane->connections)
			detailedPlane.connections.push_back(
				{ &connection, &detailedPlane, &planes[connection.toPlane->indexInOwningLevel] });
		for (ConnectionSwitch& connectionSwitch : plane->connectionSwitches)
			detailedPlane.connectionSwitches.push_back({ &connectionSwitch, &detailedPlane });
	}
	victoryPlane = &planes[level->getVictoryPlane()->indexInOwningLevel];

	//collect all rails and all connection switches, and remember which rail goes with which switch
	for (DetailedPlane& detailedPlane : planes) {
		for (DetailedConnectionSwitch& detailedConnectionSwitch : detailedPlane.connectionSwitches) {
			for (RailByteMaskData* railByteMaskData : detailedConnectionSwitch.connectionSwitch->affectedRailByteMaskData)
				getDetailedRail(railByteMaskData)->affectingSwitches.push_back(&detailedConnectionSwitch);
			allConnectionSwitches.push_back(&detailedConnectionSwitch);
		}
	}

	//connect all rails to switches
	for (vector<DetailedRail>& byteMaskRails : rails) {
		for (DetailedRail& detailedRail : byteMaskRails) {
			for (DetailedConnectionSwitch* detailedConnectionSwitch : detailedRail.affectingSwitches)
				detailedConnectionSwitch->affectedRails.push_back(&detailedRail);
		}
	}

	//connect all switch-connected rails to connections
	for (DetailedPlane& detailedPlane : planes) {
		for (DetailedConnection& detailedConnection : detailedPlane.connections) {
			if (detailedConnection.connection->railBits.data.byteIndex == Level::absentRailByteIndex)
				continue;
			for (DetailedRail& detailedRail : rails[detailedConnection.connection->railBits.data.byteIndex]) {
				//connect the rail to the connection if they match
				if (detailedRail.railByteMaskData->railBits.id == detailedConnection.connection->railBits.id) {
					detailedConnection.switchRail = &detailedRail;
					break;
				}
			}
		}
	}
}
LevelTypes::Plane::DetailedRail* LevelTypes::Plane::DetailedLevel::getDetailedRail(RailByteMaskData* railByteMaskData) {
	vector<DetailedRail>& byteMaskRails = rails[railByteMaskData->railBits.data.byteIndex];
	for (DetailedRail& detailedRail : byteMaskRails) {
		if (detailedRail.railByteMaskData == railByteMaskData)
			return &detailedRail;
	}
	byteMaskRails.push_back({ railByteMaskData });
	return &byteMaskRails.back();
}
void LevelTypes::Plane::DetailedLevel::findMilestones(RailByteMaskData::ByteMask alwaysOnBit) {
	//the victory plane is always a milestone destination
	victoryPlane->plane->milestoneIsNewBit = alwaysOnBit;

	//recursively find milestones to destination planes, and track the planes for those milestones as destination planes
	vector<DetailedPlane*> destinationPlanes ({ victoryPlane });
	for (int i = 0; i < (int)destinationPlanes.size(); i++) {
		//try to add a milestone switch for each required connection
		for (DetailedConnection* requiredConnection : findRequiredConnectionsToPlane(destinationPlanes[i]))
			requiredConnection->tryAddMilestoneSwitch(destinationPlanes);
	}
}
vector<LevelTypes::Plane::DetailedConnection*> LevelTypes::Plane::DetailedLevel::findRequiredConnectionsToPlane(
	DetailedPlane* destination)
{
	//find any path to this plane
	//assuming this plane can be found in levelPlanes, we know there must be a path to get here from the starting plane, because
	//	that's how we found this plane in the first place
	vector<DetailedPlane*> pathPlanes ({ &planes[0] });
	vector<DetailedConnection*> pathConnections;
	pathWalkToPlane(destination, true, excludeZeroConnections, pathPlanes, pathConnections, alwaysAcceptPath);

	//prep some data about our path
	vector<bool> connectionIsRequired (pathConnections.size(), true);
	vector<bool> seenPlanes (planes.size(), false);
	for (DetailedPlane* detailedPlane : pathPlanes)
		seenPlanes[detailedPlane->plane->indexInOwningLevel] = true;

	//go back and search again to find routes from each plane in the path toward later planes in the path, without going through
	//	any of the connections in the path we already found
	vector<DetailedPlane*> reroutePathPlanes;
	vector<DetailedConnection*> reroutePathConnections;
	for (int i = 0; i < (int)pathConnections.size(); i++) {
		reroutePathPlanes.push_back(pathPlanes[i]);
		reroutePathConnections.push_back(pathConnections[i]);
		auto checkIfRerouteReturnsToOriginalPath = [&pathPlanes, &reroutePathPlanes, &seenPlanes, &connectionIsRequired, i]() {
			DetailedPlane* detailedPlane = reroutePathPlanes.back();
			if (!seenPlanes[detailedPlane->plane->indexInOwningLevel])
				return true;
			//we found a plane on the original path through an alternate route
			//mark every connection between the reroute start and end planes as non-required
			//then, go back one connection
			for (int j = i; pathPlanes[j] != detailedPlane; j++)
				connectionIsRequired[j] = false;
			return false;
		};
		pathWalkToPlane(
			destination,
			true,
			excludeSingleConnection(reroutePathConnections.back()),
			reroutePathPlanes,
			reroutePathConnections,
			checkIfRerouteReturnsToOriginalPath);
	}

	//some rails which we marked as not-required have single-use switches which actually are required, because every other path
	//	to this plane still goes through a rail with that switch
	//find every single-use switch on not-required rails, and one switch at a time, exclude all connections for that switch, and
	//	see if we can still find a path to this plane; if not, then re-mark that rail as required
	vector<unsigned int> switchRailByteMasks ((size_t)level->getRailByteMaskCount(), 0);
	function<bool(DetailedConnection* connection)> excludeSwitchConnections = excludeRailByteMasks(switchRailByteMasks);
	for (int i = 0; i < (int)pathConnections.size(); i++) {
		//skip required connections
		if (connectionIsRequired[i])
			continue;
		//skip plane-plane connections and always-raised rails
		DetailedRail* pathRail = pathConnections[i]->switchRail;
		if (pathRail == nullptr)
			continue;
		//skip rails with switches that aren't single-use
		ConnectionSwitch* matchingConnectionSwitch = pathRail->affectingSwitches[0]->connectionSwitch;
		if (!matchingConnectionSwitch->isSingleUse)
			continue;
		//we found a single-use switch that is not required
		//mark all of its rail connections as excluded, and see if we can find a path to this plane
		VectorUtils::fill(switchRailByteMasks, 0U);
		matchingConnectionSwitch->writeTileOffsetByteMasks(switchRailByteMasks);
		reroutePathPlanes = { &planes[0] };
		reroutePathConnections.clear();
		//if there is not a path to this plane after excluding the switch's connections, then it is a milestone switch
		//mark this rail as required, and we'll handle marking the switch as a milestone in the below loop
		if (!pathWalkToPlane(
				destination, true, excludeSwitchConnections, reroutePathPlanes, reroutePathConnections, alwaysAcceptPath))
			connectionIsRequired[i] = true;
	}

	//remove all non-required connections
	for (int i = (int)pathConnections.size() - 1; i >= 0; i--) {
		if (!connectionIsRequired[i])
			pathConnections.erase(pathConnections.begin() + i);
	}

	return pathConnections;
}
bool LevelTypes::Plane::DetailedLevel::pathWalkToPlane(
	DetailedPlane* destination,
	bool excludeConnectionsFromSwitchesOnDestination,
	function<bool(DetailedConnection* connection)> excludeConnection,
	vector<DetailedPlane*>& inOutPathPlanes,
	vector<DetailedConnection*>& inOutPathConnections,
	function<bool()> checkPath)
{
	int initialPathPlanesCount = (int)inOutPathPlanes.size();
	DetailedPlane* nextPlane = inOutPathPlanes.back();
	vector<bool> seenPlanes (planes.size(), false);
	for (DetailedPlane* plane : inOutPathPlanes)
		seenPlanes[plane->plane->indexInOwningLevel] = true;
	//DFS to search for planes
	while (true) {
		DetailedPlane* lastPlane = nextPlane;
		for (DetailedConnection& detailedConnection : nextPlane->connections) {
			//skip a connection if:
			//- it goes to a plane that we've already seen; we only care about the planes in the path, not the connections
			//- we can't cross it without having already reached this destination plane
			//- it's been excluded
			DetailedPlane* toPlane = detailedConnection.toPlane;
			if (seenPlanes[toPlane->plane->indexInOwningLevel]
					|| (excludeConnectionsFromSwitchesOnDestination && detailedConnection.requiresSwitchesOnPlane(destination))
					|| excludeConnection(&detailedConnection))
				continue;
			//we found a valid connection, track it
			nextPlane = toPlane;
			inOutPathConnections.push_back(&detailedConnection);
			inOutPathPlanes.push_back(nextPlane);
			seenPlanes[toPlane->plane->indexInOwningLevel] = true;
			break;
		}
		//we found a connection and checkPath() accepted it
		if (nextPlane != lastPlane && checkPath()) {
			//if it goes to this plane, we're done
			if (nextPlane == destination)
				return true;
		//either we didn't find a valid connection from the last plane, or we did but checkPath() rejected it; go back to the
		//	previous plane
		} else {
			//if there are have no more planes we can visit after returning to the starting plane, we're done
			if ((int)inOutPathPlanes.size() == initialPathPlanesCount)
				return false;
			inOutPathPlanes.pop_back();
			inOutPathConnections.pop_back();
			nextPlane = inOutPathPlanes.back();
		}
	}
}
bool LevelTypes::Plane::DetailedLevel::excludeRailConnections(DetailedConnection* connection) {
	return connection->switchRail != nullptr;
}
function<bool(LevelTypes::Plane::DetailedConnection* connection)> LevelTypes::Plane::DetailedLevel::excludeSingleConnection(
	DetailedConnection* excludedConnection)
{
	return [excludedConnection](DetailedConnection* detailedConnection) { return detailedConnection == excludedConnection; };
}
function<bool(LevelTypes::Plane::DetailedConnection* connection)> LevelTypes::Plane::DetailedLevel::excludeRailByteMasks(
	vector<unsigned int>& railByteMasks)
{
	return [&railByteMasks](DetailedConnection* detailedConnection) {
		Connection* connection = detailedConnection->connection;
		return connection->railBits.data.byteIndex != Level::absentRailByteIndex
			&& (railByteMasks[connection->railBits.data.byteIndex] & connection->railTileOffsetByteMask) != 0;
	};
}
void LevelTypes::Plane::DetailedLevel::findMiniPuzzles(short alwaysOnBitId) {
	//now find mini puzzles
	//go through the rails from found switches, look at their groups, and collect new switches and their rails
	vector<DetailedConnectionSwitch*> allMiniPuzzleSwitches;
	vector<DetailedConnectionSwitch*> miniPuzzleSwitches;
	vector<DetailedRail*> miniPuzzleRails;
	for (DetailedConnectionSwitch* detailedConnectionSwitch : allConnectionSwitches) {
		//single-use rails will never be part of a mini puzzle, and skip any switches that are already part of a mini puzzle
		if (detailedConnectionSwitch->connectionSwitch->isSingleUse
				|| VectorUtils::includes(allMiniPuzzleSwitches, detailedConnectionSwitch))
			continue;
		miniPuzzleSwitches = { detailedConnectionSwitch };
		miniPuzzleRails = detailedConnectionSwitch->affectedRails;
		for (int miniPuzzleRailsI = 0; miniPuzzleRailsI < (int)miniPuzzleRails.size(); miniPuzzleRailsI++) {
			DetailedRail* detailedRail = miniPuzzleRails[miniPuzzleRailsI];
			//we know no other switch has this rail if there's only 1 group on it
			if (detailedRail->railByteMaskData->rail->getGroups().size() == 1)
				continue;
			//find the switches we haven't added yet, and add them to the list, with their rails if applicable
			for (DetailedConnectionSwitch* miniPuzzleSwitch : detailedRail->affectingSwitches) {
				if (VectorUtils::includes(miniPuzzleSwitches, miniPuzzleSwitch))
					continue;
				//new switch in the mini puzzle, add it to the list and track any of its rails that are new
				miniPuzzleSwitches.push_back(miniPuzzleSwitch);
				for (DetailedRail* otherRail : miniPuzzleSwitch->affectedRails) {
					if (!VectorUtils::includes(miniPuzzleRails, otherRail))
						miniPuzzleRails.push_back(otherRail);
				}
			}
		}

		//if we didn't find any other switches, there's no mini puzzle
		if (miniPuzzleSwitches.size() == 1)
			continue;

		//at this point, we have a mini puzzle
		#ifdef LOG_FOUND_PLANE_CONCLUSIONS
			stringstream miniPuzzleMessage;
			miniPuzzleMessage << "level " << level->getLevelN() << " mini puzzle with switches:";
			for (DetailedConnectionSwitch* miniPuzzleSwitch : miniPuzzleSwitches)
				MapState::logSwitchDescriptor(miniPuzzleSwitch->connectionSwitch->hint.data.switch0, &miniPuzzleMessage);
			Logger::debugLogger.logString(miniPuzzleMessage.str());
		#endif
		//add all the rails to all the switches, and mark switches and planes with a bit for this mini puzzle
		RailByteMaskData::ByteMask miniPuzzleBit = level->trackRailByteMaskBits(1);
		for (DetailedConnectionSwitch* miniPuzzleSwitch : miniPuzzleSwitches) {
			miniPuzzleSwitch->connectionSwitch->setMiniPuzzle(miniPuzzleBit, miniPuzzleRails);
			allMiniPuzzleSwitches.push_back(miniPuzzleSwitch);
			//track canVisitBit for this plane if every switch in the plane is in this mini puzzle
			//if this is true for a plane with more than one switch, it's fine to write it multiple times
			DetailedPlane* owningPlane = miniPuzzleSwitch->owningPlane;
			auto isSwitchInPlane = [owningPlane](DetailedConnectionSwitch* detailedConnectionSwitch) {
				return detailedConnectionSwitch->owningPlane == owningPlane;
			};
			if (VectorUtils::countMatches(miniPuzzleSwitches, isSwitchInPlane) == (int)owningPlane->connectionSwitches.size())
				owningPlane->plane->canVisitBit = miniPuzzleBit;
		}

		tryAddIsolatedArea(miniPuzzleSwitches, miniPuzzleBit, alwaysOnBitId);
	}
}
//TODO: try to find isolated areas without relying on mini puzzles
//this is a graph set partition problem
//split the level planes into an "in" set and an "out" set such that:
//- every switch has:
//	- exclusively rail connections between planes in the "out" set, or
//	- exclusively rail connections that connect to planes in the "in" set
//- there are no connections from the "out" set to the "in" set except for the above rail connections
//- the victory plane and the plane that connects to it can never be allowed in the "in" set
//then, if the "in" set only has single-use switches or the switches for those rails, it's an isolated area
//possible limited implementation:
//- start from a single-use switch, find its required rails, and going down the list, exclude that single rail and see what
//	rails are or aren't reachable
//- if every rail/switch group is either fully reachable or fully not reachable, then this creates a proper "in" set and "out"
//	set, and thus a mini puzzle and an isolated area
void LevelTypes::Plane::DetailedLevel::tryAddIsolatedArea(
	vector<DetailedConnectionSwitch*>& miniPuzzleSwitches, RailByteMaskData::ByteMask miniPuzzleBit, short alwaysOnBitId)
{
	//start by finding all planes that can't be reached without going through part of this mini puzzle
	vector<unsigned int> miniPuzzleRailByteMasks ((size_t)level->getRailByteMaskCount(), 0);
	for (DetailedConnectionSwitch* miniPuzzleSwitch : miniPuzzleSwitches)
		miniPuzzleSwitch->connectionSwitch->writeTileOffsetByteMasks(miniPuzzleRailByteMasks);
	vector<DetailedPlane*> isolatedAreaPlanes;
	findReachablePlanes(excludeRailByteMasks(miniPuzzleRailByteMasks), nullptr, &isolatedAreaPlanes);
	//nothing to do if the mini puzzle isn't required for any planes
	if (isolatedAreaPlanes.empty())
		return;
	//don't allow isolating the victory plane
	if (VectorUtils::includes(isolatedAreaPlanes, victoryPlane))
		return;

	//first, check to see if this is a pass-through mini puzzle
	tryAddPassThroughMiniPuzzle(isolatedAreaPlanes, miniPuzzleSwitches, miniPuzzleBit);

	//go through all the isolated planes, and find the connections that are mutually required by all of them
	vector<DetailedConnection*> requiredConnections = findRequiredConnectionsToPlane(isolatedAreaPlanes[0]);
	for (int i = 1; i < (int)isolatedAreaPlanes.size(); i++) {
		vector<DetailedConnection*> otherRequiredConnections = findRequiredConnectionsToPlane(isolatedAreaPlanes[i]);
		auto notRequiredForOtherSwitch = [&otherRequiredConnections](DetailedConnection* connection) {
			return !VectorUtils::includes(otherRequiredConnections, connection);
		};
		VectorUtils::filterErase(requiredConnections, notRequiredForOtherSwitch);
	}
	//nothing to do if the planes don't share any required connections
	if (requiredConnections.empty())
		return;

	//at this point, we found the last connection that is required by every plane that also requires a mini puzzle rail
	//start the search over, excluding this connection this time
	DetailedConnection* entryConnection = requiredConnections.back();
	isolatedAreaPlanes.clear();
	vector<DetailedPlane*> outsideAreaPlanes;
	findReachablePlanes(excludeSingleConnection(entryConnection), &outsideAreaPlanes, &isolatedAreaPlanes);

	//if any rail in the mini puzzle is reachable, the area is not isolated
	for (DetailedPlane* detailedPlane : outsideAreaPlanes) {
		for (Connection& connection : detailedPlane->plane->connections) {
			if (connection.railBits.data.byteIndex != Level::absentRailByteIndex
					&& (miniPuzzleRailByteMasks[connection.railBits.data.byteIndex] & connection.railTileOffsetByteMask) != 0)
				return;
		}
	}
	//if any rail that is not in the mini puzzle (and isn't the excluded entry connection) is unreachable, the area is not
	//	isolated
	for (DetailedPlane* detailedPlane : isolatedAreaPlanes) {
		for (Connection& connection : detailedPlane->plane->connections) {
			if (connection.railBits.data.byteIndex != Level::absentRailByteIndex
					&& (miniPuzzleRailByteMasks[connection.railBits.data.byteIndex] & connection.railTileOffsetByteMask) == 0
					&& connection.railBits.id != entryConnection->connection->railBits.id)
				return;
		}
	}

	//also make sure that we can get out of each plane in the isolated area without going through any rails
	vector<DetailedPlane*> pathPlanes;
	vector<DetailedConnection*> pathConnections;
	for (DetailedPlane* detailedPlane : isolatedAreaPlanes) {
		pathPlanes = { detailedPlane };
		pathConnections.clear();
		if (!pathWalkToPlane(&planes[0], false, excludeRailConnections, pathPlanes, pathConnections, alwaysAcceptPath))
			return;
	}

	//we now have the plane and rail contents of an isolated area
	//the last step is to ensure that all switches inside are either single-use, or part of the mini puzzle
	vector<DetailedConnectionSwitch*> isolatedAreaSwitches;
	for (DetailedPlane* detailedPlane : isolatedAreaPlanes) {
		for (DetailedConnectionSwitch& detailedConnectionSwitch : detailedPlane->connectionSwitches) {
			if (detailedConnectionSwitch.connectionSwitch->isSingleUse)
				isolatedAreaSwitches.push_back(&detailedConnectionSwitch);
			else if (!VectorUtils::includes(miniPuzzleSwitches, &detailedConnectionSwitch))
				return;
		}
	}
	//nothing to do if there are no switches in this area
	//should never happen with an umodified floor file once the game is released
	if (isolatedAreaSwitches.empty()) {
		stringstream message;
		message << "ERROR: level " << level->getLevelN() << " isolated area with rails";
		for (DetailedConnectionSwitch* miniPuzzleSwitch : miniPuzzleSwitches)
			MapState::logSwitchDescriptor(miniPuzzleSwitch->connectionSwitch->hint.data.switch0, &message);
		message << " was empty";
		Logger::debugLogger.logString(message.str());
		return;
	}

	//we now have a complete isolated area
	#ifdef LOG_FOUND_PLANE_CONCLUSIONS
		stringstream isolatedAreaMessage;
		isolatedAreaMessage << "level " << level->getLevelN() << " isolated area with planes";
		for (DetailedPlane* detailedPlane : isolatedAreaPlanes)
			isolatedAreaMessage << " " << detailedPlane->plane->indexInOwningLevel;
		isolatedAreaMessage << " and switches";
		for (DetailedConnectionSwitch* isolatedAreaSwitch : isolatedAreaSwitches)
			MapState::logSwitchDescriptor(isolatedAreaSwitch->connectionSwitch->hint.data.switch0, &isolatedAreaMessage);
		Logger::debugLogger.logString(isolatedAreaMessage.str());
	#endif
	//assign it to all the switches in the area
	for (DetailedConnectionSwitch* isolatedAreaSwitch : isolatedAreaSwitches)
		isolatedAreaSwitch->connectionSwitch->setIsolatedArea(isolatedAreaSwitches, miniPuzzleBit);
	//and set all always-can-visit planes in the area to be can't-visit after completing the isolated area
	//some of these may already be set to this bit
	for (DetailedPlane* detailedPlane : isolatedAreaPlanes) {
		if (detailedPlane->plane->canVisitBit.location.id == alwaysOnBitId)
			detailedPlane->plane->canVisitBit = miniPuzzleBit;
	}
}
void LevelTypes::Plane::DetailedLevel::findReachablePlanes(
	function<bool(DetailedConnection* connection)> excludeConnection,
	vector<DetailedPlane*>* outReachablePlanes,
	vector<DetailedPlane*>* outUnreachablePlanes)
{
	//find all planes that can or can't be reached with the given excluded connections
	vector<DetailedPlane*> pathPlanes ({ &planes[0] });
	vector<DetailedConnection*> pathConnections;
	vector<bool> reachablePlanes (planes.size(), false);
	reachablePlanes[0] = true;
	auto trackPlaneAndKeepSearching = [&reachablePlanes, &pathPlanes, this]() {
		reachablePlanes[pathPlanes.back()->plane->indexInOwningLevel] = true;
		return pathPlanes.back() != victoryPlane;
	};
	pathWalkToPlane(victoryPlane, false, excludeConnection, pathPlanes, pathConnections, trackPlaneAndKeepSearching);

	//collect the planes that are reachable and unreachable
	for (int i = 0; i < (int)reachablePlanes.size(); i++) {
		vector<DetailedPlane*>* outPlanes = reachablePlanes[i] ? outReachablePlanes : outUnreachablePlanes;
		if (outPlanes != nullptr)
			outPlanes->push_back(&planes[i]);
	}
}
void LevelTypes::Plane::DetailedLevel::tryAddPassThroughMiniPuzzle(
	vector<DetailedPlane*>& isolatedAreaPlanes,
	vector<DetailedConnectionSwitch*>& miniPuzzleSwitches,
	RailByteMaskData::ByteMask miniPuzzleBit)
{
	//check that there are no switches in the area besides mini puzzle switches
	vector<DetailedConnection*> outsideConnections;
	for (DetailedPlane* isolatedAreaPlane : isolatedAreaPlanes) {
		//check that the switches in this plane are all mini puzzle switches
		for (DetailedConnectionSwitch& isolatedAreaSwitch : isolatedAreaPlane->connectionSwitches) {
			if (!VectorUtils::includes(miniPuzzleSwitches, &isolatedAreaSwitch))
				return;
		}
		//track any connections extending to outside planes
		for (DetailedConnection& isolatedAreaConnection : isolatedAreaPlane->connections) {
			if (!VectorUtils::includes(isolatedAreaPlanes, isolatedAreaConnection.toPlane)
					&& isolatedAreaConnection.switchRail != nullptr)
				outsideConnections.push_back(&isolatedAreaConnection);
		}
	}

	//check that there are exactly 2 planes connected to this area by exactly 2 rails, through exactly 1 plane
	if (outsideConnections.size() != 2
			|| outsideConnections[0]->toPlane == outsideConnections[1]->toPlane
			|| outsideConnections[0]->owningPlane != outsideConnections[1]->owningPlane)
		return;

	//at this point, we have a pass-through mini puzzle
	//we don't need to check that we can get out of the planes, because pass-through mini puzzles only get deactivated from
	//	outside the mini puzzle area at a milestone somewhere else
	#ifdef LOG_FOUND_PLANE_CONCLUSIONS
		stringstream passThroughMiniPuzzleMessage;
		passThroughMiniPuzzleMessage << "level " << level->getLevelN() << " pass-through mini puzzle with planes";
		for (DetailedPlane* detailedPlane : isolatedAreaPlanes)
			passThroughMiniPuzzleMessage << " " << detailedPlane->plane->indexInOwningLevel;
		passThroughMiniPuzzleMessage << " and switches";
		for (DetailedConnectionSwitch* miniPuzzleSwitch : miniPuzzleSwitches)
			MapState::logSwitchDescriptor(miniPuzzleSwitch->connectionSwitch->hint.data.switch0, &passThroughMiniPuzzleMessage);
		Logger::debugLogger.logString(passThroughMiniPuzzleMessage.str());
	#endif
	vector<RailByteMaskData*> passThroughRails =
		{ outsideConnections[0]->switchRail->railByteMaskData, outsideConnections[1]->switchRail->railByteMaskData };
	level->trackPassThroughMiniPuzzle(passThroughRails, miniPuzzleBit);
}
void LevelTypes::Plane::DetailedLevel::findDeadRails(RailByteMaskData::BitsLocation alwaysOffBitLocation, short alwaysOnBitId) {
	//when we disable a dead rail, we can't abandon a plane if it has non-single-use switches, or is the victory plane
	auto isAlwaysLivePlane = [this](DetailedPlane* detailedPlane) {
		for (ConnectionSwitch& connectionSwitch : detailedPlane->plane->connectionSwitches) {
			if (!connectionSwitch.isSingleUse)
				return true;
		}
		return detailedPlane == victoryPlane;
	};

	//first step, find every rail that is required by any single-use switch, that only abandons single-use switches, and track
	//	the single-use switches they abandon, as will as the list of all switches that affect them
	vector<DetailedRail*> deadRails;
	vector<vector<DetailedConnectionSwitch*>> allDeadRailCompletedSwitches;
	vector<DetailedConnectionSwitch*> deadRailSwitches;
	for (DetailedConnectionSwitch* completedSwitch : allConnectionSwitches) {
		if (!completedSwitch->connectionSwitch->isSingleUse)
			continue;
		//find all rails that are required for this switch and do not restrict any planes with non-single-use switches (or the
		//	victory plane)
		for (DetailedConnection* requiredConnection : findRequiredConnectionsToPlane(completedSwitch->owningPlane)) {
			//skip plane-plane connections, always-raised rails, and rails controlled by more than one switch
			//rails controlled by more than one switch would have created a mini puzzle, and the switches for it would not be
			//	eligible to be dead rail switches
			DetailedRail* requiredRail = requiredConnection->switchRail;
			if (requiredRail == nullptr || requiredRail->affectingSwitches.size() != 1)
				continue;

			//check that we have a dead rail
			unsigned int deadRailIndex = VectorUtils::indexOf(deadRails, requiredRail);
			if (deadRailIndex == deadRails.size()) {
				//verify that this rail only restricts single-use switches, and does not restrict the victory plane
				vector<DetailedPlane*> deadPlanes;
				findReachablePlanes(excludeSingleConnection(requiredConnection), nullptr, &deadPlanes);
				if (VectorUtils::anyMatch(deadPlanes, isAlwaysLivePlane))
					continue;

				//this is a valid dead rail
				deadRails.push_back(requiredRail);
				allDeadRailCompletedSwitches.push_back(vector<DetailedConnectionSwitch*>());
				//track all the switches that affect it
				for (DetailedConnectionSwitch* deadRailSwitch : requiredRail->affectingSwitches) {
					if (!VectorUtils::includes(deadRailSwitches, deadRailSwitch))
						deadRailSwitches.push_back(deadRailSwitch);
				}
			}
			//track this single-use switch as a cause of a dead rail
			allDeadRailCompletedSwitches[deadRailIndex].push_back(completedSwitch);
		}
	}

	//now go through every switch that affects a dead rail, and mark it as a dead rail switch if all its live rails are only
	//	affected by it
	for (DetailedConnectionSwitch* deadRailSwitch : deadRailSwitches) {
		//skip switches that already have can-kick bits (single-use switches and mini puzzle switches)
		if (deadRailSwitch->connectionSwitch->canKickBit.location.id != alwaysOnBitId)
			continue;

		//now go through every rail on this switch and mark it as a live rail or dead rail, and verify that all live rails
		//	are only affected by it
		//at the same time, collect all the switches we're abandoning
		vector<bool> isDeadRail;
		vector<DetailedConnectionSwitch*> deadRailSwitchCompletedSwitches;
		auto isInvalidLiveRail =
			[&deadRails, &allDeadRailCompletedSwitches, &isDeadRail, &deadRailSwitchCompletedSwitches](DetailedRail* switchRail)
		{
			unsigned int deadRailIndex = VectorUtils::indexOf(deadRails, switchRail);
			if (deadRailIndex == deadRails.size()) {
				isDeadRail.push_back(false);
				return switchRail->affectingSwitches.size() > 1;
			}
			isDeadRail.push_back(true);
			for (DetailedConnectionSwitch* deadRailCompletedSwitch : allDeadRailCompletedSwitches[deadRailIndex]) {
				if (!VectorUtils::includes(deadRailSwitchCompletedSwitches, deadRailCompletedSwitch))
					deadRailSwitchCompletedSwitches.push_back(deadRailCompletedSwitch);
			}
			return false;
		};
		if (VectorUtils::anyMatch(deadRailSwitch->affectedRails, isInvalidLiveRail))
			continue;

		//at this point, we have a valid dead rail switch
		#ifdef LOG_FOUND_PLANE_CONCLUSIONS
			stringstream deadRailMessage;
			deadRailMessage << "level " << level->getLevelN() << " dead rail switch";
			MapState::logSwitchDescriptor(deadRailSwitch->connectionSwitch->hint.data.switch0, &deadRailMessage);
			deadRailMessage << " for completed switches";
			for (DetailedConnectionSwitch* deadRailCompletedSwitch : deadRailSwitchCompletedSwitches)
				MapState::logSwitchDescriptor(deadRailCompletedSwitch->connectionSwitch->hint.data.switch0, &deadRailMessage);
			Logger::debugLogger.logString(deadRailMessage.str());
		#endif
		deadRailSwitch->connectionSwitch->setDeadRail(
			level->trackRailByteMaskBits(1), isDeadRail, deadRailSwitchCompletedSwitches, alwaysOffBitLocation);
		//if this is the only switch in the plane, this bit can also serve as the canVisitBit
		if (deadRailSwitch->owningPlane->connectionSwitches.size() == 1)
			deadRailSwitch->owningPlane->plane->canVisitBit = deadRailSwitch->connectionSwitch->canKickBit;
	}
}

//////////////////////////////// LevelTypes::Plane ////////////////////////////////
LevelTypes::Plane::Plane(objCounterParametersComma() Level* pOwningLevel, int pIndexInOwningLevel)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
owningLevel(pOwningLevel)
, indexInOwningLevel(pIndexInOwningLevel)
, tiles()
, connectionSwitches()
, connections()
, milestoneIsNewBit(Level::absentBits)
, canVisitBit(Level::absentBits)
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
	return (int)connectionSwitches.size() - 1;
}
void LevelTypes::Plane::addPlaneConnection(Plane* toPlane) {
	//add a plane-plane connection to a plane if we don't already have one
	if (!isConnectedByPlanes(toPlane))
		connections.push_back(Connection(toPlane, Level::absentBits.location, 1, Hint(toPlane)));
}
bool LevelTypes::Plane::isConnectedByPlanes(Plane* toPlane) {
	for (Connection& connection : connections) {
		if (connection.toPlane == toPlane && connection.railBits.data.byteIndex == Level::absentRailByteIndex)
			return true;
	}
	return false;
}
void LevelTypes::Plane::addRailConnection(Plane* toPlane, RailByteMaskData* railByteMaskData, Rail* rail) {
	//add the connection to the other plane
	connections.push_back(Connection(toPlane, railByteMaskData->railBits, 1, Hint(rail)));
	//add the reverse connection to this plane if the other plane is in this level
	if (toPlane->owningLevel == owningLevel) {
		toPlane->connections.push_back(connections.back());
		toPlane->connections.back().toPlane = this;
	//the other plane is in a different level, use the victory plane and don't add a reverse connection
	} else
		connections.back().toPlane = owningLevel->getVictoryPlane();
}
void LevelTypes::Plane::addRailConnectionToSwitch(RailByteMaskData* railByteMaskData, int connectionSwitchesIndex) {
	ConnectionSwitch& connectionSwitch = connectionSwitches[connectionSwitchesIndex];
	connectionSwitch.affectedRailByteMaskData.push_back(railByteMaskData);

	//check for single-use
	Rail* rail = railByteMaskData->rail;
	char tileOffset = rail->getInitialTileOffset();
	rail->triggerMovement(rail->getInitialMovementDirection(), &tileOffset);
	if (rail->getGroups().size() > 1 || tileOffset != 0)
		connectionSwitch.isSingleUse = false;
}
#ifdef DEBUG
	void LevelTypes::Plane::validateResetSwitch(ResetSwitch* resetSwitch) {
		for (ConnectionSwitch& connectionSwitch : connectionSwitches) {
			Switch* switch0 = connectionSwitch.hint.data.switch0;
			if (!resetSwitch->hasGroupForColor(switch0->getGroup(), switch0->getColor())) {
				stringstream message;
				message << "ERROR: level " << to_string(owningLevel->getLevelN()) << ": reset switch missing";
				MapState::logSwitchDescriptor(switch0, &message);
				Logger::debugLogger.logString(message.str());
			}
		}
	}
#endif
void LevelTypes::Plane::finalizeBuilding(
	Level* level, vector<Plane*>& levelPlanes, RailByteMaskData::ByteMask alwaysOffBit, RailByteMaskData::ByteMask alwaysOnBit)
{
	//if this level has no victory plane, don't bother adding any hint data since we'll never perform a hint search
	Plane* victoryPlane = level->getVictoryPlane();
	if (victoryPlane == nullptr)
		return;

	DetailedLevel detailedLevel (level, levelPlanes);

	//find milestones and dedicated bits
	detailedLevel.findMilestones(alwaysOnBit);
	for (Plane* plane : levelPlanes)
		plane->assignCanUseBits(alwaysOffBit, alwaysOnBit);
	//we can always visit the victory plane
	victoryPlane->canVisitBit = alwaysOnBit;

	//find optimizations
	detailedLevel.findMiniPuzzles(alwaysOnBit.location.id);
	detailedLevel.findDeadRails(alwaysOffBit.location, alwaysOnBit.location.id);

	//apply extended connections
	for (Plane* plane : levelPlanes)
		plane->extendConnections();
	for (Plane* plane : levelPlanes)
		plane->removeEmptyPlaneConnections(alwaysOffBit.location.id);
}
void LevelTypes::Plane::assignCanUseBits(RailByteMaskData::ByteMask alwaysOffBit, RailByteMaskData::ByteMask alwaysOnBit) {
	//by default, we can always visit a plane with switches, and never visit a plane without switches
	canVisitBit = !connectionSwitches.empty() ? alwaysOnBit : alwaysOffBit;
	bool useMilestoneIsNewBitAsCanKickBit = true;
	for (ConnectionSwitch& connectionSwitch : connectionSwitches) {
		//single-use switches clear canKickBit when all their rails are raised
		if (connectionSwitch.isSingleUse) {
			//for one of the plane's milestones, reuse the plane's milestoneIsNewBit so that we clear it when we clear
			//	canKickBit
			if (connectionSwitch.isMilestone && useMilestoneIsNewBitAsCanKickBit) {
				connectionSwitch.canKickBit = milestoneIsNewBit;
				useMilestoneIsNewBitAsCanKickBit = false;
			//for other milestones and non-milestones, we need a new bit
			} else
				connectionSwitch.canKickBit = owningLevel->trackRailByteMaskBits(1);
			//if this is the only switch in the plane, this bit can also serve as the canVisitBit
			if (connectionSwitches.size() == 1)
				canVisitBit = connectionSwitch.canKickBit;
		//by default, we can always kick non-single-use switches
		} else
			connectionSwitch.canKickBit = alwaysOnBit;
	}
}
void LevelTypes::Plane::extendConnections() {
	//look at the (growing) list of planes reachable from this plane (directly or indirectly), and see if there are any other
	//	planes we can reach from them
	for (int connectionI = 0; connectionI < (int)connections.size(); connectionI++) {
		Connection& connection = connections[connectionI];
		//don't try to extend connections through rail connections
		if (connection.railBits.data.byteIndex != Level::absentRailByteIndex)
			continue;
		//reuse the source hint for any extended hints, this may itself be an extended hint
		//copy the connection hint instead of using a reference because the vector reallocates when it resizes
		Hint hint = connection.hint;
		int steps = connection.steps + 1;
		//look through all the planes this other plane can reach (directly), and add connections to them if we don't already
		//	have them
		for (Connection& otherConnection : connection.toPlane->connections) {
			LevelTypes::Plane* toPlane = otherConnection.toPlane;
			//don't attempt to follow non-direct connections
			if (otherConnection.steps > 1)
				//because all the direct connections come first, we can stop looking for connections from this plane
				break;
			//don't connect to this plane or a plane we're already connected to by planes
			//even if the new connection is a rail connection, all new connections take equal or more steps than previous
			//	connections
			if (toPlane == this || isConnectedByPlanes(toPlane))
				continue;
			//this is a new connection
			//copy it, but with the source connection's hint and the new step count
			connections.push_back(otherConnection);
			connections.back().steps = steps;
			connections.back().hint = hint;
		}
	}
}
void LevelTypes::Plane::removeEmptyPlaneConnections(short alwaysOffBitId) {
	auto isEmptyPlaneConnection = [alwaysOffBitId](Connection& connection) {
		//remove plane-plane and always-raised-rail connections to planes without switches
		return connection.railBits.data.byteIndex == Level::absentRailByteIndex
			&& connection.toPlane->canVisitBit.location.id == alwaysOffBitId;
	};
	VectorUtils::filterErase(connections, isEmptyPlaneConnection);
}
void LevelTypes::Plane::markStatusBitsInDraftState(vector<Plane*>& levelPlanes) {
	//mark switches as can-kick if any of their connections are lowered
	//this will also mark planes as can-visit and milestone-is-new where those bits are set to the same value
	for (Plane* plane : levelPlanes) {
		for (ConnectionSwitch& connectionSwitch : plane->connectionSwitches) {
			if (connectionSwitch.canKickBit.location.id == Level::cachedAlwaysOnBitId)
				continue;
			if (VectorUtils::anyMatch(connectionSwitch.affectedRailByteMaskData, draftRailIsLowered)
					|| (connectionSwitch.conclusionsType == ConnectionSwitch::ConclusionsType::MiniPuzzle
						&& VectorUtils::anyMatch(
							connectionSwitch.conclusionsData.miniPuzzle.otherRailBits, draftRailBitsIsLowered)))
				HintState::PotentialLevelState::draftState.railByteMasks[connectionSwitch.canKickBit.location.data.byteIndex] |=
					connectionSwitch.canKickBit.byteMask;
		}
	}

	//reset any mini puzzle bits for completed isolated areas
	//isolated areas with more than one goal switch will reset the bits once per switch, but that's fine
	for (Plane* plane : levelPlanes) {
		for (ConnectionSwitch& connectionSwitch : plane->connectionSwitches) {
			if (connectionSwitch.conclusionsType != ConnectionSwitch::ConclusionsType::IsolatedArea)
				continue;
			if (!draftBitIsActive(connectionSwitch.canKickBit.location)
					&& !VectorUtils::anyMatch(
						connectionSwitch.conclusionsData.isolatedArea.otherGoalSwitchCanKickBits, draftBitIsActive))
				HintState::PotentialLevelState::draftState.railByteMasks[
						connectionSwitch.conclusionsData.isolatedArea.miniPuzzleBit.location.data.byteIndex] &=
					~connectionSwitch.conclusionsData.isolatedArea.miniPuzzleBit.byteMask;
		}
	}
}
bool LevelTypes::Plane::draftRailBitsIsLowered(RailByteMaskData::BitsLocation railBitsLocation) {
	return ((char)(HintState::PotentialLevelState::draftState.railByteMasks[railBitsLocation.data.byteIndex]
				>> railBitsLocation.data.bitShift)
			& (char)Level::baseRailTileOffsetByteMask)
		!= 0;
}
bool LevelTypes::Plane::draftRailIsLowered(RailByteMaskData* railByteMaskData) {
	return draftRailBitsIsLowered(railByteMaskData->railBits);
}
bool LevelTypes::Plane::draftBitIsActive(RailByteMaskData::BitsLocation bitLocation) {
	return ((char)(HintState::PotentialLevelState::draftState.railByteMasks[bitLocation.data.byteIndex]
				>> bitLocation.data.bitShift)
			& 1)
		!= 0;
}
bool LevelTypes::Plane::markStatusBitsInDraftStateOnMilestone(vector<Plane*>& levelPlanes) {
	bool hasChanges = false;

	//check for dead rails
	for (Plane* plane : levelPlanes) {
		for (ConnectionSwitch& connectionSwitch : plane->connectionSwitches) {
			if (connectionSwitch.conclusionsType != ConnectionSwitch::ConclusionsType::DeadRail)
				continue;
			//skip dead rail switches that are already disabled
			if (!draftBitIsActive(connectionSwitch.canKickBit.location))
				continue;
			bool railsAreDead = true;
			for (int i = 0; i < (int)connectionSwitch.conclusionsData.deadRail.completedSwitches.size(); i++) {
				RailByteMaskData::BitsLocation completedSwitchBit =
					connectionSwitch.conclusionsData.deadRail.completedSwitches[i];
				if (completedSwitchBit.data.byteIndex == Level::absentRailByteIndex
					//if a live rail is lowered, we can't disable this switch
					? draftRailIsLowered(connectionSwitch.affectedRailByteMaskData[i])
					//if a single-use switch has not been completed, we can't disable this switch
					: draftBitIsActive(completedSwitchBit))
				{
					railsAreDead = false;
					break;
				}
			}
			if (railsAreDead) {
				HintState::PotentialLevelState::draftState.railByteMasks[connectionSwitch.canKickBit.location.data.byteIndex] &=
					~connectionSwitch.canKickBit.byteMask;
				hasChanges = true;
			}
		}
	}

	return hasChanges;
}
void LevelTypes::Plane::pursueSolutionToPlanes(HintState::PotentialLevelState* currentState, int basePotentialLevelStateSteps) {
	unsigned int bucket = currentState->railByteMasksHash % Level::PotentialLevelStatesByBucket::bucketSize;
	unsigned int* railByteMasks = currentState->railByteMasks;
	Level::CheckedPlaneData* checkedPlaneData = Level::checkedPlaneDatas + indexInOwningLevel;
	checkedPlaneData->steps = 0;
	checkedPlaneData->checkPlanesIndex = 0;
	Level::allCheckPlanes[0][0] = this;
	Level::checkPlaneCounts[0] = 1;
	Level::checkedPlaneIndices[0] = indexInOwningLevel;
	Level::checkedPlanesCount = 1;
	int maxStepsSeen = 0;
	for (int steps = 0; steps <= maxStepsSeen; steps++) {
		Plane** checkPlanes = Level::allCheckPlanes[steps];
		for (int i = Level::checkPlaneCounts[steps] - 1; i >= 0; i--) {
			Plane* checkPlane = checkPlanes[i];
			for (Connection& connection : checkPlane->connections) {
				//skip it if we can't pass
				if (connection.railBits.data.byteIndex != Level::absentRailByteIndex
						&& (railByteMasks[connection.railBits.data.byteIndex] & connection.railTileOffsetByteMask) != 0)
					continue;

				Plane* connectionToPlane = connection.toPlane;
				int toPlaneIndex = connectionToPlane->indexInOwningLevel;
				int checkedPlaneSteps = (checkedPlaneData = Level::checkedPlaneDatas + toPlaneIndex)->steps;
				int connectionSteps = steps + connection.steps;
				//skip it if it takes equal or more steps than the path we already found
				//unvisited planes have a large number for steps so this will only be true for visited planes
				if (connectionSteps >= checkedPlaneSteps)
					continue;

				//if we haven't seen this plane before, add its index in the list to remember
				if (checkedPlaneSteps == Level::CheckedPlaneData::maxStepsLimit)
					Level::checkedPlaneIndices[Level::checkedPlanesCount++] = toPlaneIndex;
				//if we have seen this plane before, remove it from its spot in further steps
				else {
					int checkPlaneCount = --Level::checkPlaneCounts[checkedPlaneSteps];
					if (checkedPlaneData->checkPlanesIndex < checkPlaneCount) {
						Plane** replacedCheckPlanes = Level::allCheckPlanes[checkedPlaneSteps];
						replacedCheckPlanes[checkedPlaneData->checkPlanesIndex] = replacedCheckPlanes[checkPlaneCount];
					}
				}

				//track its data
				checkedPlaneData->steps = connectionSteps;
				checkedPlaneData->hint =
					//use the hint from the connection for connections from the first plane, otherwise copy the hint that got to
					//	this plane
					steps == 0 ? &connection.hint : Level::checkedPlaneDatas[checkPlane->indexInOwningLevel].hint;
				int checkPlanesIndex = (checkedPlaneData->checkPlanesIndex = Level::checkPlaneCounts[connectionSteps]++);
				Level::allCheckPlanes[connectionSteps][checkPlanesIndex] = connectionToPlane;
				if (connectionSteps > maxStepsSeen)
					maxStepsSeen = connectionSteps;
				//we're done if the state can't be visited
				if ((railByteMasks[connectionToPlane->canVisitBit.location.data.byteIndex]
							& connectionToPlane->canVisitBit.byteMask)
						== 0)
					continue;

				//if it is hasAction, add a state to it
				vector<HintState::PotentialLevelState*>& potentialLevelStates =
					Level::potentialLevelStatesByBucketByPlane[toPlaneIndex].buckets[bucket];
				HintState::PotentialLevelState* nextPotentialLevelState =
					currentState->addNewState(potentialLevelStates, basePotentialLevelStateSteps + connectionSteps);
				if (nextPotentialLevelState == nullptr)
					continue;

				//fill out the new PotentialLevelState
				nextPotentialLevelState->plane = connectionToPlane;
				nextPotentialLevelState->hint = checkedPlaneData->hint;

				//if it goes to a milestone destination plane that we haven't visited yet from this state, try to frontload it
				//	instead of tracking it at its steps
				if (connectionToPlane->milestoneIsNewBit.location.data.byteIndex != Level::absentRailByteIndex
						&& (railByteMasks[connectionToPlane->milestoneIsNewBit.location.data.byteIndex]
								& connectionToPlane->milestoneIsNewBit.byteMask)
							!= 0
						&& Level::frontloadMilestoneDestinationState(nextPotentialLevelState))
					continue;

				//otherwise, track it
				Level::getNextPotentialLevelStatesForSteps(nextPotentialLevelState->steps)->push_back(nextPotentialLevelState);
			}
		}
		Level::checkPlaneCounts[steps] = 0;
	}
	//reset checked plane data
	while (Level::checkedPlanesCount > 0)
		Level::checkedPlaneDatas[Level::checkedPlaneIndices[--Level::checkedPlanesCount]].steps =
			Level::CheckedPlaneData::maxStepsLimit;
}
void LevelTypes::Plane::pursueSolutionAfterSwitches(HintState::PotentialLevelState* currentState) {
	int stepsAfterSwitchKick = currentState->steps + 1;
	unsigned int* railByteMasks = currentState->railByteMasks;
	for (ConnectionSwitch& connectionSwitch : connectionSwitches) {
		//first, check if we can even kick this switch
		if ((railByteMasks[connectionSwitch.canKickBit.location.data.byteIndex] & connectionSwitch.canKickBit.byteMask) == 0)
			continue;

		//reset the draft rail byte masks
		for (int i = HintState::PotentialLevelState::currentRailByteMaskCount - 1; i >= 0; i--)
			HintState::PotentialLevelState::draftState.railByteMasks[i] = railByteMasks[i];
		//then, go through and modify the byte mask for each affected rail
		bool allRailsAreRaised = true;
		for (RailByteMaskData* railByteMaskData : connectionSwitch.affectedRailByteMaskData) {
			RailByteMaskData::BitsLocation::Data railBitsLocation = railByteMaskData->railBits.data;
			unsigned int* railByteMask = &HintState::PotentialLevelState::draftState.railByteMasks[railBitsLocation.byteIndex];
			char shiftedRailState = (char)(*railByteMask >> railBitsLocation.bitShift);
			char movementDirectionBit = shiftedRailState & (char)Level::baseRailMovementDirectionByteMask;
			char tileOffset = shiftedRailState & (char)Level::baseRailTileOffsetByteMask;
			char movementDirection = (movementDirectionBit >> Level::railTileOffsetByteMaskBitCount) * 2 - 1;
			char resultRailState =
				railByteMaskData->rail->triggerMovement(movementDirection, &tileOffset)
						&& railByteMaskData->cachedRailColor != MapState::sawColor
					? tileOffset | (movementDirectionBit ^ Level::baseRailMovementDirectionByteMask)
					: tileOffset | movementDirectionBit;
			allRailsAreRaised = allRailsAreRaised && tileOffset == 0;
			*railByteMask =
				(*railByteMask & railByteMaskData->inverseRailByteMask)
					| ((unsigned int)resultRailState << railBitsLocation.bitShift);
		}
		//also if all rails are now raised, see if we need to flip the canKickBit
		if (allRailsAreRaised && connectionSwitch.canKickBit.location.id != Level::cachedAlwaysOnBitId) {
			switch (connectionSwitch.conclusionsType) {
				case ConnectionSwitch::ConclusionsType::MiniPuzzle:
					for (int i = (int)connectionSwitch.conclusionsData.miniPuzzle.otherRailBits.size(); true; ) {
						//we've looked at all rails and they're all raised, we can flip the canKickBit now
						if (i == 0) {
							HintState::PotentialLevelState::draftState.railByteMasks[
									connectionSwitch.canKickBit.location.data.byteIndex] &=
								~connectionSwitch.canKickBit.byteMask;
							break;
						}
						i--;
						RailByteMaskData::BitsLocation::Data railBitsLocation =
							connectionSwitch.conclusionsData.miniPuzzle.otherRailBits[i].data;
						//this rail is lowered, we can't flip canKickBit
						if (((char)(HintState::PotentialLevelState::draftState.railByteMasks[railBitsLocation.byteIndex]
										>> railBitsLocation.bitShift)
									& (char)Level::baseRailTileOffsetByteMask)
								!= 0)
							break;
					}
					break;
				case ConnectionSwitch::ConclusionsType::IsolatedArea:
					for (int i = (int)connectionSwitch.conclusionsData.isolatedArea.otherGoalSwitchCanKickBits.size(); true; ) {
						//we've looked at all switches and they're all can't-kick, we can flip the miniPuzzleBit now
						if (i == 0) {
							HintState::PotentialLevelState::draftState.railByteMasks[
									connectionSwitch.conclusionsData.isolatedArea.miniPuzzleBit.location.data.byteIndex] &=
								~connectionSwitch.conclusionsData.isolatedArea.miniPuzzleBit.byteMask;
							break;
						}
						i--;
						RailByteMaskData::BitsLocation::Data canKickBitLocation =
							connectionSwitch.conclusionsData.isolatedArea.otherGoalSwitchCanKickBits[i].data;
						//this switch is can-kick, we can't flip miniPuzzleBit
						if (((char)(HintState::PotentialLevelState::draftState.railByteMasks[canKickBitLocation.byteIndex]
										>> canKickBitLocation.bitShift)
									& 1)
								!= 0)
							break;
					}
					//fall through, this is a single-use switch that flips canKickBit
				case ConnectionSwitch::ConclusionsType::None:
				case ConnectionSwitch::ConclusionsType::DeadRail:
				default:
					//this is a single-use switch
					//or, we stumbled upon a DeadRail switch that somehow got all its rails raised
					//in any case, we can definitely flip canKickBit
					HintState::PotentialLevelState::draftState.railByteMasks[
							connectionSwitch.canKickBit.location.data.byteIndex] &=
						~connectionSwitch.canKickBit.byteMask;
					break;
			}
		}
		HintState::PotentialLevelState::draftState.setHash();
		unsigned int bucket =
			HintState::PotentialLevelState::draftState.railByteMasksHash % Level::PotentialLevelStatesByBucket::bucketSize;

		//make sure we haven't seen this state before
		vector<HintState::PotentialLevelState*>& potentialLevelStates =
			Level::potentialLevelStatesByBucketByPlane[indexInOwningLevel].buckets[bucket];
		HintState::PotentialLevelState* nextPotentialLevelState =
			HintState::PotentialLevelState::draftState.addNewState(potentialLevelStates, stepsAfterSwitchKick);
		if (nextPotentialLevelState == nullptr)
			continue;

		//if this was a milestone switch, restart the hint search from here
		if (connectionSwitch.isMilestone) {
			//update the milestone state if needed and check that it's new
			if (owningLevel->markStatusBitsInDraftStateOnMilestone()) {
				nextPotentialLevelState =
					HintState::PotentialLevelState::draftState.addNewState(potentialLevelStates, stepsAfterSwitchKick);
				if (nextPotentialLevelState == nullptr)
					continue;
			}

			Level::pushMilestone(currentState->steps);
			#ifdef LOG_STEPS_AT_EVERY_MILESTONE
				//it's ok to write these twice
				nextPotentialLevelState->priorState = currentState;
				nextPotentialLevelState->plane = this;
				nextPotentialLevelState->hint = &connectionSwitch.hint;

				#ifdef LOG_FOUND_HINT_STEPS
					nextPotentialLevelState->logSteps();
				#endif
				stringstream milestoneMessage;
				milestoneMessage << nextPotentialLevelState->steps << " steps, push milestone";
				MapState::logSwitchDescriptor(connectionSwitch.hint.data.switch0, &milestoneMessage);
				Logger::debugLogger.logString(milestoneMessage.str());
			#endif
		}

		//fill out the new PotentialLevelState
		nextPotentialLevelState->priorState = currentState;
		nextPotentialLevelState->plane = this;
		nextPotentialLevelState->hint = &connectionSwitch.hint;

		//track it, and then afterwards, travel to all planes possible
		Level::getNextPotentialLevelStatesForSteps(stepsAfterSwitchKick)->push_back(nextPotentialLevelState);
		pursueSolutionToPlanes(nextPotentialLevelState, stepsAfterSwitchKick);
	}
}
#ifdef TEST_SOLUTIONS
	HintState::PotentialLevelState* LevelTypes::Plane::findStateAtSwitch(
		vector<HintState::PotentialLevelState*>& states,
		char color,
		const char* switchGroupName,
		Plane** outPlaneWithAllSwitches,
		vector<Plane*>& outSingleSwitchPlanes,
		bool* outSwitchIsMilestone)
	{
		stringstream checkGroupName;
		for (HintState::PotentialLevelState* state : states) {
			Plane* plane = state->plane;
			for (ConnectionSwitch& connectionSwitch : plane->connectionSwitches) {
				Switch* switch0 = connectionSwitch.hint.data.switch0;
				if (switch0->getColor() != color)
					continue;
				MapState::logGroup(switch0->getGroup(), &checkGroupName);
				if (strcmp(checkGroupName.str().c_str(), switchGroupName) != 0) {
					checkGroupName.str(string());
					continue;
				}
				//we found a matching switch, write the original plane to outPlaneWithAllSwitches, clone it to contain only that
				//	single switch, and then return the state at that plane before kicking it, with the cloned plane
				*outPlaneWithAllSwitches = state->plane;
				state->plane = newPlane(plane->owningLevel, plane->indexInOwningLevel);
				state->plane->connectionSwitches.push_back(connectionSwitch);
				state->plane->connections = plane->connections;
				outSingleSwitchPlanes.push_back(state->plane);
				*outSwitchIsMilestone = connectionSwitch.isMilestone;
				return state;
			}
		}
		return nullptr;
	}
#endif
void LevelTypes::Plane::getHintRenderBounds(int* outLeftWorldX, int* outTopWorldY, int* outRightWorldX, int* outBottomWorldY) {
	*outLeftWorldX = renderLeftTileX * MapState::tileSize;
	*outTopWorldY = renderTopTileY * MapState::tileSize;
	*outRightWorldX = renderRightTileX * MapState::tileSize;
	*outBottomWorldY = renderBottomTileY * MapState::tileSize;
}
void LevelTypes::Plane::renderHint(int screenLeftWorldX, int screenTopWorldY, float alpha) {
	SpriteSheet::setRectangleColor(1.0f, 1.0f, 1.0f, alpha);
	for (Tile& tile : tiles) {
		GLint leftX = (GLint)(tile.x * MapState::tileSize - screenLeftWorldX);
		GLint topY = (GLint)(tile.y * MapState::tileSize - screenTopWorldY);
		SpriteSheet::renderPreColoredRectangle(
			leftX, topY, leftX + (GLint)MapState::tileSize, topY + (GLint)MapState::tileSize);
	}
	SpriteSheet::setRectangleColor(1.0f, 1.0f, 1.0f, 1.0f);
}
#ifdef RENDER_PLANE_IDS
	void LevelTypes::Plane::renderId(int screenLeftWorldX, int screenTopWorldY)  {
		string id = to_string(indexInOwningLevel);
		Text::Metrics metrics = Text::getMetrics(id.c_str(), 1.0f);
		Tile& tile = tiles.front();
		float leftX = (float)(tile.x * MapState::tileSize - screenLeftWorldX);
		float topY = (float)(tile.y * MapState::tileSize - screenTopWorldY + metrics.aboveBaseline);
		Text::render(id.c_str(), leftX, topY, 1.0f);
	}
#endif
void LevelTypes::Plane::countSwitchesAndConnections(
	int outSwitchCounts[4],
	int* outSingleUseSwitches,
	int* outMilestoneSwitches,
	int* outDirectPlaneConnections,
	int* outDirectRailConnections,
	int* outExtendedPlaneConnections,
	int* outExtendedRailConnections)
{
	for (ConnectionSwitch& connectionSwitch : connectionSwitches) {
		outSwitchCounts[connectionSwitch.hint.data.switch0->getColor()]++;
		if (connectionSwitch.isSingleUse)
			(*outSingleUseSwitches)++;
		if (connectionSwitch.isMilestone)
			(*outMilestoneSwitches)++;
	}
	for (Connection& connection : connections) {
		if (connection.steps == 1) {
			if (connection.railBits.data.byteIndex == Level::absentRailByteIndex)
				(*outDirectPlaneConnections)++;
			else
				(*outDirectRailConnections)++;
		} else {
			if (connection.railBits.data.byteIndex == Level::absentRailByteIndex)
				(*outExtendedPlaneConnections)++;
			else
				(*outExtendedRailConnections)++;
		}
	}
}
#ifdef LOG_FOUND_HINT_STEPS
	bool LevelTypes::Plane::isMilestoneSwitchHint(Hint* hint) {
		if (hint->type != Hint::Type::Switch)
			return false;
		for (ConnectionSwitch& connectionSwitch : connectionSwitches) {
			if (&connectionSwitch.hint != hint)
				continue;
			return connectionSwitch.isMilestone;
		}
		return false;
	}
#endif
using namespace LevelTypes;

//////////////////////////////// Level::PotentialLevelStatesByBucket ////////////////////////////////
Level::PotentialLevelStatesByBucket::PotentialLevelStatesByBucket()
: buckets() {
}
Level::PotentialLevelStatesByBucket::~PotentialLevelStatesByBucket() {}

//////////////////////////////// Level::CheckedPlaneData ////////////////////////////////
Level::CheckedPlaneData::CheckedPlaneData()
: steps(maxStepsLimit)
, checkPlanesIndex(-1)
, hint(nullptr) {
}
Level::CheckedPlaneData::~CheckedPlaneData() {
	//don't delete hint, it's owned by something else
}

//////////////////////////////// Level::PassThroughMiniPuzzle ////////////////////////////////
Level::PassThroughMiniPuzzle::PassThroughMiniPuzzle(
	vector<RailByteMaskData*>& pPassThroughRails, RailByteMaskData::ByteMask pMiniPuzzleBit)
: passThroughRails(pPassThroughRails)
, miniPuzzleBit(pMiniPuzzleBit) {
}
Level::PassThroughMiniPuzzle::~PassThroughMiniPuzzle() {}

//////////////////////////////// Level ////////////////////////////////
RailByteMaskData::ByteMask Level::absentBits (RailByteMaskData::BitsLocation(absentRailByteIndex, 0), 0);
bool Level::hintSearchIsRunning = false;
vector<Level::PotentialLevelStatesByBucket> Level::potentialLevelStatesByBucketByPlane;
vector<HintState::PotentialLevelState*> Level::replacedPotentialLevelStates;
short Level::cachedAlwaysOnBitId = Level::absentBits.location.id;
Plane*** Level::allCheckPlanes = nullptr;
int* Level::checkPlaneCounts = nullptr;
Level::CheckedPlaneData* Level::checkedPlaneDatas = nullptr;
int* Level::checkedPlaneIndices = nullptr;
int Level::checkedPlanesCount = 0;
int Level::currentPotentialLevelStateSteps = 0;
vector<int> Level::currentPotentialLevelStateStepsForMilestones;
int Level::maxPotentialLevelStateSteps = -1;
vector<int> Level::maxPotentialLevelStateStepsForMilestones;
int Level::currentMilestones = 0;
deque<HintState::PotentialLevelState*>* Level::currentNextPotentialLevelStates = nullptr;
vector<deque<HintState::PotentialLevelState*>*>* Level::currentNextPotentialLevelStatesBySteps = nullptr;
vector<vector<deque<HintState::PotentialLevelState*>*>> Level::nextPotentialLevelStatesByStepsByMilestone;
int Level::hintSearchCheckStateI = 0;
Plane* Level::cachedHintSearchVictoryPlane = nullptr;
bool Level::enableHintSearchTimeout = true;
#ifdef LOG_SEARCH_STEPS_STATS
	int* Level::statesAtStepsByPlane = nullptr;
	int* Level::statesAtStepsFromPlane = nullptr;
#endif
#ifdef TRACK_HINT_SEARCH_STATS
	int Level::hintSearchActionsChecked = 0;
	int Level::hintSearchComparisonsPerformed = 0;
#endif
int Level::foundHintSearchTotalHintSteps = 0;
int Level::foundHintSearchTotalSteps = 0;
Level::Level(objCounterParametersComma() int pLevelN, int pStartTile)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
levelN(pLevelN)
, startTile(pStartTile)
, planes()
, allRailByteMaskData()
, alwaysOffBit(absentBits)
, alwaysOnBit(absentBits)
, railByteMaskBitsTracked(0)
, victoryPlane(nullptr)
, allPassThroughMiniPuzzles()
, minimumRailColor(0)
, radioTowerHint(Hint::Type::None)
, undoResetHint(Hint::Type::UndoReset)
, searchCanceledEarlyHint(Hint::Type::SearchCanceledEarly) {
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
}
void Level::assignRadioTowerSwitch(Switch* radioTowerSwitch) {
	radioTowerHint = Hint(radioTowerSwitch);
}
void Level::assignResetSwitch(ResetSwitch* resetSwitch) {
	undoResetHint.data.resetSwitch = resetSwitch;
	searchCanceledEarlyHint.data.resetSwitch = resetSwitch;
}
int Level::trackNextRail(short railId, Rail* rail) {
	minimumRailColor = MathUtils::max(rail->getColor(), minimumRailColor);
	allRailByteMaskData.push_back(
		//only rails with groups need a byte mask
		RailByteMaskData(rail, railId, rail->getGroups().empty() ? absentBits : trackRailByteMaskBits(railByteMaskBitCount)));
	return (int)allRailByteMaskData.size() - 1;
}
RailByteMaskData::ByteMask Level::trackRailByteMaskBits(int nBits) {
	char byteIndex = (char)(railByteMaskBitsTracked / 32);
	char bitShift = (char)(railByteMaskBitsTracked % 32);
	//make sure there are enough bits to fit the new mask
	if (bitShift + nBits > 32) {
		byteIndex++;
		bitShift = 0;
		railByteMaskBitsTracked = (railByteMaskBitsTracked / 32 + 1) * 32 + nBits;
	} else
		railByteMaskBitsTracked += nBits;
	return RailByteMaskData::ByteMask(RailByteMaskData::BitsLocation(byteIndex, bitShift), nBits);
}
void Level::finalizeBuilding() {
	#ifdef DEBUG
		validateResetSwitch();
	#endif

	alwaysOffBit = trackRailByteMaskBits(1);
	alwaysOnBit = trackRailByteMaskBits(1);
	#ifdef RENDER_PLANE_IDS
		if (planes.size() >= 4) {
			sort(planes.begin() + 1, planes.end() - 1, Plane::startTilesAreAscending);
			for (int i = 1; i < (int)planes.size() - 1; i++)
				planes[i]->setIndexInOwningLevel(i);
		}
	#endif
	Plane::finalizeBuilding(this, planes, alwaysOffBit, alwaysOnBit);
}
void Level::setupHintSearchHelpers(vector<Level*>& allLevels) {
	for (Level* level : allLevels) {
		//add one PotentialLevelStatesByBucket per plane, which includes the victory plane
		while (potentialLevelStatesByBucketByPlane.size() < level->planes.size())
			potentialLevelStatesByBucketByPlane.push_back(PotentialLevelStatesByBucket());
		HintState::PotentialLevelState::maxRailByteMaskCount =
			MathUtils::max(HintState::PotentialLevelState::maxRailByteMaskCount, level->getRailByteMaskCount());
	}
	nextPotentialLevelStatesByStepsByMilestone.push_back(vector<deque<HintState::PotentialLevelState*>*>());
	//just once, fix the draft state byte list
	delete[] HintState::PotentialLevelState::draftState.railByteMasks;
	HintState::PotentialLevelState::draftState.railByteMasks =
		new unsigned int[HintState::PotentialLevelState::maxRailByteMaskCount];
	//setup plane-search helpers
	int maxPlaneCount = (int)potentialLevelStatesByBucketByPlane.size();
	//for checkPlanes, it's impossible for a path to take more than planes-count steps, so use that as the size of the array
	allCheckPlanes = new Plane**[maxPlaneCount];
	for (int i = 0; i < maxPlaneCount; i++)
		allCheckPlanes[i] = new Plane*[maxPlaneCount];
	checkPlaneCounts = new int[maxPlaneCount] {};
	checkedPlaneDatas = new CheckedPlaneData[maxPlaneCount];
	checkedPlaneIndices = new int[maxPlaneCount];
	#ifdef LOG_SEARCH_STEPS_STATS
		statesAtStepsByPlane = new int[maxPlaneCount] {};
		statesAtStepsFromPlane = new int[maxPlaneCount] {};
	#endif
}
void Level::deleteHelpers() {
	int maxPlaneCount = (int)potentialLevelStatesByBucketByPlane.size();
	potentialLevelStatesByBucketByPlane.clear();
	for (int i = 0; i < maxPlaneCount; i++)
		delete[] allCheckPlanes[i];
	delete[] allCheckPlanes;
	delete[] checkPlaneCounts;
	delete[] checkedPlaneDatas;
	delete[] checkedPlaneIndices;
	for (vector<deque<HintState::PotentialLevelState*>*>& deleteNextPotentialLevelStatesBySteps
		: nextPotentialLevelStatesByStepsByMilestone)
	{
		for (deque<HintState::PotentialLevelState*>* deleteNextPotentialLevelStates : deleteNextPotentialLevelStatesBySteps)
			delete deleteNextPotentialLevelStates;
		deleteNextPotentialLevelStatesBySteps.clear();
	}
	nextPotentialLevelStatesByStepsByMilestone.clear();
	#ifdef LOG_SEARCH_STEPS_STATS
		delete[] statesAtStepsByPlane;
		delete[] statesAtStepsFromPlane;
	#endif
}
void Level::preAllocatePotentialLevelStates() {
	auto getRailState = [](short railId, Rail* rail, char* outMovementDirection, char* outTileOffset) {
		*outMovementDirection = rail->getInitialMovementDirection();
		*outTileOffset = rail->getInitialTileOffset();
	};
	enableHintSearchTimeout = false;
	generateHint(planes[0], getRailState, minimumRailColor);
	enableHintSearchTimeout = true;
	#ifdef TEST_SOLUTIONS
		testSolutions(getRailState);
	#endif
}
#ifdef DEBUG
	void Level::validateResetSwitch() {
		ResetSwitch* resetSwitch = undoResetHint.data.resetSwitch;
		if (resetSwitch == nullptr) {
			Logger::debugLogger.logString("ERROR: level " + to_string(levelN) + ": missing reset switch");
			return;
		}

		//validate that the reset switch resets every switch found in this level
		for (Plane* plane : planes)
			plane->validateResetSwitch(resetSwitch);

		//validate that the reset switch doesn't reset any switch not found in this level
		for (short resetSwitchRailId : *resetSwitch->getAffectedRailIds()) {
			auto matchesResetSwitchRailId = [resetSwitchRailId](RailByteMaskData& railByteMaskData) {
				return railByteMaskData.railId == resetSwitchRailId;
			};
			if (!VectorUtils::anyMatch(allRailByteMaskData, matchesResetSwitchRailId))
				Logger::debugLogger.logString(
					"ERROR: level " + to_string(levelN) + ": reset switch affects rail " + to_string(resetSwitchRailId)
						+ " outside of level");
		}
	}
#endif
Hint* Level::generateHint(Plane* currentPlane, GetRailState getRailState, char lastActivatedSwitchColor) {
	hintSearchIsRunning = true;
	if (lastActivatedSwitchColor < minimumRailColor)
		return &radioTowerHint;
	else if (victoryPlane == nullptr)
		return &Hint::none;

	//prepare some logging helpers before we start the search
	#ifdef TRACK_HINT_SEARCH_STATS
		hintSearchActionsChecked = 0;
		hintSearchComparisonsPerformed = 0;
	#endif
	Logger::debugLogger.logString("begin level " + to_string(levelN) + " hint search");

	//prepare search helpers and load the base level state
	resetPlaneSearchHelpers();
	HintState::PotentialLevelState* baseLevelState = loadBasePotentialLevelState(currentPlane, getRailState);

	//search for a hint
	int timeBeforeSearch = SDL_GetTicks();
	Hint* result = performHintSearch(baseLevelState, currentPlane, timeBeforeSearch);

	//cleanup
	int timeAfterSearchBeforeCleanup = SDL_GetTicks();
	int totalUniqueStates = clearPotentialLevelStateHolders();

	int timeAfterCleanup = SDL_GetTicks();
	stringstream hintSearchPerformanceMessage;
	hintSearchPerformanceMessage
		<< "level " << levelN << " hint search:"
		<< "  uniqueStates " << totalUniqueStates
		#ifdef TRACK_HINT_SEARCH_STATS
			<< "  actionsChecked " << hintSearchActionsChecked
			<< "  comparisonsPerformed " << hintSearchComparisonsPerformed
		#endif
		<< "  searchTime " << (timeAfterSearchBeforeCleanup - timeBeforeSearch)
		<< "  cleanupTime " << (timeAfterCleanup - timeAfterSearchBeforeCleanup)
		<< "  found solution? " << (result->isAdvancement() ? "true" : "false");
	if (result->isAdvancement())
		hintSearchPerformanceMessage << "  steps " << foundHintSearchTotalSteps << "(" << foundHintSearchTotalHintSteps << ")";
	Logger::debugLogger.logString(hintSearchPerformanceMessage.str());

	hintSearchIsRunning = false;
	return result;
}
void Level::resetPlaneSearchHelpers() {
	cachedAlwaysOnBitId = alwaysOnBit.location.id;
	cachedHintSearchVictoryPlane = victoryPlane;
	currentPotentialLevelStateSteps = 0;
	maxPotentialLevelStateSteps = -1;
	currentMilestones = 0;
	currentNextPotentialLevelStatesBySteps = &nextPotentialLevelStatesByStepsByMilestone[0];
	currentNextPotentialLevelStates = getNextPotentialLevelStatesForSteps(currentPotentialLevelStateSteps);
}
HintState::PotentialLevelState* Level::loadBasePotentialLevelState(Plane* currentPlane, GetRailState getRailState) {
	//setup the draft state to use for the base potential level state
	HintState::PotentialLevelState::currentRailByteMaskCount = getRailByteMaskCount();
	for (int i = 0; i < HintState::PotentialLevelState::currentRailByteMaskCount; i++)
		HintState::PotentialLevelState::draftState.railByteMasks[i] = 0;
	for (RailByteMaskData& railByteMaskData : allRailByteMaskData) {
		char movementDirection, tileOffset;
		getRailState(railByteMaskData.railId, railByteMaskData.rail, &movementDirection, &tileOffset);
		char movementDirectionBit = ((movementDirection + 1) / 2) << Level::railTileOffsetByteMaskBitCount;
		RailByteMaskData::BitsLocation::Data railBitsLocation = railByteMaskData.railBits.data;
		if (railBitsLocation.byteIndex != absentRailByteIndex)
			HintState::PotentialLevelState::draftState.railByteMasks[railBitsLocation.byteIndex] |=
				(unsigned int)(movementDirectionBit | tileOffset) << railBitsLocation.bitShift;
	}
	HintState::PotentialLevelState::draftState.railByteMasks[alwaysOnBit.location.data.byteIndex] |= alwaysOnBit.byteMask;
	Plane::markStatusBitsInDraftState(planes);
	markStatusBitsInDraftStateOnMilestone();
	HintState::PotentialLevelState::draftState.setHash();

	//load the base potential level state
	HintState::PotentialLevelState* baseLevelState = HintState::PotentialLevelState::draftState.addNewState(
		potentialLevelStatesByBucketByPlane[currentPlane->getIndexInOwningLevel()]
			.buckets[HintState::PotentialLevelState::draftState.railByteMasksHash % PotentialLevelStatesByBucket::bucketSize],
		0);
	baseLevelState->priorState = nullptr;
	baseLevelState->plane = currentPlane;
	baseLevelState->hint = nullptr;

	return baseLevelState;
}
Hint* Level::performHintSearch(HintState::PotentialLevelState* baseLevelState, Plane* currentPlane, int startTime) {
	//find all visitable planes reachable from the current plane to start the search, including the current plane if applicable
	currentPlane->pursueSolutionToPlanes(baseLevelState, 0);
	if (currentPlane->hasSwitches())
		//always stick this state at the front, in case it's a milestone state that needed to be frontloaded in front of any
		//	other frontloaded milestone
		//if it's not, it only adds 1 extra state check per hint search
		getNextPotentialLevelStatesForSteps(0)->push_front(baseLevelState);

	//go through all states and see if there's anything we could do to get closer to the victory plane
	static constexpr int targetLoopTicks = 20;
	static int loopMaxStateCount = 1;
	int lastCheckStartTime = startTime;
	#ifdef LOG_SEARCH_STEPS_STATS
		bool loggedCurrentNextPotentialLevelStates = false;
	#endif
	do {
		//find the next queue of states to look through, and if applicable, log it
		currentNextPotentialLevelStates = (*currentNextPotentialLevelStatesBySteps)[currentPotentialLevelStateSteps];
		#ifdef LOG_SEARCH_STEPS_STATS
			if (!loggedCurrentNextPotentialLevelStates) {
				Logger::debugLogger.logString(
					to_string(currentPotentialLevelStateSteps) + " steps, "
						+ to_string(currentNextPotentialLevelStates->size()) + " states:");
				for (HintState::PotentialLevelState* potentialLevelState : *currentNextPotentialLevelStates) {
					statesAtStepsByPlane[potentialLevelState->plane->getIndexInOwningLevel()]++;
					if (potentialLevelState->priorState != nullptr)
						statesAtStepsFromPlane[potentialLevelState->priorState->plane->getIndexInOwningLevel()]++;
				}
				for (int planeI = 0; planeI < (int)planes.size(); planeI++) {
					int statesAtSteps = statesAtStepsByPlane[planeI];
					int statesFromPlane = statesAtStepsFromPlane[planeI];
					if (statesAtSteps > 0 || statesFromPlane > 0) {
						stringstream message;
						message
							<< "  plane " << setw(3) << planeI << ": "
							<< setw(4) << statesAtSteps << " states ("
							<< setw(5) << statesFromPlane << " from)";
						Logger::debugLogger.logString(message.str());
						statesAtStepsByPlane[planeI] = 0;
						statesAtStepsFromPlane[planeI] = 0;
					}
				}
				loggedCurrentNextPotentialLevelStates = true;
			} else
				Logger::debugLogger.logString(
					to_string(currentPotentialLevelStateSteps) + " steps, "
						+ to_string(currentNextPotentialLevelStates->size()) + " states");
		#endif

		//go through some or all of the states in the queue and see if any reach the victory plane
		for (hintSearchCheckStateI = MathUtils::min((int)currentNextPotentialLevelStates->size(), loopMaxStateCount);
			hintSearchCheckStateI > 0;
			hintSearchCheckStateI--)
		{
			HintState::PotentialLevelState* potentialLevelState = currentNextPotentialLevelStates->front();
			currentNextPotentialLevelStates->pop_front();
			//skip any states that were replaced with shorter routes
			if (potentialLevelState->steps == -1)
				continue;
			//if this state is at the victory plane, we're done
			Plane* nextPlane = potentialLevelState->plane;
			if (nextPlane == cachedHintSearchVictoryPlane) {
				#ifdef LOG_FOUND_HINT_STEPS
					potentialLevelState->logSteps();
				#endif
				foundHintSearchTotalSteps = potentialLevelState->steps;
				return potentialLevelState->getHint();
			//otherwise, kick a switch at this plane and advance to other planes
			} else
				nextPlane->pursueSolutionAfterSwitches(potentialLevelState);
		}

		//bail if the search was canceled or took too long
		if (!hintSearchIsRunning) {
			Logger::debugLogger.logString("hint search canceled");
			return &Hint::genericSearchCanceledEarly;
		}
		#ifdef DEBUG
			static constexpr int maxHintSearchTicks = 30000;
		#else
			static constexpr int maxHintSearchTicks = 5000;
		#endif
		int now = SDL_GetTicks();
		if (now - startTime >= maxHintSearchTicks && enableHintSearchTimeout) {
			Logger::debugLogger.logString("hint search timed out");
			return &searchCanceledEarlyHint;
		}

		//the queue is empty, advance to the queue for the next step count
		if (currentNextPotentialLevelStates->empty()) {
			currentPotentialLevelStateSteps++;
			#ifdef LOG_SEARCH_STEPS_STATS
				loggedCurrentNextPotentialLevelStates = false;
			#endif
		//there are still states left in the queue to process, increase or decrease the max state count if needed so that the
		//	search time approaches the target duration
		} else {
			int loopTicks = now - lastCheckStartTime;
			if (loopTicks < targetLoopTicks - 1) {
				loopMaxStateCount = MathUtils::max(
					loopMaxStateCount,
					loopTicks == 0
						? loopMaxStateCount * targetLoopTicks
						: MathUtils::min(loopMaxStateCount * 2, loopMaxStateCount * targetLoopTicks / loopTicks));
				#ifdef LOG_LOOP_MAX_STATE_COUNT_CHANGES
					Logger::debugLogger.logString(string("loopMaxStateCount increased to ") + to_string(loopMaxStateCount));
				#endif
			} else if (loopTicks > targetLoopTicks + 1) {
				loopMaxStateCount = MathUtils::max(1, loopMaxStateCount * targetLoopTicks / loopTicks);
				#ifdef LOG_LOOP_MAX_STATE_COUNT_CHANGES
					Logger::debugLogger.logString(string("loopMaxStateCount decreased to ") + to_string(loopMaxStateCount));
				#endif
			}
		}
		lastCheckStartTime = now;
	} while (currentPotentialLevelStateSteps <= maxPotentialLevelStateSteps || popMilestone());
	//at this point, we've exhausted all states at all steps regardless of milestones, there is no solution
	return &undoResetHint;
}
int Level::clearPotentialLevelStateHolders() {
	do {
		for (int i = currentPotentialLevelStateSteps; i <= maxPotentialLevelStateSteps; i++)
			(*currentNextPotentialLevelStatesBySteps)[i]->clear();
	} while (popMilestone());
	//only clear as many plane buckets as we used
	int totalStates = 0;
	for (int i = 0; i < (int)planes.size(); i++) {
		for (vector<HintState::PotentialLevelState*>& potentialLevelStates : potentialLevelStatesByBucketByPlane[i].buckets) {
			//these were all retained once before by PotentialLevelState::addNewState, so release them here
			for (HintState::PotentialLevelState* potentialLevelState : potentialLevelStates)
				potentialLevelState->release();
			totalStates += (int)potentialLevelStates.size();
			potentialLevelStates.clear();
		}
	}
	//release all PotentialLevelStates that were taken out of potentialLevelStatesByBucketByPlane, but couldn't be released
	//	because they were still in nextPotentialLevelStatesBySteps
	for (HintState::PotentialLevelState* potentialLevelState : replacedPotentialLevelStates)
		potentialLevelState->release();
	replacedPotentialLevelStates.clear();
	return totalStates;
}
#ifdef TEST_SOLUTIONS
	void Level::testSolutions(GetRailState getRailState) {
		if (victoryPlane == nullptr)
			return;
		string filename = "test_solutions/" + to_string(levelN) + ".txt";
		ifstream file;
		FileUtils::openFileForRead(&file, filename.c_str(), FileUtils::FileReadLocation::Installation);
		string line;
		for (int lineN = 1; getline(file, line); lineN++) {
			if (line.empty() || StringUtils::startsWith(line, "#"))
				continue;
			if (line == "start")
				testSolution(getRailState, file, lineN);
			else
				Logger::debugLogger.logString(
					"ERROR: level " + to_string(levelN) + " solution line " + to_string(lineN)
						+ ": expected \"start\" but got \"" + line + "\"");
		}
		file.close();
	}
	void Level::testSolution(GetRailState getRailState, ifstream& file, int& lineN) {
		//start by finding the initial set of planes
		resetPlaneSearchHelpers();
		HintState::PotentialLevelState* baseLevelState = loadBasePotentialLevelState(planes[0], getRailState);
		getNextPotentialLevelStatesForSteps(0)->push_back(baseLevelState);
		baseLevelState->plane->pursueSolutionToPlanes(baseLevelState, 0);

		//go through each step in the file and make sure we can activate that switch
		vector<HintState::PotentialLevelState*> statesAtSolutionStep;
		vector<Plane*> singleSwitchPlanes;
		string line;
		while (getline(file, line)) {
			lineN++;
			if (line.empty() || StringUtils::startsWith(line, "#"))
				continue;
			if (line == "end")
				break;

			//check if it's a milestone
			bool expectMilestoneSwitch = false;
			static constexpr char* milestonePrefix = "milestone: ";
			if (StringUtils::startsWith(line, milestonePrefix)) {
				expectMilestoneSwitch = true;
				line = line.c_str() + StringUtils::strlenConst(milestonePrefix);
			}

			//find switch color and then group
			static constexpr char* switchColorPrefixes[] { "red: ", "blue: ", "green: ", "white: " };
			int color = 0;
			for (; color < MapState::colorCount; color++) {
				if (StringUtils::startsWith(line, switchColorPrefixes[color]))
					break;
			}
			if (color == MapState::colorCount) {
				Logger::debugLogger.logString(
					"ERROR: level " + to_string(levelN) + " solution line " + to_string(lineN)
						+ ": missing color prefix: \"" + line + "\"");
				break;
			}
			const char* switchGroupName = line.c_str() + strlen(switchColorPrefixes[color]);

			//collect all the states and then check that we can reach the specified switch
			for (int i = currentPotentialLevelStateSteps; i <= maxPotentialLevelStateSteps; i++) {
				deque<HintState::PotentialLevelState*>* nextPotentialLevelStates = (*currentNextPotentialLevelStatesBySteps)[i];
				statesAtSolutionStep.insert(
					statesAtSolutionStep.end(), nextPotentialLevelStates->begin(), nextPotentialLevelStates->end());
				nextPotentialLevelStates->clear();
			}
			Plane* planeWithAllSwitches;
			bool switchIsMilestone;
			HintState::PotentialLevelState* stateAtSwitch = Plane::findStateAtSwitch(
				statesAtSolutionStep, color, switchGroupName, &planeWithAllSwitches, singleSwitchPlanes, &switchIsMilestone);
			if (stateAtSwitch == nullptr) {
				Logger::debugLogger.logString(
					"ERROR: level " + to_string(levelN) + " solution line " + to_string(lineN)
						+ ": unable to reach switch, or state has already been seen: \"" + line + "\"");
				break;
			}
			if (switchIsMilestone != expectMilestoneSwitch) {
				Logger::debugLogger.logString(
					"ERROR: level " + to_string(levelN) + " solution line " + to_string(lineN)
						+ ": found " + (switchIsMilestone ? "milestone" : "non-milestone")
						+ ", expected " + (expectMilestoneSwitch ? "milestone" : "non-milestone") + ": \"" + line + "\"");
			}

			//we found the switch, so go to it and kick it and advance to the next step
			currentPotentialLevelStateSteps = stateAtSwitch->steps;
			currentNextPotentialLevelStates = getNextPotentialLevelStatesForSteps(currentPotentialLevelStateSteps);
			stateAtSwitch->plane->pursueSolutionAfterSwitches(stateAtSwitch);
			statesAtSolutionStep.clear();

			//if kicking the switch resulted in a new state, we need to restore the original plane in case it had multiple
			//	switches
			deque<HintState::PotentialLevelState*>* postKickPotentialLevelStates =
				getNextPotentialLevelStatesForSteps(currentPotentialLevelStateSteps + 1);
			if (!postKickPotentialLevelStates->empty())
				postKickPotentialLevelStates->front()->plane = planeWithAllSwitches;
			else {
				Logger::debugLogger.logString(
					"ERROR: level " + to_string(levelN) + " solution line " + to_string(lineN)
						+ ": kicking switch resulted in old state: \"" + line + "\"");
				break;
			}
		}

		if (line != "end")
			Logger::debugLogger.logString(
				"ERROR: level " + to_string(levelN) + " solution: missing \"end\"");
		else if (currentNextPotentialLevelStates->empty() || currentNextPotentialLevelStates->front()->plane != victoryPlane)
			Logger::debugLogger.logString(
				"ERROR: level " + to_string(levelN) + " solution: unable to reach victory plane after all steps");
		else {
			#ifdef LOG_FOUND_HINT_STEPS
				currentNextPotentialLevelStates->front()->logSteps();
			#endif
			foundHintSearchTotalSteps = currentNextPotentialLevelStates->front()->steps;
			currentNextPotentialLevelStates->front()->getHint();
			Logger::debugLogger.logString(
				"level " + to_string(levelN) + " solution verified, "
					+ to_string(foundHintSearchTotalSteps) + "(" + to_string(foundHintSearchTotalHintSteps) + ")" + " steps");
		}
		clearPotentialLevelStateHolders();
		for (Plane* singleSwitchPlane : singleSwitchPlanes)
			delete singleSwitchPlane;
	}
#endif
deque<HintState::PotentialLevelState*>* Level::getNextPotentialLevelStatesForSteps(int nextPotentialLevelStateSteps) {
	while (maxPotentialLevelStateSteps < nextPotentialLevelStateSteps) {
		maxPotentialLevelStateSteps++;
		if ((int)currentNextPotentialLevelStatesBySteps->size() == maxPotentialLevelStateSteps)
			currentNextPotentialLevelStatesBySteps->push_back(new deque<HintState::PotentialLevelState*>());
	}
	return (*currentNextPotentialLevelStatesBySteps)[nextPotentialLevelStateSteps];
}
bool Level::frontloadMilestoneDestinationState(HintState::PotentialLevelState* state) {
	//check to see if we already have a better state frontloaded
	if (!currentNextPotentialLevelStates->empty()) {
		HintState::PotentialLevelState* lastFront = currentNextPotentialLevelStates->front();
		//the other state was not frontloaded, we can add this state
		//lastFront->steps will either be currentPotentialLevelStateSteps, or -1 to say that it was replaced
		if (lastFront->steps <= currentPotentialLevelStateSteps)
			;
		//the other state was frontloaded and it's better than the new state
		else if (lastFront->steps < state->steps)
			return false;
		//the new state is better than the other state, we can add it
		else if (state->steps < lastFront->steps)
			;
		//they take the same number of steps, but for consistency we'll always choose the one with the lower plane index
		else if (lastFront->plane->getIndexInOwningLevel() <= state->plane->getIndexInOwningLevel())
			return false;
	}
	//we expect the given state will not match the step count of the current queue, but that's fine
	currentNextPotentialLevelStates->push_front(state);
	hintSearchCheckStateI++;
	return true;
}
bool Level::markStatusBitsInDraftStateOnMilestone() {
	bool hasChanges = LevelTypes::Plane::markStatusBitsInDraftStateOnMilestone(planes);

	//check for any completed pass-through mini puzzles
	for (PassThroughMiniPuzzle& passThroughMiniPuzzle : allPassThroughMiniPuzzles) {
		if (Plane::draftBitIsActive(passThroughMiniPuzzle.miniPuzzleBit.location)
			&& !VectorUtils::anyMatch(passThroughMiniPuzzle.passThroughRails, Plane::draftRailIsLowered))
		{
			HintState::PotentialLevelState::draftState.railByteMasks[
					passThroughMiniPuzzle.miniPuzzleBit.location.data.byteIndex] &=
				~passThroughMiniPuzzle.miniPuzzleBit.byteMask;
			hasChanges = true;
		}
	}

	return hasChanges;
}
void Level::pushMilestone(int newPotentialLevelStateSteps) {
	#ifdef LOG_SEARCH_STEPS_STATS
		Logger::debugLogger.logString(
			"milestone+ : " + to_string(currentMilestones) + " > " + to_string(currentMilestones + 1));
	#endif
	currentPotentialLevelStateStepsForMilestones.push_back(currentPotentialLevelStateSteps);
	maxPotentialLevelStateStepsForMilestones.push_back(maxPotentialLevelStateSteps);
	currentMilestones++;
	if ((int)nextPotentialLevelStatesByStepsByMilestone.size() == currentMilestones)
		nextPotentialLevelStatesByStepsByMilestone.push_back(vector<deque<HintState::PotentialLevelState*>*>());
	maxPotentialLevelStateSteps = -1;
	currentNextPotentialLevelStatesBySteps = &nextPotentialLevelStatesByStepsByMilestone[currentMilestones];
	//jump to the given number of steps and then update the states queue because pushMilestone() is called in the middle of
	//	iterating it
	currentPotentialLevelStateSteps = newPotentialLevelStateSteps;
	currentNextPotentialLevelStates = getNextPotentialLevelStatesForSteps(currentPotentialLevelStateSteps);
	//set this to 1 since this is called in the middle of pursuing solutions and this is the last state we'll handle at the
	//	current number of steps
	//unless we frontload a destination state, in which case it'll extend the search loop to handle it
	hintSearchCheckStateI = 1;
}
bool Level::popMilestone() {
	#ifdef LOG_SEARCH_STEPS_STATS
		Logger::debugLogger.logString(
			"milestone- : " + to_string(currentMilestones) + " > " + to_string(currentMilestones - 1));
	#endif
	currentMilestones--;
	if (currentMilestones < 0)
		return false;
	currentPotentialLevelStateSteps = currentPotentialLevelStateStepsForMilestones.back();
	currentPotentialLevelStateStepsForMilestones.pop_back();
	maxPotentialLevelStateSteps = maxPotentialLevelStateStepsForMilestones.back();
	maxPotentialLevelStateStepsForMilestones.pop_back();
	currentNextPotentialLevelStatesBySteps = &nextPotentialLevelStatesByStepsByMilestone[currentMilestones];
	//we don't need to set nextPotentialLevelStates here because popMilestone() is only called when it's not being used
	return true;
}
void Level::logStats() {
	int switchCounts[MapState::colorCount] {};
	int singleUseSwitches = 0;
	int milestoneSwitches = 0;
	int directPlaneConnections = 0;
	int directRailConnections = 0;
	int extendedPlaneConnections = 0;
	int extendedRailConnections = 0;
	for (Plane* plane : planes)
		plane->countSwitchesAndConnections(
			switchCounts,
			&singleUseSwitches,
			&milestoneSwitches,
			&directPlaneConnections,
			&directRailConnections,
			&extendedPlaneConnections,
			&extendedRailConnections);
	int totalSwitches = 0;
	for (int switchCount : switchCounts)
		totalSwitches += switchCount;
	int directConnections = directPlaneConnections + directRailConnections;
	int extendedConnections = extendedPlaneConnections + extendedRailConnections;
	stringstream message;
	message
		<< "level " << levelN << " detected: "
		<< planes.size() << " planes, "
		<< allRailByteMaskData.size() << " rails, "
		<< totalSwitches << " switches (";
	for (int i = 0; i < 4; i++)
		message << switchCounts[i] << (i == 3 ? ", " : " ");
	message
		<< singleUseSwitches << " single-use, " << milestoneSwitches << " milestone), "
		<< directConnections << " direct connections ("
		<< directPlaneConnections << " plane, " << directRailConnections << " rail), "
		<< extendedConnections << " extended connections ("
		<< extendedPlaneConnections << " plane, " << extendedRailConnections << " rail)";
	Logger::debugLogger.logString(message.str());
}
