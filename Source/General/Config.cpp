#include "General/General.h"

//TODO: settle on final background color
const float Config::backgroundColorRed = 3.0f / 16.0f;
const float Config::backgroundColorGreen = 0.0f;
const float Config::backgroundColorBlue = 3.0f / 16.0f;
const Config::KeyBindings Config::defaultKeyBindings;
int Config::refreshRate = 60;
Config::KeyBindings Config::keyBindings;
Config::KeyBindings Config::editingKeyBindings;

Config::KeyBindings::KeyBindings()
: upKey(SDL_SCANCODE_UP)
, rightKey(SDL_SCANCODE_RIGHT)
, downKey(SDL_SCANCODE_DOWN)
, leftKey(SDL_SCANCODE_LEFT)
, kickKey(SDL_SCANCODE_SPACE) {
}
Config::KeyBindings::~KeyBindings() {}
//copy the key bindings from the other KeyBindings
void Config::KeyBindings::set(const KeyBindings* other) {
	upKey = other->upKey;
	rightKey = other->rightKey;
	downKey = other->downKey;
	leftKey = other->leftKey;
	kickKey = other->kickKey;
}
