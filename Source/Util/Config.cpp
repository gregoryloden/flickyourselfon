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

//////////////////////////////// Config::ValueSelectionSetting::SelectableValue ////////////////////////////////
ConfigTypes::ValueSelectionSetting::SelectableValue::SelectableValue(string pName, int pValue)
: name(pName)
, value(pValue) {
}
ConfigTypes::ValueSelectionSetting::SelectableValue::~SelectableValue() {}

//////////////////////////////// Config::ValueSelectionSetting ////////////////////////////////
ConfigTypes::ValueSelectionSetting::ValueSelectionSetting(
	vector<SelectableValue> pValues,
	int pDefaultSelectedValueIndex,
	string pCustomValueNameSuffix,
	string pFilePrefix,
	vector<ValueSelectionSetting*>& containingList)
: values(pValues)
, defaultSelectedValueIndex(pDefaultSelectedValueIndex)
, selectedValueIndex(pDefaultSelectedValueIndex)
, selectedValue(pValues[pDefaultSelectedValueIndex].value)
, customValueNameSuffix(pCustomValueNameSuffix)
, filePrefix(pFilePrefix) {
	containingList.push_back(this);
}
ConfigTypes::ValueSelectionSetting::~ValueSelectionSetting() {}
string ConfigTypes::ValueSelectionSetting::getSelectedValueName() {
	if (selectedValueIndex == customValueIndex)
		return to_string(selectedValue) + customValueNameSuffix;
	else
		return values[selectedValueIndex].name;
}
void ConfigTypes::ValueSelectionSetting::changeSelection(int direction) {
	int newSelectedValueIndex;
	//custom value selected - find the nearest preset value in the given direction
	//we can assume that the current value is not equal to any of the preset values, because it's impossible to set a custom
	//	value while the program is running, and we find a matching index at the start if it's possible
	if (selectedValueIndex == customValueIndex) {
		//find the first preset value greater than the current one
		//if we don't find one, treat the first index after the list as the index
		int firstGreaterIndex = (int)values.size();
		for (int i = 0; i < (int)values.size(); i++) {
			if (values[i].value > selectedValue) {
				firstGreaterIndex = i;
				break;
			}
		}
		newSelectedValueIndex = direction > 0 ? firstGreaterIndex : firstGreaterIndex - 1;
	//preset value selected - just go up or down one value
	} else
		newSelectedValueIndex = selectedValueIndex + direction;
	if (newSelectedValueIndex >= 0 && newSelectedValueIndex < (int)values.size()) {
		selectedValueIndex = newSelectedValueIndex;
		selectedValue = values[newSelectedValueIndex].value;
	}
}
using namespace ConfigTypes;

//////////////////////////////// Config ////////////////////////////////
float Config::currentPixelWidth = Config::defaultPixelWidth;
float Config::currentPixelHeight = Config::defaultPixelHeight;
int Config::windowScreenWidth = Config::gameScreenWidth;
int Config::windowScreenHeight = Config::gameScreenHeight;
int Config::windowDisplayWidth = 0;
int Config::windowDisplayHeight = 0;
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
OnOffSetting Config::showBlockedFallEdges ("showBlockedFallEdges ", Config::allMultiStateSettings);
OnOffSetting Config::autosaveEnabled ("autosaveEnabled ", Config::allMultiStateSettings);
vector<ValueSelectionSetting*> Config::allValueSelectionSettings;
ValueSelectionSetting Config::autosaveInterval (
	{
		ValueSelectionSetting::SelectableValue("5 seconds", 5),
		ValueSelectionSetting::SelectableValue("10 seconds", 10),
		ValueSelectionSetting::SelectableValue("15 seconds", 15),
		ValueSelectionSetting::SelectableValue("30 seconds", 30),
		ValueSelectionSetting::SelectableValue("1 minute", 60),
		ValueSelectionSetting::SelectableValue("2 minutes", 120),
		ValueSelectionSetting::SelectableValue("3 minutes", 180),
		ValueSelectionSetting::SelectableValue("5 minutes", 300),
		ValueSelectionSetting::SelectableValue("10 minutes", 600),
		ValueSelectionSetting::SelectableValue("15 minutes", 900),
		ValueSelectionSetting::SelectableValue("20 minutes", 1200),
		ValueSelectionSetting::SelectableValue("30 minutes", 1800),
	},
	7,
	" seconds",
	"autosaveInterval ",
	Config::allValueSelectionSettings);
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
	for (ValueSelectionSetting* valueSelectionSetting : allValueSelectionSettings) {
		if (valueSelectionSetting->selectedValueIndex != valueSelectionSetting->defaultSelectedValueIndex)
			file << valueSelectionSetting->filePrefix << valueSelectionSetting->selectedValue << "\n";
	}
	file.close();
}
void Config::loadSettings() {
	ifstream file;
	FileUtils::openFileForRead(&file, optionsFileName, FileUtils::FileReadLocation::ApplicationData);
	string line;
	while (getline(file, line))
		loadKeyBindingSetting(line)
			|| loadMultiStateSetting(line)
			|| loadVolumeSetting(line)
			|| loadValueSelectionSetting(line);
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
bool Config::loadValueSelectionSetting(string& line) {
	for (ValueSelectionSetting* valueSelectionSetting : allValueSelectionSettings) {
		if (StringUtils::startsWith(line, valueSelectionSetting->filePrefix)) {
			valueSelectionSetting->selectedValue = atoi(line.c_str() + valueSelectionSetting->filePrefix.size());
			valueSelectionSetting->selectedValueIndex = ValueSelectionSetting::customValueIndex;
			for (int i = 0; i < (int)valueSelectionSetting->values.size(); i++) {
				if (valueSelectionSetting->values[i].value == valueSelectionSetting->selectedValue) {
					valueSelectionSetting->selectedValueIndex = i;
					break;
				}
			}
			return true;
		}
	}
	return false;
}
