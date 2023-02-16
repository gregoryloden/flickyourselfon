#include "GameState/MapState/MapState.h"
#include <mutex>
#include <random>
#include "Sprites/Text.h"

class EntityState;

class Editor {
private:
	enum class Zone: unsigned char {
		Right,
		Bottom
	};
	enum class MouseDragAction: unsigned char {
		None,
		RaiseLowerTile,
		AddRemoveRail,
		AddRemoveSwitch,
		AddRemoveResetSwitch
	};
public:
	//Should only be allocated on the stack
	class EditingMutexLocker {
	private:
		static mutex editingMutex;

	public:
		EditingMutexLocker();
		virtual ~EditingMutexLocker();
	};
private:
	//Should only be allocated within an object, on the stack, or as a static object
	class RGB {
	public:
		float red;
		float green;
		float blue;

		RGB(float pRed, float pGreen, float pBlue);
		virtual ~RGB();
	};
	class Button onlyInDebug(: public ObjCounter) {
	private:
		static const RGB buttonGrayRGB;

	protected:
		int leftX;
		int topY;
		int rightX;
		int bottomY;

	public:
		Button(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		virtual ~Button();

		//draw the contents of this button over the background
		virtual void renderOverButton() {}
		//do anything that should happen only once after painting
		virtual void postPaint() {}
		//paint the map at this tile
		virtual void paintMap(int x, int y) {}
		//this button was clicked, do its associated action
		//by default, toggle selection of this button and its paint action
		virtual void onClick() { selectedButton = selectedButton == this ? nullptr : this; }
		//alter rightX and bottomY to account for the now-known width and height
		void setWidthAndHeight(int width, int height);
		//check if the click is within this button's bounds, and if it is, do the action for this button
		//returns whether the click was handled by this button
		bool tryHandleClick(int x, int y);
		//render the button background
		void render();
		//render a rectangle the color of the background to fade this button
		void renderFadedOverlay();
		//render a rectangle outline within the border of this button
		void renderHighlightOutline();
	};
	class TextButton: public Button {
	private:
		static const int buttonMaxLeftRightPadding = 4;
		static const int buttonTopBottomPadding = 2;
		static constexpr float buttonFontScale = 1.0f;

		string text;
		Text::Metrics textMetrics;
		float textLeftX;
		float textBaselineY;

	public:
		TextButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, string pText);
		virtual ~TextButton();

		//draw the contents of this text button over the background
		virtual void renderOverTextButton() {}
		//render the text above the button background
		void renderOverButton();
	};
	class SaveButton: public TextButton {
	public:
		SaveButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		virtual ~SaveButton();

		//fade the save button if it's disabled
		virtual void renderOverTextButton();
		//save the floor image
		virtual void onClick();
	};
	class ExportMapButton: public TextButton {
	private:
		static constexpr char* mapFileName = "images/map.png";

	public:
		ExportMapButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		virtual ~ExportMapButton();

		//fade the export map button if it's disabled
		virtual void renderOverTextButton();
		//export the map
		virtual void onClick();
	};
	class TileButton: public Button {
	public:
		static const int buttonSize = MapState::tileSize + 2;

	private:
		char tile;

	public:
		TileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pTile);
		virtual ~TileButton();

