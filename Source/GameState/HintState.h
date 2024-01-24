#include "Util/PooledReferenceCounter.h"

#define newHintState(type, data) produceWithArgs(HintState, type, data)
#define newHintStatePotentialLevelState(priorState, plane, draftState) \
	produceWithArgs(HintStateTypes::PotentialLevelState, priorState, plane, draftState)

class HintState;
class Level;
class Rail;
class Switch;
namespace LevelTypes {
	class Plane;
}

namespace HintStateTypes {
	enum class Type: int {
		None,
		Plane,
		Rail,
		Switch,
		UndoReset,
	};
	union Data {
		bool none; //not necessary but it gives the None type a data counterpart and it's used in the constructor
		LevelTypes::Plane* plane;
		Rail* rail;
		Switch* switch0;
	};
	class PotentialLevelState: public PooledReferenceCounter {
	public:
		static PotentialLevelState draftState;
		static int railByteMaskCount;

		PotentialLevelState* priorState;
		LevelTypes::Plane* plane;
		unsigned int* railByteMasks;
		unsigned int railByteMasksHash;
		Type type;
		Data data;

		PotentialLevelState(objCounterParameters());
		virtual ~PotentialLevelState();

		//initialize and return a PotentialLevelState
		static PotentialLevelState* produce(
			objCounterParametersComma()
			PotentialLevelState* pPriorState,
			LevelTypes::Plane* pPlane,
			PotentialLevelState* draftState);
		//release a reference to this PotentialLevelState and return it to the pool if applicable
		virtual void release();
		//set a hash based on the railByteMasks
		void setHash();
		//check whether this state already appears in the given list of potential states
		bool isNewState(vector<PotentialLevelState*>& potentialLevelStates);
		//get the hint that leads the player to the second state in the priorState stack
		HintState* getHint();
	};
}
class HintState: public PooledReferenceCounter {
public:
	static const int flashOnOffTicks = 350;
	static const int flashOnOffTotalTicks = flashOnOffTicks * 2;
	static const int flashTimes = 10;
	static const int totalDisplayTicks = flashOnOffTotalTicks * flashTimes;

	HintStateTypes::Type type;
	HintStateTypes::Data data;
	int animationEndTicksTime;

	HintState(objCounterParameters());
	virtual ~HintState();

	//initialize and return a HintState
	static HintState* produce(objCounterParametersComma() HintStateTypes::Type pType, HintStateTypes::Data pData);
	//release a reference to this HintState and return it to the pool if applicable
	virtual void release();
	//render this hint
	void render(int screenLeftWorldX, int screenTopWorldY, int ticksTime);
};
