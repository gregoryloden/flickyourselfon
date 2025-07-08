#include "GameState/HintState.h"

#define newLevel(levelN, startTile) newWithArgs(Level, levelN, startTile)
#ifdef DEBUG
	#define LOG_FOUND_PLANE_CONCLUSIONS
	#define TRACK_HINT_SEARCH_STATS
	//#define LOG_SEARCH_STEPS_STATS
	//#define LOG_STEPS_AT_EVERY_MILESTONE
	//#define LOG_LOOP_MAX_STATE_COUNT_CHANGES
	//#define RENDER_PLANE_IDS
	#define TEST_SOLUTIONS
#endif

class Level;
class Rail;
class Switch;
class ResetSwitch;

namespace LevelTypes {
	//Should only be allocated within an object, on the stack, or as a static object
	class RailByteMaskData {
	public:
		//Should only be allocated within an object, on the stack, or as a static object
		union BitsLocation {
			//Should only be allocated within an object, on the stack, or as a static object
			struct Data {
				char byteIndex;
				char bitShift;
			};

			Data data;
			unsigned short id;

			BitsLocation(char byteIndex, char bitShift): data({ byteIndex, bitShift }) {}

			bool operator==(const BitsLocation& other) { return id == other.id; }
		};
		//Should only be allocated within an object, on the stack, or as a static object
		struct ByteMask {
			BitsLocation location;
			unsigned int byteMask;

			ByteMask(BitsLocation pLocation, int nBits);
		};

		Rail* rail;
		short railId;
		char cachedRailColor;
		BitsLocation railBits;
		unsigned int inverseRailByteMask;

		RailByteMaskData(Rail* pRail, short pRailId, ByteMask pRailBits);
		virtual ~RailByteMaskData();
	};
	class Plane onlyInDebug(: public ObjCounter) {
	private:
		struct DetailedConnectionSwitch;
		struct DetailedPlane;
		struct DetailedRail;

		//Should only be allocated within an object, on the stack, or as a static object
		class Tile {
		public:
			int x;
			int y;

			Tile(int pX, int pY);
			virtual ~Tile();
		};
		//Should only be allocated within an object, on the stack, or as a static object
		class ConnectionSwitch {
		public:
			enum class ConclusionsType: unsigned char {
				None,
				MiniPuzzle,
				DeadRail,
			};
			//Should only be allocated within an object, on the stack, or as a static object
			union ConclusionsData {
				//Should only be allocated within an object, on the stack, or as a static object
				struct MiniPuzzle {
					vector<RailByteMaskData::BitsLocation> otherRailBits;
				};
				//Should only be allocated within an object, on the stack, or as a static object
				struct DeadRail {
					vector<RailByteMaskData::BitsLocation> completedSwitches;
				};

				//make sure the default constructor doesn't construct any of the other members
				bool none;
				MiniPuzzle miniPuzzle;
				DeadRail deadRail;

				ConclusionsData(): none() {}
				~ConclusionsData() {}
			};

			vector<RailByteMaskData*> affectedRailByteMaskData;
			Hint hint;
			RailByteMaskData::ByteMask canKickBit;
			bool isSingleUse;
			bool isMilestone;
			ConclusionsType conclusionsType;
			ConclusionsData conclusionsData;

			ConnectionSwitch(Switch* switch0);
			ConnectionSwitch(const ConnectionSwitch& other);
			virtual ~ConnectionSwitch();

			//destruct whichever ConclusionsData we currently have stored in conclusionsData, based on conclusionsType
			void destructConclusions();
			//go through every affected rail and write its tile offset byte mask to the given byte masks
			void writeTileOffsetByteMasks(vector<unsigned int>& railByteMasks);
			//set this ConnectionSwitch to be part of a mini puzzle
			void setMiniPuzzle(RailByteMaskData::ByteMask miniPuzzleBit, vector<DetailedRail*>& miniPuzzleRails);
			//set this ConnectionSwitch to track dead rails
			void setDeadRail(
				RailByteMaskData::ByteMask deadRailBit,
				vector<bool>& isDeadRail,
				vector<DetailedConnectionSwitch*>& deadRailCompletedSwitches,
				RailByteMaskData::BitsLocation alwaysOffBitLocation);
		};
		//Should only be allocated within an object, on the stack, or as a static object
		class Connection {
		public:
			Plane* toPlane;
			RailByteMaskData::BitsLocation railBits;
			unsigned int railTileOffsetByteMask;
			int steps;
			Hint hint;

