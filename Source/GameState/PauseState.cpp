#include "PauseState.h"
#include "Audio/Audio.h"
#include "GameState/GameState.h"
#include "GameState/MapState/MapState.h"
#include "GameState/KickAction.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

#define newPauseState(parentState, pauseMenu, pauseOption, selectingKey, endPauseDecision) \
	produceWithArgs(PauseState, parentState, pauseMenu, pauseOption, selectingKey, endPauseDecision)
#define newPauseMenu(title, options) newWithArgs(PauseState::PauseMenu, title, options)
#define newNavigationOption(displayText, subMenu) newWithArgs(PauseState::NavigationOption, displayText, subMenu)
#define newControlsNavigationOption(displayText, subMenu) \
	newWithArgs(PauseState::ControlsNavigationOption, displayText, subMenu)
#define newKeyBindingOption(setting, displayPrefix) newWithArgs(PauseState::KeyBindingOption, setting, displayPrefix)
#define newDefaultKeyBindingsOption() newWithoutArgs(PauseState::DefaultKeyBindingsOption)
#define newAcceptKeyBindingsOption() newWithoutArgs(PauseState::AcceptKeyBindingsOption)
#define newMultiStateOption(setting, displayPrefix) newWithArgs(PauseState::MultiStateOption, setting, displayPrefix)
#define newVolumeSettingOption(setting, displayPrefix) newWithArgs(PauseState::VolumeSettingOption, setting, displayPrefix)
#define newEndPauseOption(displayText, endPauseDecision) newWithArgs(PauseState::EndPauseOption, displayText, endPauseDecision)

//////////////////////////////// PauseState::PauseMenu ////////////////////////////////
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
void PauseState::PauseMenu::getTotalHeightAndMetrics(
	KeyBindingOption* selectingKeyBindingOption, float* outTotalHeight, vector<Text::Metrics>* outOptionsMetrics)
{
	float totalHeight = titleMetrics.getTotalHeight();
	for (PauseOption* option : options) {
		Text::Metrics optionMetrics = option == selectingKeyBindingOption
			? selectingKeyBindingOption->getSelectingDisplayTextMetrics(true)
			: option->getDisplayTextMetrics();
		totalHeight += optionMetrics.getTotalHeight();
		outOptionsMetrics->push_back(optionMetrics);
	}
	*outTotalHeight = totalHeight - titleMetrics.topPadding - outOptionsMetrics->back().bottomPadding;
}
int PauseState::PauseMenu::findHighlightedOption(int mouseX, int mouseY) {
	float screenX = mouseX / Config::currentPixelWidth;
	float screenY = mouseY / Config::currentPixelHeight;
	float totalHeight;
	vector<Text::Metrics> optionsMetrics;
	getTotalHeightAndMetrics(nullptr, &totalHeight, &optionsMetrics);
	float optionMiddle = Config::gameScreenWidth * 0.5f;
	float optionTop = (Config::gameScreenHeight - totalHeight) * 0.5f + titleMetrics.getTotalHeight() - titleMetrics.topPadding;
	for (int i = 0; i < (int)optionsMetrics.size(); i++) {
		Text::Metrics& metrics = optionsMetrics[i];
		float optionBottom = optionTop + metrics.getTotalHeight();
		float halfWidth = metrics.charactersWidth * 0.5f;
		if (screenX >= optionMiddle - halfWidth
				&& screenX <= optionMiddle + halfWidth
				&& screenY >= optionTop
				&& screenY <= optionBottom)
			return i;
		optionTop = optionBottom;
	}
	return -1;
}
void PauseState::PauseMenu::loadAffectedKeyBindingSettings(vector<ConfigTypes::KeyBindingSetting*>* affectedSettings) {
	for (PauseOption* option : options)
		option->loadAffectedKeyBindingSetting(affectedSettings);
}
void PauseState::PauseMenu::render(int selectedOption, KeyBindingOption* selectingKeyBindingOption) {
	//render a translucent rectangle
	SpriteSheet::renderFilledRectangle(
		0.0f, 0.0f, 0.0f, 0.5f, 0, 0, (GLint)Config::gameScreenWidth, (GLint)Config::gameScreenHeight);

	//first find the height of the pause options so that we can vertically center them
	float totalHeight;
	vector<Text::Metrics> optionsMetrics;
	getTotalHeightAndMetrics(selectingKeyBindingOption, &totalHeight, &optionsMetrics);

	//then render them all
	float screenCenterX = (float)Config::gameScreenWidth * 0.5f;
	float optionsBaseline = ((float)Config::gameScreenHeight - totalHeight) * 0.5f + titleMetrics.aboveBaseline;

	Text::render(title.c_str(), screenCenterX - titleMetrics.charactersWidth * 0.5f, optionsBaseline, titleFontScale);
	Text::Metrics* lastMetrics = &titleMetrics;

	bool wasEnabled = true;
	int optionsCount = (int)options.size();
	for (int i = 0; i < optionsCount; i++) {
		PauseOption* option = options[i];
		if (option->enabled != wasEnabled) {
			wasEnabled = option->enabled;
			glColor4f(1.0f, 1.0f, 1.0f, wasEnabled ? 1.0f : 0.5f);
		}

		Text::Metrics& optionMetrics = optionsMetrics[i];
		optionsBaseline += optionMetrics.getBaselineDistanceBelow(lastMetrics);
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

		lastMetrics = &optionMetrics;
	}
}

