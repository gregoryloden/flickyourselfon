#include "CircularStateQueue.h"
#include "GameState/GameState.h"
#include "Util/Logger.h"

template class CircularStateQueue<GameState>;
template class CircularStateQueue<Logger::Message>;

template <class Type> CircularStateQueue<Type>::Node::Node(objCounterParametersComma() Type* pState)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
next(nullptr)
, state(pState) {
}
template <class Type> CircularStateQueue<Type>::Node::~Node() {
	//don't delete next, let the owning queue handle it
	delete state;
}
template <class Type> CircularStateQueue<Type>::CircularStateQueue(
	objCounterParametersComma() Type* writeHeadState, Type* nextState)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
writeHead(newWithArgs(Node, writeHeadState))
, readHead(nullptr)
, lastStateWasRead(true) {
	Node* next = newWithArgs(Node, nextState);
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
//if the write head's next state is writable, return it
template <class Type> Type* CircularStateQueue<Type>::getNextWritableState() {
	return writeHead->next != readHead ? writeHead->next->state : nullptr;
}
//insert a writable state after the write head
template <class Type> void CircularStateQueue<Type>::addWritableState(Type* state) {
	Node* nextWritableNode = newWithArgs(Node, state);
	nextWritableNode->next = writeHead->next;
	writeHead->next = nextWritableNode;
}
//advance the write head to the next state and set the read head if necessary
template <class Type> void CircularStateQueue<Type>::finishWritingToState() {
	writeHead = writeHead->next;
	if (lastStateWasRead) {
		readHead = readHead->next;
		lastStateWasRead = false;
	}
}
//if we haven't read our state yet, return it
template <class Type> Type* CircularStateQueue<Type>::getNextReadableState() {
	return !lastStateWasRead ? readHead->state : nullptr;
}
//advance the read head if there are more readable states
template <class Type> void CircularStateQueue<Type>::finishReadingFromState() {
	if (readHead == writeHead)
		lastStateWasRead = true;
	else
		readHead = readHead->next;
}
//if there are multiple readable states, return the last one and mark the rest as writable
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
