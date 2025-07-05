#ifdef STEAM
#include "ThirdParty/Steam.h"
#pragma warning(push)
#pragma warning(disable: 4996)
#include <steam_api.h>
#pragma warning(pop)

bool Steam::isActive = false;
bool Steam::fixLevelEndAchievements = false;
ISteamUserStats* Steam::steamUserStats = nullptr;
bool Steam::restartAppIfNecessary() {
	return SteamAPI_RestartAppIfNecessary(appId);
}
bool Steam::init() {
	if (!(isActive = SteamAPI_Init()))
		return false;
	steamUserStats = SteamUserStats();
	return true;
}
void Steam::shutdown() {
	SteamAPI_Shutdown();
}
void Steam::unlockLevelEndAchievement(int level) {
	if (!isActive)
		return;
	unlockLevelEndAchievements(level, level);
}
void Steam::unlockLevelEndAchievements(int levelMin, int levelMax) {
	if (levelMin < 1 || levelMax > 7)
		return;
	for (int level = levelMin; level <= levelMax; level++) {
		string achievementName = "COMPLETED_LEVEL_" + to_string(level);
		steamUserStats->SetAchievement(achievementName.c_str());
	}
	steamUserStats->StoreStats();
}
void Steam::tryFixLevelEndAchievements(int upToLevel) {
	if (!isActive || !fixLevelEndAchievements)
		return;
	unlockLevelEndAchievements(1, upToLevel);
}
void Steam::unlockEndGameAchievement() {
	if (!isActive)
		return;
	steamUserStats->SetAchievement("COMPLETED_GAME");
	steamUserStats->StoreStats();
}
#endif