//////////////////////////////// PauseState::PauseOption ////////////////////////////////
PauseState::PauseOption::PauseOption(objCounterParametersComma() string pDisplayText)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
displayText(pDisplayText)
, displayTextMetrics(Text::getMetrics(pDisplayText.c_str(), displayTextFontScale))
, enabled(true) {
}
PauseState::PauseOption::~PauseOption() {}
void PauseState::PauseOption::render(float leftX, float baselineY) {
	Text::render(displayText.c_str(), leftX, baselineY, displayTextFontScale);
}
void PauseState::PauseOption::updateDisplayText(const string& newDisplayText) {
	displayText = newDisplayText;
	displayTextMetrics = Text::getMetrics(newDisplayText.c_str(), displayTextFontScale);
}

//////////////////////////////// PauseState::NavigationOption ////////////////////////////////
PauseState::NavigationOption::NavigationOption(objCounterParametersComma() string pDisplayText, PauseMenu* pSubMenu)
: PauseOption(objCounterArgumentsComma() pDisplayText)
, subMenu(pSubMenu) {
}
PauseState::NavigationOption::~NavigationOption() {
	delete subMenu;
}
PauseState* PauseState::NavigationOption::handle(PauseState* currentState) {
	return currentState->navigateToMenu(subMenu);
}

//////////////////////////////// PauseState::ControlsNavigationOption ////////////////////////////////
PauseState::ControlsNavigationOption::ControlsNavigationOption(
	objCounterParametersComma() string pDisplayText, PauseMenu* pSubMenu)
: NavigationOption(objCounterArgumentsComma() pDisplayText, pSubMenu)
, affectedSettings() {
	pSubMenu->loadAffectedKeyBindingSettings(&affectedSettings);
}
PauseState::ControlsNavigationOption::~ControlsNavigationOption() {}
PauseState* PauseState::ControlsNavigationOption::handle(PauseState* currentState) {
	for (ConfigTypes::KeyBindingSetting* keyBindingSetting : Config::allKeyBindingSettings)
		keyBindingSetting->editingValue = SDL_SCANCODE_UNKNOWN;
	for (ConfigTypes::KeyBindingSetting* keyBindingSetting : affectedSettings)
		keyBindingSetting->editingValue = keyBindingSetting->value;
	return NavigationOption::handle(currentState);
}

//////////////////////////////// PauseState::KeyBindingOption ////////////////////////////////
float PauseState::KeyBindingOption::cachedKeySelectingTextWidth = 0.0f;
float PauseState::KeyBindingOption::cachedKeySelectingTextFontScale = 0.0f;
PauseState::KeyBindingOption::KeyBindingOption(
	objCounterParametersComma() ConfigTypes::KeyBindingSetting* pSetting, string displayPrefix)
