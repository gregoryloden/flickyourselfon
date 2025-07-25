#include "PauseState.h"
#include "Audio/Audio.h"
#include "GameState/GameState.h"
#include "GameState/MapState/MapState.h"
#include "GameState/KickAction.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

#define newPauseState(parentState, pauseMenu, pauseOption, selectingKeyBindingOption, selectedLevelN, endPauseDecision) \
	produceWithArgs(\
		PauseState, parentState, pauseMenu, pauseOption, selectingKeyBindingOption, selectedLevelN, endPauseDecision)
#define newPauseMenu(title, subtitles, options) newWithArgs(PauseState::PauseMenu, title, subtitles, options)
#define newLevelSelectMenu(title, options) newWithArgs(PauseState::LevelSelectMenu, title, options)
#define newNavigationOption(displayText, subMenu) newWithArgs(PauseState::NavigationOption, displayText, subMenu)
#define newControlsNavigationOption(displayText, subMenu) \
	newWithArgs(PauseState::ControlsNavigationOption, displayText, subMenu)
#define newNewGameOption() newWithoutArgs(PauseState::NewGameOption)
#define newKeyBindingOption(setting, displayPrefix) newWithArgs(PauseState::KeyBindingOption, setting, displayPrefix)
#define newDefaultKeyBindingsOption() newWithoutArgs(PauseState::DefaultKeyBindingsOption)
#define newAcceptKeyBindingsOption() newWithoutArgs(PauseState::AcceptKeyBindingsOption)
#define newMultiStateOption(setting, displayPrefix) newWithArgs(PauseState::MultiStateOption, setting, displayPrefix)
#define newVolumeSettingOption(setting, displayPrefix) newWithArgs(PauseState::VolumeSettingOption, setting, displayPrefix)
#define newValueSelectionOption(setting, displayPrefix) newWithArgs(PauseState::ValueSelectionOption, setting, displayPrefix)
#define newLevelSelectOption(displayText, levelN) newWithArgs(PauseState::LevelSelectOption, displayText, levelN)
#define newEndPauseOption(displayText, endPauseDecision) newWithArgs(PauseState::EndPauseOption, displayText, endPauseDecision)

//////////////////////////////// PauseState::PauseMenu ////////////////////////////////
PauseState::PauseMenu::PauseMenu(
	objCounterParametersComma() string pTitle, vector<string> pSubtitles, vector<PauseOption*> pOptions)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
