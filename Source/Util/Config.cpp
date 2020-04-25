#include "Config.h"
#include "Util/FileUtils.h"
#include "Util/StringUtils.h"

//////////////////////////////// Config::KeyBindings ////////////////////////////////
Config::KeyBindings::KeyBindings()
: upKey(SDL_SCANCODE_UP)
, rightKey(SDL_SCANCODE_RIGHT)
, downKey(SDL_SCANCODE_DOWN)
, leftKey(SDL_SCANCODE_LEFT)
, kickKey(SDL_SCANCODE_SPACE)
, showConnectionsKey(SDL_SCANCODE_Z) {
}
Config::KeyBindings::~KeyBindings() {}
//copy the key bindings from the other KeyBindings
void Config::KeyBindings::set(const KeyBindings* other) {
	upKey = other->upKey;
	rightKey = other->rightKey;
	downKey = other->downKey;
	leftKey = other->leftKey;
	kickKey = other->kickKey;
	showConnectionsKey = other->showConnectionsKey;
}
//get the name for this key code
const char* Config::KeyBindings::getKeyName(SDL_Scancode key) {
	switch (key) {
		case SDL_SCANCODE_LEFT: return u8"←";
		case SDL_SCANCODE_UP: return u8"↑";
		case SDL_SCANCODE_RIGHT: return u8"→";
		case SDL_SCANCODE_DOWN: return u8"↓";
		default: return SDL_GetKeyName(SDL_GetKeyFromScancode(key));
	}
}

//////////////////////////////// Config::KickIndicators ////////////////////////////////
Config::KickIndicators::KickIndicators()
: climb(true)
, fall(true)
, rail(true)
, switch0(true)
, resetSwitch(true) {
}
Config::KickIndicators::~KickIndicators() {}

//////////////////////////////// Config ////////////////////////////////
#ifdef EDITOR
	const float Config::defaultPixelWidth = 3.0f;
	const float Config::defaultPixelHeight = 3.0f;
#else
	const float Config::defaultPixelWidth = 4.0f;
	const float Config::defaultPixelHeight = 4.0f;
