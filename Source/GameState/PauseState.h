#include "Util/PooledReferenceCounter.h"
#include "Sprites/Text.h"

#define newBasePauseState() produceWithoutArgs(PauseState)

//a single instance of a PauseState is immutable and shared between states
//PauseStates are added when entering a child and released when exiting, using the same parent instance
class PauseState: public PooledReferenceCounter {
private:
	class PauseOption;
	class KeyBindingOption;

	class PauseMenu onlyInDebug(: public ObjCounter) {
	private:
		static const float titleFontScale;
		static const float selectedOptionAngleBracketPadding;

		string title;
		Text::Metrics titleMetrics;
		vector<PauseOption*> options;

	public:
		PauseMenu(objCounterParametersComma() string pTitle, vector<PauseOption*> pOptions);
		~PauseMenu();

		int getOptionsCount() { return options.size(); }
		PauseOption* getOption(int optionIndex) { return options[optionIndex]; }
		void render(int selectedOption, KeyBindingOption* selectingKeyBindingOption);
	};
	class PauseOption onlyInDebug(: public ObjCounter) {
	private:
		string displayText;
		Text::Metrics displayTextMetrics;
	public:
		static const float displayTextFontScale;

		PauseOption(objCounterParametersComma() string pDisplayText);
		~PauseOption();

		//get the metrics of the text that this option will render
		//PauseOption caches the metrics of the regular display text and will return it, subclasses can override this and return
		//	a different value if they draw something else
		virtual Text::Metrics getDisplayTextMetrics() { return displayTextMetrics; }
		//handle this option being selected, return the pause state to use as a result
		virtual PauseState* handle(PauseState* currentState) = 0;
		virtual void render(float leftX, float baselineY);
	};
	class NavigationOption: public PauseOption {
	private:
		PauseMenu* subMenu;

	public:
		NavigationOption(objCounterParametersComma() string pDisplayText, PauseMenu* pSubMenu);
		~NavigationOption();

		virtual PauseState* handle(PauseState* currentState);
	};
	class ControlsNavigationOption: public NavigationOption {
	public:
		ControlsNavigationOption(objCounterParametersComma() PauseMenu* pSubMenu);
		~ControlsNavigationOption();

		virtual PauseState* handle(PauseState* currentState);
	};
	class KeyBindingOption: public PauseOption {
	public:
		enum class BoundKey: int {
			Up,
			Right,
			Down,
			Left,
			Kick
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
		~KeyBindingOption();

		virtual Text::Metrics getDisplayTextMetrics();
		Text::Metrics getSelectingDisplayTextMetrics(bool selecting);
		virtual PauseState* handle(PauseState* currentState);
		virtual void render(float leftX, float baselineY);
		void renderSelecting(float leftX, float baselineY, bool selecting);
	private:
		void ensureCachedKeyMetrics(bool selecting);
		static string getBoundKeyActionText(BoundKey pBoundKey);
		SDL_Scancode getBoundKeyScancode();
	public:
		void setBoundKeyScancode(SDL_Scancode keyScancode);
	};
	class DefaultKeyBindingsOption: public PauseOption {
	public:
		DefaultKeyBindingsOption(objCounterParameters());
		~DefaultKeyBindingsOption();

		virtual PauseState* handle(PauseState* currentState);
	};
	class AcceptKeyBindingsOption: public PauseOption {
	public:
		AcceptKeyBindingsOption(objCounterParameters());
		~AcceptKeyBindingsOption();

		virtual PauseState* handle(PauseState* currentState);
	};
	class QuitGameOption: public PauseOption {
	public:
		QuitGameOption(objCounterParameters());
		~QuitGameOption();

		virtual PauseState* handle(PauseState* currentState);
	};

	static PauseMenu* baseMenu;

	ReferenceCounterHolder<PauseState> parentState;
	PauseMenu* pauseMenu;
	int pauseOption;
	KeyBindingOption* selectingKeyBindingOption;
	bool shouldQuitGame;

public:
	PauseState(objCounterParameters());
	~PauseState();

private:
	static PauseState* produce(
		objCounterParametersComma()
		PauseState* pParentState,
		PauseMenu* pPauseMenu,
		int pPauseOption,
		KeyBindingOption* pSelectingKeyBindingOption);
public:
	static PauseState* produce(objCounterParameters());
	virtual void release();
	virtual void prepareReturnToPool();
	//return whether updates and renders should stop, to pass on to the GameState
	bool getShouldQuitGame() { return shouldQuitGame; }
	static void loadMenu();
	static void unloadMenu();
	PauseState* getNextPauseState();
private:
	PauseState* handleKeyPress(SDL_Scancode keyScancode);
public:
	PauseState* navigateToMenu(PauseMenu* menu);
	PauseState* beginKeySelection(KeyBindingOption* pSelectingKeyBindingOption);
	PauseState* produceQuitGameState();
	void render();
};