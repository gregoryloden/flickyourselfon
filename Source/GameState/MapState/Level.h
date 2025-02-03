#include "GameState/HintState.h"

#define newLevel(levelN, startTile) newWithArgs(Level, levelN, startTile)
#ifdef DEBUG
	#define TRACK_HINT_SEARCH_STATS
	#define LOG_FOUND_HINT_STEPS
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

			ConnectionSwitch(Switch* switch0, int levelN);
			virtual ~ConnectionSwitch();
		};
		//Should only be allocated within an object, on the stack, or as a static object
		class Connection {
		public:
			Plane* toPlane;
			int railByteIndex;
			unsigned int railTileOffsetByteMask;
			Hint hint;

			Connection(Plane* pToPlane, int pRailByteIndex, int pRailTileOffsetByteMask, Rail* rail, int levelN);
			virtual ~Connection();
		};

		Level* owningLevel;
		int indexInOwningLevel;
		vector<Tile> tiles;
		vector<ConnectionSwitch> connectionSwitches;
		vector<Connection> connections;
		int renderLeftTileX;
		int renderTopTileY;
		int renderRightTileX;
		int renderBottomTileY;

	public:
		Plane(objCounterParametersComma() Level* pOwningLevel, int pIndexInOwningLevel);
		virtual ~Plane();

		Level* getOwningLevel() { return owningLevel; }
		int getIndexInOwningLevel() { return indexInOwningLevel; }
		void addRailConnectionToSwitch(LevelTypes::RailByteMaskData* railByteMaskData, int connectionSwitchesIndex) {
			connectionSwitches[connectionSwitchesIndex].affectedRailByteMaskData.push_back(railByteMaskData);
		}
		//add a tile
		void addTile(int x, int y);
		//add a switch
		//returns the index of the switch in this plane
		int addConnectionSwitch(Switch* switch0);
		//add a connection to another plane, if we don't already have one
		//returns true if the connection was redundant or added, and false if we need to add a new rail connection
		bool addConnection(Plane* toPlane, Rail* rail);
		//add a rail connection to another plane
		void addRailConnection(Plane* toPlane, LevelTypes::RailByteMaskData* railByteMaskData, Rail* rail);
		//reset the indexInOwningLevel to 0 for this plane, and set it to the given value on the given plane
		//assumes this plane is a victory plane for a previous level, meaning its true indexInOwningLevel will always be 0
		void writeVictoryPlaneIndex(Plane* victoryPlane, int pIndexInOwningLevel);
		//follow all possible actions, and see if any of them lead to reaching the victory plane
		Hint* pursueSolution(HintState::PotentialLevelState* currentState);
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
	//Should only be allocated within an object, on the stack, or as a static object
	class PotentialLevelStatesByBucket {
	public:
		static const int bucketSize = 257;

		vector<HintState::PotentialLevelState*> buckets[bucketSize];

		PotentialLevelStatesByBucket();
		virtual ~PotentialLevelStatesByBucket();
	};

	static const int absentRailByteIndex = -1;
	static const int railTileOffsetByteMaskBitCount = 3;
	static const int railMovementDirectionByteMaskBitCount = 1;
	static const int railByteMaskBitCount = railTileOffsetByteMaskBitCount + railMovementDirectionByteMaskBitCount;
	static const unsigned int baseRailTileOffsetByteMask = (1 << railTileOffsetByteMaskBitCount) - 1;
	static const unsigned int baseRailMovementDirectionByteMask =
		((1 << railMovementDirectionByteMaskBitCount) - 1) << railTileOffsetByteMaskBitCount;
	static const unsigned int baseRailByteMask = (1 << railByteMaskBitCount) - 1;

	static vector<PotentialLevelStatesByBucket> potentialLevelStatesByBucketByPlane;
	static deque<HintState::PotentialLevelState*> nextPotentialLevelStates;
	static LevelTypes::Plane* cachedHintSearchVictoryPlane;
	#ifdef TRACK_HINT_SEARCH_STATS
		static int hintSearchActionsChecked;
		static int hintSearchUniqueStates;
		static int hintSearchComparisonsPerformed;
		static int foundHintSearchTotalSteps;
	#endif

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
	void assignVictoryPlane(LevelTypes::Plane* pVictoryPlane) { victoryPlane = pVictoryPlane; }
	void assignRadioTowerSwitch(Switch* radioTowerSwitch) { radioTowerHint.data.switch0 = radioTowerSwitch; }
	LevelTypes::RailByteMaskData* getRailByteMaskData(int i) { return &allRailByteMaskData[i]; }
	int getRailByteMaskCount() { return (railByteMaskBitsTracked + 31) / 32; }
	//add a new plane to this level
	LevelTypes::Plane* addNewPlane();
	//create a byte mask for a new rail
	//returns the index into the internal byte mask vector for use in getRailByteMaskData()
	int trackNextRail(short railId, Rail* rail);
	//initialize PotentialLevelState helper objects to accomodate all of the given levels
	static void setupPotentialLevelStateHelpers(vector<Level*>& allLevels);
	//generate a hint to solve this level from the start, to save time in the future allocating PotentialLevelStates when
	//	generating hints
	void preAllocatePotentialLevelStates();
	//generate a hint based on the initial state in this level
	Hint* generateHint(
		LevelTypes::Plane* currentPlane,
		function<void(short railId, Rail* rail, char* outMovementDirection, char* outTileOffset)> getRailState,
		char lastActivatedSwitchColor);
	//log basic information about this level
	void logStats();
};
