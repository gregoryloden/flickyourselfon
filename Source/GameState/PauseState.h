#include "Util/PooledReferenceCounter.h"
#include "Sprites/Text.h"

enum class KickActionType: int;
namespace ConfigTypes {
	class KeyBindingSetting;
	class MultiStateSetting;
	class VolumeSetting;
	class ValueSelectionSetting;
}

//a single instance of a PauseState is immutable and shared between states
//PauseStates are added when entering a child and released when exiting, using the same parent instance
class PauseState: public PooledReferenceCounter {
private:
	class KeyBindingOption;
	class PauseOption;

public:
	enum class EndPauseDecision: int {
		//save the game
		Save = 1 << 0,
		//reset the game state to the start of the game
		Reset = 1 << 1,
		//exit the game, shut down the program
		Exit = 1 << 2,
		//load the game that has already been set by/at this pause state
		//doesn't do anything specific besides playing music
		Load = 1 << 3,
		//show the current hint
		RequestHint = 1 << 4,
		//confirm selecting the current level
		SelectLevel = 1 << 5,
	};
private:
	class PauseMenu onlyInDebug(: public ObjCounter) {
	private:
		static constexpr float titleFontScale = 2.0f;
		static constexpr float selectedOptionAngleBracketPadding = 4.0f;

		string title;
		Text::Metrics titleMetrics;
	protected:
		vector<PauseOption*> options;

	public:
		PauseMenu(objCounterParametersComma() string pTitle, vector<PauseOption*> pOptions);
		virtual ~PauseMenu();

		int getOptionsCount() { return (int)options.size(); }
		PauseOption* getOption(int optionIndex) { return options[optionIndex]; }
	private:
		//calculate the total height for this pause menu
		void getTotalHeightAndMetrics(
			KeyBindingOption* selectingKeyBindingOption, float* outTotalHeight, vector<Text::Metrics>* optionsMetrics);
	public:
		//handle selecting an option in this menu
		void handleSelectOption(int pauseOption, int* outSelectedLevelN);
		//find the option that the mouse is hovering over
		int findHighlightedOption(int mouseX, int mouseY, float* outOptionX);
		//load all key binding settings from this menu's options
		void loadAffectedKeyBindingSettings(vector<ConfigTypes::KeyBindingSetting*>* affectedSettings);
		//render this menu
		void render(int selectedOption, KeyBindingOption* selectingKeyBindingOption);
	};
	class LevelSelectMenu: public PauseMenu {
	public:
		LevelSelectMenu(objCounterParametersComma() string pTitle, vector<PauseOption*> pOptions);
		virtual ~LevelSelectMenu();

		//enable or disable level select options depending on how many levels are unlocked
		void enableDisableLevelOptions(int levelsUnlocked);
	};
	class PauseOption onlyInDebug(: public ObjCounter) {
	private:
		string displayText;
		Text::Metrics displayTextMetrics;
	public:
		bool enabled;

		static constexpr float displayTextFontScale = 1.0f;

		PauseOption(objCounterParametersComma() string pDisplayText);
		virtual ~PauseOption();

		//get the metrics of the text that this option will render
		//PauseOption caches the metrics of the regular display text and will return it, subclasses can override this and return
		//	a different value if they draw something else
		virtual Text::Metrics getDisplayTextMetrics() { return displayTextMetrics; }
		//load a key binding setting into the list, if applicable
		virtual void loadAffectedKeyBindingSetting(vector<ConfigTypes::KeyBindingSetting*>* affectedSettings) {}
		//handle a side direction input, return the pause state to use as a result
		//by default, return the given state, as most pause options do not handle side direction input
		virtual PauseState* handleSide(PauseState* currentState, int direction) { return currentState; }
		//handle navigating to this pause option in the menu
		virtual void handleSelect(int* outSelectedLevelN) {}
		//render the PauseOption
		virtual void render(float leftX, float baselineY);
		//update the display text and its metrics
		void updateDisplayText(const string& newDisplayText);
		//handle this option being selected at the given x, return the pause state to use as a result
		virtual PauseState* handleWithX(PauseState* currentState, float x, bool isDrag);
		//handle this option being selected, return the pause state to use as a result
		virtual PauseState* handle(PauseState* currentState) = 0;
	};
	class NavigationOption: public PauseOption {
	private:
		PauseMenu* subMenu;

	public:
		NavigationOption(objCounterParametersComma() string pDisplayText, PauseMenu* pSubMenu);
		virtual ~NavigationOption();

