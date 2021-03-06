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
	Type* getNextWritableState();
	void addWritableState(Type* state);
	void finishWritingToState();
	Type* getNextReadableState();
	void finishReadingFromState();
	Type* advanceToLastReadableState();
};
