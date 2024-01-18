#include "Util/PooledReferenceCounter.h"

#define newHintState(type, data) produceWithArgs(HintState, type, data)
#define newHintStatePotentialLevelState(priorState, plane, draftState) \
	produceWithArgs(HintStateTypes::PotentialLevelState, priorState, plane, draftState)

class HintState;
class Level;
namespace LevelTypes {
	class Plane;
}

namespace HintStateTypes {
	enum class Type: int {
		None,
		Plane,
		Rail,
		Switch,
	};
	union Data {
		bool none; //not necessary but it gives the None type a data counterpart and it's used in the constructor
		LevelTypes::Plane* plane;
		short railId;
		short switchId;
	};
	class PotentialLevelState: public PooledReferenceCounter {
	public:
		static PotentialLevelState draftState;

		PotentialLevelState* priorState;
		LevelTypes::Plane* plane;
		int railByteMaskCount;
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
		//resize the rail byte masks to the given size, if needed
		void resizeRailByteMasks(int pRailByteMaskCount);
		//check whether this state already appears in the given list of potential states
		bool isNewState(vector<PotentialLevelState*>& potentialLevelStates);
		//get the hint that leads the player to the second state in the priorState stack
		HintState* getHint();
	};
}
class HintState: public PooledReferenceCounter {
public:
	HintStateTypes::Type type;
	HintStateTypes::Data data;
	int animationEndTicksTime;

	HintState(objCounterParameters());
	virtual ~HintState();

	//initialize and return a HintState
	static HintState* produce(objCounterParametersComma() HintStateTypes::Type pType, HintStateTypes::Data pData);
	//release a reference to this HintState and return it to the pool if applicable
	virtual void release();
};
