#include "General/General.h"

template <class Type> class CircularStateQueue;
class GameState;

//main runner of the game
int gameMain(int argc, char* argv[]);
//main render loop for the game
void renderLoop(CircularStateQueue<GameState>* gameStateQueue);
//show a popup message box
//returns the result of the message box
int messageBox(const char* message, UINT messageBoxType);
//wait for another thread to set the given condition to true after it locks the given mutex, using a combination of sleeping and
//	locking/unlocking the given mutex
void waitForOtherThread(mutex& waitMutex, bool* resumeCondition);
