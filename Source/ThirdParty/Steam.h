#ifdef STEAM
#include "General/General.h"

class ISteamUserStats;

class Steam {
private:
	static constexpr unsigned int appId = 3828290;
	
	static bool isActive;
	static ISteamUserStats* steamUserStats;

public:
	//Prevent allocation
	Steam() = delete;
	//restart the program through steam if it wasn't run through Steam
	static bool restartAppIfNecessary();
	//initialize the Steam API and structures
	static bool init();
	//shut down the Steam API
	static void shutdown();
	//unlock the achievement for the given level
	static void unlockLevelEndAchievement(int level);
};
#endif