			Connection(Plane* pToPlane, RailByteMaskData::BitsLocation pRailBits, int pSteps, Hint& pHint);
			virtual ~Connection();
		};
		//Should only be allocated within an object, on the stack, or as a static object
		struct DetailedConnectionSwitch {
			ConnectionSwitch* connectionSwitch;
			DetailedPlane* owningPlane;
			vector<DetailedRail*> affectedRails;
		};
		//Should only be allocated within an object, on the stack, or as a static object
		struct DetailedConnection {
			Connection* connection;
			DetailedPlane* owningPlane;
			DetailedPlane* toPlane;
			DetailedRail* switchRail;

			//returns true if this is a rail connection that starts lowered, and can only be raised by switches on the given
			//	plane
			bool requiresSwitchesOnPlane(DetailedPlane* plane);
			//call this only on required connections
			//if this is a rail connection with a single-use switch, mark the switch as a milestone
			//if this is a rail connection controlled by only one switch (single-use or not), add its plane to the list of
			//	destination planes
			void tryAddMilestoneSwitch(vector<DetailedPlane*>& outDestinationPlanes);
		};
		//Should only be allocated within an object, on the stack, or as a static object
		struct DetailedPlane {
			Plane* plane;
			vector<DetailedConnectionSwitch> connectionSwitches;
			vector<DetailedConnection> connections;
		};
		//Should only be allocated within an object, on the stack, or as a static object
		struct DetailedRail {
			RailByteMaskData* railByteMaskData;
			vector<DetailedConnectionSwitch*> affectingSwitches;
		};
		//Should only be allocated within an object, on the stack, or as a static object
		class DetailedLevel {
		private:
			Level* level;
			vector<DetailedPlane> planes;
			vector<vector<DetailedRail>> rails;
			vector<DetailedConnectionSwitch*> allConnectionSwitches;
			DetailedPlane* victoryPlane;

		public:
			DetailedLevel(Level* pLevel, vector<Plane*>& levelPlanes);
			virtual ~DetailedLevel();

		private:
			//indicates that a path-walk should not exclude any connections
			static bool excludeZeroConnections(DetailedConnection* connection) { return false; }
			//indicates that a path-walk should accept all paths
			static bool alwaysAcceptPath() { return true; }
			//get a DetailedRail for the given RailByteMaskData, creating it if necessary
			//the returned value should not be stored until every rail in the level has been created
			DetailedRail* getDetailedRail(RailByteMaskData* railByteMaskData);
		public:
			//start from the first plane, go through all connections and planes, find planes and rails that are required to get
			//	to the end, see which of them have single-use switches, and mark those switch connections as milestones
			//then recursively repeat the process, instead ending at the planes of those milestone switches
			//must be called before extending connections or removing connections to non-victory planes without switches
			void findMilestones(RailByteMaskData::ByteMask alwaysOnBit);
		private:
			//find all connections that must be crossed in order to get to the given plane from the start plane
			vector<DetailedConnection*> findRequiredConnectionsToPlane(
				DetailedPlane* destination, bool excludeConnectionsFromSwitchesOnDestination);
			//search for paths to every remaining plane until we reach the given plane, without going through any excluded
			//	connections or connections that require access to switches on the given plane
			//assumes there is at least one plane in inOutPathPlanes, and starts the walk from the end of the path described by
			//	inOutPathPlanes, with inOutPathConnections detailing the connections going from the plane at the same index to
			//	the plane at the next index
			//assumes the given plane is not already in inOutPathPlanes
			//calls checkPath() at every step after reaching a new plane, and if checkPath() returns:
			//- false: discards the most recently found plane from the path and continues searching
			//- true: continues searching with the plane in the path, or returns if the path ends at the given plane
			//returns:
			//- true if checkPath() returned true after we reached the given plane
			//	- inOutPathPlanes and inOutPathConnections will contain the path as it existed when that happened
			//- false if we never reached the given plane or checkPath() never returned true after doing so
			//	- inOutPathPlanes and inOutPathConnections will contain their original contents
			bool pathWalkToPlane(
				DetailedPlane* destination,
				bool excludeConnectionsFromSwitchesOnDestination,
				function<bool(DetailedConnection* connection)> excludeConnection,
				vector<DetailedPlane*>& inOutPathPlanes,
				vector<DetailedConnection*>& inOutPathConnections,
				function<bool()> checkPath);
			//indicates that a path-walk should exclude rail connections
			static bool excludeRailConnections(DetailedConnection* connection);
			//indicates that a path-walk should exclude the given connection
			static function<bool(DetailedConnection* connection)> excludeSingleConnection(
				DetailedConnection* excludedConnection);
			//indicates that a path-walk should exclude connections that match the given rail byte masks
			static function<bool(DetailedConnection* connection)> excludeRailByteMasks(vector<unsigned int>& railByteMasks);
		public:
			//find sets of 2 or more switches that have rails in common
			//must be called after setting default bits and before extending connections or removing connections to non-victory
			//	planes without switches
			void findMiniPuzzles(short alwaysOnBitId);
		private:
			//find all planes that are reachable or unreachable when certain connections are excluded, and write them to
			//	outReachablePlanes or outUnreachablePlanes respectively, if provided
			void findReachablePlanes(
				function<bool(DetailedConnection* connection)> excludeConnection,
				vector<DetailedPlane*>* outReachablePlanes,
				vector<DetailedPlane*>* outUnreachablePlanes);
			//see if the given mini puzzle is part of an area with a single entrance and exit, and if so, track it
			void tryAddPassThroughMiniPuzzle(
				vector<DetailedPlane*>& isolatedAreaPlanes,
				vector<DetailedConnectionSwitch*>& miniPuzzleSwitches,
				RailByteMaskData::ByteMask miniPuzzleBit);
		public:
			//find switches with exclusive control over their rails with rails that are only used to get to milestones
			void findDeadRails(RailByteMaskData::BitsLocation alwaysOffBitLocation, short alwaysOnBitId);
			//find areas that are not reachable by rail outside the area
			void findIsolatedAreas(short alwaysOnBitId);
		private:
			//follow the given connection and see if it leads to a valid isolated area
			void tryAddIsolatedAreaThroughConnection(
				DetailedConnection* baseRailConnection,
				vector<DetailedPlane*>& basePlanes,
				vector<bool>& seenPlanes,
				vector<DetailedConnection*>& ignorePathConnections,
				short alwaysOnBitId);
		};

