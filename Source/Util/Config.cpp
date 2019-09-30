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
, showConnectionsKey(SDL_SCANCODE_LSHIFT) {
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

//////////////////////////////// Config ////////////////////////////////
const float Config::defaultPixelWidth = 3.0f;
const float Config::defaultPixelHeight = 3.0f;
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
float Config::currentPixelWidth = Config::defaultPixelWidth;
float Config::currentPixelHeight = Config::defaultPixelHeight;
int Config::refreshRate = 60;
Config::KeyBindings Config::keyBindings;
Config::KeyBindings Config::editingKeyBindings;
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
	}
	file.close();
}