: PauseOption(objCounterArgumentsComma() displayPrefix + ":")
, setting(pSetting)
, cachedKeyScancode(SDL_SCANCODE_UNKNOWN)
, cachedKeyName("")
, cachedKeyTextMetrics()
, cachedKeyBackgroundMetrics() {
}
PauseState::KeyBindingOption::~KeyBindingOption() {}
Text::Metrics PauseState::KeyBindingOption::getDisplayTextMetrics() {
	return getSelectingDisplayTextMetrics(false);
}
Text::Metrics PauseState::KeyBindingOption::getSelectingDisplayTextMetrics(bool selecting) {
	Text::Metrics metrics = PauseOption::getDisplayTextMetrics();
	ensureCachedKeyMetrics();

	metrics.charactersWidth +=
		(float)interKeyActionAndKeyBackgroundSpacing
			+ (selecting ? cachedKeySelectingTextWidth : cachedKeyBackgroundMetrics.charactersWidth);
	metrics.aboveBaseline = cachedKeyBackgroundMetrics.aboveBaseline;
	metrics.belowBaseline = cachedKeyBackgroundMetrics.belowBaseline;
	metrics.topPadding = cachedKeyBackgroundMetrics.topPadding;
	metrics.bottomPadding = cachedKeyBackgroundMetrics.bottomPadding;
	return metrics;
}
PauseState* PauseState::KeyBindingOption::handle(PauseState* currentState) {
	return currentState->beginKeySelection(this);
}
void PauseState::KeyBindingOption::render(float leftX, float baselineY) {
	renderSelecting(leftX, baselineY, false);
}
void PauseState::KeyBindingOption::renderSelecting(float leftX, float baselineY, bool selecting) {
	//render the action
	PauseOption::render(leftX, baselineY);

	ensureCachedKeyMetrics();
	leftX += PauseOption::getDisplayTextMetrics().charactersWidth + (float)interKeyActionAndKeyBackgroundSpacing;

	//render the key-selecting text
	if (selecting)
		Text::render(keySelectingText, leftX, baselineY, cachedKeySelectingTextFontScale);
	//render the key background and name
	else
		Text::renderWithKeyBackgroundWithMetrics(
			cachedKeyName.c_str(), leftX, baselineY, &cachedKeyTextMetrics, &cachedKeyBackgroundMetrics);
}
void PauseState::KeyBindingOption::ensureCachedKeyMetrics() {
	SDL_Scancode currentBoundKeyScancode = setting->editingValue;
	if (currentBoundKeyScancode != cachedKeyScancode) {
		cachedKeyScancode = currentBoundKeyScancode;
		cachedKeyName = ConfigTypes::KeyBindingSetting::getKeyName(currentBoundKeyScancode);
		cachedKeyTextMetrics = Text::getMetrics(cachedKeyName.c_str(), PauseOption::getDisplayTextMetrics().fontScale);
		cachedKeyBackgroundMetrics = Text::getKeyBackgroundMetrics(&cachedKeyTextMetrics);
	}
	if (cachedKeySelectingTextFontScale != cachedKeyTextMetrics.fontScale) {
		cachedKeySelectingTextFontScale = cachedKeyTextMetrics.fontScale;
		cachedKeySelectingTextWidth = Text::getMetrics(keySelectingText, cachedKeySelectingTextFontScale).charactersWidth;
	}
}
void PauseState::KeyBindingOption::setKeyBindingSettingEditingScancode(SDL_Scancode keyScancode) {
	setting->editingValue = keyScancode;
}

//////////////////////////////// PauseState::DefaultKeyBindingsOption ////////////////////////////////
PauseState::DefaultKeyBindingsOption::DefaultKeyBindingsOption(objCounterParameters())
: PauseOption(objCounterArgumentsComma() "defaults") {
}
PauseState::DefaultKeyBindingsOption::~DefaultKeyBindingsOption() {}
PauseState* PauseState::DefaultKeyBindingsOption::handle(PauseState* currentState) {
	for (ConfigTypes::KeyBindingSetting* keyBindingSetting : Config::allKeyBindingSettings) {
		if (keyBindingSetting->editingValue != SDL_SCANCODE_UNKNOWN)
			keyBindingSetting->editingValue = keyBindingSetting->defaultValue;
	}
	return currentState;
}