		//render the tile above the button
		virtual void renderOverButton();
		//if noisy tile selection is off, highlight this tile for tile painting
		//if noisy tile selection is on, add this tile to the noisy tile list
		virtual void onClick();
		//set the tile at this position
		virtual void paintMap(int x, int y);
	};
	class HeightButton: public Button {
	public:
		static const int buttonWidth = MapState::tileSize + 2;
		static const int buttonHeight = 10;

	private:
		char height;

	public:
		HeightButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pHeight);
		virtual ~HeightButton();

		char getHeight() { return height; }
		//render the height graphic above the button
		virtual void renderOverButton();
		//select a height as the painting action
		virtual void onClick();
		//set the height at this position
		virtual void paintMap(int x, int y);
	};
	class PaintBoxRadiusButton: public Button {
	public:
		static const int maxRadius = 7;
		static const int buttonSize = maxRadius + 2;
	private:
		static const RGB boxRGB;

		char radius;
		bool isXRadius;

	public:
		PaintBoxRadiusButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pRadius, bool pIsXRadius);
		virtual ~PaintBoxRadiusButton();

		char getRadius() { return radius; }
		//render the box radius above the button
		virtual void renderOverButton();
		//select a radius to use when painting
		virtual void onClick();
	};
	class EvenPaintBoxRadiusButton: public Button {
	public:
		static const int buttonSize = PaintBoxRadiusButton::maxRadius + 2;
	private:
		static const RGB boxRGB;
		static const RGB lineRGB;

		bool isXEvenRadius;
	public:
		bool isSelected;

		EvenPaintBoxRadiusButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, bool pIsXEvenRadius);
		virtual ~EvenPaintBoxRadiusButton();

		//render the box radius extension above the button
		virtual void renderOverButton();
		//add or remove an extra row below/to the right of the paint box
		virtual void onClick();
	};
	class NoiseButton: public TextButton {
	public:
		NoiseButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		virtual ~NoiseButton();

		//shuffle the tile at this position
		virtual void paintMap(int x, int y);
	};
	class NoiseTileButton: public Button {
	public:
		static const int buttonWidth = MapState::tileSize + 2;
		static const int buttonHeight = MapState::tileSize + 4;

		char tile;
		int count;

		NoiseTileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		virtual ~NoiseTileButton();

		//render the tile above the button, as well as the count below it in base 4
		virtual void renderOverButton();
		//remove a count from this button
		virtual void onClick();
	};
	class RaiseLowerTileButton: public Button {
	public:
		static const int buttonWidth = 13;
		static const int buttonHeight = 10;

	private:
		bool isRaiseTileButton;

	public:
		RaiseLowerTileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, bool pIsRaiseTileButton);
		virtual ~RaiseLowerTileButton();

		//render a raised or lowered platform above the button
		virtual void renderOverButton();
		//raise or lower the tile at this position
		virtual void paintMap(int x, int y);
		//set the drag action so that we don't raise/lower tiles while dragging
		virtual void postPaint();
	private:
		//set the default floor tile and then randomize it
		static void setAppropriateDefaultFloorTile(int x, int y, char expectedFloorHeight);
	};
	class ShuffleTileButton: public Button {
	public:
		static const int buttonWidth = 14;
		static const int buttonHeight = 11;
		static const RGB arrowRGB;

		ShuffleTileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		virtual ~ShuffleTileButton();

		//render floor and wall tiles with arrows pointing between them
		virtual void renderOverButton();
		//assign a random tile from the same tile group that this tile is in
		virtual void paintMap(int x, int y);
		//assign a new value to the tile if possible
		//returns true if a new value was assigned, or was not eligible to be asssigned
		static bool shuffleTile(int x, int y);
	};
	class SwitchButton: public Button {
	public:
		static const int buttonSize = MapState::switchSize + 2;

	private:
		char color;

	public:
		SwitchButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pColor);
		virtual ~SwitchButton();

		//render the switch above the button
		virtual void renderOverButton();
		//select this switch as the painting action
		virtual void onClick();
		//set a switch at this position
		virtual void paintMap(int x, int y);
	};
	class RailButton: public Button {
	public:
		static const int buttonSize = MapState::tileSize + 2;

	private:
		char color;

	public:
		RailButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pColor);
		virtual ~RailButton();

		//render the rail above the button
		virtual void renderOverButton();
		//select this rail as the painting action
		virtual void onClick();
		//set a rail at this position
		virtual void paintMap(int x, int y);
	};
	class RailToggleMovementDirectionButton : public Button {
	public:
		static const int buttonSize = MapState::tileSize + 2;
		static const RGB arrowRGB;

	public:
		RailToggleMovementDirectionButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		virtual ~RailToggleMovementDirectionButton();

		//render the up and down arrows above the button
		virtual void renderOverButton();
		//toggle the movement direction of the rail at this position
		virtual void paintMap(int x, int y);
	};
	class RailTileOffsetButton: public Button {
	public:
		static const int buttonSize = MapState::tileSize + 2;
		static const RGB arrowRGB;

	private:
		char tileOffset;

	public:
		RailTileOffsetButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pTileOffset);
		virtual ~RailTileOffsetButton();

		//render the rail above the button
		virtual void renderOverButton();
		//select this rail tile offset as the painting action
		virtual void onClick();
		//adjust the tile offset of the rail at this position
		virtual void paintMap(int x, int y);
	};
	class ResetSwitchButton: public Button {
	public:
		static const int buttonWidth = MapState::tileSize + 2;
		static const int buttonHeight = MapState::tileSize * 2 + 2;

		ResetSwitchButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		virtual ~ResetSwitchButton();

		//render the reset switch above the button
		virtual void renderOverButton();
		//select the reset switch as the painting action
		virtual void onClick();
		//set a reset switch at this position
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
		virtual ~RailSwitchGroupButton();

		char getRailSwitchGroup() { return railSwitchGroup; }
		//render the group above the button
		virtual void renderOverButton();
		//select a group to use when painting switches or rails
		virtual void onClick();
	};

	static const int noiseTileButtonMaxCount = 16;
	static const int railSwitchGroupCount = 64;
	static const RGB blackRGB;
	static const RGB whiteRGB;
	static const RGB backgroundRGB;
	static const RGB heightFloorRGB;
	static const RGB heightWallRGB;

