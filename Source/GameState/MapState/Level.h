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

		//get the rail tile offset byte mask corresponding to the bit shift
		int getRailTileOffsetByteMask();
	};
	class Plane onlyInDebug(: public ObjCounter) {
	private:
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
			};
			//Should only be allocated within an object, on the stack, or as a static object
			union ConclusionsData {
				//Should only be allocated within an object, on the stack, or as a static object
				struct MiniPuzzle {
					vector<RailByteMaskData::BitsLocation> otherRailBits;
				};

				//make sure the default constructor doesn't construct any of the other members
				bool none;
				MiniPuzzle miniPuzzle;

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

			//go through every affected rail and write its tile offset byte mask to the given byte masks
			void writeTileOffsetByteMasks(vector<unsigned int>& railByteMasks);
			//set this ConnectionSwitch to be part of a mini puzzle
			void setMiniPuzzle(RailByteMaskData::ByteMask miniPuzzleBit, vector<RailByteMaskData*>& miniPuzzleRails);
		};
		//Should only be allocated within an object, on the stack, or as a static object
		class Connection {
		public:
			Plane* toPlane;
			char railByteIndex;
			unsigned int railTileOffsetByteMask;
			int steps;
			Hint hint;

			Connection(
				Plane* pToPlane,
				char pRailByteIndex,
				unsigned int pRailTileOffsetByteMask,
				int pSteps,
				Rail* rail,
				Plane* hintPlane);
			virtual ~Connection();

			//returns the first switch in the given list of planes that controls this rail, or nullptr if one was not found
			//writes the matching RailByteMaskData for the rail if a matching switch was found, as well as the switch's Plane
			ConnectionSwitch* findMatchingSwitch(
				vector<Plane*>& levelPlanes, RailByteMaskData** outRailByteMaskData, Plane** outPlane);
			//returns whether this connection is a rail connection that matches the given RailByteMaskData
			bool matchesRail(RailByteMaskData* railByteMaskData);
			//returns true if this is a rail connection that starts lowered, and can only be raised by switches on the given
			//	plane
			bool requiresSwitchesOnPlane(Plane* plane);
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
		//add a direct rail connection to another plane that is already connected to this plane
		void addReverseRailConnection(Plane* toPlane, Rail* rail);
		//add the data of a rail connection to the switch for the given index
		void addRailConnectionToSwitch(RailByteMaskData* railByteMaskData, int connectionSwitchesIndex);
		//start from the first plane, go through all connections and planes, find planes and rails that are required to get to
		//	the end, see which of them have single-use switches, and mark those switch connections as milestones
		//then recursively repeat the process, instead ending at the planes of those milestone switches
		//must be called before extending connections or removing non-hasAction plane connections
		static void findMilestones(vector<Plane*>& levelPlanes, RailByteMaskData::ByteMask alwaysOnBit);
	private:
		//find milestones that enable access to this plane, and record their planes in outDestinationPlanes
		void findMilestonesToThisPlane(vector<Plane*>& levelPlanes, vector<Plane*>& outDestinationPlanes);
		//find all rail connections that must be crossed in order to get to this plane from the start plane
		vector<Connection*> findRequiredRailConnectionsToThisPlane(vector<Plane*>& levelPlanes);
		//search for paths to every remaining plane in levelPlanes until we reach this plane, without going through any excluded
		//	connections or connections that require access to switches on this plane
		//assumes there is at least one plane in inOutPathPlanes, and starts the walk from the end of the path described by
		//	inOutPathPlanes, with inOutPathConnections detailing the connections going from the plane at the same index to the
		//	plane at the next index
		//calls checkPath() at every step after reaching a new plane, and if checkPath() returns:
		//- false: discards the most recently found plane from the path and continues searching
		//- true: continues searching with the plane in the path, or returns if the path ends at this plane
		//inOutPathPlanes and inOutPathConnections will contain:
		//- the path as it existed when checkPath() returned true after reaching this plane, or
		//- their original contents if checkPath() never returned true after reaching this plane
		void pathWalkToThisPlane(
			vector<Plane*>& levelPlanes,
			function<bool(Connection* connection)> excludeConnection,
			vector<Plane*>& inOutPathPlanes,
			vector<Connection*>& inOutPathConnections,
			function<bool()> checkPath);
		//track this plane as a milestone destination plane
		void trackAsMilestoneDestination();
	public:
		//find sets of 2 or more switches that have rails in common
		static void findMiniPuzzles(
			vector<Plane*>& levelPlanes, RailByteMaskData::ByteMask alwaysOffBit, RailByteMaskData::ByteMask alwaysOnBit);
		//copy and add plane-plane and rail connections from all planes that are reachable through plane-plane connections from
		//	this plane
		void extendConnections();
		//remove plane-plane connections to planes that don't have any switches
		void removeEmptyPlaneConnections();
		#ifdef DEBUG
			//validate that the reset switch resets all the switches in this plane
			void validateResetSwitch(ResetSwitch* resetSwitch);
		#endif
		//set bits in the draft state where applicable:
		//- set bits where milestones are new
		//- set bits where switches can be kicked
		static void markStatusBitsInDraftState(vector<Plane*>& levelPlanes);
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
	//finish setup of this level:
	//- add a dedicated victory plane
	//- store multi-purpose bit locations
	//- find planes with milestone switches
	//- find mini puzzles
	//- add extended connections to the planes of this level
	//- remove plane-plane connections to planes without switches
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
		void validateResetSwitch(ResetSwitch* resetSwitch);
	#endif
	//generate a hint based on the initial state in this level
	Hint* generateHint(LevelTypes::Plane* currentPlane, GetRailState getRailState, char lastActivatedSwitchColor);
private:
	//setup some state to be used during plane searches
	void resetPlaneSearchHelpers();
	//create a potential level state set with the given state retriever, loading it into potentialLevelStatesByBucketByPlane
	HintState::PotentialLevelState* loadBasePotentialLevelState(LevelTypes::Plane* currentPlane, GetRailState getRailState);
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
	//insert the given state to be the next state that we check in our hint search
	static void frontloadMilestoneDestinationState(HintState::PotentialLevelState* state);
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
