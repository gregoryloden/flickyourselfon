#ifdef STEAM
#include "General/General.h"

class Steam {
private:
	static constexpr unsigned int appId = 3828290;
	
public:
	//Prevent allocation
	Steam() = delete;
	//restart the program through steam if it wasn't run through Steam
	static bool restartAppIfNecessary();
	//initialize the Steam API
	static bool init();
	//shut down the Steam API
	static void shutdown();
};
#endif