		//navigate to this option's submenu
		virtual PauseState* handle(PauseState* currentState);
	};
	class ControlsNavigationOption: public NavigationOption {
	private:
		vector<ConfigTypes::KeyBindingSetting*> affectedSettings;

	public:
		ControlsNavigationOption(objCounterParametersComma() string pDisplayText, PauseMenu* pSubMenu);
		virtual ~ControlsNavigationOption();

		//navigate to the controls submenu after setting up the menu key bindings
		virtual PauseState* handle(PauseState* currentState);
	};
	class KeyBindingOption: public PauseOption {
	private:
		static const int interKeyActionAndKeyBackgroundSpacing = 4;
		static constexpr char* keySelectingText = "[press any key]";

		static float cachedKeySelectingTextWidth;
		static float cachedKeySelectingTextFontScale;

		ConfigTypes::KeyBindingSetting* setting;
		SDL_Scancode cachedKeyScancode;
		string cachedKeyName;
		Text::Metrics cachedKeyTextMetrics;
		Text::Metrics cachedKeyBackgroundMetrics;

	public:
		KeyBindingOption(objCounterParametersComma() ConfigTypes::KeyBindingSetting* pSetting, string displayPrefix);
		virtual ~KeyBindingOption();

		//add the key binding setting into the list
		virtual void loadAffectedKeyBindingSetting(vector<ConfigTypes::KeyBindingSetting*>* affectedSettings) {
			affectedSettings->push_back(setting);
		}
		//return metrics that include the key name and background
		virtual Text::Metrics getDisplayTextMetrics();
		//return metrics that include either the key-selecting text or the key name and background
		Text::Metrics getSelectingDisplayTextMetrics(bool selecting);
		//start selecting a key for this option
		virtual PauseState* handle(PauseState* currentState);
		//render the text, the key name, and the key background
		virtual void render(float leftX, float baselineY);
		//render the text and either the key-selecting text or the key name and background
		void renderSelecting(float leftX, float baselineY, bool selecting);
	private:
		//if we have a new bound key, cache its metrics
		void ensureCachedKeyMetrics();
	public:
		//update the editing scancode for this option's setting
		//this also changes the scancode for past states, but since this is atomic and scancodes are retrieved only once per
		//	frame, this shouldn't be a problem
		void setKeyBindingSettingEditingScancode(SDL_Scancode keyScancode);
	};
	class DefaultKeyBindingsOption: public PauseOption {
	public:
		DefaultKeyBindingsOption(objCounterParameters());
		virtual ~DefaultKeyBindingsOption();

		//reset the currently editing keys to the defaults
		virtual PauseState* handle(PauseState* currentState);
	};
	class AcceptKeyBindingsOption: public PauseOption {
	public:
		AcceptKeyBindingsOption(objCounterParameters());
		virtual ~AcceptKeyBindingsOption();

		//accept the new key bindings and save them to the options file
		virtual PauseState* handle(PauseState* currentState);
	};
	class MultiStateOption: public PauseOption {
	private:
		ConfigTypes::MultiStateSetting* setting;
		string displayPrefix;

	public:
		MultiStateOption(objCounterParametersComma() ConfigTypes::MultiStateSetting* pSetting, string pDisplayPrefix);
		virtual ~MultiStateOption();

		//cycle the state for this option
		virtual PauseState* handle(PauseState* currentState);
		//cycle the state for this option in either direction
		virtual PauseState* handleSide(PauseState* currentState, int direction);
	};
	class VolumeSettingOption: public PauseOption {
	private:
		ConfigTypes::VolumeSetting* setting;
		string displayPrefix;
		float widthBeforeVolume;
		float widthBeforeVolumeIncrements;
		float volumeIncrementWidth;

	public:
		VolumeSettingOption(objCounterParametersComma() ConfigTypes::VolumeSetting* pSetting, string pDisplayPrefix);
		virtual ~VolumeSettingOption();

		//selecting a volume setting doesn't do anything
		virtual PauseState* handle(PauseState* currentState) { return currentState; }
		//get the string to represent the volume
		static string getVolume(ConfigTypes::VolumeSetting* pSetting);
	private:
		//apply the given volume to the setting
		void applyVolume(int volume);
	public:
		//increase or decrease the volume
		virtual PauseState* handleSide(PauseState* currentState, int direction);
		//adjust the volume if the mouse is within the volume display area
		virtual PauseState* handleWithX(PauseState* currentState, float x, bool isDrag);
	};
	class ValueSelectionOption: public PauseOption {
	private:
		ConfigTypes::ValueSelectionSetting* setting;
		string displayPrefix;

