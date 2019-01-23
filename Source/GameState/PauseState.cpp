#include "PauseState.h"

#define newPauseState(parentState, pauseMenu, pauseOption) produceWithArgs(PauseState, parentState, pauseMenu, pauseOption)
#define newPauseMenu(title, options) newWithArgs(PauseState::PauseMenu, title, options)
#define newNavigationOption(displayText, subMenu) newWithArgs(PauseState::NavigationOption, displayText, subMenu)

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
void PauseState::PauseMenu::render(int selectedOption) {
	//TODO: do something with these arrows
	char* arrows = u8"←↑→↓";

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
		Text::Metrics optionMetrics = option->getDisplayTextMetrics();
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
		Text::Metrics& optionMetrics = optionsMetrics[i];
		optionsBaseline += optionMetrics.topPadding + optionMetrics.aboveBaseline;
		float leftX = screenCenterX - optionMetrics.charactersWidth * 0.5f;
		options[i]->render(leftX, optionsBaseline);

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
PauseState::PauseOption::PauseOption(objCounterParameters())
onlyInDebug(: ObjCounter(objCounterArguments())) {
}
PauseState::PauseOption::~PauseOption() {}


//////////////////////////////// PauseState::NavigationOption ////////////////////////////////
PauseState::NavigationOption::NavigationOption(objCounterParametersComma() string pDisplayText, PauseMenu* pSubMenu)
: PauseOption(objCounterArguments())
, displayText(pDisplayText)
, displayTextMetrics(Text::getMetrics(pDisplayText.c_str(), displayTextFontScale))
, subMenu(pSubMenu) {
}
PauseState::NavigationOption::~NavigationOption() {
	delete subMenu;
}
//update this pause state as appropriate
PauseState* PauseState::NavigationOption::handle(PauseState* currentState) {
	return currentState->navigateToMenu(subMenu);
}
//render the NavigationOption
void PauseState::NavigationOption::render(float leftX, float baselineY) {
	Text::render(displayText.c_str(), leftX, baselineY, displayTextFontScale);
}

//////////////////////////////// PauseState ////////////////////////////////
PauseState::PauseMenu* PauseState::baseMenu = nullptr;
PauseState::PauseState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, parentState(nullptr)
, pauseMenu(nullptr)
, pauseOption(0)
, shouldQuitGame(false) {
}
PauseState::~PauseState() {}
//initialize and return a PauseState
PauseState* PauseState::produce(
	objCounterParametersComma() PauseState* pParentState, PauseMenu* pCurrentPauseMenu, int pPauseOption)
{
	initializeWithNewFromPool(p, PauseState)
	p->parentState.set(pParentState);
	p->pauseMenu = pCurrentPauseMenu;
	p->pauseOption = pPauseOption;
	return p;
}
//return a new pause state at the base menu
PauseState* PauseState::produce(objCounterParameters()) {
	return produce(objCounterArgumentsComma() nullptr, baseMenu, 0);
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
			newNavigationOption("controls", newPauseMenu("Controls", { newNavigationOption("back", nullptr) })) COMMA
			newNavigationOption("exit", nullptr)
		});
}
//delete the base menu
void PauseState::unloadMenu() {
	delete baseMenu;
}
//if any keys were pressed, return a new updated pause state, otherwise return this non-updated state
PauseState* PauseState::getNextPauseState() {
	//handle events
	PauseState* nextPauseState = this;
	SDL_Event gameEvent;
	while (SDL_PollEvent(&gameEvent) != 0) {
		switch (gameEvent.type) {
			case SDL_QUIT:
				nextPauseState->shouldQuitGame = true;
				return nextPauseState;
			case SDL_KEYDOWN: {
				if (gameEvent.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
					return nullptr;
				else if (nextPauseState == this)
					nextPauseState = newPauseState(parentState.get(), pauseMenu, pauseOption);

				int optionsCount = nextPauseState->pauseMenu->getOptionsCount();
				switch (gameEvent.key.keysym.scancode) {
					case SDL_SCANCODE_ESCAPE:
						return nullptr;
					case SDL_SCANCODE_UP:
						nextPauseState->pauseOption = (nextPauseState->pauseOption + optionsCount - 1) % optionsCount;
						break;
					case SDL_SCANCODE_DOWN:
						nextPauseState->pauseOption = (nextPauseState->pauseOption + 1) % optionsCount;
						break;
					default:
						break;
				}
				break;
			}
			default:
				break;
		}
	}
	return nextPauseState;
}
//if we were given a menu, return a pause state with that pause menu, otherwise go up a level
PauseState* PauseState::navigateToMenu(PauseMenu* menu) {
	return menu != nullptr ? produceWithArgs(PauseState, this, menu, 0) : parentState.get();
}
//render the pause menu over the screen
void PauseState::render() {
	pauseMenu->render(pauseOption);
}
