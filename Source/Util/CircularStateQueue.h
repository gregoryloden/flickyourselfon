#include "General/General.h"

#define newCircularStateQueue(type, writeHeadState, nextState) newWithArgs(CircularStateQueue<type>, writeHeadState, nextState)

template <class Type> class CircularStateQueue onlyInDebug(: public ObjCounter) {
private:
	class Node onlyInDebug(: public ObjCounter) {
	public:
		Node* next;
		Type* state;

		Node(objCounterParametersComma() Type* pState);
		virtual ~Node();
	};

	//writeHead always points to the most recently written state
	Node* writeHead;
	//if lastStateWasRead is true, readHead points to the most recently read state
	//if lastStateWasRead is false, readHead points to the next readable state
	Node* readHead;
	//signals whether the read head has already been read
	bool lastStateWasRead;
	int statesCount;

public:
	CircularStateQueue(objCounterParametersComma() Type* writeHeadState, Type* nextState);
	virtual ~CircularStateQueue();

	int getStatesCount() { return statesCount; }
	//if the write head's next state is writable, return it
	Type* getNextWritableState();
	//insert a writable state after the write head
	void addWritableState(Type* state);
	//advance the write head to the next state and set the read head if necessary
	void finishWritingToState();
	//if there is a state we can read from, return it
	Type* getNextReadableState();
	//advance the read head if there are more readable states
	void finishReadingFromState();
	//if there are multiple readable states, return the last one and mark the rest as writable
	Type* advanceToLastReadableState();
};
