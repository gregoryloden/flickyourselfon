﻿#include "PauseState.h"

#define newPauseState(parentState, pauseMenu, pauseOption, selectingKey) \
	produceWithArgs(PauseState, parentState, pauseMenu, pauseOption, selectingKey)
#define newPauseMenu(title, options) newWithArgs(PauseState::PauseMenu, title, options)
#define newNavigationOption(displayText, subMenu) newWithArgs(PauseState::NavigationOption, displayText, subMenu)
#define newControlsNavigationOption(subMenu) newWithArgs(PauseState::ControlsNavigationOption, subMenu)
#define newKeyBindingOption(boundKey) newWithArgs(PauseState::KeyBindingOption, boundKey)
#define newDefaultKeyBindingsOption() newWithoutArgs(PauseState::DefaultKeyBindingsOption)
#define newAcceptKeyBindingsOption() newWithoutArgs(PauseState::AcceptKeyBindingsOption)
#define newQuitGameOption() newWithoutArgs(PauseState::QuitGameOption)

//////////////////////////////// PauseState::PauseMenu ////////////////////////////////
const float PauseState::PauseMenu::titleFontScale = 2.0f;
const float PauseState::PauseMenu::selectedOptionAngleBracketPadding = 4.0f;
PauseState::PauseMenu::PauseMenu(objCounterParametersComma() string pTitle, vector<PauseOption*> pOptions)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
title(pTitle)
, titleMetrics(Text::getMetrics(pTitle.c_str(), titleFontScale))
, options(pOptions) {
}
PauseState::PauseMenu::~PauseMenu() {
	for (PauseOption* option : options)
		delete option;
}
//render this menu
void PauseState::PauseMenu::render(int selectedOption, KeyBindingOption* selectingKeyBindingOption) {
	glEnable(GL_BLEND);
	glColor4f(Config::backgroundColorRed, Config::backgroundColorGreen, Config::backgroundColorBlue, 5.0f / 8.0f);
	glBegin(GL_QUADS);
	glVertex2i(0, 0);
	glVertex2i(Config::gameScreenWidth, 0);
	glVertex2i(Config::gameScreenWidth, Config::gameScreenHeight);
	glVertex2i(0, Config::gameScreenHeight);
	glEnd();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	//first find the height of the pause options so that we can vertically center them
	Text::Metrics titleMetrics = Text::getMetrics(title.c_str(), titleFontScale);
	float totalHeight = titleMetrics.aboveBaseline + titleMetrics.belowBaseline + titleMetrics.bottomPadding;
	float lastBottomPadding = 0;
	vector<Text::Metrics> optionsMetrics;
	for (PauseOption* option : options) {
		Text::Metrics optionMetrics =
			option == selectingKeyBindingOption
				? selectingKeyBindingOption->getSelectingDisplayTextMetrics(true)
				: option->getDisplayTextMetrics();
		lastBottomPadding = optionMetrics.bottomPadding;
		totalHeight += optionMetrics.topPadding + optionMetrics.aboveBaseline + optionMetrics.belowBaseline + lastBottomPadding;
		optionsMetrics.push_back(optionMetrics);
	}
	totalHeight -= lastBottomPadding;

	//then render them all
	float screenCenterX = (float)Config::gameScreenWidth * 0.5f;
	float optionsBaseline = ((float)Config::gameScreenHeight - totalHeight) * 0.5f + titleMetrics.aboveBaseline;

	Text::render(title.c_str(), screenCenterX - titleMetrics.charactersWidth * 0.5f, optionsBaseline, titleFontScale);
	optionsBaseline += titleMetrics.belowBaseline + titleMetrics.bottomPadding;

	int optionsCount = options.size();
	for (int i = 0; i < optionsCount; i++) {
		PauseOption* option = options[i];
		Text::Metrics& optionMetrics = optionsMetrics[i];
		optionsBaseline += optionMetrics.topPadding + optionMetrics.aboveBaseline;
		float leftX = screenCenterX - optionMetrics.charactersWidth * 0.5f;
		if (option == selectingKeyBindingOption)
			selectingKeyBindingOption->renderSelecting(leftX, optionsBaseline, true);
		else
			option->render(leftX, optionsBaseline);

		if (i == selectedOption) {
			Text::Metrics angleBracketMetrics = Text::getMetrics("<", PauseOption::displayTextFontScale);
			Text::render(
				"<",
				leftX - angleBracketMetrics.charactersWidth - selectedOptionAngleBracketPadding,
				optionsBaseline,
				PauseOption::displayTextFontScale);
			Text::render(
				">",
				leftX + optionMetrics.charactersWidth + selectedOptionAngleBracketPadding,
				optionsBaseline,
				PauseOption::displayTextFontScale);
		}

		optionsBaseline += optionMetrics.belowBaseline + optionMetrics.bottomPadding;
	}
}

