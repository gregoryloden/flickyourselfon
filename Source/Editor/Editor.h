#include "General/General.h"
#include "Sprites/Text.h"

class EntityState;

class Editor {
private:
	enum class Zone: unsigned char {
		Right,
		Bottom
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class RGB {
	public:
		float red;
		float green;
		float blue;

		RGB(float pRed, float pGreen, float pBlue);
		~RGB();
	};
	class Button onlyInDebug(: public ObjCounter) {
	private:
		static const float buttonGrayRGB;

	protected:
		int leftX;
		int topY;
		int rightX;
		int bottomY;

	public:
		Button(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		~Button();

		void setWidthAndHeight(int width, int height);
		bool tryHandleClick(int x, int y);
		virtual void render();
		//this button was clicked, do its associated action
		virtual void doAction() = 0;
	};
	class TextButton: public Button {
	private:
		static const int buttonMaxLeftRightPadding = 4;
		static const int buttonTopBottomPadding = 2;
		static const float buttonFontScale;

		string text;
		Text::Metrics textMetrics;
		float textLeftX;
		float textBaselineY;

	public:
		TextButton(objCounterParametersComma() string pText, Zone zone, int zoneLeftX, int zoneTopY);
		~TextButton();

		virtual void render();
	};
	class SaveButton: public TextButton {
	public:
		SaveButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		~SaveButton();

		virtual void doAction();
	};
	class ExportMapButton: public TextButton {
	private:
		static const char* mapFileName;

	public:
		ExportMapButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		~ExportMapButton();

		virtual void doAction();
	};
	class TileButton: public Button {
	public:
		static const int buttonSize;

	private:
		char tile;

	public:
		TileButton(objCounterParametersComma() char pTile, Zone zone, int zoneLeftX, int zoneTopY);
		~TileButton();

		virtual void render();
		virtual void doAction();
	};
	class HeightButton: public Button {
	public:
		static const int buttonWidth;
		static const int buttonHeight;
	private:
		static const RGB heightFloorRGB;
		static const RGB heightWallRGB;

		char height;

	public:
		HeightButton(objCounterParametersComma() char pHeight, Zone zone, int zoneLeftX, int zoneTopY);
		~HeightButton();

		virtual void render();
		virtual void doAction();
	};
	class PaintBoxRadiusButton: public Button {
	public:
		static const int buttonSize;
	private:
		static const RGB boxRGB;

		char radius;

	public:
		PaintBoxRadiusButton(objCounterParametersComma() char pRadius, Zone zone, int zoneLeftX, int zoneTopY);
		~PaintBoxRadiusButton();

		virtual void render();
		virtual void doAction();
	};
	class NoiseButton: public TextButton {
	public:
		NoiseButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		~NoiseButton();

		virtual void doAction();
	};
	class NoiseTileButton: public Button {
	public:
		static const int buttonWidth;
		static const int buttonHeight;

	private:
		char tile;
		short count;

	public:
		NoiseTileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		~NoiseTileButton();

		virtual void render();
		virtual void doAction();
	};

	static const int paintBoxMaxRadius = 7;
	static const int noiseTileButtonMaxCount = 16;
	static const RGB backgroundRGB;

	static SaveButton* saveButton;
	static ExportMapButton* exportMapButton;
	static TileButton** tileButtons;
	static HeightButton** heightButtons;
	static PaintBoxRadiusButton** paintBoxRadiusButtons;
	static NoiseButton* noiseButton;
	static NoiseTileButton** noiseTileButtons;

public:
	static void loadButtons();
	static void unloadButtons();
private:
	static void getMouseMapXY(int screenLeftWorldX, int screenTopWorldY, int* outMapX, int* outMapY);
public:
	static void handleClick(SDL_MouseButtonEvent& clickEvent);
	static void render(EntityState* camera, int ticksTime);
};