		Level* owningLevel;
		int indexInOwningLevel;
		vector<Tile> tiles;
		vector<ConnectionSwitch> connectionSwitches;
		vector<Connection> connections;
		RailByteMaskData::ByteMask milestoneIsNewBit;
		RailByteMaskData::ByteMask canVisitBit;
		int renderLeftTileX;
		int renderTopTileY;
		int renderRightTileX;
		int renderBottomTileY;

	public:
		Plane(objCounterParametersComma() Level* pOwningLevel, int pIndexInOwningLevel);
		virtual ~Plane();

		Level* getOwningLevel() { return owningLevel; }
		int getIndexInOwningLevel() { return indexInOwningLevel; }
		bool hasSwitches() { return !connectionSwitches.empty(); }
		#ifdef RENDER_PLANE_IDS
			void setIndexInOwningLevel(int pIndexInOwningLevel) { indexInOwningLevel = pIndexInOwningLevel; }
			static bool startTilesAreAscending(Plane* a, Plane* b) {
				return a->tiles[0].y < b->tiles[0].y || (a->tiles[0].y == b->tiles[0].y && a->tiles[0].x < b->tiles[0].x);
			}
		#endif
		//add a tile
		void addTile(int x, int y);
		//add a switch
		//returns the index of the switch in this plane
		int addConnectionSwitch(Switch* switch0);
		//add a direct plane-plane connection to another plane, if we don't already have a plane-plane connection
		void addPlaneConnection(Plane* toPlane);
	private:
		//check if this plane has any direct or extended plane-plane connection to the given plane
		bool isConnectedByPlanes(Plane* toPlane);
	public:
		//add a direct rail connection to another plane
		void addRailConnection(Plane* toPlane, RailByteMaskData* railByteMaskData, Rail* rail);
		//add the data of a rail connection to the switch for the given index
		void addRailConnectionToSwitch(RailByteMaskData* railByteMaskData, int connectionSwitchesIndex);
		#ifdef DEBUG
			//validate that the reset switch resets every switch in this plane
			void validateResetSwitch(ResetSwitch* resetSwitch);
		#endif
		//finish setup of the level planes, analyze them and store any helpful conclusions we find on them, and optimize their
		//	connections for faster traversal through the level in hint searches
		static void finalizeBuilding(
			Level* level,
			vector<Plane*>& levelPlanes,
			RailByteMaskData::ByteMask alwaysOffBit,
			RailByteMaskData::ByteMask alwaysOnBit);
	private:
		//set can-visit and can-kick bits where applicable on planes and switches respectively
		//must be called after finding milestones
		void assignCanUseBits(RailByteMaskData::ByteMask alwaysOffBit, RailByteMaskData::ByteMask alwaysOnBit);
		//copy and add plane-plane and rail connections from all planes that are reachable through plane-plane connections from
		//	this plane
		void extendConnections();
		//remove plane-plane connections to planes that don't have any switches
		//must be called after assigning can-visit bits
		void removeEmptyPlaneConnections(short alwaysOffBitId);
	public:
		//set bits in the draft state where applicable:
		//- set bits where milestones are new
		//- set bits where switches can be kicked
		static void markStatusBitsInDraftState(vector<Plane*>& levelPlanes);
		//returns whether the rail at the given bits location is lowered in the draft state
		static bool draftRailBitsIsLowered(RailByteMaskData::BitsLocation railBitsLocation);
		//returns whether the given rail is lowered in the draft state
		static bool draftRailIsLowered(RailByteMaskData* railByteMaskData);
		//returns whether the bit at the given location is active in the draft state
		static bool draftBitIsActive(RailByteMaskData::BitsLocation bitLocation);
		//set bits in the draft state where applicable:
		//- clear bits where switches can no longer be kicked
		//returns whether any bits were changed
		static bool markStatusBitsInDraftStateOnMilestone(vector<Plane*>& levelPlanes);
		//follow all possible paths to other planes, adding states at those planes to the current hint search queues
		void pursueSolutionToPlanes(HintState::PotentialLevelState* currentState, int basePotentialLevelStateSteps);
		//kick each switch in this plane, and then pursue solutions from those states
		void pursueSolutionAfterSwitches(HintState::PotentialLevelState* currentState);
		#ifdef TEST_SOLUTIONS
			//go through all the given states and see if one has a plane with a switch matching the given description
			//if there is one, the state will have a clone of the original plane containing only that matching switch, and the
			//	original plane with all switches will be written to outPlaneWithAllSwitches
			//the clone plane is added to outSingleSwitchPlanes; callers are responsible for deleting it
			//returns the state with the single-switch-cloned matching plane if one was found
			static HintState::PotentialLevelState* findStateAtSwitch(
				vector<HintState::PotentialLevelState*>& states,
				char color,
				const char* switchGroupName,
				Plane** outPlaneWithAllSwitches,
				vector<Plane*>& outSingleSwitchPlanes,
				bool* outSwitchIsMilestone);
		#endif
		//get the bounds of the hint to render for this plane
		void getHintRenderBounds(int* outLeftWorldX, int* outTopWorldY, int* outRightWorldX, int* outBottomWorldY);
		//render boxes over every tile in this plane
		void renderHint(int screenLeftWorldX, int screenTopWorldY, float alpha);
		#ifdef RENDER_PLANE_IDS
			//render indexInOwningLevel over the first tile of this plane
			void renderId(int screenLeftWorldX, int screenTopWorldY);
		#endif
		//track the switches in this plane
		void countSwitchesAndConnections(
			int outSwitchCounts[4],
			int* outSingleUseSwitches,
			int* outMilestoneSwitches,
			int* outDirectPlaneConnections,
			int* outDirectRailConnections,
			int* outExtendedPlaneConnections,
			int* outExtendedRailConnections);
		#ifdef LOG_FOUND_HINT_STEPS
			//check if the given hint is for a milestone switch
			bool isMilestoneSwitchHint(Hint* hint);
		#endif
	};
}
class Level onlyInDebug(: public ObjCounter) {
public:
	typedef function<void(short railId, Rail* rail, char* outMovementDirection, char* outTileOffset)> GetRailState;
	//Should only be allocated within an object, on the stack, or as a static object
	class PotentialLevelStatesByBucket {
	public:
		static constexpr int bucketSize = 257;