//////////////////////////////// PauseState::AcceptKeyBindingsOption ////////////////////////////////
PauseState::AcceptKeyBindingsOption::AcceptKeyBindingsOption(objCounterParameters())
: PauseOption(objCounterArgumentsComma() "accept") {
}
PauseState::AcceptKeyBindingsOption::~AcceptKeyBindingsOption() {}
PauseState* PauseState::AcceptKeyBindingsOption::handle(PauseState* currentState) {
	for (ConfigTypes::KeyBindingSetting* keyBindingSetting : Config::allKeyBindingSettings) {
		if (keyBindingSetting->editingValue != SDL_SCANCODE_UNKNOWN)
			keyBindingSetting->value = keyBindingSetting->editingValue;
	}
	Config::saveSettings();
	return currentState->navigateToMenu(nullptr);
}

//////////////////////////////// PauseState::MultiStateOption ////////////////////////////////
PauseState::MultiStateOption::MultiStateOption(
	objCounterParametersComma() ConfigTypes::MultiStateSetting* pSetting, string pDisplayPrefix)
: PauseOption(objCounterArgumentsComma() pDisplayPrefix + ": " + pSetting->getSelectedOption())
, setting(pSetting)
, displayPrefix(pDisplayPrefix) {
}
PauseState::MultiStateOption::~MultiStateOption() {}
PauseState* PauseState::MultiStateOption::handle(PauseState* currentState) {
	return handleSide(currentState, 1);
}
PauseState* PauseState::MultiStateOption::handleSide(PauseState* currentState, int direction) {
	setting->cycleState(direction);
	updateDisplayText(displayPrefix + ": " + setting->getSelectedOption());
	Config::saveSettings();
	return currentState;
}

//////////////////////////////// PauseState::VolumeSettingOption ////////////////////////////////
PauseState::VolumeSettingOption::VolumeSettingOption(
	objCounterParametersComma() ConfigTypes::VolumeSetting* pSetting, string pDisplayPrefix)
: PauseOption(objCounterArgumentsComma() pDisplayPrefix + ": " + getVolume(pSetting))
, setting(pSetting)
, displayPrefix(pDisplayPrefix) {
}
PauseState::VolumeSettingOption::~VolumeSettingOption() {}
string PauseState::VolumeSettingOption::getVolume(ConfigTypes::VolumeSetting* pSetting) {
	string result = "";
	int i = 0;
	for (; i < pSetting->volume; i++)
		result += '|';
	for (; i < ConfigTypes::VolumeSetting::maxVolume; i++)
		result += '.';
	return result;
}
PauseState* PauseState::VolumeSettingOption::handleSide(PauseState* currentState, int direction) {
	setting->volume = MathUtils::min(ConfigTypes::VolumeSetting::maxVolume, MathUtils::max(0, setting->volume + direction));
	updateDisplayText(displayPrefix + ": " + getVolume(setting));
	Audio::applyVolume();
	Config::saveSettings();
	return currentState;
}

//////////////////////////////// PauseState::EndPauseOption ////////////////////////////////
PauseState::EndPauseOption::EndPauseOption(objCounterParametersComma() string pDisplayText, int pEndPauseDecision)
: PauseOption(objCounterArgumentsComma() pDisplayText)
, endPauseDecision(pEndPauseDecision) {
}
PauseState::EndPauseOption::~EndPauseOption() {}
PauseState* PauseState::EndPauseOption::handle(PauseState* currentState) {
	return currentState->produceEndPauseState(endPauseDecision);
}

