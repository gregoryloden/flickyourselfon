#include "GameState/HintState.h"

#define newLevel(levelN, startTile) newWithArgs(Level, levelN, startTile)
#ifdef DEBUG
	#define TRACK_HINT_SEARCH_STATS
	#define LOG_FOUND_HINT_STEPS
	//#define LOG_SEARCH_STEPS_STATS
	#define TEST_SOLUTIONS
#endif

class Level;
class Rail;
class Switch;
namespace LevelTypes {
	class RailByteMaskData;
}

namespace LevelTypes {
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
			vector<RailByteMaskData*> affectedRailByteMaskData;
			Hint hint;
			bool isSingleUse;
			bool isMilestone;

			ConnectionSwitch(Switch* switch0);
			virtual ~ConnectionSwitch();
		};
		//Should only be allocated within an object, on the stack, or as a static object
		class Connection {
		public:
			Plane* toPlane;
			int railByteIndex;
			unsigned int railTileOffsetByteMask;
			int steps;
			Hint hint;

			Connection(
				Plane* pToPlane, int pRailByteIndex, int pRailTileOffsetByteMask, int pSteps, Rail* rail, Plane* hintPlane);
			virtual ~Connection();
		};

		Level* owningLevel;
		int indexInOwningLevel;
		vector<Tile> tiles;
		vector<ConnectionSwitch> connectionSwitches;
		vector<Connection> connections;
		bool hasAction;
		int renderLeftTileX;
		int renderTopTileY;
		int renderRightTileX;
		int renderBottomTileY;

	public:
		Plane(objCounterParametersComma() Level* pOwningLevel, int pIndexInOwningLevel);
		virtual ~Plane();

		Level* getOwningLevel() { return owningLevel; }
		int getIndexInOwningLevel() { return indexInOwningLevel; }
		bool getHasAction() { return hasAction; }
		void setHasAction() { hasAction = true; }
		//add a tile
		void addTile(int x, int y);
		//add a switch
		//returns the index of the switch in this plane
		int addConnectionSwitch(Switch* switch0);
		//add a plane-plane connection to another plane, whether directly connected or connected through other planes, if we
		//	don't already have one
		void addPlaneConnection(Plane* toPlane, int steps, Plane* hintPlane);
		//add a rail connection to another plane, whether directly connected or connected through other planes
		void addRailConnection(
			Plane* toPlane, LevelTypes::RailByteMaskData* railByteMaskData, int steps, Rail* rail, Plane* hintPlane);
		//add a rail connection to another plane that is already connected to the given fromPlane
		//for direct connections, fromPlane is this
		//for extended connections, fromPlane is the last plane reachable from this plane by plane-plane connections
		void addReverseRailConnection(Plane* fromPlane, Plane* toPlane, int steps, Rail* rail, Plane* hintPlane);
		//add the data of a rail connection to the switch for the given index
		void addRailConnectionToSwitch(RailByteMaskData* railByteMaskData, int connectionSwitchesIndex);
		//start from the first plane, go through all connections and planes, find planes and rails that are required to get to
		//	the end, see which of them have single-use switches, and mark those switch connections as milestones
		static void findMilestones(vector<Plane*>& levelPlanes);
		//follow all possible paths to other planes, and return a hint if any of those other planes are the victory plane
		Hint* pursueSolutionToPlanes(HintState::PotentialLevelState* currentState, int basePotentialLevelStateSteps);
		//kick each switch in this plane, and then pursue solutions from those states
		Hint* pursueSolutionAfterSwitches(HintState::PotentialLevelState* currentState, int stepsAfterSwitchKick);
		#ifdef TEST_SOLUTIONS
			//go through all the given states and see if one has a switch matching the given description
			//if there is one, the Plane on it will be a clone of the matching plane with only the matching switch connection
			//callers are reponsible for tracking and deleting this plane
			static HintState::PotentialLevelState* findStateAtSwitch(
				vector<HintState::PotentialLevelState*>& states, char color, const char* switchGroupName);
		#endif
		//get the bounds of the hint to render for this plane
		void getHintRenderBounds(int* outLeftWorldX, int* outTopWorldY, int* outRightWorldX, int* outBottomWorldY);
		//render boxes over every tile in this plane
		void renderHint(int screenLeftWorldX, int screenTopWorldY, float alpha);
		//track the switches in this plane
		void countSwitches(int outSwitchCounts[4], int* outSingleUseSwitches);
		#ifdef LOG_FOUND_HINT_STEPS
			//log all the steps of a hint state
			void logSteps(HintState::PotentialLevelState* hintState);
			//log a single hint step
			void logHint(stringstream& stepsMessage, Hint* hint, unsigned int* railByteMasks);
			//log the rail byte masks
			void logRailByteMasks(stringstream& stepsMessage, unsigned int* railByteMasks);
		#endif
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class RailByteMaskData {
	public:
		short railId;
		int railByteIndex;
		int railBitShift;
		Rail* rail;
		unsigned int inverseRailByteMask;

		RailByteMaskData(short railId, int pRailByteIndex, int pRailBitShift, Rail* pRail);
		virtual ~RailByteMaskData();
	};
}
class Level onlyInDebug(: public ObjCounter) {
public:
	typedef function<void(short railId, Rail* rail, char* outMovementDirection, char* outTileOffset)> GetRailState;
	//Should only be allocated within an object, on the stack, or as a static object
	class PotentialLevelStatesByBucket {
	public:
		static const int bucketSize = 257;