		vector<HintState::PotentialLevelState*> buckets[bucketSize];

		PotentialLevelStatesByBucket();
		virtual ~PotentialLevelStatesByBucket();
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class CheckedPlaneData {
	public:
		static constexpr int maxStepsLimit = MAXINT32;

		int steps;
		int checkPlanesIndex;
		Hint* hint;

		CheckedPlaneData();
		virtual ~CheckedPlaneData();
	};
private:
	//Should only be allocated within an object, on the stack, or as a static object
	class PassThroughMiniPuzzle {
	public:
		vector<LevelTypes::RailByteMaskData*> passThroughRails;
		LevelTypes::RailByteMaskData::ByteMask miniPuzzleBit;

		PassThroughMiniPuzzle(
			vector<LevelTypes::RailByteMaskData*>& pPassThroughRails, LevelTypes::RailByteMaskData::ByteMask pMiniPuzzleBit);
		virtual ~PassThroughMiniPuzzle();
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class IsolatedArea {
	public:
		vector<LevelTypes::RailByteMaskData::BitsLocation> goalSwitchCanKickBits;
		vector<LevelTypes::RailByteMaskData::BitsLocation> abandonCanUseBits;
		LevelTypes::RailByteMaskData::ByteMask sharedAbandonBit;

		IsolatedArea(
			vector<LevelTypes::RailByteMaskData::BitsLocation>& pGoalSwitchCanKickBits,
			vector<LevelTypes::RailByteMaskData::BitsLocation>& pAbandonCanUseBits,
			LevelTypes::RailByteMaskData::ByteMask pSharedAbandonBit);
		virtual ~IsolatedArea();
	};

public:
	static constexpr char absentRailByteIndex = -1;
	static constexpr int railTileOffsetByteMaskBitCount = 3;
	static constexpr int railMovementDirectionByteMaskBitCount = 1;
	static constexpr int railByteMaskBitCount = railTileOffsetByteMaskBitCount + railMovementDirectionByteMaskBitCount;
	static constexpr unsigned int baseRailTileOffsetByteMask = (1 << railTileOffsetByteMaskBitCount) - 1;
	static constexpr unsigned int baseRailMovementDirectionByteMask =
		((1 << railMovementDirectionByteMaskBitCount) - 1) << railTileOffsetByteMaskBitCount;

