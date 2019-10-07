#include "General/General.h"

template <class Type> class CircularStateQueue;
class GameState;

#ifdef __cplusplus
extern "C"
#endif
int gameMain();
void renderLoop(CircularStateQueue<GameState>* gameStateQueue);
