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
		static const float buttonFontScale;
		static const float buttonGrayRGB;

		string text;
		Text::Metrics textMetrics;
		GLint leftX;
		GLint topY;

	public:
		Button(objCounterParametersComma() string pText, Zone zone, GLint pLeftX, GLint pTopY);
		~Button();

		void render();
	};
	class SaveButton: public Button {
	public:
		SaveButton(objCounterParametersComma() Zone zone, GLint pLeftX, GLint pTopY);
		~SaveButton();
	};
	class ExportMapButton: public Button {
	public:
		ExportMapButton(objCounterParametersComma() Zone zone, GLint pLeftX, GLint pTopY);
		~ExportMapButton();
	};

	static const float backgroundRed;
	static const float backgroundGreen;
	static const float backgroundBlue;

	static SaveButton* saveButton;
	static ExportMapButton* exportMapButton;

public:
	static void loadButtons();
	static void unloadButtons();
	static void render();
};