//////////////////////////////// PauseState::PauseOption ////////////////////////////////
const float PauseState::PauseOption::displayTextFontScale = 1.0f;
PauseState::PauseOption::PauseOption(objCounterParametersComma() string pDisplayText)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
displayText(pDisplayText)
, displayTextMetrics(Text::getMetrics(pDisplayText.c_str(), displayTextFontScale)) {
}
PauseState::PauseOption::~PauseOption() {}
//render the NavigationOption
void PauseState::PauseOption::render(float leftX, float baselineY) {
	Text::render(displayText.c_str(), leftX, baselineY, displayTextFontScale);
}

//////////////////////////////// PauseState::NavigationOption ////////////////////////////////
PauseState::NavigationOption::NavigationOption(objCounterParametersComma() string pDisplayText, PauseMenu* pSubMenu)
: PauseOption(objCounterArgumentsComma() pDisplayText)
, subMenu(pSubMenu) {
}
PauseState::NavigationOption::~NavigationOption() {
	delete subMenu;
}
//navigate to this option's submenu
PauseState* PauseState::NavigationOption::handle(PauseState* currentState) {
	return currentState->navigateToMenu(subMenu);
}

//////////////////////////////// PauseState::ControlsNavigationOption ////////////////////////////////
PauseState::ControlsNavigationOption::ControlsNavigationOption(objCounterParametersComma() PauseMenu* pSubMenu)
: NavigationOption(objCounterArgumentsComma() "controls", pSubMenu) {
}
PauseState::ControlsNavigationOption::~ControlsNavigationOption() {}
//navigate to the controls submenu after setting up the menu key bindings
PauseState* PauseState::ControlsNavigationOption::handle(PauseState* currentState) {
	Config::editingKeyBindings.set(&Config::keyBindings);
	return NavigationOption::handle(currentState);
}

