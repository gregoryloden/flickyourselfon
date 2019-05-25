#include "Editor.h"
#include "GameState/EntityState.h"
#include "GameState/GameState.h"
#include "GameState/MapState.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

#ifdef EDITOR
	#define newSaveButton(zone, leftX, topY) newWithArgs(Editor::SaveButton, zone, leftX, topY)
	#define newExportMapButton(zone, leftX, topY) newWithArgs(Editor::ExportMapButton, zone, leftX, topY)
	#define newTileButton(zone, leftX, topY, tile) newWithArgs(Editor::TileButton, zone, leftX, topY, tile)
	#define newHeightButton(zone, leftX, topY, height) newWithArgs(Editor::HeightButton, zone, leftX, topY, height)
	#define newPaintBoxRadiusButton(zone, leftX, topY, radius) \
		newWithArgs(Editor::PaintBoxRadiusButton, zone, leftX, topY, radius)
	#define newNoiseButton(zone, leftX, topY) newWithArgs(Editor::NoiseButton, zone, leftX, topY)
	#define newNoiseTileButton(zone, leftX, topY) newWithArgs(Editor::NoiseTileButton, zone, leftX, topY)
	#define newSwitchButton(zone, leftX, topY, color) newWithArgs(Editor::SwitchButton, zone, leftX, topY, color)
	#define newRailButton(zone, leftX, topY, color) newWithArgs(Editor::RailButton, zone, leftX, topY, color)
	#define newRailTileOffsetButton(zone, leftX, topY, tileOffset) \
		newWithArgs(Editor::RailTileOffsetButton, zone, leftX, topY, tileOffset)
	#define newRailSwitchGroupButton(zone, leftX, topY, railSwitchGroup) \
		newWithArgs(Editor::RailSwitchGroupButton, zone, leftX, topY, railSwitchGroup)

	//////////////////////////////// Editor::RGB ////////////////////////////////
	Editor::RGB::RGB(float pRed, float pGreen, float pBlue)
	: red(pRed)
	, green(pGreen)
	, blue(pBlue) {
	}
	Editor::RGB::~RGB() {}

	//////////////////////////////// Editor::Button ////////////////////////////////
	const float Editor::Button::buttonGrayRGB = 0.5f;
	Editor::Button::Button(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
	: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
	leftX(zone == Zone::Right ? zoneLeftX + Config::gameScreenWidth : zoneLeftX)
	, topY(zone == Zone::Bottom ? zoneTopY + Config::gameScreenHeight : zoneTopY)
	, rightX(0)
	, bottomY(0) {
		//subclasses will set these using setWidthAndHeight
		rightX = leftX;
		bottomY = topY;
	}
	Editor::Button::~Button() {}
	//alter rightX and bottomY to account for the now-known width and height
	void Editor::Button::setWidthAndHeight(int width, int height) {
		rightX = leftX + width;
		bottomY = topY + height;
	}
	//check if the click is within this button's bounds, and if it is, do the action for this button
	//returns whether the click was handled by this button
	bool Editor::Button::tryHandleClick(int x, int y) {
		if (x < leftX || x >= rightX || y < topY || y >= bottomY)
			return false;
		doAction();
		return true;
	}
	//render the button background
	void Editor::Button::render() {
		SpriteSheet::renderFilledRectangle(
			buttonGrayRGB, buttonGrayRGB, buttonGrayRGB, 1.0f, (GLint)leftX, (GLint)topY, (GLint)rightX, (GLint)bottomY);
	}
	//render a rectangle the color of the background to fade this button
	void Editor::Button::renderFadedOverlay() {
		SpriteSheet::renderFilledRectangle(
			backgroundRGB.red,
			backgroundRGB.green,
			backgroundRGB.blue,
			0.5f,
			(GLint)leftX,
			(GLint)topY,
			(GLint)rightX,
			(GLint)bottomY);
	}
	//render a rectangle outline within the border of this button
	void Editor::Button::renderHighlightOutline() {
		SpriteSheet::renderRectangleOutline(
			1.0f, 1.0f, 1.0f, 1.0f, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
	}
	//subclasses override this to paint the map at this tile
	void Editor::Button::paintMap(int x, int y) {}

	//////////////////////////////// Editor::TextButton ////////////////////////////////
	const float Editor::TextButton::buttonFontScale = 1.0f;
	Editor::TextButton::TextButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, string pText)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, text(pText)
	, textMetrics(Text::getMetrics(pText.c_str(), buttonFontScale))
	, textLeftX(0)
	, textBaselineY(0) {
		int textWidth = (int)textMetrics.charactersWidth;
		int leftRightPadding = MathUtils::min(textWidth / 5 + 1, buttonMaxLeftRightPadding);
		setWidthAndHeight(
			textWidth + leftRightPadding * 2,
			(int)(textMetrics.aboveBaseline + textMetrics.belowBaseline) + buttonTopBottomPadding * 2);
		textLeftX = (float)(leftX + leftRightPadding);
		textBaselineY = (float)(topY + buttonTopBottomPadding) + textMetrics.aboveBaseline;
	}
	Editor::TextButton::~TextButton() {}
	//render the text above the button background
	void Editor::TextButton::render() {
		Button::render();
		Text::render(text.c_str(), textLeftX, textBaselineY, textMetrics.fontScale);
	}

	//////////////////////////////// Editor::SaveButton ////////////////////////////////
	Editor::SaveButton::SaveButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
	: TextButton(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY, "Save") {
	}
	Editor::SaveButton::~SaveButton() {}
	//fade the save button if it's disabled
	void Editor::SaveButton::render() {
		TextButton::render();
		if (saveButtonDisabled)
			renderFadedOverlay();
	}
	//save the floor image
	void Editor::SaveButton::doAction() {
		if (saveButtonDisabled)
			return;

		int mapWidth = MapState::mapWidth();
		int mapHeight = MapState::mapHeight();

		SDL_Surface* floorSurface = SDL_CreateRGBSurface(0, mapWidth, mapHeight, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
		SDL_Renderer* floorRenderer = SDL_CreateSoftwareRenderer(floorSurface);

		for (int mapY = 0; mapY < mapHeight; mapY++) {
			for (int mapX = 0; mapX < mapWidth; mapX++) {
				char height = MapState::getHeight(mapX, mapY);
				char railSwitchData = (Uint8)MapState::getRailSwitchFloorSaveData(mapX, mapY);
				if (height == MapState::emptySpaceHeight)
					//if we don't have a rail/switch, use 254 for red since bit 0 indicates a rail/switch
					SDL_SetRenderDrawColor(floorRenderer, railSwitchData != 0 ? railSwitchData : 254, 255, 255, 255);
				else {
					char tile = MapState::getTile(mapX, mapY);
					SDL_SetRenderDrawColor(
						floorRenderer,
						(Uint8)railSwitchData,
						(Uint8)((tile + 1) * (char)MapState::tileDivisor - 1),
						(Uint8)((height + 1) * (char)MapState::heightDivisor - 1),
						255);
				}
				SDL_RenderDrawPoint(floorRenderer, mapX, mapY);
			}
		}

		IMG_SavePNG(floorSurface, MapState::floorFileName);
		SDL_DestroyRenderer(floorRenderer);
		SDL_FreeSurface(floorSurface);

		//throw away the whole save file becuase rail/switch ids may have changed
		ofstream file;
		file.open(GameState::savedGameFileName);
		file.close();

		saveButtonDisabled = true;
	}

	//////////////////////////////// Editor::ExportMapButton ////////////////////////////////
	const char* Editor::ExportMapButton::mapFileName = "images/map.png";
	Editor::ExportMapButton::ExportMapButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
	: TextButton(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY, "Export Map") {
	}
	Editor::ExportMapButton::~ExportMapButton() {}
	//fade the export map button if it's disabled
	void Editor::ExportMapButton::render() {
		TextButton::render();
		if (exportMapButtonDisabled)
			renderFadedOverlay();
	}
	//export the map
	void Editor::ExportMapButton::doAction() {
		if (exportMapButtonDisabled)
			return;

		int mapWidth = MapState::mapWidth();
		int mapHeight = MapState::mapHeight();

		SDL_Surface* tilesSurface = IMG_Load(SpriteRegistry::tilesFileName);
		SDL_Surface* mapSurface = SDL_CreateRGBSurface(
			0,
			mapWidth * MapState::tileSize,
			mapHeight * MapState::tileSize,
			tilesSurface->format->BitsPerPixel,
			tilesSurface->format->Rmask,
			tilesSurface->format->Gmask,
			tilesSurface->format->Bmask,
			tilesSurface->format->Amask);
		SDL_Renderer* mapRenderer = SDL_CreateSoftwareRenderer(mapSurface);
		SDL_Texture* tilesTexture = SDL_CreateTextureFromSurface(mapRenderer, tilesSurface);

		for (int mapY = 0; mapY < mapHeight; mapY++) {
			for (int mapX = 0; mapX < mapWidth; mapX++) {
				if (MapState::getHeight(mapX, mapY) == MapState::emptySpaceHeight)
					continue;

				int destinationX = mapX * MapState::tileSize;
				int destinationY = mapY * MapState::tileSize;
				int tileX = (int)MapState::getTile(mapX, mapY) * MapState::tileSize;
				SDL_Rect source { tileX, 0, MapState::tileSize, MapState::tileSize };
				SDL_Rect destination { destinationX, destinationY, MapState::tileSize, MapState::tileSize };
				SDL_RenderCopy(mapRenderer, tilesTexture, &source, &destination);
			}
		}

		IMG_SavePNG(mapSurface, mapFileName);
		SDL_DestroyTexture(tilesTexture);
		SDL_DestroyRenderer(mapRenderer);
		SDL_FreeSurface(mapSurface);
		SDL_FreeSurface(tilesSurface);

		exportMapButtonDisabled = true;
	}

	//////////////////////////////// Editor::TileButton ////////////////////////////////
	const int Editor::TileButton::buttonSize = MapState::tileSize + 2;
	Editor::TileButton::TileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pTile)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, tile(pTile) {
		setWidthAndHeight(buttonSize, buttonSize);
	}
	Editor::TileButton::~TileButton() {}
	//render the tile above the button
	void Editor::TileButton::render() {
		Button::render();
		glDisable(GL_BLEND);
		SpriteRegistry::tiles->renderSpriteAtScreenPosition((int)tile, 0, (GLint)leftX + 1, (GLint)topY + 1);
	}
	//if noisy tile selection is off, highlight this tile for tile painting
	//if noisy tile selection is on, add this tile to the noisy tile list
	void Editor::TileButton::doAction() {
		if (selectedButton == this)
			selectedButton = nullptr;
		else if (selectedButton != noiseButton)
			selectedButton = this;
		else
			addNoiseTile(tile);
	}
	//set the tile at this position
	void Editor::TileButton::paintMap(int x, int y) {
		MapState::setTile(x, y, tile);
	}

	//////////////////////////////// Editor::HeightButton ////////////////////////////////
	const int Editor::HeightButton::buttonWidth = MapState::tileSize + 2;
	const int Editor::HeightButton::buttonHeight = 10;
	const Editor::RGB Editor::HeightButton::heightFloorRGB (0.0f, 0.75f, 9.0f / 16.0f);
	const Editor::RGB Editor::HeightButton::heightWallRGB (5.0f / 8.0f, 3.0f / 8.0f, 0.25f);
	Editor::HeightButton::HeightButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pHeight)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, height(pHeight) {
		setWidthAndHeight(buttonWidth, buttonHeight);
	}
	Editor::HeightButton::~HeightButton() {}
	//render the height graphic above the button
	void Editor::HeightButton::render() {
		Button::render();
		if (height == MapState::emptySpaceHeight)
			SpriteSheet::renderFilledRectangle(
				1.0f, 1.0f, 1.0f, 1.0f, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
		else {
			SpriteSheet::renderFilledRectangle(
				0.0f, 0.0f, 0.0f, 1.0f, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
			const RGB* heightRGB = (height & 1) == 0 ? &heightFloorRGB : &heightWallRGB;
			GLint heightY = topY + (GLint)MapState::heightCount / 2 - (GLint)((height + 1) / 2);
			SpriteSheet::renderFilledRectangle(
				heightRGB->red,
				heightRGB->green,
				heightRGB->blue,
				1.0f,
				(GLint)leftX + 1,
				heightY,
				(GLint)rightX - 1,
				heightY + 1);
		}
	}
	//select a height as the painting action
	void Editor::HeightButton::doAction() {
		selectedButton = selectedButton == this ? nullptr : this;
		lastSelectedHeightButton = this;
	}
	//set the tile at this position
	void Editor::HeightButton::paintMap(int x, int y) {
		MapState::setHeight(x, y, height);
	}

	//////////////////////////////// Editor::PaintBoxRadiusButton ////////////////////////////////
	const int Editor::PaintBoxRadiusButton::buttonSize = paintBoxMaxRadius + 2;
	const Editor::RGB Editor::PaintBoxRadiusButton::boxRGB (9.0f / 16.0f, 3.0f / 8.0f, 5.0f / 8.0f);
	Editor::PaintBoxRadiusButton::PaintBoxRadiusButton(
		objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pRadius)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, radius(pRadius) {
		setWidthAndHeight(buttonSize, buttonSize);
	}
	Editor::PaintBoxRadiusButton::~PaintBoxRadiusButton() {}
	//render the box radius above the button
	void Editor::PaintBoxRadiusButton::render() {
		Button::render();
		SpriteSheet::renderFilledRectangle(
			0.0f, 0.0f, 0.0f, 1.0f, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
		GLint boxLeftX = (GLint)leftX + 1;
		GLint boxTopY = (GLint)topY + 1;
		GLint boxSize = (GLint)(radius + 1);
		SpriteSheet::renderFilledRectangle(
			boxRGB.red, boxRGB.green, boxRGB.blue, 1.0f, boxLeftX, boxTopY, boxLeftX + boxSize, boxTopY + boxSize);
	}
	//select a radius to use when painting
	void Editor::PaintBoxRadiusButton::doAction() {
		selectedPaintBoxRadiusButton = this;
	}

	//////////////////////////////// Editor::NoiseButton ////////////////////////////////
	Editor::NoiseButton::NoiseButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
	: TextButton(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY, "Noise") {
	}
	Editor::NoiseButton::~NoiseButton() {}
	//toggle noisy tile selection
	void Editor::NoiseButton::doAction() {
		selectedButton = selectedButton == this ? nullptr : this;
	}
	//set the tile at this position
	void Editor::NoiseButton::paintMap(int x, int y) {
		discrete_distribution<int>& d = *randomDistribution;
		MapState::setTile(x, y, noiseTileButtons[d(*randomEngine)]->tile);
	}

	//////////////////////////////// Editor::NoiseTileButton ////////////////////////////////
	const int Editor::NoiseTileButton::buttonWidth = MapState::tileSize + 2;
	const int Editor::NoiseTileButton::buttonHeight = MapState::tileSize + 4;
	Editor::NoiseTileButton::NoiseTileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, tile(-1)
	, count(0) {
		setWidthAndHeight(buttonWidth, buttonHeight);
	}
	Editor::NoiseTileButton::~NoiseTileButton() {}
	//render the tile above the button, as well as the count below it in base 4
	void Editor::NoiseTileButton::render() {
		Button::render();
		if (tile >= 0) {
			glDisable(GL_BLEND);
			SpriteRegistry::tiles->renderSpriteAtScreenPosition((int)tile, 0, (GLint)leftX + 1, (GLint)topY + 1);
			int drawCount = count;
			for (int i = 0; i < MapState::tileSize; i++) {
				float digitRGB = (float)(drawCount % 4) / 3.0f;
				drawCount /= 4;
				GLint dotLeft = (GLint)(leftX + i + 1);
				GLint dotTop = (GLint)(topY + MapState::tileSize + 1);
				SpriteSheet::renderFilledRectangle(
					digitRGB, digitRGB, digitRGB, 1.0f, dotLeft, dotTop, dotLeft + 1, dotTop + 2);
			}
		} else
			SpriteSheet::renderFilledRectangle(
				0.0f, 0.0f, 0.0f, 1.0f, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
	}
	//remove a count from this button
	void Editor::NoiseTileButton::doAction() {
		if (tile >= 0)
			removeNoiseTile(tile);
	}

	//////////////////////////////// Editor::SwitchButton ////////////////////////////////
	const int Editor::SwitchButton::buttonSize = MapState::switchSize + 2;
	Editor::SwitchButton::SwitchButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pColor)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, color(pColor) {
		setWidthAndHeight(buttonSize, buttonSize);
	}
	Editor::SwitchButton::~SwitchButton() {}
	//render the switch above the button
	void Editor::SwitchButton::render() {
		Button::render();
		SpriteSheet::renderFilledRectangle(
			0.0f, 0.0f, 0.0f, 1.0f, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
		glEnable(GL_BLEND);
		SpriteRegistry::switches->renderSpriteAtScreenPosition((int)(color * 2 + 1), 0, (GLint)leftX + 1, (GLint)topY + 1);
	}
	//select this switch as the painting action
	void Editor::SwitchButton::doAction() {
		selectedButton = selectedButton == this ? nullptr : this;
		lastSelectedSwitchButton = this;
	}
	//set a switch at this position
	void Editor::SwitchButton::paintMap(int x, int y) {
		MapState::setSwitch(x, y, color, selectedRailSwitchGroupButton->getRailSwitchGroup());
	}

	//////////////////////////////// Editor::RailButton ////////////////////////////////
	const int Editor::RailButton::buttonSize = MapState::tileSize + 2;
	Editor::RailButton::RailButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pColor)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, color(pColor) {
		setWidthAndHeight(buttonSize, buttonSize);
	}
	Editor::RailButton::~RailButton() {}
	//render the rail above the button
	void Editor::RailButton::render() {
		Button::render();
		SpriteSheet::renderFilledRectangle(
			(color == MapState::squareColor || color == MapState::sineColor) ? 0.75f : 0.0f,
			(color == MapState::sawColor || color == MapState::sineColor) ? 0.75f : 0.0f,
			(color == MapState::triangleColor || color == MapState::sineColor) ? 0.75f : 0.0f,
			1.0f,
			(GLint)leftX + 1,
			(GLint)topY + 1,
			(GLint)rightX - 1,
			(GLint)bottomY - 1);
		glEnable(GL_BLEND);
		SpriteRegistry::rails->renderSpriteAtScreenPosition(0, 0, (GLint)leftX + 1, (GLint)topY + 1);
	}
	//select this rail as the painting action
	void Editor::RailButton::doAction() {
		selectedButton = selectedButton == this ? nullptr : this;
		lastSelectedRailButton = this;
	}
	//set a rail at this position
	void Editor::RailButton::paintMap(int x, int y) {
		MapState::setRail(x, y, color, selectedRailSwitchGroupButton->getRailSwitchGroup());
	}

	//////////////////////////////// Editor::RailTileOffsetButton ////////////////////////////////
	const int Editor::RailTileOffsetButton::buttonSize = MapState::tileSize + 2;
	Editor::RailTileOffsetButton::RailTileOffsetButton(
		objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pTileOffset)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, tileOffset(pTileOffset) {
		setWidthAndHeight(buttonSize, buttonSize);
	}
	Editor::RailTileOffsetButton::~RailTileOffsetButton() {}
	//render the rail above the button
	void Editor::RailTileOffsetButton::render() {
		Button::render();
		SpriteSheet::renderFilledRectangle(
			0.0f, 0.0f, 0.0f, 1.0f, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
		glEnable(GL_BLEND);
		SpriteRegistry::rails->renderSpriteAtScreenPosition(0, 0, (GLint)leftX + 1, (GLint)topY + 1);
		GLint arrowTopY1 = (GLint)(tileOffset < 0 ? topY + 1 : bottomY - 2);
		GLint arrowTopY2 = (GLint)(tileOffset < 0 ? topY + 2 : bottomY - 3);
		SpriteSheet::renderFilledRectangle(
			0.75f, 0.75f, 0.75f, 1.0f, (GLint)leftX + 3, arrowTopY1, (GLint)rightX - 3, arrowTopY1 + 1);
		SpriteSheet::renderFilledRectangle(
			0.75f, 0.75f, 0.75f, 1.0f, (GLint)leftX + 2, arrowTopY2, (GLint)rightX - 2, arrowTopY2 + 1);
	}
	//select this rail tile offset as the painting action
	void Editor::RailTileOffsetButton::doAction() {
		selectedButton = selectedButton == this ? nullptr : this;
		lastSelectedRailTileOffsetButton = this;
	}
	//adjust the tile offset of the rail at this position
	void Editor::RailTileOffsetButton::paintMap(int x, int y) {
		MapState::adjustRailInitialTileOffset(x, y, tileOffset);
	}

	//////////////////////////////// Editor::RailSwitchGroupButton ////////////////////////////////
	Editor::RailSwitchGroupButton::RailSwitchGroupButton(
		objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pRailSwitchGroup)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, railSwitchGroup(pRailSwitchGroup) {
		setWidthAndHeight(buttonSize, buttonSize);
	}
	Editor::RailSwitchGroupButton::~RailSwitchGroupButton() {}
	//render the group above the button
	void Editor::RailSwitchGroupButton::render() {
		Button::render();
		renderGroupRect(railSwitchGroup, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
	}
	//select a group to use when painting switches or rails
	void Editor::RailSwitchGroupButton::doAction() {
		selectedRailSwitchGroupButton = this;
	}

	//////////////////////////////// Editor ////////////////////////////////
	const Editor::RGB Editor::backgroundRGB (0.25f, 0.75f, 0.75f);
	vector<Editor::Button*> Editor::buttons;
	Editor::NoiseButton* Editor::noiseButton = nullptr;
	Editor::NoiseTileButton** Editor::noiseTileButtons = nullptr;
	Editor::Button* Editor::selectedButton = nullptr;
	Editor::PaintBoxRadiusButton* Editor::selectedPaintBoxRadiusButton = nullptr;
	Editor::RailSwitchGroupButton* Editor::selectedRailSwitchGroupButton = nullptr;
	Editor::HeightButton* Editor::lastSelectedHeightButton = nullptr;
	Editor::SwitchButton* Editor::lastSelectedSwitchButton = nullptr;
	Editor::RailButton* Editor::lastSelectedRailButton = nullptr;
	Editor::RailTileOffsetButton* Editor::lastSelectedRailTileOffsetButton = nullptr;
	default_random_engine* Editor::randomEngine = nullptr;
	discrete_distribution<int>* Editor::randomDistribution = nullptr;
	bool Editor::saveButtonDisabled = true;
	bool Editor::exportMapButtonDisabled = false;
	//build all the editor buttons
	void Editor::loadButtons() {
		buttons.push_back(newSaveButton(Zone::Right, 5, 5));
		buttons.push_back(newExportMapButton(Zone::Right, 45, 5));
		for (char i = 0; i < (char)MapState::tileCount; i++)
			buttons.push_back(
				newTileButton(Zone::Right, 5 + TileButton::buttonSize * (i % 16), 23 + TileButton::buttonSize * (i / 16), i));
		for (char i = 0; i < (char)MapState::heightCount; i++)
			buttons.push_back(
				newHeightButton(
					Zone::Right, 5 + HeightButton::buttonWidth * (i / 2), 58 + HeightButton::buttonHeight * (i % 2), i));
		for (char i = 0; i < (char)paintBoxMaxRadius; i++) {
			PaintBoxRadiusButton* button =
				newPaintBoxRadiusButton(Zone::Right, 5 + PaintBoxRadiusButton::buttonSize * i, 81, i);
			if (i == 0)
				selectedPaintBoxRadiusButton = button;
			buttons.push_back(button);
		}
		noiseButton = newNoiseButton(Zone::Right, 73, 58);
		buttons.push_back(noiseButton);
		noiseTileButtons = new NoiseTileButton*[noiseTileButtonMaxCount];
		for (char i = 0; i < (char)noiseTileButtonMaxCount; i++) {
			NoiseTileButton* button = newNoiseTileButton(
				Zone::Right, 73 + NoiseTileButton::buttonWidth * (i % 8), 74 + NoiseTileButton::buttonHeight * (i / 8));
			noiseTileButtons[i] = button;
			buttons.push_back(button);
		}
		for (char i = 0; i < 4; i++)
			buttons.push_back(newSwitchButton(Zone::Right, 5 + SwitchButton::buttonSize * i, 97, i));
		for (char i = 0; i < 4; i++)
			buttons.push_back(newRailButton(Zone::Right, 5 + RailButton::buttonSize * i, 114, i));
		for (char i = -1; i <= 1; i += 2)
			buttons.push_back(newRailTileOffsetButton(Zone::Right, 44 + RailTileOffsetButton::buttonSize * i / 2, 114, i));
		for (char i = 0; i < (char)railSwitchGroupCount; i++) {
			RailSwitchGroupButton* button = newRailSwitchGroupButton(
				Zone::Right, 5 + TileButton::buttonSize * (i % 16), 125 + TileButton::buttonSize * (i / 16), i);
			if (i == 0)
				selectedRailSwitchGroupButton = button;
			buttons.push_back(button);
		}
	}
	//delete all the editor buttons
	void Editor::unloadButtons() {
		for (Button* button : buttons)
			delete button;
		buttons.clear();
		delete randomEngine;
		randomEngine = nullptr;
		delete randomDistribution;
		randomDistribution = nullptr;
	}
	//return the height of the selected height button, or -1 if it's not selected
	char Editor::getSelectedHeight() {
		return selectedButton != nullptr && selectedButton == lastSelectedHeightButton
			? lastSelectedHeightButton->getHeight()
			: -1;
	}
	//convert the mouse position to map coordinates
	void Editor::getMouseMapXY(int screenLeftWorldX, int screenTopWorldY, int* outMapX, int* outMapY) {
		int mouseX;
		int mouseY;
		SDL_GetMouseState(&mouseX, &mouseY);
		int scaledMouseX = (int)((float)mouseX / Config::currentPixelWidth);
		int scaledMouseY = (int)((float)mouseY / Config::currentPixelHeight);
		*outMapX = (scaledMouseX + screenLeftWorldX) / MapState::tileSize;
		*outMapY = (scaledMouseY + screenTopWorldY) / MapState::tileSize;
	}
	//see if we clicked on any buttons or the game screen
	void Editor::handleClick(SDL_MouseButtonEvent& clickEvent, EntityState* camera, int ticksTime) {
		int screenX = (int)((float)clickEvent.x / Config::currentPixelWidth);
		int screenY = (int)((float)clickEvent.y / Config::currentPixelHeight);
		for (Button* button : buttons) {
			if (button->tryHandleClick(screenX, screenY))
				return;
		}

		//we didn't hit any of the buttons, check if we're painting part of the map
		if (screenX < Config::gameScreenWidth
			&& screenY < Config::gameScreenHeight
			&& selectedButton != nullptr
			&& (selectedButton != noiseButton || noiseTileButtons[0]->tile != -1))
		{
			if (selectedButton == noiseButton) {
				delete randomEngine;
				randomEngine = new default_random_engine(ticksTime);

				vector<double> tilesDistribution;
				for (int i = 0; i < noiseTileButtonMaxCount; i++)
					tilesDistribution.push_back((double)(noiseTileButtons[i]->count));
				delete randomDistribution;
				randomDistribution = new discrete_distribution<int>(tilesDistribution.begin(), tilesDistribution.end());
			}

			int screenLeftWorldX = MapState::getScreenLeftWorldX(camera, ticksTime);
			int screenTopWorldY = MapState::getScreenTopWorldY(camera, ticksTime);
			int mouseMapX;
			int mouseMapY;
			getMouseMapXY(screenLeftWorldX, screenTopWorldY, &mouseMapX, &mouseMapY);

			int radius =
				(selectedButton == lastSelectedSwitchButton
						|| selectedButton == lastSelectedRailButton
						|| selectedButton == lastSelectedRailTileOffsetButton)
					? 0
					: (int)selectedPaintBoxRadiusButton->getRadius();

			int lowMapX = MathUtils::max(mouseMapX - radius, 0);
			int lowMapY = MathUtils::max(mouseMapY - radius, 0);
			int highMapX = MathUtils::min(mouseMapX + radius, MapState::mapWidth() - 1);
			int highMapY = MathUtils::min(mouseMapY + radius, MapState::mapHeight() - 1);
			for (int mapX = lowMapX; mapX <= highMapX; mapX++) {
				for (int mapY = lowMapY; mapY <= highMapY; mapY++) {
					selectedButton->paintMap(mapX, mapY);
				}
			}

			saveButtonDisabled = false;
			exportMapButtonDisabled = false;
		}
	}
	//draw the editor interface
	void Editor::render(EntityState* camera, int ticksTime) {
		//if we've selected something to paint, draw a box around the area we'll paint
		if (selectedButton != nullptr) {
			//get the map coordinate of the mouse
			int screenLeftWorldX = MapState::getScreenLeftWorldX(camera, ticksTime);
			int screenTopWorldY = MapState::getScreenTopWorldY(camera, ticksTime);
			int mouseMapX;
			int mouseMapY;
			getMouseMapXY(screenLeftWorldX, screenTopWorldY, &mouseMapX, &mouseMapY);

			//draw a mouse selection box
			int boxTopLeftMapOffset;
			int boxMapSize;
			if (selectedButton == lastSelectedSwitchButton) {
				boxTopLeftMapOffset = 0;
				boxMapSize = 2;
			} else if (selectedButton == lastSelectedRailButton || selectedButton == lastSelectedRailTileOffsetButton) {
				boxTopLeftMapOffset = 0;
				boxMapSize = 1;
			} else {
				boxTopLeftMapOffset = (int)selectedPaintBoxRadiusButton->getRadius();
				boxMapSize = (boxTopLeftMapOffset * 2 + 1);
			}
			GLint boxScreenSize = (GLint)(boxMapSize * MapState::tileSize);
			GLint boxLeftX = (GLint)((mouseMapX - boxTopLeftMapOffset) * MapState::tileSize - screenLeftWorldX);
			GLint boxTopY = (GLint)((mouseMapY - boxTopLeftMapOffset) * MapState::tileSize - screenTopWorldY);
			GLint boxRightX = boxLeftX + boxScreenSize;
			GLint boxBottomY = boxTopY + boxScreenSize;
			SpriteSheet::renderRectangleOutline(1.0f, 1.0f, 1.0f, 1.0f, boxLeftX, boxTopY, boxRightX, boxBottomY);
		}

		//draw the 2 background rectangles around the game view
		//right zone
		SpriteSheet::renderFilledRectangle(
			backgroundRGB.red,
			backgroundRGB.green,
			backgroundRGB.blue,
			1.0f,
			(GLint)Config::gameScreenWidth,
			0,
			(GLint)Config::windowScreenWidth,
			(GLint)Config::windowScreenHeight);
		//bottom zone
		SpriteSheet::renderFilledRectangle(
			backgroundRGB.red,
			backgroundRGB.green,
			backgroundRGB.blue,
			1.0f,
			0,
			(GLint)Config::gameScreenHeight,
			(GLint)Config::gameScreenWidth,
			(GLint)Config::windowScreenHeight);

		//draw the buttons
		for (Button* button : buttons)
			button->render();

		if (selectedButton != nullptr)
			selectedButton->renderHighlightOutline();
		selectedRailSwitchGroupButton->renderHighlightOutline();
		selectedPaintBoxRadiusButton->renderHighlightOutline();
	}
	//draw a graphic to represent this rail/switch group
	void Editor::renderGroupRect(char group, int leftX, int topY, int rightX, int bottomY) {
		GLint midX = (GLint)((leftX + rightX) / 2);
		//bits 0-2
		SpriteSheet::renderFilledRectangle(
			(float)(group & 1),
			(float)((group & 2) >> 1),
			(float)((group & 4) >> 2),
			1.0f,
			(GLint)leftX,
			(GLint)topY,
			midX,
			(GLint)bottomY);
		//bits 3-5
		SpriteSheet::renderFilledRectangle(
			(float)((group & 8) >> 3),
			(float)((group & 16) >> 4),
			(float)((group & 32) >> 5),
			1.0f,
			midX,
			(GLint)topY,
			(GLint)rightX,
			(GLint)bottomY);
	}
	//add to the count of this tile
	void Editor::addNoiseTile(char tile) {
		//find the index of our tile, or the first empty tile
		int foundTileIndex = 0;
		bool foundFreeTile = false;
		for (; true; foundTileIndex++) {
			if (foundTileIndex == noiseTileButtonMaxCount)
				return;
			char currentTile = noiseTileButtons[foundTileIndex]->tile;
			if (currentTile == tile)
				break;
			else if (currentTile == -1) {
				foundFreeTile = true;
				break;
			};
		}
		//we found a tile
		NoiseTileButton* button = noiseTileButtons[foundTileIndex];
		if (foundFreeTile) {
			button->tile = tile;
			button->count = 1;
		} else
			button->count++;
	}
	//remove from the count of this tile
	void Editor::removeNoiseTile(char tile) {
		//find the index of our tile
		int foundTileIndex = 0;
		for (; true; foundTileIndex++) {
			if (foundTileIndex == noiseTileButtonMaxCount)
				return;
			else if (noiseTileButtons[foundTileIndex]->tile == tile)
				break;
		}
		NoiseTileButton* button = noiseTileButtons[foundTileIndex];
		button->count--;
		//we removed the last case of this tile, remove it from the set
		if (button->count == 0) {
			for (foundTileIndex++; foundTileIndex < noiseTileButtonMaxCount; foundTileIndex++) {
				NoiseTileButton* nextButton = noiseTileButtons[foundTileIndex];
				button->count = nextButton->count;
				button->tile = nextButton->tile;
				button = nextButton;
			}
			button->count = 0;
			button->tile = -1;
		}
	}
#endif
