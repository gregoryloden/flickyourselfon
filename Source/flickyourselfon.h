#include "General/General.h"

class GameState;
template <class Type> class CircularStateQueue;

#ifdef __cplusplus
extern "C"
#endif
int gameMain();
void renderLoop(CircularStateQueue<GameState>* gameStateQueue);