title(pTitle)
, titleMetrics(Text::getMetrics(pTitle.c_str(), titleFontScale))
, subtitles(pSubtitles)
, subtitlesMetrics()
, titleAndSubtitlesTotalHeight(titleMetrics.getTotalHeight() - titleMetrics.topPadding)
, options(pOptions) {
	for (string& subtitle : subtitles) {
		subtitlesMetrics.push_back(Text::getMetrics(subtitle.c_str(), PauseOption::displayTextFontScale));
		titleAndSubtitlesTotalHeight += subtitlesMetrics.back().getTotalHeight();
	}
}
PauseState::PauseMenu::~PauseMenu() {
	for (PauseOption* option : options)
		delete option;
}
void PauseState::PauseMenu::getDisplayMetrics(
	KeyBindingOption* selectingKeyBindingOption, float* outMenuTop, vector<Text::Metrics>* outOptionsMetrics)
{
	float totalHeight = titleAndSubtitlesTotalHeight;
	for (PauseOption* option : options) {
		Text::Metrics optionMetrics = option == selectingKeyBindingOption
			? selectingKeyBindingOption->getSelectingDisplayTextMetrics(true)
			: option->getDisplayTextMetrics();
		totalHeight += optionMetrics.getTotalHeight();
		outOptionsMetrics->push_back(optionMetrics);
	}
	totalHeight -= outOptionsMetrics->back().bottomPadding;
	*outMenuTop = (Config::gameScreenHeight - totalHeight) * 0.5f;
}
void PauseState::PauseMenu::handleSelectOption(int pauseOption, int* outSelectedLevelN) {
	PauseOption* pauseOptionVal = options[pauseOption];
	if (pauseOptionVal->enabled)
		pauseOptionVal->handleSelect(outSelectedLevelN);
}
int PauseState::PauseMenu::findHighlightedOption(int mouseX, int mouseY, float* outOptionX) {
	float screenX = mouseX / Config::currentPixelWidth;
	float screenY = mouseY / Config::currentPixelHeight;
	float menuTop;
	vector<Text::Metrics> optionsMetrics;
	getDisplayMetrics(nullptr, &menuTop, &optionsMetrics);
	float optionMiddle = Config::gameScreenWidth * 0.5f;
	float optionTop = menuTop + titleAndSubtitlesTotalHeight;
	for (int i = 0; i < (int)optionsMetrics.size(); i++) {
		Text::Metrics& metrics = optionsMetrics[i];
		float optionBottom = optionTop + metrics.getTotalHeight();
		float halfWidth = metrics.charactersWidth * 0.5f;
		if (screenX >= optionMiddle - halfWidth
			&& screenX <= optionMiddle + halfWidth
			&& screenY >= optionTop
			&& screenY <= optionBottom)
		{
			*outOptionX = screenX - optionMiddle + halfWidth;
			return i;
		}
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
	float menuTop;
	vector<Text::Metrics> optionsMetrics;
	getDisplayMetrics(selectingKeyBindingOption, &menuTop, &optionsMetrics);

	//then render them all
	float screenCenterX = (float)Config::gameScreenWidth * 0.5f;
	float optionsBaseline = menuTop + titleMetrics.aboveBaseline;

	Text::render(title.c_str(), screenCenterX - titleMetrics.charactersWidth * 0.5f, optionsBaseline, titleFontScale);
	Text::Metrics* lastMetrics = &titleMetrics;

	for (int i = 0; i < (int)subtitles.size(); i++) {
		Text::Metrics& subtitleMetrics = subtitlesMetrics[i];
		optionsBaseline += subtitleMetrics.getBaselineDistanceBelow(lastMetrics);
		Text::render(
			subtitles[i].c_str(),
			screenCenterX - subtitleMetrics.charactersWidth * 0.5f,
			optionsBaseline,
			PauseOption::displayTextFontScale);
		lastMetrics = &subtitleMetrics;
	}

	bool wasEnabled = true;
	int optionsCount = (int)options.size();
	for (int i = 0; i < optionsCount; i++) {
		PauseOption* option = options[i];
		if (option->enabled != wasEnabled) {
			wasEnabled = option->enabled;
			Text::setRenderColor(1.0f, 1.0f, 1.0f, wasEnabled ? 1.0f : 0.5f);
		}

		Text::Metrics& optionMetrics = optionsMetrics[i];
		optionsBaseline += optionMetrics.getBaselineDistanceBelow(lastMetrics);
		float leftX = screenCenterX - optionMetrics.charactersWidth * 0.5f;
		if (option == selectingKeyBindingOption)
			selectingKeyBindingOption->renderSelecting(leftX, optionsBaseline, true);
		else
			option->render(leftX, optionsBaseline);

		if (i == selectedOption) {
			static constexpr float selectedOptionAngleBracketPadding = 4.0f;
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
	Text::setRenderColor(1.0f, 1.0f, 1.0f, 1.0f);
}

//////////////////////////////// PauseState::LevelSelectMenu ////////////////////////////////
PauseState::LevelSelectMenu::LevelSelectMenu(objCounterParametersComma() string pTitle, vector<PauseOption*> pOptions)
: PauseMenu(objCounterArgumentsComma() pTitle, {}, pOptions) {
}
PauseState::LevelSelectMenu::~LevelSelectMenu() {
}
void PauseState::LevelSelectMenu::enableDisableLevelOptions(int levelsUnlocked) {
	int levelCount = MapState::getLevelCount();
	for (int i = 1; i <= levelCount; i++)
		options[i - 1]->enabled = i <= levelsUnlocked;
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
PauseState* PauseState::PauseOption::handleWithX(PauseState* currentState, float x, bool isDrag) {
	return isDrag ? currentState : handle(currentState);
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

//////////////////////////////// PauseState::NewGameOption ////////////////////////////////
PauseState::NewGameOption::NewGameOption(objCounterParameters())
: NavigationOption(objCounterArgumentsComma() "new game", buildResetGameMenu())
, warnResetGame(true) {
}
PauseState::NewGameOption::~NewGameOption() {}
PauseState* PauseState::NewGameOption::handle(PauseState* currentState) {
	return warnResetGame
		? NavigationOption::handle(currentState)
		: currentState->produceEndPauseState((int)EndPauseDecision::Reset);
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
, displayPrefix(pDisplayPrefix + ": ") {
}
PauseState::MultiStateOption::~MultiStateOption() {}
PauseState* PauseState::MultiStateOption::handle(PauseState* currentState) {
	return handleSide(currentState, 1);
}
PauseState* PauseState::MultiStateOption::handleSide(PauseState* currentState, int direction) {
	setting->cycleState(direction);
	updateDisplayText(displayPrefix + setting->getSelectedOption());
	Config::saveSettings();
	return currentState;
}

//////////////////////////////// PauseState::VolumeSettingOption ////////////////////////////////
PauseState::VolumeSettingOption::VolumeSettingOption(
	objCounterParametersComma() ConfigTypes::VolumeSetting* pSetting, string pDisplayPrefix)
: PauseOption(objCounterArgumentsComma() pDisplayPrefix + ": " + getVolume(pSetting))
, setting(pSetting)
, displayPrefix(pDisplayPrefix + ": ")
, widthBeforeVolume(
	Text::getMetrics(displayPrefix.c_str(), displayTextFontScale).charactersWidth
		+ Text::getInterCharacterSpacing(displayTextFontScale))
, widthBeforeVolumeIncrements(
	//position 0 volume to be halfway between the bracket and the left volume bar
	widthBeforeVolume
		+ Text::getMetrics("[", displayTextFontScale).charactersWidth
		+ Text::getInterCharacterSpacing(displayTextFontScale) * 0.5f)
, volumeIncrementWidth(
	Text::getMetrics(".", displayTextFontScale).charactersWidth + Text::getInterCharacterSpacing(displayTextFontScale))
{
	//move the 0-volume line so that clicking the first bar sets the volume to 1
	//only clicking at the left bracket will set the volume to 0
	widthBeforeVolumeIncrements -= volumeIncrementWidth;
}
PauseState::VolumeSettingOption::~VolumeSettingOption() {}
string PauseState::VolumeSettingOption::getVolume(ConfigTypes::VolumeSetting* pSetting) {
	string result = "[";
	int i = 0;
	for (; i < pSetting->volume; i++)
		result += '|';
	for (; i < ConfigTypes::VolumeSetting::maxVolume; i++)
		result += '.';
	result += ']';
	return result;
}
void PauseState::VolumeSettingOption::applyVolume(int volume) {
	setting->volume = MathUtils::min(ConfigTypes::VolumeSetting::maxVolume, MathUtils::max(0, volume));
	updateDisplayText(displayPrefix + getVolume(setting));
	Audio::applyVolume();
	Config::saveSettings();
}
PauseState* PauseState::VolumeSettingOption::handleSide(PauseState* currentState, int direction) {
	applyVolume(setting->volume + direction);
	return currentState;
}
PauseState* PauseState::VolumeSettingOption::handleWithX(PauseState* currentState, float x, bool isDrag) {
	if (x > widthBeforeVolume) {
		int oldVolume = setting->volume;
		applyVolume((int)((x - widthBeforeVolumeIncrements) / volumeIncrementWidth));
		if (isDrag && oldVolume != setting->volume)
			Audio::selectSound->play(0);
	}
	return currentState;
}

//////////////////////////////// PauseState::ValueSelectionOption ////////////////////////////////
PauseState::ValueSelectionOption::ValueSelectionOption(
	objCounterParametersComma() ConfigTypes::ValueSelectionSetting* pSetting, string pDisplayPrefix)
: PauseOption(objCounterArgumentsComma() pDisplayPrefix + ": " + pSetting->getSelectedValueName())
, setting(pSetting)
, displayPrefix(pDisplayPrefix + ": ") {
}
PauseState::ValueSelectionOption::~ValueSelectionOption() {}
PauseState* PauseState::ValueSelectionOption::handleSide(PauseState* currentState, int direction) {
	setting->changeSelection(direction);
	updateDisplayText(displayPrefix + setting->getSelectedValueName());
	Config::saveSettings();
	return currentState;
}

//////////////////////////////// PauseState::LevelSelectOption ////////////////////////////////
PauseState::LevelSelectOption::LevelSelectOption(objCounterParametersComma() string pDisplayText, int pLevelN)
: PauseOption(objCounterArgumentsComma() pDisplayText)
, levelN(pLevelN) {
}
PauseState::LevelSelectOption::~LevelSelectOption() {}
PauseState* PauseState::LevelSelectOption::handle(PauseState* currentState) {
	return currentState->produceEndPauseState(
		(int)EndPauseDecision::SelectLevel | (currentState->isAtHomeMenu() ? (int)EndPauseDecision::Load : 0));
}
void PauseState::LevelSelectOption::handleSelect(int* outSelectedLevelN) {
	*outSelectedLevelN = levelN;
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
PauseState::LevelSelectMenu* PauseState::baseLevelSelectMenu = nullptr;
PauseState::PauseMenu* PauseState::homeMenu = nullptr;
PauseState::LevelSelectMenu* PauseState::homeLevelSelectMenu = nullptr;
PauseState::NewGameOption* PauseState::homeNewGameOption = nullptr;
PauseState::PauseState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, parentState(nullptr)
, pauseMenu(nullptr)
, pauseOption(0)
, selectingKeyBindingOption(nullptr)
, selectedLevelN(0)
, endPauseDecision(0) {
}
PauseState::~PauseState() {}
PauseState* PauseState::produce(
	objCounterParametersComma()
	PauseState* pParentState,
	PauseMenu* pCurrentPauseMenu,
	int pPauseOption,
	KeyBindingOption* pSelectingKeyBindingOption,
	int pSelectedLevelN,
	int pEndPauseDecision)
{
	initializeWithNewFromPool(p, PauseState)
	p->parentState.set(pParentState);
	p->pauseMenu = pCurrentPauseMenu;
	p->pauseOption = pPauseOption;
	p->selectingKeyBindingOption = pSelectingKeyBindingOption;
	p->selectedLevelN = pSelectedLevelN;
	p->endPauseDecision = pEndPauseDecision;
	return p;
}
PauseState* PauseState::produceBasePauseScreen(int levelsUnlocked) {
	baseLevelSelectMenu->enableDisableLevelOptions(levelsUnlocked);
	return newPauseState(nullptr, baseMenu, 0, nullptr, 0, 0);
}
PauseState* PauseState::produceHomeScreen(int levelsUnlocked) {
	homeLevelSelectMenu->enableDisableLevelOptions(levelsUnlocked);
	return newPauseState(nullptr, homeMenu, levelsUnlocked == 0 ? 2 : 0, nullptr, 0, 0);
}
pooledReferenceCounterDefineRelease(PauseState)
void PauseState::prepareReturnToPool() {
	parentState.clear();
}
void PauseState::loadMenus() {
	baseMenu = newPauseMenu(
		"Pause",
		{},
		{
			newNavigationOption("resume", nullptr) COMMA
			newEndPauseOption("save + resume", (int)EndPauseDecision::Save) COMMA
			newNavigationOption(
				"request hint",
				newPauseMenu(
					"Request Hint?",
					{},
					{
						newNavigationOption("cancel", nullptr) COMMA
						newEndPauseOption("request hint", (int)EndPauseDecision::RequestHint) COMMA
					})) COMMA
			buildOptionsMenuOption() COMMA
			buildLevelSelectMenuOption(&baseLevelSelectMenu) COMMA
			newNavigationOption("reset game", buildResetGameMenu()) COMMA
			newEndPauseOption("save + exit", (int)EndPauseDecision::Save | (int)EndPauseDecision::Exit) COMMA
			newEndPauseOption("exit", (int)EndPauseDecision::Exit) COMMA
		});
	homeMenu = newPauseMenu(
		GameState::titleGameName,
		{},
		{
			newEndPauseOption("continue", (int)EndPauseDecision::Load) COMMA
			buildLevelSelectMenuOption(&homeLevelSelectMenu) COMMA
			(homeNewGameOption = newNewGameOption()) COMMA
			buildOptionsMenuOption() COMMA
			newEndPauseOption("exit", (int)EndPauseDecision::Exit) COMMA
		});
}
PauseState::PauseOption* PauseState::buildOptionsMenuOption() {
	return newNavigationOption(
		"options",
		newPauseMenu(
			"Options",
			{},
			{
				newControlsNavigationOption(
					"player controls",
					newPauseMenu(
						"Player Controls",
						{},
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
						{},
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
						{},
						{
							newMultiStateOption(&Config::climbKickIndicator, "climb indicator") COMMA
							newMultiStateOption(&Config::fallKickIndicator, "fall indicator") COMMA
							newMultiStateOption(&Config::railKickIndicator, "rail indicator") COMMA
							newMultiStateOption(&Config::switchKickIndicator, "switch indicator") COMMA
							newMultiStateOption(&Config::resetSwitchKickIndicator, "reset switch indicator") COMMA
							newNavigationOption("back", nullptr) COMMA
						})) COMMA
				newNavigationOption(
					"visibility",
					newPauseMenu(
						"Visibility",
						{},
						{
							newMultiStateOption(&Config::showConnectionsMode, "show-connections mode") COMMA
							newMultiStateOption(&Config::heightBasedShading, "height-based shading") COMMA
							newMultiStateOption(&Config::showActivatedSwitchWaves, "show activated switch waves") COMMA
							newMultiStateOption(&Config::showBlockedFallEdges, "show blocked fall edges") COMMA
							newMultiStateOption(&Config::solutionBlockedWarning, "\"solution blocked\" warning") COMMA
							newNavigationOption("back", nullptr) COMMA
						})) COMMA
				newNavigationOption(
					"autosave",
					newPauseMenu(
						"Autosave",
						{},
						{
							newMultiStateOption(&Config::autosaveAtIntervalsEnabled, "autosave at intervals") COMMA
							newValueSelectionOption(&Config::autosaveInterval, "autosave interval") COMMA
							newMultiStateOption(&Config::autosaveEveryNewLevelEnabled, "autosave every new level") COMMA
							newNavigationOption("back", nullptr) COMMA
						})) COMMA
				newNavigationOption(
					"audio settings",
					newPauseMenu(
						"Audio Settings",
						{},
						{
							newVolumeSettingOption(&Config::masterVolume, "master") COMMA
							newVolumeSettingOption(&Config::musicVolume, "music") COMMA
							newVolumeSettingOption(&Config::soundsVolume, "sounds") COMMA
							newNavigationOption("back", nullptr) COMMA
						})) COMMA
				newNavigationOption("back", nullptr) COMMA
			}));
}
PauseState::PauseOption* PauseState::buildLevelSelectMenuOption(LevelSelectMenu** outLevelSelectMenu) {
	int levelCount = MapState::getLevelCount();
	vector<PauseOption*> options;
	for (int levelN = 1; levelN <= levelCount; levelN++)
		options.push_back(newLevelSelectOption("level " + to_string(levelN), levelN));
	options.push_back(newNavigationOption("back", nullptr));
	return newNavigationOption("level select", *outLevelSelectMenu = newLevelSelectMenu("Level Select", options));
}
PauseState::PauseMenu* PauseState::buildResetGameMenu() {
	return newPauseMenu(
		"Reset Game?",
		{
			"Erase all progress and" COMMA
			"start over from the beginning?" COMMA
			"Does not overwrite the save file" COMMA
			"until the next save/autosave" COMMA
			" " COMMA
		},
		{
			newNavigationOption("cancel", nullptr) COMMA
			newEndPauseOption("reset game", (int)EndPauseDecision::Reset) COMMA
		});
}
void PauseState::unloadMenus() {
	delete baseMenu;
	delete homeMenu;
}
void PauseState::disableContinueOptions() {
	homeMenu->getOption(0)->enabled = false;
	homeMenu->getOption(1)->enabled = false;
	homeNewGameOption->disableWarnResetGame();
}
bool PauseState::isAtHomeMenu() {
	for (PauseState* menuState = this; menuState != nullptr; menuState = menuState->parentState.get()) {
		if (menuState->pauseMenu == homeMenu)
			return true;
	}
	return false;
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
		return newPauseState(parentState.get(), pauseMenu, pauseOption, nullptr, 0, 0);
	}

	int optionsCount = pauseMenu->getOptionsCount();
	switch (keyScancode) {
		case SDL_SCANCODE_ESCAPE:
			if (isAtHomeMenu())
				return this;
			Audio::confirmSound->play(0);
			return nullptr;
		case SDL_SCANCODE_BACKSPACE:
			if (pauseMenu == homeMenu)
				return this;
			Audio::confirmSound->play(0);
			return navigateToMenu(nullptr);
		case SDL_SCANCODE_UP:
			return selectNewOption((pauseOption + optionsCount - 1) % optionsCount);
		case SDL_SCANCODE_DOWN:
			return selectNewOption((pauseOption + 1) % optionsCount);
		case SDL_SCANCODE_LEFT:
		case SDL_SCANCODE_RIGHT:
		case SDL_SCANCODE_RETURN: {
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
	float optionX;
	int newPauseOption = pauseMenu->findHighlightedOption((int)motionEvent.x, (int)motionEvent.y, &optionX);
	if (newPauseOption < 0)
		return this;
	return motionEvent.state != 0
		//a mouse button is down - handle movement if it's within the same pause option
		? newPauseOption == pauseOption ? pauseMenu->getOption(newPauseOption)->handleWithX(this, optionX, true) : this
		//no mouse button is down - select an option if it's different
		: newPauseOption != pauseOption ? selectNewOption(newPauseOption) : this;
}
PauseState* PauseState::handleMouseClick(SDL_MouseButtonEvent clickEvent) {
	//can't change selection while selecting a key binding
	if (selectingKeyBindingOption != nullptr)
		return this;
	float optionX;
	int clickPauseOption = pauseMenu->findHighlightedOption((int)clickEvent.x, (int)clickEvent.y, &optionX);
	if (clickPauseOption < 0)
		return this;
	PauseOption* pauseOptionVal = pauseMenu->getOption(clickPauseOption);
	if (!pauseOptionVal->enabled)
		return this;
	Audio::confirmSound->play(0);
	return pauseOptionVal->handleWithX(this, optionX, false);
}
PauseState* PauseState::selectNewOption(int newPauseOption) {
	Audio::selectSound->play(0);
	int newSelectedLevelN = 0;
	pauseMenu->handleSelectOption(newPauseOption, &newSelectedLevelN);
	return newPauseState(parentState.get(), pauseMenu, newPauseOption, nullptr, newSelectedLevelN, 0);
}
PauseState* PauseState::navigateToMenu(PauseMenu* menu) {
	if (menu != nullptr) {
		int newSelectedLevelN = 0;
		menu->handleSelectOption(0, &newSelectedLevelN);
		return newPauseState(this, menu, 0, nullptr, newSelectedLevelN, 0);
	}
	return parentState.get();
}
PauseState* PauseState::beginKeySelection(KeyBindingOption* pSelectingKeyBindingOption) {
	return newPauseState(parentState.get(), pauseMenu, pauseOption, pSelectingKeyBindingOption, 0, 0);
}
PauseState* PauseState::produceEndPauseState(int pEndPauseDecision) {
	return newPauseState(
		parentState.get(), pauseMenu, pauseOption, selectingKeyBindingOption, selectedLevelN, pEndPauseDecision);
}
void PauseState::render() {
	pauseMenu->render(pauseOption, selectingKeyBindingOption);
}