	static LevelTypes::RailByteMaskData::ByteMask absentBits;
private:
	static bool hintSearchIsRunning;
public:
	static vector<PotentialLevelStatesByBucket> potentialLevelStatesByBucketByPlane;
	static vector<HintState::PotentialLevelState*> replacedPotentialLevelStates;
	static short cachedAlwaysOnBitId;
	static LevelTypes::Plane*** allCheckPlanes;
	static int* checkPlaneCounts;
	static CheckedPlaneData* checkedPlaneDatas;
	static int* checkedPlaneIndices;
	static int checkedPlanesCount;
private:
	static int currentPotentialLevelStateSteps;
	static vector<int> currentPotentialLevelStateStepsForMilestones;
	static int maxPotentialLevelStateSteps;
	static vector<int> maxPotentialLevelStateStepsForMilestones;
	static int currentMilestones;
	static deque<HintState::PotentialLevelState*>* currentNextPotentialLevelStates;
	static vector<deque<HintState::PotentialLevelState*>*>* currentNextPotentialLevelStatesBySteps;
	static vector<vector<deque<HintState::PotentialLevelState*>*>> nextPotentialLevelStatesByStepsByMilestone;
	static int hintSearchCheckStateI;
	static LevelTypes::Plane* cachedHintSearchVictoryPlane;
	#ifdef LOG_SEARCH_STEPS_STATS
		static int* statesAtStepsByPlane;
		static int* statesAtStepsFromPlane;
	#endif
	static bool enableHintSearchTimeout;
public:
	#ifdef TRACK_HINT_SEARCH_STATS
		static int hintSearchActionsChecked;
		static int hintSearchComparisonsPerformed;
	#endif
	static int foundHintSearchTotalHintSteps;
private:
	static int foundHintSearchTotalSteps;

	int levelN;
	int startTile;
	vector<LevelTypes::Plane*> planes;
	vector<LevelTypes::RailByteMaskData> allRailByteMaskData;
	LevelTypes::RailByteMaskData::ByteMask alwaysOffBit;
	LevelTypes::RailByteMaskData::ByteMask alwaysOnBit;
	int railByteMaskBitsTracked;
	LevelTypes::Plane* victoryPlane;
	vector<PassThroughMiniPuzzle> allPassThroughMiniPuzzles;
	vector<IsolatedArea> allIsolatedAreas;
	char minimumRailColor;
	Hint radioTowerHint;
	Hint undoResetHint;
	Hint searchCanceledEarlyHint;

public:
	Level(objCounterParametersComma() int pLevelN, int pStartTile);
	virtual ~Level();