//////////////////////////////// PauseState::KeyBindingOption ////////////////////////////////
const char* PauseState::KeyBindingOption::keySelectingText = "[press any key]";
float PauseState::KeyBindingOption::cachedKeySelectingTextWidth = 0.0f;
float PauseState::KeyBindingOption::cachedKeySelectingTextFontScale = 0.0f;
PauseState::KeyBindingOption::KeyBindingOption(objCounterParametersComma() BoundKey pBoundKey)
: PauseOption(objCounterArgumentsComma() getBoundKeyActionText(pBoundKey))
, boundKey(pBoundKey)
, cachedKeyScancode(SDL_SCANCODE_UNKNOWN)
, cachedKeyName("")
, cachedKeyTextMetrics()
, cachedKeyBackgroundMetrics() {
}
PauseState::KeyBindingOption::~KeyBindingOption() {}
//return metrics that include the key name and background
Text::Metrics PauseState::KeyBindingOption::getDisplayTextMetrics() {
	return getSelectingDisplayTextMetrics(false);
}
//return metrics that include either the key-selecting text or the key name and background
Text::Metrics PauseState::KeyBindingOption::getSelectingDisplayTextMetrics(bool selecting) {
	Text::Metrics metrics = PauseOption::getDisplayTextMetrics();
	ensureCachedKeyMetrics(selecting);

	metrics.charactersWidth +=
		(float)interKeyActionAndKeyBackgroundSpacing
			+ (selecting ? cachedKeySelectingTextWidth : cachedKeyBackgroundMetrics.charactersWidth);
	metrics.aboveBaseline = cachedKeyBackgroundMetrics.aboveBaseline;
	metrics.belowBaseline = cachedKeyBackgroundMetrics.belowBaseline;
	metrics.topPadding = cachedKeyBackgroundMetrics.topPadding;
	metrics.bottomPadding = cachedKeyBackgroundMetrics.bottomPadding;
	return metrics;
}
//start selecting a key for this option
PauseState* PauseState::KeyBindingOption::handle(PauseState* currentState) {
	return currentState->beginKeySelection(this);
}
//render the text, the key name, and the key background
void PauseState::KeyBindingOption::render(float leftX, float baselineY) {
	renderSelecting(leftX, baselineY, false);
}
//render the text and either the key-selecting text or the key name and background
void PauseState::KeyBindingOption::renderSelecting(float leftX, float baselineY, bool selecting) {
	//render the action
	PauseOption::render(leftX, baselineY);

	ensureCachedKeyMetrics(selecting);
	leftX += PauseOption::getDisplayTextMetrics().charactersWidth + (float)interKeyActionAndKeyBackgroundSpacing;

	//render the key-selecting text
	if (selecting)
		Text::render(keySelectingText, leftX, baselineY, cachedKeySelectingTextFontScale);
	//render the key background and name
	else {
		Text::renderKeyBackground(leftX, baselineY, &cachedKeyBackgroundMetrics);
		leftX += (cachedKeyBackgroundMetrics.charactersWidth - cachedKeyTextMetrics.charactersWidth) * 0.5f;
		//the key background has 1 pixel on the left border and 3 on the right, render text 1 font pixel to the left
		Text::render(cachedKeyName.c_str(), leftX - cachedKeyTextMetrics.fontScale, baselineY, cachedKeyTextMetrics.fontScale);
	}
}
//if we have a new bound key, cache its metrics
void PauseState::KeyBindingOption::ensureCachedKeyMetrics(bool selecting) {
	SDL_Scancode currentBoundKeyScancode = getBoundKeyScancode();
	if (currentBoundKeyScancode != cachedKeyScancode) {
		cachedKeyScancode = currentBoundKeyScancode;
		switch (currentBoundKeyScancode) {
			case SDL_SCANCODE_LEFT: cachedKeyName = u8"←"; break;
			case SDL_SCANCODE_UP: cachedKeyName = u8"↑"; break;
			case SDL_SCANCODE_RIGHT: cachedKeyName = u8"→"; break;
			case SDL_SCANCODE_DOWN: cachedKeyName = u8"↓"; break;
			default:
				cachedKeyName = SDL_GetKeyName(SDL_GetKeyFromScancode(currentBoundKeyScancode));
				break;
		}
		cachedKeyTextMetrics = Text::getMetrics(cachedKeyName.c_str(), PauseOption::getDisplayTextMetrics().fontScale);
		cachedKeyBackgroundMetrics = Text::getKeyBackgroundMetrics(&cachedKeyTextMetrics);
	}
	if (cachedKeySelectingTextFontScale != cachedKeyTextMetrics.fontScale) {
		cachedKeySelectingTextFontScale = cachedKeyTextMetrics.fontScale;
		cachedKeySelectingTextWidth = Text::getMetrics(keySelectingText, cachedKeySelectingTextFontScale).charactersWidth;
	}
}
//get the name of the action we're binding a key to
//static so that we can use it in the option's constructor
string PauseState::KeyBindingOption::getBoundKeyActionText(BoundKey pBoundKey) {
	switch (pBoundKey) {
		case BoundKey::Up: return "up:";
		case BoundKey::Right: return "right:";
		case BoundKey::Down: return "down:";
		case BoundKey::Left: return "left:";
		case BoundKey::Kick: return "kick:";
		default: return "";
	}
}
//get the currently-editing scancode for our bound key
SDL_Scancode PauseState::KeyBindingOption::getBoundKeyScancode() {
	switch (boundKey) {
		case BoundKey::Up: return Config::editingKeyBindings.upKey;
		case BoundKey::Right: return Config::editingKeyBindings.rightKey;
		case BoundKey::Down: return Config::editingKeyBindings.downKey;
		case BoundKey::Left: return Config::editingKeyBindings.leftKey;
		case BoundKey::Kick: return Config::editingKeyBindings.kickKey;
		default: return SDL_SCANCODE_UNKNOWN;
	}
}
//update the scancode for this option's bound key
//this also changes the scancode for past states, but since this is atomic and scancodes are retrieved only once per frame,
//	this shouldn't be a problem
void PauseState::KeyBindingOption::setBoundKeyScancode(SDL_Scancode keyScancode) {
	switch (boundKey) {
		case BoundKey::Up: Config::editingKeyBindings.upKey = keyScancode; break;
		case BoundKey::Right: Config::editingKeyBindings.rightKey = keyScancode; break;
		case BoundKey::Down: Config::editingKeyBindings.downKey = keyScancode; break;
		case BoundKey::Left: Config::editingKeyBindings.leftKey = keyScancode; break;
		case BoundKey::Kick: Config::editingKeyBindings.kickKey = keyScancode; break;
		default: break;
	}
}

