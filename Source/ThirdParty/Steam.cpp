#ifdef STEAM
#include "ThirdParty/Steam.h"
#pragma warning(push)
#pragma warning(disable: 4996)
#include <steam_api.h>
#pragma warning(pop)

bool Steam::restartAppIfNecessary() {
	return SteamAPI_RestartAppIfNecessary(appId);
}
bool Steam::init() {
	return SteamAPI_Init();
}
void Steam::shutdown() {
	SteamAPI_Shutdown();
}
#endif