	int getLevelN() { return levelN; }
	int getStartTile() { return startTile; }
	LevelTypes::RailByteMaskData* getRailByteMaskData(int i) { return &allRailByteMaskData[i]; }
	int getRailByteMaskCount() { return (railByteMaskBitsTracked + 31) / 32; }
	LevelTypes::Plane* getVictoryPlane() { return victoryPlane; }
	void trackPassThroughMiniPuzzle(
		vector<LevelTypes::RailByteMaskData*>& passThroughRails, LevelTypes::RailByteMaskData::ByteMask miniPuzzleBit)
	{
		allPassThroughMiniPuzzles.push_back(PassThroughMiniPuzzle(passThroughRails, miniPuzzleBit));
	}
	void trackIsolatedArea(
		vector<LevelTypes::RailByteMaskData::BitsLocation>& goalSwitchCanKickBits,
		vector<LevelTypes::RailByteMaskData::BitsLocation>& abandonCanUseBits,
		LevelTypes::RailByteMaskData::ByteMask sharedAbandonBit)
	{
		allIsolatedAreas.push_back(IsolatedArea(goalSwitchCanKickBits, abandonCanUseBits, sharedAbandonBit));
	}
	static void cancelHintSearch() { hintSearchIsRunning = false; }
	//add a new plane to this level
	LevelTypes::Plane* addNewPlane();
	//add a special plane for use as the victory plane
	void addVictoryPlane();
	//set the switch for the radio tower hint
	void assignRadioTowerSwitch(Switch* radioTowerSwitch);
	//set the reset switch for the undo/reset hint
	void assignResetSwitch(ResetSwitch* resetSwitch);
	//create a byte mask for a new rail
	//returns the index into the internal byte mask vector for use in getRailByteMaskData()
	int trackNextRail(short railId, Rail* rail);
	//register the given number of bits in the rail byte mask, and return the bits data
	LevelTypes::RailByteMaskData::ByteMask trackRailByteMaskBits(int nBits);
	//finish setup of this level
	void finalizeBuilding();
	//setup helper objects used by all levels in hint searching
	static void setupHintSearchHelpers(vector<Level*>& allLevels);
	//delete helpers used in hint searching
	static void deleteHelpers();
	//generate a hint to solve this level from the start, to save time in the future allocating PotentialLevelStates when
	//	generating hints
	void preAllocatePotentialLevelStates();
	#ifdef DEBUG
		//validate that the reset switch resets all the switches of this level, and no more
		void validateResetSwitch();
	#endif
	//generate a hint based on the initial state in this level
	Hint* generateHint(LevelTypes::Plane* currentPlane, GetRailState getRailState, char lastActivatedSwitchColor);
private:
	//setup some state to be used during plane searches
	void resetPlaneSearchHelpers();
	//create a potential level state set with the given state retriever, loading it into potentialLevelStatesByBucketByPlane
	HintState::PotentialLevelState* loadBasePotentialLevelState(LevelTypes::Plane* currentPlane, GetRailState getRailState);
	//set bits in the draft state where applicable:
	//- set bits where switches can be kicked and planes can be visited
	void markStatusBitsInDraftState();
	//begin the hint search after all the helpers have been set up
	Hint* performHintSearch(HintState::PotentialLevelState* baseLevelState, LevelTypes::Plane* currentPlane, int startTime);
	//release all potential level states used and clear the structures that held them
	//returns how many unique states were held
	int clearPotentialLevelStateHolders();
	#ifdef TEST_SOLUTIONS
		//load the solution file for this level and test that the solutions in it follow a valid path to the victory plane
		void testSolutions(GetRailState getRailState);
		//read steps from the input and test that they follow a valid path to the victory plane
		void testSolution(GetRailState getRailState, ifstream& file, int& lineN);
	#endif
public:
	//get the queue of next potential level states corresponding to the given steps
	static deque<HintState::PotentialLevelState*>* getNextPotentialLevelStatesForSteps(int nextPotentialLevelStateSteps);
	//insert the given state to be the next state that we check in our hint search, assuming we haven't already done so with a
	//	better state
	//returns whether the state was inserted
	static bool frontloadMilestoneDestinationState(HintState::PotentialLevelState* state);
	//set bits in the draft state where applicable:
	//- clear bits where switches can no longer be kicked
	//returns whether any bits were changed
	bool markStatusBitsInDraftStateOnMilestone();
	//save away the current states to check, and start over with a new set at the given number of steps
	static void pushMilestone(int newPotentialLevelStateSteps);
private:
	//restore states to check from a previous milestone
	//should only be called when there are no states in the currentNextPotentialLevelStatesBySteps queues
	//returns whether there was a previous milestone to restore to
	static bool popMilestone();
public:
	//log basic information about this level
	void logStats();
};
