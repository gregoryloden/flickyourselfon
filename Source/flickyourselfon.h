#include "General/General.h"

template <class Type> class CircularStateQueue;
class GameState;

#ifdef __cplusplus
extern "C"
#endif
int gameMain(int argc, char* argv[]);
void renderLoop(CircularStateQueue<GameState>* gameStateQueue);