#endif
//TODO: settle on final background color
const float Config::backgroundColorRed = 3.0f / 16.0f;
const float Config::backgroundColorGreen = 0.0f;
const float Config::backgroundColorBlue = 3.0f / 16.0f;
const Config::KeyBindings Config::defaultKeyBindings;
const char* Config::optionsFileName = "fyo.options";
const string Config::upKeyBindingFilePrefix = "upKey ";
const string Config::rightKeyBindingFilePrefix = "rightKey ";
const string Config::downKeyBindingFilePrefix = "downKey ";
const string Config::leftKeyBindingFilePrefix = "leftKey ";
const string Config::kickKeyBindingFilePrefix = "kickKey ";
const string Config::showConnectionsKeyBindingFilePrefix = "showConnectionsKey ";
const string Config::climbKickIndicatorFilePrefix = "climb ";
const string Config::fallKickIndicatorFilePrefix = "fall ";
const string Config::railKickIndicatorFilePrefix = "rail ";
const string Config::switchKickIndicatorFilePrefix = "switch ";
const string Config::resetSwitchKickIndicatorFilePrefix = "resetSwitch ";
float Config::currentPixelWidth = Config::defaultPixelWidth;
float Config::currentPixelHeight = Config::defaultPixelHeight;
int Config::refreshRate = 60;
Config::KeyBindings Config::keyBindings;
Config::KeyBindings Config::editingKeyBindings;
Config::KickIndicators Config::kickIndicators;
//save the key bindings to the save file
void Config::saveSettings() {
	ofstream file;
	FileUtils::openFileForWrite(&file, optionsFileName, ios::out | ios::trunc);
	if (keyBindings.upKey != defaultKeyBindings.upKey)
		file << upKeyBindingFilePrefix << (int)keyBindings.upKey << "\n";
	if (keyBindings.rightKey != defaultKeyBindings.rightKey)
		file << rightKeyBindingFilePrefix << (int)keyBindings.rightKey << "\n";
	if (keyBindings.downKey != defaultKeyBindings.downKey)
		file << downKeyBindingFilePrefix << (int)keyBindings.downKey << "\n";
	if (keyBindings.leftKey != defaultKeyBindings.leftKey)
		file << leftKeyBindingFilePrefix << (int)keyBindings.leftKey << "\n";
	if (keyBindings.kickKey != defaultKeyBindings.kickKey)
		file << kickKeyBindingFilePrefix << (int)keyBindings.kickKey << "\n";
	if (keyBindings.showConnectionsKey != defaultKeyBindings.showConnectionsKey)
		file << showConnectionsKeyBindingFilePrefix << (int)keyBindings.showConnectionsKey << "\n";
	if (!kickIndicators.climb)
		file << climbKickIndicatorFilePrefix << "off\n";
	if (!kickIndicators.fall)
		file << fallKickIndicatorFilePrefix << "off\n";
	if (!kickIndicators.rail)
		file << railKickIndicatorFilePrefix << "off\n";
	if (!kickIndicators.switch0)
		file << switchKickIndicatorFilePrefix << "off\n";
	if (!kickIndicators.resetSwitch)
		file << resetSwitchKickIndicatorFilePrefix << "off\n";
	file.close();
}
//load the key bindings from the save file, if it exists
void Config::loadSettings() {
	ifstream file;
	FileUtils::openFileForRead(&file, optionsFileName);
	string line;
	while (getline(file, line)) {
		if (StringUtils::startsWith(line, upKeyBindingFilePrefix))
			keyBindings.upKey = (SDL_Scancode)atoi(line.c_str() + upKeyBindingFilePrefix.size());
		else if (StringUtils::startsWith(line, rightKeyBindingFilePrefix))
			keyBindings.rightKey = (SDL_Scancode)atoi(line.c_str() + rightKeyBindingFilePrefix.size());
		else if (StringUtils::startsWith(line, downKeyBindingFilePrefix))
			keyBindings.downKey = (SDL_Scancode)atoi(line.c_str() + downKeyBindingFilePrefix.size());
		else if (StringUtils::startsWith(line, leftKeyBindingFilePrefix))
			keyBindings.leftKey = (SDL_Scancode)atoi(line.c_str() + leftKeyBindingFilePrefix.size());
		else if (StringUtils::startsWith(line, kickKeyBindingFilePrefix))
			keyBindings.kickKey = (SDL_Scancode)atoi(line.c_str() + kickKeyBindingFilePrefix.size());
		else if (StringUtils::startsWith(line, showConnectionsKeyBindingFilePrefix))
			keyBindings.showConnectionsKey = (SDL_Scancode)atoi(line.c_str() + showConnectionsKeyBindingFilePrefix.size());
		else if (StringUtils::startsWith(line, climbKickIndicatorFilePrefix))
			kickIndicators.climb = strcmp(line.c_str() + climbKickIndicatorFilePrefix.size(), "off") != 0;
		else if (StringUtils::startsWith(line, fallKickIndicatorFilePrefix))
			kickIndicators.fall = strcmp(line.c_str() + fallKickIndicatorFilePrefix.size(), "off") != 0;
		else if (StringUtils::startsWith(line, railKickIndicatorFilePrefix))
			kickIndicators.rail = strcmp(line.c_str() + railKickIndicatorFilePrefix.size(), "off") != 0;
		else if (StringUtils::startsWith(line, switchKickIndicatorFilePrefix))
			kickIndicators.switch0 = strcmp(line.c_str() + switchKickIndicatorFilePrefix.size(), "off") != 0;
		else if (StringUtils::startsWith(line, resetSwitchKickIndicatorFilePrefix))
			kickIndicators.resetSwitch = strcmp(line.c_str() + resetSwitchKickIndicatorFilePrefix.size(), "off") != 0;
	}
	file.close();
}
