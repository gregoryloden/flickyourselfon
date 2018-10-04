#include "General.h"

template <class Type> class CircularStateQueue onlyInDebug(: public ObjCounter) {
private:
	class Node onlyInDebug(: public ObjCounter) {
	public:
		Node* next;
		Type* state;
		bool needsRead;

		Node(Type* pState);
		~Node();
	};

	//writeHead always points to the most recently written state
	Node* writeHead;
	//readHead always points to the most recently read state
	Node* readHead;
	//readHeadNext points to the next state to read unless a state was inserted before it
	Node* readHeadNext;

public:
	CircularStateQueue(Type* writeHeadState, Type* readHeadState);
	~CircularStateQueue();

	Type* getNextWritableState();
	void addWritableState(Type* state);
	void finishWritingToState();
	Type* getNextReadableState();
	Type* advanceToLastReadableState();
	void finishReadingFromState();
};