//////////////////////////////// PauseState::DefaultKeyBindingsOption ////////////////////////////////
PauseState::DefaultKeyBindingsOption::DefaultKeyBindingsOption(objCounterParameters())
: PauseOption(objCounterArgumentsComma() "defaults") {
}
PauseState::DefaultKeyBindingsOption::~DefaultKeyBindingsOption() {}
//reset the currently editing keys to the defaults
PauseState* PauseState::DefaultKeyBindingsOption::handle(PauseState* currentState) {
	Config::editingKeyBindings.set(&Config::defaultKeyBindings);
	return currentState;
}

//////////////////////////////// PauseState::AcceptKeyBindingsOption ////////////////////////////////
PauseState::AcceptKeyBindingsOption::AcceptKeyBindingsOption(objCounterParameters())
: PauseOption(objCounterArgumentsComma() "accept") {
}
PauseState::AcceptKeyBindingsOption::~AcceptKeyBindingsOption() {}
//reset the currently editing keys to the defaults
PauseState* PauseState::AcceptKeyBindingsOption::handle(PauseState* currentState) {
	Config::keyBindings.set(&Config::editingKeyBindings);
	return currentState->navigateToMenu(nullptr);
}

//////////////////////////////// PauseState::QuitGameOption ////////////////////////////////
PauseState::QuitGameOption::QuitGameOption(objCounterParameters())
: PauseOption(objCounterArgumentsComma() "exit") {
}
PauseState::QuitGameOption::~QuitGameOption() {}
//return a pause state to quit the game
PauseState* PauseState::QuitGameOption::handle(PauseState* currentState) {
	return currentState->produceQuitGameState();
}

