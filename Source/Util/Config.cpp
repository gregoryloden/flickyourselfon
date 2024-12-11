#include "Config.h"
#include "Util/FileUtils.h"
#include "Util/StringUtils.h"

//////////////////////////////// Config::KeyBindingSetting ////////////////////////////////
ConfigTypes::KeyBindingSetting::KeyBindingSetting(
	SDL_Scancode pDefaultValue, string pFilePrefix, vector<KeyBindingSetting*>& containingList)
: defaultValue(pDefaultValue)
, value(pDefaultValue)
, editingValue(pDefaultValue)
, filePrefix(pFilePrefix) {
	containingList.push_back(this);
}
ConfigTypes::KeyBindingSetting::~KeyBindingSetting() {}
const char* ConfigTypes::KeyBindingSetting::getKeyName(SDL_Scancode key) {
	switch (key) {
		case SDL_SCANCODE_LEFT: return u8"←";
		case SDL_SCANCODE_UP: return u8"↑";
		case SDL_SCANCODE_RIGHT: return u8"→";
		case SDL_SCANCODE_DOWN: return u8"↓";
		default: return SDL_GetKeyName(SDL_GetKeyFromScancode(key));
	}
}

//////////////////////////////// Config::MultiStateSetting ////////////////////////////////
ConfigTypes::MultiStateSetting::MultiStateSetting(
	vector<string> pOptions, string pFilePrefix, vector<MultiStateSetting*>& containingList)
: options(pOptions)
, state(0)
, filePrefix(pFilePrefix) {
	containingList.push_back(this);
}
ConfigTypes::MultiStateSetting::~MultiStateSetting() {}

//////////////////////////////// Config::OnOffSetting ////////////////////////////////
ConfigTypes::OnOffSetting::OnOffSetting(string pFilePrefix, vector<MultiStateSetting*>& containingList)
: MultiStateSetting({ "on", "off" }, pFilePrefix, containingList) {
}
ConfigTypes::OnOffSetting::~OnOffSetting() {}

//////////////////////////////// Config::HoldToggleSetting ////////////////////////////////
ConfigTypes::HoldToggleSetting::HoldToggleSetting(string pFilePrefix, vector<MultiStateSetting*>& containingList)
: MultiStateSetting({ "hold", "toggle" }, pFilePrefix, containingList) {
}
ConfigTypes::HoldToggleSetting::~HoldToggleSetting() {}

//////////////////////////////// Config::VolumeSetting ////////////////////////////////
ConfigTypes::VolumeSetting::VolumeSetting(string pFilePrefix, vector<VolumeSetting*>& containingList)
: volume(defaultVolume)
, filePrefix(pFilePrefix) {
	containingList.push_back(this);
}
ConfigTypes::VolumeSetting::~VolumeSetting() {}
using namespace ConfigTypes;