		vector<HintState::PotentialLevelState*> buckets[bucketSize];

		PotentialLevelStatesByBucket();
		virtual ~PotentialLevelStatesByBucket();
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class CheckedPlaneData {
	public:
		static const int maxStepsLimit = MAXINT32;

		int steps;
		int checkPlanesIndex;
		Hint* hint;

		CheckedPlaneData();
		virtual ~CheckedPlaneData();
	};

	static const int absentRailByteIndex = -1;
	static const int railTileOffsetByteMaskBitCount = 3;
	static const int railMovementDirectionByteMaskBitCount = 1;
	static const int railByteMaskBitCount = railTileOffsetByteMaskBitCount + railMovementDirectionByteMaskBitCount;
	static const unsigned int baseRailTileOffsetByteMask = (1 << railTileOffsetByteMaskBitCount) - 1;
	static const unsigned int baseRailMovementDirectionByteMask =
		((1 << railMovementDirectionByteMaskBitCount) - 1) << railTileOffsetByteMaskBitCount;
	static const unsigned int baseRailByteMask = (1 << railByteMaskBitCount) - 1;
private:
	#ifdef DEBUG
		static const int maxHintSearchTicks = 30000;
	#else
		static const int maxHintSearchTicks = 5000;
	#endif
			

	static bool hintSearchIsRunning;
public:
	static vector<PotentialLevelStatesByBucket> potentialLevelStatesByBucketByPlane;
	static vector<HintState::PotentialLevelState*> replacedPotentialLevelStates;
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
public:
	static LevelTypes::Plane* cachedHintSearchVictoryPlane;
private:
	#ifdef LOG_SEARCH_STEPS_STATS
		static int* statesAtStepsByPlane;
		static int statesAtStepsByPlaneCount;
	#endif
public:
	#ifdef TRACK_HINT_SEARCH_STATS
		static int hintSearchActionsChecked;
		static int hintSearchComparisonsPerformed;
	#endif
	static int foundHintSearchTotalHintSteps;
	static int foundHintSearchTotalSteps;

private:
	int levelN;
	int startTile;
	vector<LevelTypes::Plane*> planes;
	vector<LevelTypes::RailByteMaskData> allRailByteMaskData;
	int railByteMaskBitsTracked;
	LevelTypes::Plane* victoryPlane;
	char minimumRailColor;
	Hint radioTowerHint;

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
	//create a byte mask for a new rail
	//returns the index into the internal byte mask vector for use in getRailByteMaskData()
	int trackNextRail(short railId, Rail* rail);
	//setup helper objects used by all levels in hint searching
	static void setupHintSearchHelpers(vector<Level*>& allLevels);
	//delete helpers used in hint searching
	static void deleteHelpers();
	//generate a hint to solve this level from the start, to save time in the future allocating PotentialLevelStates when
	//	generating hints
	void preAllocatePotentialLevelStates();
	//generate a hint based on the initial state in this level
	Hint* generateHint(LevelTypes::Plane* currentPlane, GetRailState getRailState, char lastActivatedSwitchColor);
private:
	//setup some state to be used during plane searches
	void resetPlaneSearchHelpers();
	//create a potential level state set with the given state retriever, loading it into potentialLevelStatesByBucketByPlane
	HintState::PotentialLevelState* loadBasePotentialLevelState(LevelTypes::Plane* currentPlane, GetRailState getRailState);
	//begin the hint search after all the helpers have been set up
	static Hint* performHintSearch(
		HintState::PotentialLevelState* baseLevelState, LevelTypes::Plane* currentPlane, int startTime);
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
	//save away the current states to check, and start over with a new set
	static void pushMilestone();
private:
	//restore states to check from a previous milestone
	//returns whether there was a previous milestone to restore to
	static bool popMilestone();
public:
	//log basic information about this level
	void logStats();
};
