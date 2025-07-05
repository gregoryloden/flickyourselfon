#ifdef STEAM
#include "ThirdParty/Steam.h"
#pragma warning(push)
#pragma warning(disable: 4996)
#include <steam_api.h>
#pragma warning(pop)

bool Steam::isActive = false;
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
	if (!isActive || level < 1 || level > 7)
		return;
	string achievementName = "COMPLETED_LEVEL_" + to_string(level);
	steamUserStats->SetAchievement(achievementName.c_str());
	steamUserStats->StoreStats();
}
#endif
