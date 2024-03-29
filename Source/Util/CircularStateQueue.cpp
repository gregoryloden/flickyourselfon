#include "CircularStateQueue.h"
#include "GameState/GameState.h"
#include "Util/Logger.h"

#define newNode(state) newWithArgs(CircularStateQueue::Node, state)

//////////////////////////////// CircularStateQueue::Node ////////////////////////////////
template <class Type> CircularStateQueue<Type>::Node::Node(objCounterParametersComma() Type* pState)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
next(nullptr)
, state(pState) {
}
template <class Type> CircularStateQueue<Type>::Node::~Node() {
	//don't delete next, let the owning queue handle it
	delete state;
}

//////////////////////////////// CircularStateQueue ////////////////////////////////
template <class Type>
CircularStateQueue<Type>::CircularStateQueue(objCounterParametersComma() Type* writeHeadState, Type* nextState)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
writeHead(newNode(writeHeadState))
, readHead(nullptr)
, lastStateWasRead(true)
, statesCount(2) {
	Node* next = newNode(nextState);
	writeHead->next = next;
	next->next = writeHead;
	readHead = writeHead;
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
template <class Type> Type* CircularStateQueue<Type>::getNextWritableState() {
	return writeHead->next != readHead ? writeHead->next->state : nullptr;
}
template <class Type> void CircularStateQueue<Type>::addWritableState(Type* state) {
	Node* nextWritableNode = newNode(state);
	nextWritableNode->next = writeHead->next;
	writeHead->next = nextWritableNode;
	statesCount++;
}
template <class Type> void CircularStateQueue<Type>::finishWritingToState() {
	writeHead = writeHead->next;
}
template <class Type> Type* CircularStateQueue<Type>::getNextReadableState() {
	//we haven't read from the read head yet, return the state
	if (!lastStateWasRead)
		return readHead->state;
	//we already read from the read head, but there's another state available, advance to it and return it
	else if (readHead != writeHead) {
		readHead = readHead->next;
		lastStateWasRead = false;
		return readHead->state;
	//we already read from the read head and there is no other state available
	} else
		return nullptr;
}
template <class Type> void CircularStateQueue<Type>::finishReadingFromState() {
	if (readHead == writeHead)
		lastStateWasRead = true;
	else
		readHead = readHead->next;
}
template <class Type> Type* CircularStateQueue<Type>::advanceToLastReadableState() {
	Type* state = getNextReadableState();
	if (state == nullptr)
		return nullptr;
	while (readHead != writeHead) {
		finishReadingFromState();
		state = getNextReadableState();
	}
	return state;
}

template class CircularStateQueue<GameState>;
template class CircularStateQueue<Logger::Message>;