public:
	static bool isActive;
private:
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
	static ResetSwitchButton* lastSelectedResetSwitchButton;
	static MouseDragAction lastMouseDragAction;
	static int lastMouseDragMapX;
	static int lastMouseDragMapY;
	static default_random_engine* randomEngine;
	static discrete_distribution<int>* noiseTilesDistribution;
	static discrete_distribution<int> floorTileDistribution;
	static uniform_int_distribution<int> wallTileDistribution;
	static uniform_int_distribution<int> platformRightFloorTileDistribution;
	static uniform_int_distribution<int> platformLeftFloorTileDistribution;
	static uniform_int_distribution<int> platformTopFloorTileDistribution;
	static uniform_int_distribution<int> groundLeftFloorTileDistribution;
	static uniform_int_distribution<int> groundRightFloorTileDistribution;
	static bool saveButtonDisabled;
	static bool exportMapButtonDisabled;
public:
	static bool needsGameStateSave;

	//build all the editor buttons
	static void loadButtons();
	//delete all the editor buttons
	static void unloadButtons();
	//return the height of the selected height button, or -1 if it's not selected
	static char getSelectedHeight();
private:
	//convert the mouse position to map coordinates
	static void getMouseMapXY(int screenLeftWorldX, int screenTopWorldY, int* outMapX, int* outMapY);
public:
	//see if we clicked on any buttons or the game screen
	static void handleClick(SDL_MouseButtonEvent& clickEvent, bool isDrag, EntityState* camera, int ticksTime);
	//draw the editor interface
	static void render(EntityState* camera, int ticksTime);
private:
	//render a rectangle of the given color in the given region;
	static void renderRGBRect(const RGB& rgb, float alpha, int leftX, int topY, int rightX, int bottomY);
	//add to the count of this tile
	static void addNoiseTile(char tile);
	//remove from the count of this tile
	static void removeNoiseTile(char tile);
	//returns true iff we clicked on a tile or dragged onto a new tile
	static bool clickedNewTile(int x, int y, MouseDragAction mouseDragAction);
};
