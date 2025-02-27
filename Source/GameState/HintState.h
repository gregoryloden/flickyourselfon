#ifndef HINT_STATE_H
#define HINT_STATE_H
#include "Util/PooledReferenceCounter.h"

#define newHintState(hint, animationEndTicksTime) produceWithArgs(HintState, hint, animationEndTicksTime)

class Rail;
class Switch;
namespace LevelTypes {
	class Plane;
}

//Should only be allocated within an object, on the stack, or as a static object
class Hint {
public:
	enum class Type: int {
		None,
		Plane,
		Rail,
		Switch,
		UndoReset,
		SearchCanceledEarly,
	};
	union Data {
		bool none; //not necessary but it gives the None type a data counterpart and it's used in the constructor
		LevelTypes::Plane* plane;
		Rail* rail;
		Switch* switch0;
	};

	static Hint none;
	static Hint undoReset;
	static Hint searchCanceledEarly;

	Type type;
	Data data;

	Hint(Type pType);
	virtual ~Hint();

	bool isAdvancement() { return type == Type::Plane || type == Type::Rail || type == Type::Switch; }
};
class HintState: public PooledReferenceCounter {
public:
	class PotentialLevelState: public PooledReferenceCounter {
	public:
		static PotentialLevelState draftState;
		static int maxRailByteMaskCount;
		static int currentRailByteMaskCount;

		PotentialLevelState* priorState;
		unsigned int* railByteMasks;
		unsigned int railByteMasksHash;
		int steps;
		LevelTypes::Plane* plane;
		Hint* hint;

		PotentialLevelState(objCounterParameters());
		virtual ~PotentialLevelState();

		//initialize and return a PotentialLevelState
		//retains the PotentialLevelState, callers are responsible for releasing it
		static PotentialLevelState* produce(
			objCounterParametersComma() PotentialLevelState* priorStateAndDraftState, int pSteps);
		//release a reference to this PotentialLevelState and return it to the pool if applicable
		virtual void release();
		//set a hash based on the railByteMasks
		void setHash();
		//check if the given list of potential states has a state matching this state, and if applicable (see below), create a
		//	new one (using this state as the prior state and draft state), add it to the list, and retain it
		//callers are responsible for releasing it
		//- if there was no matching state: returns the new state after adding it to the end of the list
		//- if there was a matching state with equal or fewer steps than the given steps: returns nullptr
		//- if there was a matching state with more steps than the given steps: returns the new state after setting steps on the
		//	old state to -1 and replacing it in the list with the new state
		PotentialLevelState* addNewState(vector<PotentialLevelState*>& potentialLevelStates, int pSteps);
		//get the hint that leads the player to the second state in the priorState stack
		Hint* getHint();
	};

private:
	static const int flashOnOffTicks = 350;
	static const int flashOnOffTotalTicks = flashOnOffTicks * 2;
	static const int flashTimes = 10;
	static const int totalDisplayTicks = flashOnOffTotalTicks * flashTimes;
	static const int offscreenEdgeSpacing = 12;
	static const int offscreenArrowMinEdgeSpacing = 1;
	static const int offscreenArrowMaxEdgeSpacing = offscreenArrowMinEdgeSpacing + offscreenEdgeSpacing;
	static const int offscreenArrowToHintSpacing = 1;

	Hint* hint;
	int animationEndTicksTime;
	float offscreenArrowAlpha;
	int renderLeftWorldX;
	int renderTopWorldY;
	int renderRightWorldX;
	int renderBottomWorldY;

public:
	HintState(objCounterParameters());
	virtual ~HintState();

	Hint::Type getHintType() { return hint->type; }
	//initialize and return a HintState
	static HintState* produce(objCounterParametersComma() Hint* pHint, int animationStartTicksTime);
	//release a reference to this HintState and return it to the pool if applicable
	virtual void release();
	//render this hint
	void render(int screenLeftWorldX, int screenTopWorldY, int ticksTime);
	//render an arrow pointing to this hint which is offscreen, using the alpha saved from render()
	void renderOffscreenArrow(int screenLeftWorldX, int screenTopWorldY);
};
#endif
