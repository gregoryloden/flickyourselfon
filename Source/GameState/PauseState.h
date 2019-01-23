#include "Util/PooledReferenceCounter.h"
#include "Sprites/Text.h"

#define newBasePauseState() produceWithoutArgs(PauseState)

//a single instance of a PauseState is immutable and shared between states
//PauseStates are added when entering a child and released when exiting, using the same parent instance
class PauseState: public PooledReferenceCounter {
private:
	class PauseOption;

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
		void render(int selectedOption);
	};
	class PauseOption onlyInDebug(: public ObjCounter) {
	public:
		static const float displayTextFontScale;

		PauseOption(objCounterParameters());
		~PauseOption();

		virtual Text::Metrics getDisplayTextMetrics() = 0;
		virtual PauseState* handle(PauseState* currentState) = 0;
		virtual void render(float leftX, float baselineY) = 0;
	};
	class NavigationOption: public PauseOption {
	private:
		string displayText;
		Text::Metrics displayTextMetrics;
		PauseMenu* subMenu;

	public:
		NavigationOption(objCounterParametersComma() string pDisplayText, PauseMenu* pSubMenu);
		~NavigationOption();

		virtual Text::Metrics getDisplayTextMetrics() { return displayTextMetrics; }
		virtual PauseState* handle(PauseState* currentState);
		virtual void render(float leftX, float baselineY);
	};

	static PauseMenu* baseMenu;

	ReferenceCounterHolder<PauseState> parentState;
	PauseMenu* pauseMenu;
	int pauseOption;
	bool shouldQuitGame;

public:
	PauseState(objCounterParameters());
	~PauseState();

private:
	static PauseState* produce(objCounterParametersComma() PauseState* pParentState, PauseMenu* pPauseMenu, int pPauseOption);
public:
	static PauseState* produce(objCounterParameters());
	virtual void release();
	virtual void prepareReturnToPool();
	//return whether updates and renders should stop, to pass on to the GameState
	bool getShouldQuitGame() { return shouldQuitGame; }
	static void loadMenu();
	static void unloadMenu();
	PauseState* getNextPauseState();
	PauseState* navigateToMenu(PauseMenu* menu);
	void render();
};
