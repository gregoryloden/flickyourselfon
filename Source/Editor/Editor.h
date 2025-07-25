#include "GameState/MapState/MapState.h"
#include <random>
#include "Sprites/Text.h"

#ifdef DEBUG
	#define VALIDATE_MAP_TILES
#endif

class EntityState;

class Editor {
private:
	enum class Zone: unsigned char {
		Right,
		Bottom,
	};
	enum class MouseDragAction: unsigned char {
		None,
		RaiseLowerTile,
		AddRemoveRail,
		AddRemoveSwitch,
		AddRemoveResetSwitch,
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
	//Should only be allocated within an object, on the stack, or as a static object
	class TileDistribution {
	public:
		char min;
		char max;
		function<char(default_random_engine&)> getRandomTile;

		TileDistribution(
			char pMin,
			char pMax,
			function<char(default_random_engine&)> pGetRandomTile,
			vector<TileDistribution*>& containingList);
		TileDistribution(char pMin, char pMax, vector<TileDistribution*>& containingList);
		virtual ~TileDistribution();
	};
	class Button onlyInDebug(: public ObjCounter) {
	public:
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
		//optionally expand the paint map area from a given 1x1 tile centered on the mouse, based on whether or not we are going
		//	to actively paint every tile in this area or just render it
		//by default, expand the paint area to account for the paint box radius whether rendering or painting
		//subclasses can override to provide a different size or to use the initial values that are already assigned
		virtual void expandPaintMapArea(
			int* boxLeftMapX, int* boxTopMapY, int* boxRightMapX, int* boxBottomMapY, bool activePaint);
		//render the button background
		void render();
		//render a rectangle the color of the background to fade this button
		void renderFadedOverlay();
		//render a rectangle outline within the border of this button
		void renderHighlightOutline();
	};
	class TextButton: public Button {
	private:
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
		static constexpr int buttonSize = MapState::tileSize + 2;

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
		static constexpr int buttonWidth = MapState::tileSize + 2;
		static constexpr int buttonHeight = 10;

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
		static constexpr int maxRadius = 7;
		static constexpr int buttonSize = maxRadius + 2;
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
	private:
		static constexpr int buttonSize = PaintBoxRadiusButton::maxRadius + 2;
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
		static constexpr int buttonWidth = MapState::tileSize + 2;
		static constexpr int buttonHeight = MapState::tileSize + 4;

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
	private:
		static constexpr int buttonWidth = 13;
		static constexpr int buttonHeight = 10;

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
	private:
		static constexpr int buttonWidth = 14;
		static constexpr int buttonHeight = 11;
		static const RGB arrowRGB;

	public:
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
	class ExtendPlatformButton: public Button {
	private:
		static constexpr int buttonWidth = 15;
		static constexpr int buttonHeight = 13;
		static const RGB arrowRGB;

		bool collapse;

	public:
		ExtendPlatformButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, bool pCollapse);
		virtual ~ExtendPlatformButton();

		//render a platform with 4 arrows, one per direction
		virtual void renderOverButton();
		//extend the platform
		virtual void paintMap(int x, int y);
	};
	class SwitchButton: public Button {
	public:
		static constexpr int buttonSize = MapState::switchSize + 2;

	private:
		char color;

	public:
		SwitchButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pColor);
		virtual ~SwitchButton();

		char getColor() { return color; }
		//render the switch above the button
		virtual void renderOverButton();
		//set a switch at this position
		virtual void paintMap(int x, int y);
		//track this switch button for other buttons
		virtual void onClick();
		//if painting/placing a switch: leave the 1x1 area
		//if rendering: expand the area to 2x2 with the top-left at the mouse position
		virtual void expandPaintMapArea(
			int* boxLeftMapX, int* boxTopMapY, int* boxRightMapX, int* boxBottomMapY, bool activePaint);
	};
	class RailButton: public Button {
	public:
		static constexpr int buttonSize = MapState::tileSize + 2;

	private:
		char color;

	public:
		RailButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pColor);
		virtual ~RailButton();

