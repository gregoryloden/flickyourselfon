#include "CircularStateQueue.h"
#include "GameState.h"

template class CircularStateQueue<GameState>;

template <class Type> CircularStateQueue<Type>::CircularStateQueue(Type* writeHeadState, Type* readHeadState)
: onlyInDebug(ObjCounter() COMMA)
writeHead(nullptr)
, readHead(nullptr)
, readHeadNext(nullptr) {
	writeHead = newTracked(Node, (writeHeadState));
	readHead = newTracked(Node, (readHeadState));
	readHeadNext = writeHead;
	writeHead->next = readHead;
	readHead->next = writeHead;
	writeHead->needsRead = true;
}
template <class Type> CircularStateQueue<Type>::~CircularStateQueue() {
	Node* node = writeHead->next;
	writeHead->next = nullptr;
	while (node != nullptr) {
		Node* nodeToDelete = node;
		node = node->next;
		delete nodeToDelete;
	}
}
//if the write head's next state is writable, return it
template <class Type> Type* CircularStateQueue<Type>::getNextWritableState() {
	return !writeHead->next->needsRead ? writeHead->next->state : nullptr;
}
//insert a writable state after the write head
template <class Type> void CircularStateQueue<Type>::addWritableState(Type* state) {
	Node* nextWritableNode = newTracked(Node, (state));
	nextWritableNode->next = writeHead->next;
	writeHead->next = nextWritableNode;
}
//advance the write head to the next state and mark it as readable
template <class Type> void CircularStateQueue<Type>::finishWritingToState() {
	(writeHead = writeHead->next)->needsRead = true;
}
//if the read head next state is now readable, return it
//if not, but there's a new readable state after the read head, shift back the second read head return the state
//otherwise return nullptr
template <class Type> Type* CircularStateQueue<Type>::getNextReadableState() {
	if (readHeadNext->needsRead)
		return readHeadNext->state;
	else if (readHead->next->needsRead) {
		return (readHeadNext = readHead->next)->state;
	} else
		return nullptr;
}
//if there are multiple readable states, return the last one and mark the rest as writable
template <class Type> Type* CircularStateQueue<Type>::advanceToLastReadableState() {
	while (true) {
		Type* state = getNextReadableState();
		if (!readHeadNext->next->needsRead)
			return state;
		finishReadingFromState();
	}
}
//advance the read heads to the next states and mark the first one as writable
template <class Type> void CircularStateQueue<Type>::finishReadingFromState() {
	(readHead = readHeadNext)->needsRead = false;
	readHeadNext = readHead->next;
}
template <class Type> CircularStateQueue<Type>::Node::Node(Type* pState)
: onlyInDebug(ObjCounter() COMMA)
next(nullptr)
, state(pState)
, needsRead(false) {
}
template <class Type> CircularStateQueue<Type>::Node::~Node() {
	//don't delete next, let the owning queue handle it
	delete state;
}
