#include "General/General.h"
#include <random>
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
		void renderFadedOverlay();
		void renderHighlightOutline();
		virtual void paintMap(int x, int y);
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
		TextButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, string pText);
		~TextButton();

		virtual void render();
	};
	class SaveButton: public TextButton {
	public:
		SaveButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		~SaveButton();

		virtual void render();
		virtual void doAction();
	};
	class ExportMapButton: public TextButton {
	private:
		static const char* mapFileName;

	public:
		ExportMapButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		~ExportMapButton();

		virtual void render();
		virtual void doAction();
	};
	class TileButton: public Button {
	public:
		static const int buttonSize;

	private:
		char tile;

	public:
		TileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pTile);
		~TileButton();

		virtual void render();
		virtual void doAction();
		virtual void paintMap(int x, int y);
	};
	class HeightButton: public Button {
	public:
		static const int buttonWidth;
		static const int buttonHeight;

	private:
		char height;

	public:
		HeightButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pHeight);
		~HeightButton();

		char getHeight() { return height; }
		virtual void render();
		virtual void doAction();
		virtual void paintMap(int x, int y);
	};
	class PaintBoxRadiusButton: public Button {
	public:
		static const int buttonSize;
	private:
		static const RGB boxRGB;

		char radius;
		bool isXRadius;

	public:
		PaintBoxRadiusButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pRadius, bool pIsXRadius);
		~PaintBoxRadiusButton();

		char getRadius() { return radius; }
		virtual void render();
		virtual void doAction();
	};
	class EvenPaintBoxRadiusButton: public Button {
	public:
		static const int buttonSize;
	private:
		static const RGB boxRGB;
		static const RGB lineRGB;

		bool isXEvenRadius;
	public:
		bool isSelected;

		EvenPaintBoxRadiusButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, bool pIsXEvenRadius);
		~EvenPaintBoxRadiusButton();

		virtual void render();
		virtual void doAction();
	};
	class NoiseButton: public TextButton {
	public:
		NoiseButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		~NoiseButton();

		virtual void doAction();
		virtual void paintMap(int x, int y);
	};
	class NoiseTileButton: public Button {
	public:
		static const int buttonWidth;
		static const int buttonHeight;

		char tile;
		int count;

		NoiseTileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		~NoiseTileButton();

		virtual void render();
		virtual void doAction();
	};
	class RaiseLowerTileButton: public Button {
	public:
		static const int buttonWidth;
		static const int buttonHeight;

	private:
		bool isRaiseTileButton;

	public:
		RaiseLowerTileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, bool pIsRaiseTileButton);
		~RaiseLowerTileButton();

		virtual void render();
		virtual void doAction();
		virtual void paintMap(int x, int y);
	};
	class SwitchButton: public Button {
	public:
		static const int buttonSize;

	private:
		char color;

	public:
		SwitchButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pColor);
		~SwitchButton();

		virtual void render();
		virtual void doAction();
		virtual void paintMap(int x, int y);
	};
	class RailButton: public Button {
	public:
		static const int buttonSize;

	private:
		char color;

	public:
		RailButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pColor);
		~RailButton();

		virtual void render();
		virtual void doAction();
		virtual void paintMap(int x, int y);
	};
	class RailTileOffsetButton: public Button {
	public:
		static const int buttonSize;

	private:
		char tileOffset;

	public:
		RailTileOffsetButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pTileOffset);
		~RailTileOffsetButton();

		virtual void render();
		virtual void doAction();
		virtual void paintMap(int x, int y);
	};
	class RailSwitchGroupButton: public Button {
	public:
		static const int groupSquareSize = 6;
		static const int groupSquareHalfSize = groupSquareSize / 2;
		static const int buttonSize = groupSquareSize + 2;

	private:
		char railSwitchGroup;

	public:
		RailSwitchGroupButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pRailSwitchGroup);
		~RailSwitchGroupButton();

		char getRailSwitchGroup() { return railSwitchGroup; }
		virtual void render();
		virtual void doAction();
	};

	static const int paintBoxMaxRadius = 7;
	static const int noiseTileButtonMaxCount = 16;
	static const int railSwitchGroupCount = 64;
	static const RGB backgroundRGB;
	static const RGB heightFloorRGB;
	static const RGB heightWallRGB;

	static vector<Button*> buttons;
	static EvenPaintBoxRadiusButton* evenPaintBoxXRadiusButton;
	static EvenPaintBoxRadiusButton* evenPaintBoxYRadiusButton;
	static NoiseButton* noiseButton;
	static NoiseTileButton** noiseTileButtons;
	static RaiseLowerTileButton* lowerTileButton;
	static Button* selectedButton;
	static PaintBoxRadiusButton* selectedPaintBoxXRadiusButton;
	static PaintBoxRadiusButton* selectedPaintBoxYRadiusButton;
	static RailSwitchGroupButton* selectedRailSwitchGroupButton;
	static HeightButton* lastSelectedHeightButton;
	static SwitchButton* lastSelectedSwitchButton;
	static RailButton* lastSelectedRailButton;
	static RailTileOffsetButton* lastSelectedRailTileOffsetButton;
	static default_random_engine* randomEngine;
	static discrete_distribution<int>* randomDistribution;
	static bool saveButtonDisabled;
	static bool exportMapButtonDisabled;

public:
	static void loadButtons();
	static void unloadButtons();
	static char getSelectedHeight();
private:
	static void getMouseMapXY(int screenLeftWorldX, int screenTopWorldY, int* outMapX, int* outMapY);
public:
	static void handleClick(SDL_MouseButtonEvent& clickEvent, EntityState* camera, int ticksTime);
	static void render(EntityState* camera, int ticksTime);
	static void renderGroupRect(char group, int leftX, int topY, int rightX, int bottomY);
	static void renderRGBRect(const RGB& rgb, float alpha, int leftX, int topY, int rightX, int bottomY);
private:
	static void addNoiseTile(char tile);
	static void removeNoiseTile(char tile);
};
