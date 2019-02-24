#include "General/General.h"
#include "Sprites/Text.h"

class Editor {
private:
	enum class Zone: unsigned char {
		Right,
		Bottom
	};
	class Button onlyInDebug(: public ObjCounter) {
	private:
		static const int buttonMaxLeftRightPadding = 4;
		static const int buttonTopBottomPadding = 2;
		static const float buttonFontScale;
		static const float buttonGrayRGB;

		string text;
		Text::Metrics textMetrics;
		int leftX;
		int topY;
		int rightX;
		int bottomY;
		float textLeftX;
		float textBaselineY;

	public:
		Button(objCounterParametersComma() string pText, Zone zone, int zoneLeftX, int zoneTopY);
		~Button();

		void render();
		bool tryHandleClick(int x, int y);
		//this button was clicked, do its associated action
		virtual void doAction() = 0;
	};
	class SaveButton: public Button {
	public:
		SaveButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		~SaveButton();

		virtual void doAction();
	};
	class ExportMapButton: public Button {
	public:
		ExportMapButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		~ExportMapButton();

		virtual void doAction();
	};

	static const float backgroundRed;
	static const float backgroundGreen;
	static const float backgroundBlue;

	static SaveButton* saveButton;
	static ExportMapButton* exportMapButton;

public:
	static void loadButtons();
	static void unloadButtons();
	static void handleClick(SDL_MouseButtonEvent& clickEvent);
	static void render();
};