//////////////////////////////// PauseState ////////////////////////////////
PauseState::PauseMenu* PauseState::baseMenu = nullptr;
PauseState::PauseMenu* PauseState::homeMenu = nullptr;
PauseState::PauseState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, parentState(nullptr)
, pauseMenu(nullptr)
, pauseOption(0)
, selectingKeyBindingOption(nullptr)
, endPauseDecision(0) {
}
PauseState::~PauseState() {}
PauseState* PauseState::produce(
	objCounterParametersComma()
	PauseState* pParentState,
	PauseMenu* pCurrentPauseMenu,
	int pPauseOption,
	KeyBindingOption* pSelectingKeyBindingOption,
	int pEndPauseDecision)
{
	initializeWithNewFromPool(p, PauseState)
	p->parentState.set(pParentState);
	p->pauseMenu = pCurrentPauseMenu;
	p->pauseOption = pPauseOption;
	p->selectingKeyBindingOption = pSelectingKeyBindingOption;
	p->endPauseDecision = pEndPauseDecision;
	return p;
}
PauseState* PauseState::produceBasePauseScreen() {
	return newPauseState(nullptr, baseMenu, 0, nullptr, 0);
}
PauseState* PauseState::produceHomeScreen(bool selectNewGame) {
	return newPauseState(nullptr, homeMenu, selectNewGame ? 2 : 0, nullptr, 0);
}
pooledReferenceCounterDefineRelease(PauseState)
void PauseState::prepareReturnToPool() {
	parentState.set(nullptr);
}
void PauseState::loadMenus() {
	baseMenu = newPauseMenu(
		"Pause",
		{
			newNavigationOption("resume", nullptr) COMMA
			newEndPauseOption("save + resume", (int)EndPauseDecision::Save) COMMA
			newNavigationOption(
				"request hint",
				newPauseMenu(
					"Request Hint?",
					{
						newNavigationOption("cancel", nullptr) COMMA
						newEndPauseOption("request hint", (int)EndPauseDecision::RequestHint) COMMA
					})) COMMA
			buildOptionsMenuOption() COMMA
			buildLevelSelectMenuOption() COMMA
			newNavigationOption(
				"reset game",
				newPauseMenu(
					"Reset Game?",
					{
						newNavigationOption("cancel", nullptr) COMMA
						newEndPauseOption("reset game", (int)EndPauseDecision::Reset) COMMA
					})) COMMA
			newEndPauseOption("save + exit", (int)EndPauseDecision::Save | (int)EndPauseDecision::Exit) COMMA
			newEndPauseOption("exit", (int)EndPauseDecision::Exit) COMMA
		});
	homeMenu = newPauseMenu(
		GameState::titleGameName,
		{
			newEndPauseOption("continue", (int)EndPauseDecision::Load) COMMA
			buildLevelSelectMenuOption() COMMA
			newEndPauseOption("new game", (int)EndPauseDecision::Reset) COMMA
			buildOptionsMenuOption() COMMA
			newEndPauseOption("exit", (int)EndPauseDecision::Exit) COMMA
		});
}
PauseState::PauseOption* PauseState::buildOptionsMenuOption() {
	return newNavigationOption(
		"options",
		newPauseMenu(
			"Options",
			{
				newControlsNavigationOption(
					"player controls",
					newPauseMenu(
						"Player Controls",
						{
							newKeyBindingOption(&Config::upKeyBinding, "up") COMMA
							newKeyBindingOption(&Config::rightKeyBinding, "right") COMMA
							newKeyBindingOption(&Config::downKeyBinding, "down") COMMA
							newKeyBindingOption(&Config::leftKeyBinding, "left") COMMA
							newKeyBindingOption(&Config::kickKeyBinding, "kick/climb/fall") COMMA
							newKeyBindingOption(&Config::sprintKeyBinding, "sprint") COMMA
							newDefaultKeyBindingsOption() COMMA
							newAcceptKeyBindingsOption() COMMA
							newNavigationOption("back", nullptr) COMMA
						})) COMMA
				newControlsNavigationOption(
					"map controls",
					newPauseMenu(
						"Map Controls",
						{
							newKeyBindingOption(&Config::undoKeyBinding, "undo") COMMA
							newKeyBindingOption(&Config::redoKeyBinding, "redo") COMMA
							newKeyBindingOption(&Config::showConnectionsKeyBinding, "show connections") COMMA
							newKeyBindingOption(&Config::mapCameraKeyBinding, "map camera") COMMA
							newDefaultKeyBindingsOption() COMMA
							newAcceptKeyBindingsOption() COMMA
							newNavigationOption("back", nullptr) COMMA
						})) COMMA
				newNavigationOption(
					"kick indicators",
					newPauseMenu(
						"Kick Indicators",
						{
							newMultiStateOption(&Config::climbKickIndicator, "climb indicator") COMMA
							newMultiStateOption(&Config::fallKickIndicator, "fall indicator") COMMA
							newMultiStateOption(&Config::railKickIndicator, "rail indicator") COMMA
							newMultiStateOption(&Config::switchKickIndicator, "switch indicator") COMMA
							newMultiStateOption(&Config::resetSwitchKickIndicator, "reset switch indicator") COMMA
							newNavigationOption("back", nullptr) COMMA
						})) COMMA
				newNavigationOption(
					"accessibility",
					newPauseMenu(
						"Accessibility",
						{
							newMultiStateOption(&Config::showConnectionsMode, "show-connections mode") COMMA
							newMultiStateOption(&Config::heightBasedShading, "height-based shading") COMMA
							newNavigationOption("back", nullptr) COMMA
						})) COMMA
				newNavigationOption(
					"audio settings",
					newPauseMenu(
						"Audio Settings",
						{
							newVolumeSettingOption(&Config::masterVolume, "master") COMMA
							newVolumeSettingOption(&Config::musicVolume, "music") COMMA
							newVolumeSettingOption(&Config::soundsVolume, "sounds") COMMA
							newNavigationOption("back", nullptr) COMMA
						})) COMMA
				newNavigationOption("back", nullptr) COMMA
			}));
}
PauseState::PauseOption* PauseState::buildLevelSelectMenuOption() {
	vector<PauseOption*> options;
	for (int i = 1; i <= levelCount; i++) {
		stringstream s;
		s << "level " << i;
		options.push_back(newNavigationOption(s.str(), nullptr));
	}
	options.push_back(newNavigationOption("back", nullptr));
	return newNavigationOption("level select", newPauseMenu("Level Select", options));
}
void PauseState::unloadMenus() {
	delete baseMenu;
	delete homeMenu;
}
void PauseState::disableContinueOptions() {
	homeMenu->getOption(0)->enabled = false;
	homeMenu->getOption(1)->enabled = false;
}
PauseState* PauseState::getNextPauseState() {
	PauseState* nextPauseState = this;
	//handle events
	SDL_Event gameEvent;
	while (SDL_PollEvent(&gameEvent) != 0) {
		PauseState* lastPauseState = nextPauseState;

		if (gameEvent.type == SDL_QUIT)
			nextPauseState = nextPauseState->produceEndPauseState((int)EndPauseDecision::Exit);
		else if (gameEvent.type == SDL_KEYDOWN)
			nextPauseState = nextPauseState->handleKeyPress(gameEvent.key.keysym.scancode);
		else if (gameEvent.type == SDL_MOUSEMOTION)
			nextPauseState = nextPauseState->handleMouseMotion(gameEvent.motion);
		else if (gameEvent.type == SDL_MOUSEBUTTONDOWN)
			nextPauseState = nextPauseState->handleMouseClick(gameEvent.button);

		//if we got a new pause state, make sure the old one is returned to the pool if this is its only reference
		if (nextPauseState != lastPauseState)
			ReferenceCounterHolder<PauseState> lastPauseStateHolder (lastPauseState);
		if (nextPauseState == nullptr || (nextPauseState->endPauseDecision & (int)EndPauseDecision::Exit) != 0)
			return nextPauseState;
	}
	return nextPauseState;
}
PauseState* PauseState::handleKeyPress(SDL_Scancode keyScancode) {
	if (selectingKeyBindingOption != nullptr) {
		if (keyScancode != SDL_SCANCODE_ESCAPE)
			selectingKeyBindingOption->setKeyBindingSettingEditingScancode(keyScancode);
		Audio::confirmSound->play(0);
		return newPauseState(parentState.get(), pauseMenu, pauseOption, nullptr, 0);
	}

	int optionsCount = pauseMenu->getOptionsCount();
	switch (keyScancode) {
		case SDL_SCANCODE_ESCAPE:
			for (PauseState* menuState = this; menuState != nullptr; menuState = menuState->parentState.get()) {
				if (menuState->pauseMenu == homeMenu)
					return this;
			}
			Audio::confirmSound->play(0);
			return nullptr;
		case SDL_SCANCODE_BACKSPACE:
			if (pauseMenu == homeMenu)
				return this;
			Audio::confirmSound->play(0);
			return navigateToMenu(nullptr);
		case SDL_SCANCODE_UP:
			Audio::selectSound->play(0);
			return newPauseState(parentState.get(), pauseMenu, (pauseOption + optionsCount - 1) % optionsCount, nullptr, 0);
		case SDL_SCANCODE_DOWN:
			Audio::selectSound->play(0);
			return newPauseState(parentState.get(), pauseMenu, (pauseOption + 1) % optionsCount, nullptr, 0);
		case SDL_SCANCODE_LEFT:
		case SDL_SCANCODE_RIGHT:
		case SDL_SCANCODE_RETURN:
		{
			PauseOption* pauseOptionVal = pauseMenu->getOption(pauseOption);
			if (!pauseOptionVal->enabled)
				return this;
			Audio::confirmSound->play(0);
			if (keyScancode == SDL_SCANCODE_LEFT)
				return pauseOptionVal->handleSide(this, -1);
			else if (keyScancode == SDL_SCANCODE_RIGHT)
				return pauseOptionVal->handleSide(this, 1);
			else
				return pauseOptionVal->handle(this);
		}
		default:
			//allow the kick button to confirm menu options, as long as the button isn't already in use
			if (keyScancode == Config::kickKeyBinding.value)
				return handleKeyPress(SDL_SCANCODE_RETURN);
			return this;
	}
}
PauseState* PauseState::handleMouseMotion(SDL_MouseMotionEvent motionEvent) {
	//can't change selection while selecting a key binding
	if (selectingKeyBindingOption != nullptr)
		return this;
	int newPauseOption = pauseMenu->findHighlightedOption((int)motionEvent.x, (int)motionEvent.y);
	if (newPauseOption < 0 || newPauseOption == pauseOption)
		return this;
	Audio::selectSound->play(0);
	return newPauseState(parentState.get(), pauseMenu, newPauseOption, nullptr, 0);
}
PauseState* PauseState::handleMouseClick(SDL_MouseButtonEvent clickEvent) {
	//can't change selection while selecting a key binding
	if (selectingKeyBindingOption != nullptr)
		return this;
	int clickPauseOption = pauseMenu->findHighlightedOption((int)clickEvent.x, (int)clickEvent.y);
	if (clickPauseOption < 0)
		return this;
	PauseOption* pauseOptionVal = pauseMenu->getOption(clickPauseOption);
	if (!pauseOptionVal->enabled)
		return this;
	Audio::confirmSound->play(0);
	return pauseOptionVal->handle(this);
}
PauseState* PauseState::navigateToMenu(PauseMenu* menu) {
	return menu != nullptr ? newPauseState(this, menu, 0, nullptr, 0) : parentState.get();
}
PauseState* PauseState::beginKeySelection(KeyBindingOption* pSelectingKeyBindingOption) {
	return newPauseState(parentState.get(), pauseMenu, pauseOption, pSelectingKeyBindingOption, 0);
}
PauseState* PauseState::produceEndPauseState(int pEndPauseDecision) {
	return newPauseState(parentState.get(), pauseMenu, pauseOption, selectingKeyBindingOption, pEndPauseDecision);
}
void PauseState::render() {
	pauseMenu->render(pauseOption, selectingKeyBindingOption);
}
