#include "Util/PooledReferenceCounter.h"
#include "Sprites/Text.h"

#define newBasePauseState() produceWithoutArgs(PauseState)

enum class KickActionType: int;

//a single instance of a PauseState is immutable and shared between states
//PauseStates are added when entering a child and released when exiting, using the same parent instance
class PauseState: public PooledReferenceCounter {
private:
	class KeyBindingOption;
	class PauseOption;

public:
	enum class EndPauseDecision: int {
		Save = 0x1,
		Reset = 0x2,
		Exit = 0x4
	};
private:
	class PauseMenu onlyInDebug(: public ObjCounter) {
	private:
		static const float titleFontScale;
		static const float selectedOptionAngleBracketPadding;

		string title;
		Text::Metrics titleMetrics;
		vector<PauseOption*> options;

	public:
		PauseMenu(objCounterParametersComma() string pTitle, vector<PauseOption*> pOptions);
		virtual ~PauseMenu();

		int getOptionsCount() { return (int)options.size(); }
		PauseOption* getOption(int optionIndex) { return options[optionIndex]; }
		//render this menu
		void render(int selectedOption, KeyBindingOption* selectingKeyBindingOption);
	};
	class PauseOption onlyInDebug(: public ObjCounter) {
	private:
		string displayText;
		Text::Metrics displayTextMetrics;
	public:
		static const float displayTextFontScale;

		PauseOption(objCounterParametersComma() string pDisplayText);
		virtual ~PauseOption();

		//get the metrics of the text that this option will render
		//PauseOption caches the metrics of the regular display text and will return it, subclasses can override this and return
		//	a different value if they draw something else
		virtual Text::Metrics getDisplayTextMetrics() { return displayTextMetrics; }
		//render the PauseOption
		virtual void render(float leftX, float baselineY);
		//update the display text and its metrics
		void updateDisplayText(string& newDisplayText);
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
	public:
		ControlsNavigationOption(objCounterParametersComma() PauseMenu* pSubMenu);
		virtual ~ControlsNavigationOption();

		//navigate to the controls submenu after setting up the menu key bindings
		virtual PauseState* handle(PauseState* currentState);
	};
	class KeyBindingOption: public PauseOption {
	public:
		enum class BoundKey: int {
			Up,
			Right,
			Down,
			Left,
			Kick,
			ShowConnections
		};

	private:
		static const int interKeyActionAndKeyBackgroundSpacing = 4;
		static const char* keySelectingText;

		static float cachedKeySelectingTextWidth;
		static float cachedKeySelectingTextFontScale;

		BoundKey boundKey;
		SDL_Scancode cachedKeyScancode;
		string cachedKeyName;
		Text::Metrics cachedKeyTextMetrics;
		Text::Metrics cachedKeyBackgroundMetrics;

	public:
		KeyBindingOption(objCounterParametersComma() BoundKey pBoundKey);
		virtual ~KeyBindingOption();

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
		//get the name of the action we're binding a key to
		//static so that we can use it in the option's constructor
		static string getBoundKeyActionText(BoundKey pBoundKey);
		//get the currently-editing scancode for our bound key
		SDL_Scancode getBoundKeyScancode();
	public:
		//update the scancode for this option's bound key
		//this also changes the scancode for past states, but since this is atomic and scancodes are retrieved only once per
		//	frame, this shouldn't be a problem
		void setBoundKeyScancode(SDL_Scancode keyScancode);
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
	class KickIndicatorOption: public PauseOption {
	private:
		KickActionType action;

	public:
		KickIndicatorOption(objCounterParametersComma() KickActionType pAction);
		virtual ~KickIndicatorOption();

		//toggle the kick indicator for this action
		virtual PauseState* handle(PauseState* currentState);
	private:
		//get the name of the action we're binding a key to, and its current config setting
		//static so that we can use it in the option's constructor
		static string getKickActionSettingText(KickActionType pAction);
	};
	class EndPauseOption: public PauseOption {
	private:
		int endPauseDecision;

	public:
		EndPauseOption(objCounterParametersComma() int pEndPauseDecision);
		virtual ~EndPauseOption();

		int getEndPauseDecision() { return endPauseDecision; }
		//return a pause state to quit the game
		virtual PauseState* handle(PauseState* currentState);
	};

	static PauseMenu* baseMenu;

	ReferenceCounterHolder<PauseState> parentState;
	PauseMenu* pauseMenu;
	int pauseOption;
	KeyBindingOption* selectingKeyBindingOption;
	int endPauseDecision;

public:
	PauseState(objCounterParameters());
	virtual ~PauseState();

	//return a bit field of EndPauseDecision specifying whether we should save, quit
	int getEndPauseDecision() { return endPauseDecision; }
private:
	//initialize and return a PauseState
	static PauseState* produce(
		objCounterParametersComma()
		PauseState* pParentState,
		PauseMenu* pPauseMenu,
		int pPauseOption,
		KeyBindingOption* pSelectingKeyBindingOption,
		int pEndPauseDecision);
public:
	//return a new pause state at the base menu
	static PauseState* produce(objCounterParameters());
	//release a reference to this PauseState and return it to the pool if applicable
	virtual void release();
protected:
	//release the parent state before this is returned to the pool
	virtual void prepareReturnToPool();
public:
	//build the base pause menu
	static void loadMenu();
	//delete the base menu
	static void unloadMenu();
	//if any keys were pressed, return a new updated pause state, otherwise return this non-updated state
	//if the game was closed, return whatever intermediate state we have specifying to quit the game
	PauseState* getNextPauseState();
private:
	//handle the keypress and return the resulting new pause state
	PauseState* handleKeyPress(SDL_Scancode keyScancode);
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
