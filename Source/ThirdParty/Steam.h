#ifdef STEAM
#include "General/General.h"

class ISteamUserStats;

class Steam {
private:
	static constexpr unsigned int appId = 3828290;

	static bool isActive;
	static bool fixLevelEndAchievements;
	static ISteamUserStats* steamUserStats;

public:
	//Prevent allocation
	Steam() = delete;
	static void setFixLevelEndAchievements() { fixLevelEndAchievements = true; }
	//restart the program through steam if it wasn't run through Steam
	static bool restartAppIfNecessary();
	//initialize the Steam API and structures
	static bool init();
	//shut down the Steam API
	static void shutdown();
	//unlock the achievement for the given level
	static void unlockLevelEndAchievement(int level);
private:
	//unlock the achievements for the given range of levels
	static void unlockLevelEndAchievements(int levelMin, int levelMax);
public:
	//unlock all achievements up to the given level, if applicable
	static void tryFixLevelEndAchievements(int upToLevel);
	//unlock the achievement for the end of the game
	static void unlockEndGameAchievement();
};
#endif
