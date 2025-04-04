#include "Level.h"
#include "GameState/MapState/MapState.h"
#include "GameState/MapState/Rail.h"
#include "GameState/MapState/Switch.h"
#include "GameState/MapState/ResetSwitch.h"
#include "Sprites/SpriteSheet.h"
#ifdef TEST_SOLUTIONS
	#include "Util/FileUtils.h"
#endif
#include "Util/Logger.h"
#ifdef TEST_SOLUTIONS
	#include "Util/StringUtils.h"
#endif

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
, hint(Hint::Type::Switch)
, isSingleUse(true)
, isMilestone(false) {
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
void LevelTypes::Plane::addPlaneConnection(Plane* toPlane) {
	//add a plane-plane connection to a plane if we don't already have one
	if (!isConnectedByPlanes(toPlane))
		connections.push_back(Connection(toPlane, Level::absentRailByteIndex, 0, 1, nullptr, nullptr));
}
bool LevelTypes::Plane::isConnectedByPlanes(Plane* toPlane) {
	for (Connection& connection : connections) {
		if (connection.toPlane == toPlane && connection.railByteIndex == Level::absentRailByteIndex)
			return true;
	}
	return false;
}
void LevelTypes::Plane::addRailConnection(Plane* toPlane, RailByteMaskData* railByteMaskData, Rail* rail) {
	if (toPlane->owningLevel != owningLevel)
		toPlane = owningLevel->getVictoryPlane();
	connections.push_back(
		Connection(toPlane, railByteMaskData->railByteIndex, railByteMaskData->getRailTileOffsetByteMask(), 1, rail, nullptr));
}
void LevelTypes::Plane::addReverseRailConnection(Plane* toPlane, Rail* rail) {
	//add a rail connection to a plane that is already connected to this plane by the given rail
	for (Connection& connection : toPlane->connections) {
		if (connection.toPlane != this || connection.hint.type != Hint::Type::Rail || connection.hint.data.rail != rail)
			continue;
		connections.push_back(
			Connection(toPlane, connection.railByteIndex, connection.railTileOffsetByteMask, 1, rail, nullptr));
		break;
	}
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
void LevelTypes::Plane::findMilestones(vector<Plane*>& levelPlanes) {
	Plane* victoryPlane = levelPlanes[0]->owningLevel->getVictoryPlane();
	if (victoryPlane == nullptr)
		return;
	vector<Plane*> destinationPlanes ({ victoryPlane });
	for (int i = 0; i < (int)destinationPlanes.size(); i++)
		destinationPlanes[i]->findMilestonesToThisPlane(levelPlanes, destinationPlanes);
}
void LevelTypes::Plane::findMilestonesToThisPlane(vector<Plane*>& levelPlanes, vector<Plane*>& outDestinationPlanes) {
	Plane* nextPlane = levelPlanes[0];
	//DFS to find a path to the end
	bool* seenPlanes = new bool[levelPlanes.size()] {};
	vector<Plane*> pathPlanes ({ nextPlane });
	vector<Connection*> pathConnections;
	while (true) {
		Plane* lastPlane = nextPlane;
		for (Connection& connection : nextPlane->connections) {
			Plane* toPlane = connection.toPlane;
			if (!seenPlanes[toPlane->indexInOwningLevel]) {
				nextPlane = toPlane;
				pathConnections.push_back(&connection);
				pathPlanes.push_back(nextPlane);
				seenPlanes[toPlane->indexInOwningLevel] = true;
				break;
			}
		}
		if (nextPlane == lastPlane) {
			//since we know that the level has this plane, we can assume there is a path to get here (which is how we found this
			//	plane), which means that there is another plane/connection to try, which means that these lists won't be empty,
			//	and pathPlanes will still not be empty after popping
			pathPlanes.pop_back();
			pathConnections.pop_back();
			nextPlane = pathPlanes.back();
		} else if (nextPlane == this)
			break;
	}

	//prep some data about our path
	bool* requiredConnections = new bool[pathConnections.size()];
	for (int i = 0; i < (int)pathConnections.size(); i++)
		requiredConnections[i] = true;
	for (int i = 0; i < (int)levelPlanes.size(); i++)
		seenPlanes[i] = false;
	for (Plane* plane : pathPlanes)
		seenPlanes[plane->indexInOwningLevel] = true;

	//DFS again to find routes from each plane in the path toward later planes in the path, without going through any of the
	//	connections in the path we already found
	bool* rerouteSeenPlanes = new bool[levelPlanes.size()];
	vector<Plane*> reroutePathPlanes;
	for (int i = 0; i < (int)pathConnections.size(); i++) {
		nextPlane = pathPlanes[i];
		reroutePathPlanes.push_back(nextPlane);
		Connection* nextMainPathConnection = pathConnections[i];
		for (int j = 0; j < (int)levelPlanes.size(); j++)
			rerouteSeenPlanes[j] = false;
		for (Plane* plane : reroutePathPlanes)
			rerouteSeenPlanes[plane->indexInOwningLevel] = true;
		while (true) {
			Plane* lastPlane = nextPlane;
			for (Connection& connection : nextPlane->connections) {
				Plane* toPlane = connection.toPlane;
				if (!rerouteSeenPlanes[toPlane->indexInOwningLevel] && &connection != nextMainPathConnection) {
					nextPlane = toPlane;
					reroutePathPlanes.push_back(nextPlane);
					rerouteSeenPlanes[toPlane->indexInOwningLevel] = true;
					break;
				}
			}
			if (nextPlane == lastPlane) {
				//stop if we've exhausted all paths from the plane for this iteration
				if ((int)reroutePathPlanes.size() == i + 1)
					break;
				reroutePathPlanes.pop_back();
				nextPlane = reroutePathPlanes.back();
			//we found a plane on the original path through an alternate route
			//mark every connection between the reroute start and end planes as non-required
			//then, go back one connection
			} else if (seenPlanes[nextPlane->indexInOwningLevel]) {
				for (int j = i; pathPlanes[j] != nextPlane; j++)
					requiredConnections[j] = false;
				reroutePathPlanes.pop_back();
				nextPlane = reroutePathPlanes.back();
			}
		}
	}

	//now we have a list of which connections are required
	//go through and find their switches
	for (int i = 0; i < (int)pathConnections.size(); i++) {
		//skip non-required connections
		if (!requiredConnections[i])
			continue;
		//skip plane-plane connections
		Connection* railConnection = pathConnections[i];
		if (railConnection->railByteIndex == Level::absentRailByteIndex)
			continue;
		//if we get here, this connection is the only way to get to the next plane, and it is a rail connection
		//look for a switch that only controls this rail
		ConnectionSwitch* matchingConnectionSwitch = nullptr;
		RailByteMaskData* matchingRailByteMaskData = nullptr;
		for (Plane* plane : levelPlanes) {
			//see if any of the switches in this plane match this rail
			for (ConnectionSwitch& connectionSwitch : plane->connectionSwitches) {
				for (RailByteMaskData* railByteMaskData : connectionSwitch.affectedRailByteMaskData) {
					if (railByteMaskData->railByteIndex == railConnection->railByteIndex
						&& railByteMaskData->getRailTileOffsetByteMask() == railConnection->railTileOffsetByteMask)
					{
						matchingConnectionSwitch = &connectionSwitch;
						matchingRailByteMaskData = railByteMaskData;
						break;
					}
				}
				if (matchingConnectionSwitch != nullptr)
					break;
			}
			if (matchingConnectionSwitch == nullptr)
				continue;
			//if we already found this switch as a milestone, we don't need to mark or track it again
			else if (matchingConnectionSwitch->isMilestone)
				break;

			//we found a matching switch for this rail
			//if it's single-use, it's a milestone
			if (matchingConnectionSwitch->isSingleUse)
				matchingConnectionSwitch->isMilestone = true;
			//if this switch is the only switch to control the rail (whether it's single-use or not), and the rail starts out
			//	lowered, track the switch's plane as a destination plane
			Rail* rail = matchingRailByteMaskData->rail;
			if (rail->getGroups().size() == 1 && rail->getInitialTileOffset() != 0) {
				//track its plane if it isn't already tracked
				bool destinationPlaneAlreadyIncluded = false;
				for (Plane* destinationPlane : outDestinationPlanes) {
					if (destinationPlane == plane) {
						destinationPlaneAlreadyIncluded = true;
						break;
					}
				}
				if (!destinationPlaneAlreadyIncluded)
					outDestinationPlanes.push_back(plane);
			}
			break;
		}
	}
	delete[] seenPlanes;
	delete[] requiredConnections;
	delete[] rerouteSeenPlanes;
}
void LevelTypes::Plane::extendConnections() {
	//look at the (growing) list of planes reachable from this plane (directly or indirectly), and see if there are any other
	//	planes we can reach from them
	for (int connectionI = 0; connectionI < (int)connections.size(); connectionI++) {
		Connection& connection = connections[connectionI];
		//don't try to extend connections through rail connections
		if (connection.railByteIndex != Level::absentRailByteIndex)
			continue;
		LevelTypes::Plane* otherPlane = connection.toPlane;
		//since all connections at this point are plane-plane connections, we know the hint type is Plane, and points to the
		//	first plane that approaches the to-plane
		LevelTypes::Plane* hintPlane = connection.hint.data.plane;
		int steps = connection.steps + 1;
		//look through all the planes this other plane can reach (directly), and add connections to them if we don't already
		//	have them
		for (Connection& otherConnection : otherPlane->connections) {
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
			//plane-plane connection
			if (otherConnection.railByteIndex == Level::absentRailByteIndex)
				connections.push_back(Connection(toPlane, Level::absentRailByteIndex, 0, steps, nullptr, hintPlane));
			//rail connection
			else
				connections.push_back(
					Connection(
						toPlane,
						otherConnection.railByteIndex,
						otherConnection.railTileOffsetByteMask,
						steps,
						nullptr,
						hintPlane));
		}
	}
}
void LevelTypes::Plane::removeNonHasActionPlaneConnections() {
	for (auto iter = connections.end(); iter != connections.begin(); ) {
		Connection& connection = *(--iter);
		if (connection.railByteIndex == Level::absentRailByteIndex && !connection.toPlane->hasAction)
			iter = connections.erase(iter);
	}
}
#ifdef DEBUG
	//validate that the reset switch resets all the switches in this plane
	void LevelTypes::Plane::validateResetSwitch(ResetSwitch* resetSwitch) {
		for (ConnectionSwitch& connectionSwitch : connectionSwitches) {
			Switch* switch0 = connectionSwitch.hint.data.switch0;
			if (!resetSwitch->hasGroupForColor(switch0->getGroup(), switch0->getColor())) {
				stringstream message;
				message
					<< "ERROR: level " << to_string(owningLevel->getLevelN())
					<< ": reset switch missing c" << (int)switch0->getColor() << " ";
				MapState::logGroup(switch0->getGroup(), &message);
				Logger::debugLogger.logString(message.str());
			}
		}
	}
#endif
Hint* LevelTypes::Plane::pursueSolutionToPlanes(
	HintState::PotentialLevelState* currentState, int basePotentialLevelStateSteps)
{
	unsigned int bucket = currentState->railByteMasksHash % Level::PotentialLevelStatesByBucket::bucketSize;
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
				if (connection.railByteIndex >= 0
						&& (currentState->railByteMasks[connection.railByteIndex] & connection.railTileOffsetByteMask) != 0)
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
				//we're done if the state isn't hasAction
				if (!connectionToPlane->hasAction)
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

				//if it goes to the victory plane, return the hint for the first transition
				if (connectionToPlane == Level::cachedHintSearchVictoryPlane) {
					#ifdef LOG_FOUND_HINT_STEPS
						logSteps(nextPotentialLevelState);
					#endif
					Level::foundHintSearchTotalSteps = nextPotentialLevelState->steps;
					//reset checked plane data
					while (Level::checkedPlanesCount > 0)
						Level::checkedPlaneDatas[Level::checkedPlaneIndices[--Level::checkedPlanesCount]].steps =
							Level::CheckedPlaneData::maxStepsLimit;
					for (; steps <= maxStepsSeen; steps++)
						Level::checkPlaneCounts[steps] = 0;
					return nextPotentialLevelState->getHint();
				}
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
	return nullptr;
}
Hint* LevelTypes::Plane::pursueSolutionAfterSwitches(HintState::PotentialLevelState* currentState, int stepsAfterSwitchKick) {
	for (ConnectionSwitch& connectionSwitch : connectionSwitches) {
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
		unsigned int bucket =
			HintState::PotentialLevelState::draftState.railByteMasksHash % Level::PotentialLevelStatesByBucket::bucketSize;

		//make sure we haven't seen this state before
		vector<HintState::PotentialLevelState*>& potentialLevelStates =
			Level::potentialLevelStatesByBucketByPlane[indexInOwningLevel].buckets[bucket];
		HintState::PotentialLevelState* nextPotentialLevelState =
			HintState::PotentialLevelState::draftState.addNewState(potentialLevelStates, stepsAfterSwitchKick);
		if (nextPotentialLevelState == nullptr)
			continue;
		//also don't bother with any states that lower the rail(s) of a single-rail switch
		if (connectionSwitch.isSingleUse && lastTileOffset == 0) {
			potentialLevelStates.pop_back();
			nextPotentialLevelState->release();
			continue;
		}

		//if this was a milestone switch, restart the hint search from here
		if (connectionSwitch.isMilestone)
			Level::pushMilestone();

		//fill out the new PotentialLevelState and track it
		nextPotentialLevelState->priorState = currentState;
		nextPotentialLevelState->plane = this;
		nextPotentialLevelState->hint = &connectionSwitch.hint;
		Level::getNextPotentialLevelStatesForSteps(stepsAfterSwitchKick)->push_back(nextPotentialLevelState);

		//then afterwards, travel to all planes possible, and check to see if they yielded a hint
		Hint* result = pursueSolutionToPlanes(nextPotentialLevelState, stepsAfterSwitchKick);
		if (result != nullptr)
			return result;
	}

	return nullptr;
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
				state->plane->hasAction = true;
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
			if (connection.railByteIndex == Level::absentRailByteIndex)
				(*outDirectPlaneConnections)++;
			else
				(*outDirectRailConnections)++;
		} else {
			if (connection.railByteIndex == Level::absentRailByteIndex)
				(*outExtendedPlaneConnections)++;
			else
				(*outExtendedRailConnections)++;
		}
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
int LevelTypes::RailByteMaskData::getRailTileOffsetByteMask() {
	return Level::baseRailTileOffsetByteMask << railBitShift;
}
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

//////////////////////////////// Level ////////////////////////////////
bool Level::hintSearchIsRunning = false;
vector<Level::PotentialLevelStatesByBucket> Level::potentialLevelStatesByBucketByPlane;
vector<HintState::PotentialLevelState*> Level::replacedPotentialLevelStates;
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
#ifdef LOG_SEARCH_STEPS_STATS
	int* Level::statesAtStepsByPlane = nullptr;
	int Level::statesAtStepsByPlaneCount = 0;
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
, railByteMaskBitsTracked(0)
, victoryPlane(nullptr)
, minimumRailColor(0)
, radioTowerHint(Hint::Type::None) {
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
void Level::assignRadioTowerSwitch(Switch* radioTowerSwitch) {
	radioTowerHint.type = Hint::Type::Switch;
	radioTowerHint.data.switch0 = radioTowerSwitch;
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
void Level::findMilestonesAndExtendConnections() {
	Plane::findMilestones(planes);
	for (Plane* plane : planes)
		plane->extendConnections();
	for (Plane* plane : planes)
		plane->removeNonHasActionPlaneConnections();
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
}
void Level::preAllocatePotentialLevelStates() {
	auto getRailState =
		[](short railId, Rail* rail, char* outMovementDirection, char* outTileOffset) {
			*outMovementDirection = rail->getInitialMovementDirection();
			*outTileOffset = rail->getInitialTileOffset();
		};
	generateHint(planes[0], getRailState, minimumRailColor);
	#ifdef TEST_SOLUTIONS
		testSolutions(getRailState);
	#endif
}
#ifdef DEBUG
	void Level::validateResetSwitch(ResetSwitch* resetSwitch) {
		//validate that every switch in this level can be found on the reset switch
		for (Plane* plane : planes)
			plane->validateResetSwitch(resetSwitch);

		//validate that every rail affected by the reset switch belongs in this level
		for (short resetSwitchRailId : *resetSwitch->getAffectedRailIds()) {
			bool foundRailInLevel = false;
			for (RailByteMaskData& railByteMaskData : allRailByteMaskData) {
				if (railByteMaskData.railId == resetSwitchRailId) {
					foundRailInLevel = true;
					break;
				}
			}
			if (!foundRailInLevel)
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
	#ifdef LOG_SEARCH_STEPS_STATS
		statesAtStepsByPlaneCount = planes.size();
		statesAtStepsByPlane = new int[statesAtStepsByPlaneCount] {};
	#endif
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
	#ifdef LOG_SEARCH_STEPS_STATS
		delete[] statesAtStepsByPlane;
	#endif

	hintSearchIsRunning = false;
	return result;
}
void Level::resetPlaneSearchHelpers() {
	cachedHintSearchVictoryPlane = victoryPlane;
	currentPotentialLevelStateSteps = 0;
	maxPotentialLevelStateSteps = -1;
	currentMilestones = 0;
	currentNextPotentialLevelStatesBySteps = &nextPotentialLevelStatesByStepsByMilestone[0];
}
HintState::PotentialLevelState* Level::loadBasePotentialLevelState(LevelTypes::Plane* currentPlane, GetRailState getRailState) {
	//setup the draft state to use for the base potential level state
	HintState::PotentialLevelState::currentRailByteMaskCount = getRailByteMaskCount();
	for (int i = 0; i < HintState::PotentialLevelState::currentRailByteMaskCount; i++)
		HintState::PotentialLevelState::draftState.railByteMasks[i] = 0;
	for (RailByteMaskData& railByteMaskData : allRailByteMaskData) {
		char movementDirection, tileOffset;
		getRailState(railByteMaskData.railId, railByteMaskData.rail, &movementDirection, &tileOffset);
		char movementDirectionBit = ((movementDirection + 1) / 2) << Level::railTileOffsetByteMaskBitCount;
		HintState::PotentialLevelState::draftState.railByteMasks[railByteMaskData.railByteIndex] |=
			(unsigned int)(movementDirectionBit | tileOffset) << railByteMaskData.railBitShift;
	}
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
	//find all hasAction planes reachable from the current plane to start the search
	Hint* result = currentPlane->pursueSolutionToPlanes(baseLevelState, 0);
	//if we can get to the victory plane without kicking any switches, we're done
	if (result != nullptr)
		return result;
	//include the current plane to search if it's hasAction
	if (currentPlane->getHasAction())
		getNextPotentialLevelStatesForSteps(0)->push_back(baseLevelState);

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
				for (HintState::PotentialLevelState* potentialLevelState : *currentNextPotentialLevelStates)
					statesAtStepsByPlane[potentialLevelState->plane->getIndexInOwningLevel()]++;
				for (int planeI = 0; planeI < (int)statesAtStepsByPlaneCount; planeI++) {
					int statesAtSteps = statesAtStepsByPlane[planeI];
					if (statesAtSteps > 0) {
						Logger::debugLogger.logString(
							"  plane " + to_string(planeI) + ": " + to_string(statesAtSteps) + " states");
						statesAtStepsByPlane[planeI] = 0;
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
			result = potentialLevelState->plane->pursueSolutionAfterSwitches(
				potentialLevelState, Level::currentPotentialLevelStateSteps + 1);
			if (result != nullptr)
				return result;
		}

		//bail if the search was canceled or took too long
		if (!hintSearchIsRunning) {
			Logger::debugLogger.logString("hint search canceled");
			return &Hint::searchCanceledEarly;
		}
		#ifdef DEBUG
			static constexpr int maxHintSearchTicks = 30000;
		#else
			static constexpr int maxHintSearchTicks = 5000;
		#endif
		int now = SDL_GetTicks();
		if (now - startTime >= maxHintSearchTicks) {
			Logger::debugLogger.logString("hint search timed out");
			return &Hint::searchCanceledEarly;
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
			if (loopTicks < targetLoopTicks - 1)
				loopMaxStateCount = loopTicks == 0
					? loopMaxStateCount * targetLoopTicks
					: MathUtils::min(loopMaxStateCount * 2, loopMaxStateCount * targetLoopTicks / loopTicks);
			else if (loopTicks > targetLoopTicks + 1)
				loopMaxStateCount = MathUtils::max(1, loopMaxStateCount * targetLoopTicks / loopTicks);
		}
		lastCheckStartTime = now;
	} while (currentPotentialLevelStateSteps <= maxPotentialLevelStateSteps || popMilestone());
	//at this point, we've exhausted all states at all steps regardless of milestones, there is no solution
	return &Hint::undoReset;
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
		Hint* result = nullptr;
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
			} else if (switchIsMilestone != expectMilestoneSwitch) {
				Logger::debugLogger.logString(
					"ERROR: level " + to_string(levelN) + " solution line " + to_string(lineN)
						+ ": found " + (switchIsMilestone ? "milestone" : "non-milestone")
						+ ", expected " + (expectMilestoneSwitch ? "milestone" : "non-milestone") + ": \"" + line + "\"");
				break;
			}

			//we found the switch, so kick it and advance to the next step
			currentPotentialLevelStateSteps = stateAtSwitch->steps;
			result = stateAtSwitch->plane->pursueSolutionAfterSwitches(stateAtSwitch, currentPotentialLevelStateSteps + 1);
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
		else if (result == nullptr)
			Logger::debugLogger.logString(
				"ERROR: level " + to_string(levelN) + " solution: unable to reach victory plane after all steps");
		else
			Logger::debugLogger.logString(
				"level " + to_string(levelN) + " solution verified, "
					+ to_string(foundHintSearchTotalSteps) + "(" + to_string(foundHintSearchTotalHintSteps) + ")" + " steps");
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
void Level::pushMilestone() {
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
	//we need to set these here because pushMilestone() is called in the middle of iterating this queue
	currentNextPotentialLevelStates = getNextPotentialLevelStatesForSteps(currentPotentialLevelStateSteps);
	hintSearchCheckStateI = 0;
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