		char getColor() { return color; }
		//whether painting or rendering, leave the 1x1 area
		virtual void expandPaintMapArea(
			int* boxLeftMapX, int* boxTopMapY, int* boxRightMapX, int* boxBottomMapY, bool activePaint) {}
		//render the rail above the button
		virtual void renderOverButton();
		//set a rail at this position
		virtual void paintMap(int x, int y);
		//track this rail button for other buttons
		virtual void onClick();
	};
	class RailMovementMagnitudeButton: public Button {
	public:
		static constexpr int buttonSize = MapState::tileSize + 2;
	private:
		static const RGB arrowRGB;

		char magnitudeAdd;

	public:
		RailMovementMagnitudeButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pMagnitudeAdd);
		virtual ~RailMovementMagnitudeButton();

		//whether painting or rendering, leave the 1x1 area
		virtual void expandPaintMapArea(
			int* boxLeftMapX, int* boxTopMapY, int* boxRightMapX, int* boxBottomMapY, bool activePaint) {}
		//render the rail and arrow tip above the button
		virtual void renderOverButton();
		//adjust the movement magnitude of the rail at this position
		virtual void paintMap(int x, int y);
	};
	class RailToggleMovementDirectionButton: public Button {
	public:
		static constexpr int buttonSize = MapState::tileSize + 2;
	private:
		static const RGB arrowRGB;

	public:
		RailToggleMovementDirectionButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		virtual ~RailToggleMovementDirectionButton();

		//whether painting or rendering, leave the 1x1 area
		virtual void expandPaintMapArea(
			int* boxLeftMapX, int* boxTopMapY, int* boxRightMapX, int* boxBottomMapY, bool activePaint) {}
		//render the up and down arrows above the button
		virtual void renderOverButton();
		//toggle the movement direction of the rail at this position
		virtual void paintMap(int x, int y);
	};
	class RailTileOffsetButton: public Button {
	public:
		static constexpr int buttonSize = MapState::tileSize + 2;
	private:
		static const RGB arrowRGB;

		char tileOffset;

	public:
		RailTileOffsetButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pTileOffset);
		virtual ~RailTileOffsetButton();

		//whether painting or rendering, leave the 1x1 area
		virtual void expandPaintMapArea(
			int* boxLeftMapX, int* boxTopMapY, int* boxRightMapX, int* boxBottomMapY, bool activePaint) {}
		//render the rail and arrow tip above the button
		virtual void renderOverButton();
		//adjust the tile offset of the rail at this position
		virtual void paintMap(int x, int y);
	};
	class ResetSwitchButton: public Button {
	private:
		static constexpr int buttonWidth = MapState::tileSize + 2;
		static constexpr int buttonHeight = MapState::tileSize * 2 + 2;

	public:
		ResetSwitchButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY);
		virtual ~ResetSwitchButton();

		//render the reset switch above the button
		virtual void renderOverButton();
		//set a reset switch at this position
		virtual void paintMap(int x, int y);
		//if painting/placing a reset switch: leave the 1x1 area
		//if rendering: expand the area to 1x2 with the bottom at the mouse position
		virtual void expandPaintMapArea(
			int* boxLeftMapX, int* boxTopMapY, int* boxRightMapX, int* boxBottomMapY, bool activePaint);
	};
	class RailSwitchGroupButton: public Button {
	public:
		static constexpr int groupSquareSize = 6;
		static constexpr int buttonSize = groupSquareSize + 2;

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

	static constexpr int noiseTileButtonMaxCount = 16;
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
	static vector<NoiseTileButton*> noiseTileButtons;
	static RaiseLowerTileButton* lowerTileButton;
	static Button* selectedButton;
	static PaintBoxRadiusButton* selectedPaintBoxXRadiusButton;
	static PaintBoxRadiusButton* selectedPaintBoxYRadiusButton;
	static RailSwitchGroupButton* selectedRailSwitchGroupButton;
	static HeightButton* lastSelectedHeightButton;
	static SwitchButton* lastSelectedSwitchButton;
	static RailButton* lastSelectedRailButton;
	static MouseDragAction lastMouseDragAction;
	static int lastMouseDragMapX;
	static int lastMouseDragMapY;
	static default_random_engine* randomEngine;
	static discrete_distribution<int>* noiseTilesDistribution;
	static vector<TileDistribution*> allTileDistributions;
	static TileDistribution floorTileDistribution;
	static TileDistribution wallTileDistribution;
	static TileDistribution platformRightFloorTileDistribution;
	static TileDistribution platformLeftFloorTileDistribution;
	static TileDistribution platformTopFloorTileDistribution;
	static TileDistribution groundLeftFloorTileDistribution;
	static TileDistribution groundRightFloorTileDistribution;
	static bool saveButtonDisabled;
	static bool exportMapButtonDisabled;
public:
	static bool needsGameStateSave;

	//Prevent allocation
	Editor() = delete;
	//build all the editor buttons
	static void loadButtons();
	//delete all the editor buttons
	static void unloadButtons();
	#ifdef VALIDATE_MAP_TILES
		//make sure every tile is in the right tile group based on the heights of tiles near it
		static void Editor::validateMapTiles();
	#endif
	//return the height of the selected height button, or -1 if it's not selected
	static char getSelectedHeight();
private:
	//convert the mouse position to map coordinates
	static void getMouseMapXY(int screenLeftWorldX, int screenTopWorldY, int* outMapX, int* outMapY);
public:
	//see if we clicked on any buttons or the game screen
	static void handleClick(SDL_MouseButtonEvent& clickEvent, bool isDrag, EntityState* camera, int ticksTime);
private:
	//get the distribution of compatible tiles for the given tile, or nullptr if this tile is unique
	static TileDistribution* getTileDistribution(char tile);
public:
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