	public:
		ValueSelectionOption(objCounterParametersComma() ConfigTypes::ValueSelectionSetting* pSetting, string pDisplayPrefix);
		virtual ~ValueSelectionOption();

		//selecting a value selection setting doesn't do anything
		virtual PauseState* handle(PauseState* currentState) { return currentState; }
		//select a different value
		virtual PauseState* handleSide(PauseState* currentState, int direction);
	};
	class LevelSelectOption: public PauseOption {
	private:
		int levelN;

	public:
		LevelSelectOption(objCounterParametersComma() string pDisplayText, int pLevelN);
		virtual ~LevelSelectOption();

		//the player is already positioned properly, resume the game and have the player do any level-select finalizing
		virtual PauseState* handle(PauseState* currentState);
		//select this level
		virtual void handleSelect(int* outSelectedLevelN);
	};
	class EndPauseOption: public PauseOption {
	private:
		int endPauseDecision;

	public:
		EndPauseOption(objCounterParametersComma() string pDisplayText, int pEndPauseDecision);
		virtual ~EndPauseOption();

		int getEndPauseDecision() { return endPauseDecision; }
		//return a pause state to quit the game
		virtual PauseState* handle(PauseState* currentState);
	};

	static PauseMenu* baseMenu;
	static LevelSelectMenu* baseLevelSelectMenu;
	static PauseMenu* homeMenu;
	static LevelSelectMenu* homeLevelSelectMenu;

	ReferenceCounterHolder<PauseState> parentState;
	PauseMenu* pauseMenu;
	int pauseOption;
	KeyBindingOption* selectingKeyBindingOption;
	int selectedLevelN;
	int endPauseDecision;

public:
	PauseState(objCounterParameters());
	virtual ~PauseState();

	//return a bit field of EndPauseDecision specifying whether we should save, quit
	int getEndPauseDecision() { return endPauseDecision; }
	//return the level number being selected, if applicable
	int getSelectedLevelN() { return selectedLevelN; }
private:
	//initialize and return a PauseState
	static PauseState* produce(
		objCounterParametersComma()
		PauseState* pParentState,
		PauseMenu* pPauseMenu,
		int pPauseOption,
		KeyBindingOption* pSelectingKeyBindingOption,
		int pSelectedLevelN,
		int pEndPauseDecision);
public:
	//return a new pause state at the base menu
	static PauseState* produceBasePauseScreen(int levelsUnlocked);
	//return a new pause state at the home screen menu
	static PauseState* produceHomeScreen(int levelsUnlocked);
	//release a reference to this PauseState and return it to the pool if applicable
	virtual void release();
protected:
	//release the parent state before this is returned to the pool
	virtual void prepareReturnToPool();
public:
	//build the base pause menus
	static void loadMenus();
private:
	//build the options menu and containing option
	static PauseOption* buildOptionsMenuOption();
	//build the level select menu and containing option
	static PauseOption* buildLevelSelectMenuOption(LevelSelectMenu** levelSelectMenu);
public:
	//delete the base menus
	static void unloadMenus();
	//disable the continue option and level select menu if there is no save data
	static void disableContinueOptions();
	//check if this pause state is at the home menu or one of the menus under it
	bool isAtHomeMenu();
	//if any keys were pressed, return a new updated pause state, otherwise return this non-updated state
	//if the game was closed, return whatever intermediate state we have specifying to quit the game
	PauseState* getNextPauseState();
private:
	//handle the keypress and return the resulting new pause state
	PauseState* handleKeyPress(SDL_Scancode keyScancode);
	//handle the mouse motion and return the resulting new pause state
	PauseState* handleMouseMotion(SDL_MouseMotionEvent motionEvent);
	//handle the mouse click and return the resulting new pause state
	PauseState* handleMouseClick(SDL_MouseButtonEvent clickEvent);
	//return a new state with the new pause option selected, after alerting it (or its menu) that it was selected
	PauseState* selectNewOption(int newPauseOption);
public:
	//if we were given a menu, return a pause state with that pause menu, otherwise go up a level
	PauseState* navigateToMenu(PauseMenu* menu);
	//return a copy of this pause state set to listen for a key selection
	PauseState* beginKeySelection(KeyBindingOption* pSelectingKeyBindingOption);
	//return a clone of this state that specifies that we should resume the game or exit
	PauseState* produceEndPauseState(int pEndPauseDecision);
	//render the pause menu over the screen
	void render();
};
