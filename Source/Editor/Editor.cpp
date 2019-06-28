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
	#define newPaintBoxRadiusButton(zone, leftX, topY, radius, isXRadius) \
		newWithArgs(Editor::PaintBoxRadiusButton, zone, leftX, topY, radius, isXRadius)
	#define newEvenPaintBoxRadiusButton(zone, leftX, topY, isXEvenRadius) \
		newWithArgs(Editor::EvenPaintBoxRadiusButton, zone, leftX, topY, isXEvenRadius)
	#define newNoiseButton(zone, leftX, topY) newWithArgs(Editor::NoiseButton, zone, leftX, topY)
	#define newNoiseTileButton(zone, leftX, topY) newWithArgs(Editor::NoiseTileButton, zone, leftX, topY)
	#define newRaiseLowerTileButton(zone, leftX, topY, isRaiseTileButton) \
		newWithArgs(Editor::RaiseLowerTileButton, zone, leftX, topY, isRaiseTileButton)
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

		int mapExtensionLeftX = 0;
		int mapExtensionRightX = 0;
		int mapExtensionTopY = 0;
		int mapExtensionBottomY = 0;
		int exportedMapWidth = mapExtensionLeftX + MapState::mapWidth() + mapExtensionRightX;
		int exportedMapHeight = mapExtensionTopY + MapState::mapHeight() + mapExtensionBottomY;

		SDL_Surface* floorSurface =
			SDL_CreateRGBSurface(0, exportedMapWidth, exportedMapHeight, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
		SDL_Renderer* floorRenderer = SDL_CreateSoftwareRenderer(floorSurface);

		for (int exportedMapY = 0; exportedMapY < exportedMapWidth; exportedMapY++) {
			for (int exportedMapX = 0; exportedMapX < exportedMapHeight; exportedMapX++) {
				int mapX = exportedMapX - mapExtensionLeftX;
				int mapY = exportedMapY - mapExtensionTopY;
				if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight)
					//we're extending the map, write an empty tile
					SDL_SetRenderDrawColor(floorRenderer, 254, 255, 255, 255);
				else {
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
				}
				SDL_RenderDrawPoint(floorRenderer, exportedMapX, exportedMapY);
			}
		}

		IMG_SavePNG(floorSurface, MapState::floorFileName);
		SDL_DestroyRenderer(floorRenderer);
		SDL_FreeSurface(floorSurface);

		//request to save the game becuase rail/switch ids may have changed
		needsGameStateSave = true;

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
		objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pRadius, bool pIsXRadius)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, radius(pRadius)
	, isXRadius(pIsXRadius) {
		setWidthAndHeight(buttonSize, buttonSize);
	}
	Editor::PaintBoxRadiusButton::~PaintBoxRadiusButton() {}
	//render the box radius above the button
	void Editor::PaintBoxRadiusButton::render() {
		Button::render();
		SpriteSheet::renderFilledRectangle(
			0.0f, 0.0f, 0.0f, 1.0f, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
		if (isXRadius) {
			GLint boxLeftX = (GLint)leftX + 1;
			GLint boxTopY = (GLint)topY + 3;
			SpriteSheet::renderFilledRectangle(
				boxRGB.red, boxRGB.green, boxRGB.blue, 1.0f, boxLeftX, boxTopY, boxLeftX + (GLint)radius + 1, boxTopY + 3);
		} else {
			GLint boxLeftX = (GLint)leftX + 3;
			GLint boxTopY = (GLint)topY + 1;
			SpriteSheet::renderFilledRectangle(
				boxRGB.red, boxRGB.green, boxRGB.blue, 1.0f, boxLeftX, boxTopY, boxLeftX + 3, boxTopY + (GLint)radius + 1);
		}
	}
	//select a radius to use when painting
	void Editor::PaintBoxRadiusButton::doAction() {
		if (isXRadius)
			selectedPaintBoxXRadiusButton = this;
		else
			selectedPaintBoxYRadiusButton = this;
	}

	//////////////////////////////// Editor::EvenPaintBoxRadiusButton ////////////////////////////////
	const int Editor::EvenPaintBoxRadiusButton::buttonSize = paintBoxMaxRadius + 2;
	const Editor::RGB Editor::EvenPaintBoxRadiusButton::boxRGB (0.5f, 0.5f, 0.5f);
	const Editor::RGB Editor::EvenPaintBoxRadiusButton::lineRGB (0.75f, 0.75f, 0.75f);
	Editor::EvenPaintBoxRadiusButton::EvenPaintBoxRadiusButton(
		objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, bool pIsXEvenRadius)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, isXEvenRadius(pIsXEvenRadius)
	, isSelected(false) {
		setWidthAndHeight(buttonSize, buttonSize);
	}
	Editor::EvenPaintBoxRadiusButton::~EvenPaintBoxRadiusButton() {}
	//render the box radius extension above the button
	void Editor::EvenPaintBoxRadiusButton::render() {
		Button::render();
		SpriteSheet::renderFilledRectangle(
			0.0f, 0.0f, 0.0f, 1.0f, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
		if (isXEvenRadius) {
			GLint boxLeftX = (GLint)leftX + 3;
			GLint boxTopY = (GLint)topY + 1;
			GLint boxBottomY = (GLint)bottomY - 1;
			SpriteSheet::renderFilledRectangle(
				boxRGB.red, boxRGB.green, boxRGB.blue, 1.0f, boxLeftX, boxTopY, boxLeftX + 3, boxBottomY);
			SpriteSheet::renderFilledRectangle(
				lineRGB.red, lineRGB.green, lineRGB.blue, 1.0f, boxLeftX + 3, boxTopY, boxLeftX + 4, boxBottomY);
		} else {
			GLint boxTopY = (GLint)topY + 3;
			GLint boxLeftX = (GLint)leftX + 1;
			GLint boxRightX = (GLint)rightX - 1;
			SpriteSheet::renderFilledRectangle(
				boxRGB.red, boxRGB.green, boxRGB.blue, 1.0f, boxLeftX, boxTopY, boxRightX, boxTopY + 3);
			SpriteSheet::renderFilledRectangle(
				lineRGB.red, lineRGB.green, lineRGB.blue, 1.0f, boxLeftX, boxTopY + 3, boxRightX, boxTopY + 4);
		}
	}
	//add or remove an extra row below/to the right of the paint box
	void Editor::EvenPaintBoxRadiusButton::doAction() {
		isSelected = !isSelected;
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

	//////////////////////////////// Editor::RaiseLowerTileButton ////////////////////////////////
	const int Editor::RaiseLowerTileButton::buttonWidth = 13;
	const int Editor::RaiseLowerTileButton::buttonHeight = 10;
	Editor::RaiseLowerTileButton::RaiseLowerTileButton(
		objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, bool pIsRaiseTileButton)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, isRaiseTileButton(pIsRaiseTileButton) {
		setWidthAndHeight(buttonWidth, buttonHeight);
	}
	Editor::RaiseLowerTileButton::~RaiseLowerTileButton() {}
	//render the a raised or lowered platform above the button
	void Editor::RaiseLowerTileButton::render() {
		Button::render();
		SpriteSheet::renderFilledRectangle(
			0.0f, 0.0f, 0.0f, 1.0f, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
		if (isRaiseTileButton) {
			renderRGBRect(heightFloorRGB, 1.0f, leftX + 2, topY + 2, rightX - 2, bottomY - 4);
			renderRGBRect(heightWallRGB, 1.0f, leftX + 2, bottomY - 4, rightX - 2, bottomY - 2);
		} else {
			renderRGBRect(heightWallRGB, 1.0f, leftX + 2, topY + 2, rightX - 2, topY + 4);
			renderRGBRect(heightFloorRGB, 1.0f, leftX + 2, topY + 4, rightX - 2, bottomY - 2);
		}
	}
	//select raising or lowering as the painting action
	void Editor::RaiseLowerTileButton::doAction() {
		selectedButton = selectedButton == this ? nullptr : this;
	}
	//raise or lower the tile at this position
	void Editor::RaiseLowerTileButton::paintMap(int x, int y) {
		//don't raise or lower this tile if it's a wall tile, or if we're dragging
		char height = MapState::getHeight(x, y);
		if (lastMouseDragAction == MouseDragAction::RaiseLowerTile || height % 2 != 0)
			return;

		char newFloorY;
		char newHeight;
		char replacementHeight;
		if (isRaiseTileButton) {
			if (height == MapState::highestFloorHeight)
				return;
			newFloorY = y - 1;
			newHeight = height + 2;
			replacementHeight = height + 1;
		} else {
			if (height == 0)
				return;
			newFloorY = y + 1;
			newHeight = height - 2;

			//if the top tile has a lower height than this one, match the height of the highest adjacent floor tile lower than
			//	where this was
			char topHeight = MapState::getHeight(x, y - 1);
			if ((topHeight <= newHeight && topHeight % 2 == 0) || topHeight == MapState::emptySpaceHeight) {
				replacementHeight = topHeight;
				char sideHeights[] = { MapState::getHeight(x - 1, y), MapState::getHeight(x + 1, y) };
				for (int i = 0; i < 2; i++) {
					char sideHeight = sideHeights[i];
					if (sideHeight <= newHeight
							&& sideHeight % 2 == 0
							&& (replacementHeight == MapState::emptySpaceHeight || sideHeight > replacementHeight))
						replacementHeight = sideHeight;
				}
			//otherwise use the regular wall tile
			} else
				replacementHeight = height - 1;
		}
		MapState::setHeight(x, y, replacementHeight);
		MapState::setHeight(x, newFloorY, newHeight);

		//we replaced the floor with a wall, just set the tile
		if (replacementHeight % 2 != 0)
			MapState::setTile(x, y, MapState::defaultWallTile);
		//we copied an adjacent floor tile, set the right one
		else
			MapState::setAppropriateDefaultFloorTile(x, y, replacementHeight);
		MapState::setAppropriateDefaultFloorTile(x, newFloorY, newHeight);
		//set the heights for the old adjacent tiles, whether they match the replacement height or the old height
		MapState::setAppropriateDefaultFloorTile(x - 1, y, height);
		MapState::setAppropriateDefaultFloorTile(x + 1, y, height);
		if (replacementHeight % 2 == 0) {
			MapState::setAppropriateDefaultFloorTile(x - 1, y, replacementHeight);
			MapState::setAppropriateDefaultFloorTile(x + 1, y, replacementHeight);
		}
		//set the heights for the new adjacent tiles, whether they match the new height or the old height
		MapState::setAppropriateDefaultFloorTile(x - 1, newFloorY, height);
		MapState::setAppropriateDefaultFloorTile(x - 1, newFloorY, newHeight);
		MapState::setAppropriateDefaultFloorTile(x + 1, newFloorY, height);
		MapState::setAppropriateDefaultFloorTile(x + 1, newFloorY, newHeight);
		//and also set the tile below if we lowered this tile and the one below is a floor tile
		if (!isRaiseTileButton) {
			char belowHeight = MapState::getHeight(x, newFloorY + 1);
			if (belowHeight % 2 == 0)
				MapState::setAppropriateDefaultFloorTile(x, newFloorY + 1, belowHeight);
		}
	}
	//set the drag action so that we don't raise/lower tiles while dragging
	void Editor::RaiseLowerTileButton::postPaint() {
		lastMouseDragAction = MouseDragAction::RaiseLowerTile;
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
		if (lastMouseDragAction == MouseDragAction::None)
			lastMouseDragAction = MouseDragAction::AddRemoveRail;
		//if we're dragging, don't do anything unless we clicked 1 tile away from the last tile
		else if (MathUtils::abs(x - lastMouseDragMapX) + MathUtils::abs(y - lastMouseDragMapY) != 1)
			return;

		MapState::setRail(x, y, color, selectedRailSwitchGroupButton->getRailSwitchGroup());
		lastMouseDragMapX = x;
		lastMouseDragMapY = y;
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
	const Editor::RGB Editor::heightFloorRGB (0.0f, 0.75f, 9.0f / 16.0f);
	const Editor::RGB Editor::heightWallRGB (5.0f / 8.0f, 3.0f / 8.0f, 0.25f);
	vector<Editor::Button*> Editor::buttons;
	Editor::EvenPaintBoxRadiusButton* Editor::evenPaintBoxXRadiusButton = nullptr;
	Editor::EvenPaintBoxRadiusButton* Editor::evenPaintBoxYRadiusButton = nullptr;
	Editor::NoiseButton* Editor::noiseButton = nullptr;
	Editor::NoiseTileButton** Editor::noiseTileButtons = nullptr;
	Editor::RaiseLowerTileButton* Editor::lowerTileButton = nullptr;
	Editor::Button* Editor::selectedButton = nullptr;
	Editor::PaintBoxRadiusButton* Editor::selectedPaintBoxXRadiusButton = nullptr;
	Editor::PaintBoxRadiusButton* Editor::selectedPaintBoxYRadiusButton = nullptr;
	Editor::RailSwitchGroupButton* Editor::selectedRailSwitchGroupButton = nullptr;
	Editor::HeightButton* Editor::lastSelectedHeightButton = nullptr;
	Editor::SwitchButton* Editor::lastSelectedSwitchButton = nullptr;
	Editor::RailButton* Editor::lastSelectedRailButton = nullptr;
	Editor::RailTileOffsetButton* Editor::lastSelectedRailTileOffsetButton = nullptr;
	Editor::MouseDragAction Editor::lastMouseDragAction = Editor::MouseDragAction::None;
	int Editor::lastMouseDragMapX = -1;
	int Editor::lastMouseDragMapY = -1;
	default_random_engine* Editor::randomEngine = nullptr;
	discrete_distribution<int>* Editor::randomDistribution = nullptr;
	bool Editor::saveButtonDisabled = true;
	bool Editor::exportMapButtonDisabled = false;
	bool Editor::needsGameStateSave = false;
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
					Zone::Right, 5 + HeightButton::buttonWidth * i / 2, 58 + HeightButton::buttonHeight * (i % 2), i));
		for (char i = 0; i < (char)paintBoxMaxRadius; i++) {
			PaintBoxRadiusButton* button =
				newPaintBoxRadiusButton(Zone::Right, 5 + PaintBoxRadiusButton::buttonSize * i, 81, i, true);
			if (i == 0)
				selectedPaintBoxXRadiusButton = button;
			buttons.push_back(button);
		}
		for (char i = 0; i < (char)paintBoxMaxRadius; i++) {
			PaintBoxRadiusButton* button =
				newPaintBoxRadiusButton(Zone::Right, 5 + PaintBoxRadiusButton::buttonSize * i, 91, i, false);
			if (i == 0)
				selectedPaintBoxYRadiusButton = button;
			buttons.push_back(button);
		}
		evenPaintBoxXRadiusButton = newEvenPaintBoxRadiusButton(Zone::Right, 69, 81, true);
		buttons.push_back(evenPaintBoxXRadiusButton);
		evenPaintBoxYRadiusButton = newEvenPaintBoxRadiusButton(Zone::Right, 69, 91, false);
		buttons.push_back(evenPaintBoxYRadiusButton);
		noiseButton = newNoiseButton(Zone::Right, 108, 58);
		buttons.push_back(noiseButton);
		noiseTileButtons = new NoiseTileButton*[noiseTileButtonMaxCount];
		for (char i = 0; i < (char)noiseTileButtonMaxCount; i++) {
			NoiseTileButton* button = newNoiseTileButton(
				Zone::Right, 81 + NoiseTileButton::buttonWidth * (i % 8), 72 + NoiseTileButton::buttonHeight * (i / 8));
			noiseTileButtons[i] = button;
			buttons.push_back(button);
		}
		buttons.push_back(newRaiseLowerTileButton(Zone::Right, 76, 58, true));
		lowerTileButton = newRaiseLowerTileButton(Zone::Right, 92, 58, false);
		buttons.push_back(lowerTileButton);
		for (char i = 0; i < 4; i++)
			buttons.push_back(newSwitchButton(Zone::Right, 5 + SwitchButton::buttonSize * i, 108, i));
		for (char i = 0; i < 4; i++)
			buttons.push_back(newRailButton(Zone::Right, 64 + RailButton::buttonSize * i, 114, i));
		for (char i = -1; i <= 1; i += 2)
			buttons.push_back(newRailTileOffsetButton(Zone::Right, 103 + RailTileOffsetButton::buttonSize * i / 2, 114, i));
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
	void Editor::handleClick(SDL_MouseButtonEvent& clickEvent, bool isDrag, EntityState* camera, int ticksTime) {
		if (!isDrag)
			lastMouseDragAction = MouseDragAction::None;

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
			int lowMapX = mouseMapX;
			int lowMapY = mouseMapY;
			int highMapX = mouseMapX;
			int highMapY = mouseMapY;

			if (selectedButton != lastSelectedSwitchButton
				&& selectedButton != lastSelectedRailButton
				&& selectedButton != lastSelectedRailTileOffsetButton)
			{
				int xRadius = selectedPaintBoxXRadiusButton->getRadius();
				int yRadius = selectedPaintBoxYRadiusButton->getRadius();
				lowMapX = MathUtils::max(mouseMapX - xRadius, 0);
				lowMapY = MathUtils::max(mouseMapY - yRadius, 0);
				highMapX = MathUtils::min(
					mouseMapX + xRadius + (evenPaintBoxXRadiusButton->isSelected ? 1 : 0), MapState::mapWidth() - 1);
				highMapY = MathUtils::min(
					mouseMapY + yRadius + (evenPaintBoxYRadiusButton->isSelected ? 1 : 0), MapState::mapHeight() - 1);
			}

			//we need to iterate from bottom to top if we're lowering tiles,
			//	otherwise changes in one row would overwrite changes in another row
			if (selectedButton == lowerTileButton) {
				for (int mapY = highMapY; mapY >= lowMapY; mapY--) {
					for (int mapX = lowMapX; mapX <= highMapX; mapX++) {
						selectedButton->paintMap(mapX, mapY);
					}
				}
			} else {
				for (int mapY = lowMapY; mapY <= highMapY; mapY++) {
					for (int mapX = lowMapX; mapX <= highMapX; mapX++) {
						selectedButton->paintMap(mapX, mapY);
					}
				}
			}
			selectedButton->postPaint();

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
			int boxLeftMapOffset = 0;
			int boxTopMapOffset = 0;
			int boxMapWidth;
			int boxMapHeight;
			if (selectedButton == lastSelectedSwitchButton) {
				boxMapWidth = 2;
				boxMapHeight = 2;
			} else if (selectedButton == lastSelectedRailButton || selectedButton == lastSelectedRailTileOffsetButton) {
				boxMapWidth = 1;
				boxMapHeight = 1;
			} else {
				boxLeftMapOffset = (int)selectedPaintBoxXRadiusButton->getRadius();
				boxTopMapOffset = (int)selectedPaintBoxYRadiusButton->getRadius();
				boxMapWidth = (boxLeftMapOffset * 2 + 1 + (evenPaintBoxXRadiusButton->isSelected ? 1 : 0));
				boxMapHeight = (boxTopMapOffset * 2 + 1 + (evenPaintBoxYRadiusButton->isSelected ? 1 : 0));
			}
			GLint boxLeftX = (GLint)((mouseMapX - boxLeftMapOffset) * MapState::tileSize - screenLeftWorldX);
			GLint boxTopY = (GLint)((mouseMapY - boxTopMapOffset) * MapState::tileSize - screenTopWorldY);
			GLint boxRightX = boxLeftX + (GLint)(boxMapWidth * MapState::tileSize);
			GLint boxBottomY = boxTopY + (GLint)(boxMapHeight * MapState::tileSize);
			SpriteSheet::renderRectangleOutline(1.0f, 1.0f, 1.0f, 1.0f, boxLeftX, boxTopY, boxRightX, boxBottomY);
		}

		//draw the right and bottom background rectangles around the game view
		renderRGBRect(backgroundRGB, 1.0f, Config::gameScreenWidth, 0, Config::windowScreenWidth, Config::windowScreenHeight);
		renderRGBRect(backgroundRGB, 1.0f, 0, Config::gameScreenHeight, Config::gameScreenWidth, Config::windowScreenHeight);

		//draw the buttons
		for (Button* button : buttons)
			button->render();

		if (selectedButton != nullptr)
			selectedButton->renderHighlightOutline();
		selectedRailSwitchGroupButton->renderHighlightOutline();
		selectedPaintBoxXRadiusButton->renderHighlightOutline();
		selectedPaintBoxYRadiusButton->renderHighlightOutline();
		if (evenPaintBoxXRadiusButton->isSelected)
			evenPaintBoxXRadiusButton->renderHighlightOutline();
		if (evenPaintBoxYRadiusButton->isSelected)
			evenPaintBoxYRadiusButton->renderHighlightOutline();
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
	//render a rectangle of the given color in the given region;
	void Editor::renderRGBRect(const RGB& rgb, float alpha, int leftX, int topY, int rightX, int bottomY) {
		SpriteSheet::renderFilledRectangle(
			(GLfloat)rgb.red,
			(GLfloat)rgb.green,
			(GLfloat)rgb.blue,
			(GLfloat)alpha,
			(GLint)leftX,
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