//////////////////////////////// Config ////////////////////////////////
float Config::currentPixelWidth = Config::defaultPixelWidth;
float Config::currentPixelHeight = Config::defaultPixelHeight;
int Config::windowScreenWidth = Config::gameScreenWidth;
int Config::windowScreenHeight = Config::gameScreenHeight;
int Config::refreshRate = 60;
vector<KeyBindingSetting*> Config::allKeyBindingSettings;
KeyBindingSetting Config::upKeyBinding (SDL_SCANCODE_UP, "upKey ", Config::allKeyBindingSettings);
KeyBindingSetting Config::rightKeyBinding (SDL_SCANCODE_RIGHT, "rightKey ", Config::allKeyBindingSettings);
KeyBindingSetting Config::downKeyBinding (SDL_SCANCODE_DOWN, "downKey ", Config::allKeyBindingSettings);
KeyBindingSetting Config::leftKeyBinding (SDL_SCANCODE_LEFT, "leftKey ", Config::allKeyBindingSettings);
KeyBindingSetting Config::kickKeyBinding (SDL_SCANCODE_SPACE, "kickKey ", Config::allKeyBindingSettings);
KeyBindingSetting Config::sprintKeyBinding (SDL_SCANCODE_LSHIFT, "sprintKey ", Config::allKeyBindingSettings);
KeyBindingSetting Config::undoKeyBinding (SDL_SCANCODE_Z, "undoKey ", Config::allKeyBindingSettings);
KeyBindingSetting Config::redoKeyBinding (SDL_SCANCODE_X, "redoKey ", Config::allKeyBindingSettings);
KeyBindingSetting Config::showConnectionsKeyBinding (SDL_SCANCODE_C, "showConnectionsKey ", Config::allKeyBindingSettings);
KeyBindingSetting Config::mapCameraKeyBinding (SDL_SCANCODE_M, "mapCameraKey ", Config::allKeyBindingSettings);
vector<MultiStateSetting*> Config::allMultiStateSettings;
OnOffSetting Config::climbKickIndicator ("climb ", Config::allMultiStateSettings);
OnOffSetting Config::fallKickIndicator ("fall ", Config::allMultiStateSettings);
OnOffSetting Config::railKickIndicator ("rail ", Config::allMultiStateSettings);
OnOffSetting Config::switchKickIndicator ("switch ", Config::allMultiStateSettings);
OnOffSetting Config::resetSwitchKickIndicator ("resetSwitch ", Config::allMultiStateSettings);
HoldToggleSetting Config::showConnectionsMode ("showConnectionsMode ", Config::allMultiStateSettings);
MultiStateSetting Config::heightBasedShading (
	{ "normal", "off", "extra" }, "heightBasedShading ", Config::allMultiStateSettings);
OnOffSetting Config::showActivatedSwitchWaves ("showActivatedSwitchWaves ", Config::allMultiStateSettings);
vector<VolumeSetting*> Config::allVolumeSettings;
VolumeSetting Config::masterVolume ("masterVolume ", Config::allVolumeSettings);
VolumeSetting Config::musicVolume ("musicVolume ", Config::allVolumeSettings);
VolumeSetting Config::soundsVolume ("soundsVolume ", Config::allVolumeSettings);
void Config::saveSettings() {
	ofstream file;
	FileUtils::openFileForWrite(&file, optionsFileName, ios::out | ios::trunc);
	for (KeyBindingSetting* keyBindingSetting : allKeyBindingSettings) {
		if (keyBindingSetting->value != keyBindingSetting->defaultValue)
			file << keyBindingSetting->filePrefix << (int)keyBindingSetting->value << "\n";
	}
	for (MultiStateSetting* multiStateSetting : allMultiStateSettings) {
		if (multiStateSetting->state != 0)
			file << multiStateSetting->filePrefix << multiStateSetting->state << "\n";
	}
	for (VolumeSetting* volumeSetting : allVolumeSettings) {
		if (volumeSetting->volume != VolumeSetting::defaultVolume)
			file << volumeSetting->filePrefix << volumeSetting->volume << "\n";
	}
	file.close();
}
void Config::loadSettings() {
	ifstream file;
	FileUtils::openFileForRead(&file, optionsFileName, FileUtils::FileReadLocation::ApplicationData);
	string line;
	while (getline(file, line))
		loadKeyBindingSetting(line) || loadMultiStateSetting(line) || loadVolumeSetting(line);
	file.close();
}
bool Config::loadKeyBindingSetting(string& line) {
	for (KeyBindingSetting* keyBindingSetting : allKeyBindingSettings) {
		if (StringUtils::startsWith(line, keyBindingSetting->filePrefix)) {
			keyBindingSetting->value = (SDL_Scancode)atoi(line.c_str() + keyBindingSetting->filePrefix.size());
			return true;
		}
	}
	return false;
}
bool Config::loadMultiStateSetting(string& line) {
	for (MultiStateSetting* multiStateSetting : allMultiStateSettings) {
		if (StringUtils::startsWith(line, multiStateSetting->filePrefix)) {
			multiStateSetting->state = atoi(line.c_str() + multiStateSetting->filePrefix.size());
			return true;
		}
	}
	return false;
}
bool Config::loadVolumeSetting(string& line) {
	for (VolumeSetting* volumeSetting : allVolumeSettings) {
		if (StringUtils::startsWith(line, volumeSetting->filePrefix)) {
			volumeSetting->volume = atoi(line.c_str() + volumeSetting->filePrefix.size());
			return true;
		}
	}
	return false;
}