//////////////////////////////// PauseState ////////////////////////////////
PauseState::PauseMenu* PauseState::baseMenu = nullptr;
PauseState::PauseState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, parentState(nullptr)
, pauseMenu(nullptr)
, pauseOption(0)
, selectingKeyBindingOption(nullptr)
, shouldQuitGame(false) {
}
PauseState::~PauseState() {}
//initialize and return a PauseState
PauseState* PauseState::produce(
	objCounterParametersComma()
	PauseState* pParentState,
	PauseMenu* pCurrentPauseMenu,
	int pPauseOption,
	KeyBindingOption* pSelectingKeyBindingOption)
{
	initializeWithNewFromPool(p, PauseState)
	p->parentState.set(pParentState);
	p->pauseMenu = pCurrentPauseMenu;
	p->pauseOption = pPauseOption;
	p->selectingKeyBindingOption = pSelectingKeyBindingOption;
	return p;
}
//return a new pause state at the base menu
PauseState* PauseState::produce(objCounterParameters()) {
	return produce(objCounterArgumentsComma() nullptr, baseMenu, 0, false);
}
pooledReferenceCounterDefineRelease(PauseState)
//release the parent state before this is returned to the pool
void PauseState::prepareReturnToPool() {
	parentState.set(nullptr);
}
//build the base pause menu
void PauseState::loadMenu() {
	baseMenu = newPauseMenu(
		"Pause",
		{
			newNavigationOption("resume", nullptr) COMMA
			newControlsNavigationOption(
				newPauseMenu(
					"Controls",
					{
						newKeyBindingOption(KeyBindingOption::BoundKey::Up) COMMA
						newKeyBindingOption(KeyBindingOption::BoundKey::Right) COMMA
						newKeyBindingOption(KeyBindingOption::BoundKey::Down) COMMA
						newKeyBindingOption(KeyBindingOption::BoundKey::Left) COMMA
						newKeyBindingOption(KeyBindingOption::BoundKey::Kick) COMMA
						newDefaultKeyBindingsOption() COMMA
						newAcceptKeyBindingsOption() COMMA
						newNavigationOption("back", nullptr)
					})) COMMA
			newQuitGameOption()
		});
}
//delete the base menu
void PauseState::unloadMenu() {
	delete baseMenu;
}
//if any keys were pressed, return a new updated pause state, otherwise return this non-updated state
//if the game was closed, return whatever intermediate state we have specifying to quit the game
PauseState* PauseState::getNextPauseState() {
	PauseState* nextPauseState = this;
	//handle events
	SDL_Event gameEvent;
	while (SDL_PollEvent(&gameEvent) != 0) {
		PauseState* lastPauseState = nextPauseState;

		if (gameEvent.type == SDL_QUIT)
			nextPauseState = nextPauseState->produceQuitGameState();
		else if (gameEvent.type == SDL_KEYDOWN)
			nextPauseState = nextPauseState->handleKeyPress(gameEvent.key.keysym.scancode);

		//if we got a new pause state, make sure the old one is returned to the pool if this is its only reference
		if (nextPauseState != lastPauseState) {
			lastPauseState->retain();
			lastPauseState->release();
		}
		if (nextPauseState == nullptr || nextPauseState->shouldQuitGame)
			return nextPauseState;
	}
	return nextPauseState;
}
//handle the keypress and return the resulting new pause state
PauseState* PauseState::handleKeyPress(SDL_Scancode keyScancode) {
	if (selectingKeyBindingOption != nullptr) {
		if (keyScancode == SDL_SCANCODE_ESCAPE)
			return this;
		selectingKeyBindingOption->setBoundKeyScancode(keyScancode);
		return newPauseState(parentState.get(), pauseMenu, pauseOption, nullptr);
	}

	int optionsCount = pauseMenu->getOptionsCount();
	switch (keyScancode) {
		case SDL_SCANCODE_ESCAPE:
			return nullptr;
		case SDL_SCANCODE_UP:
			return newPauseState(parentState.get(), pauseMenu, (pauseOption + optionsCount - 1) % optionsCount, nullptr);
		case SDL_SCANCODE_DOWN:
			return newPauseState(parentState.get(), pauseMenu, (pauseOption + 1) % optionsCount, nullptr);
		case SDL_SCANCODE_SPACE:
		case SDL_SCANCODE_RETURN:
			return pauseMenu->getOption(pauseOption)->handle(this);
		default:
			return this;
	}
}
//if we were given a menu, return a pause state with that pause menu, otherwise go up a level
PauseState* PauseState::navigateToMenu(PauseMenu* menu) {
	return menu != nullptr ? newPauseState(this, menu, 0, nullptr) : parentState.get();
}
//return a copy of this pause state set to listen for a key selection
PauseState* PauseState::beginKeySelection(KeyBindingOption* pSelectingKeyBindingOption) {
	return newPauseState(parentState.get(), pauseMenu, pauseOption, pSelectingKeyBindingOption);
}
//we're quitting the game, return a clone of this state that quits the game
PauseState* PauseState::produceQuitGameState() {
	PauseState* p = newPauseState(parentState.get(), pauseMenu, pauseOption, selectingKeyBindingOption);
	p->shouldQuitGame = true;
	return p;
}
//render the pause menu over the screen
void PauseState::render() {
	pauseMenu->render(pauseOption, selectingKeyBindingOption);
}