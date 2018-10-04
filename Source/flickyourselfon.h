#include "General.h"

class GameState;
template <class Type> class CircularStateQueue;

void renderLoop(CircularStateQueue<GameState>* gameStateQueue);
