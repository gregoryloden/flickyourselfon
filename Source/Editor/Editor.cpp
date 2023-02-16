#include "Editor.h"
#include "GameState/GameState.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"
#include "Util/FileUtils.h"

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
#define newShuffleTileButton(zone, leftX, topY) newWithArgs(Editor::ShuffleTileButton, zone, leftX, topY)
#define newSwitchButton(zone, leftX, topY, color) newWithArgs(Editor::SwitchButton, zone, leftX, topY, color)
#define newRailButton(zone, leftX, topY, color) newWithArgs(Editor::RailButton, zone, leftX, topY, color)
#define newRailTileOffsetButton(zone, leftX, topY, tileOffset) \
	newWithArgs(Editor::RailTileOffsetButton, zone, leftX, topY, tileOffset)
#define newRailToggleMovementDirectionButton(zone, leftX, topY) \
	newWithArgs(Editor::RailToggleMovementDirectionButton, zone, leftX, topY)
#define newResetSwitchButton(zone, leftX, topY) newWithArgs(Editor::ResetSwitchButton, zone, leftX, topY)
#define newRailSwitchGroupButton(zone, leftX, topY, railSwitchGroup) \
	newWithArgs(Editor::RailSwitchGroupButton, zone, leftX, topY, railSwitchGroup)

//////////////////////////////// Editor::EditingMutexLocker ////////////////////////////////
mutex Editor::EditingMutexLocker::editingMutex;
Editor::EditingMutexLocker::EditingMutexLocker() {
	if (isActive)
		editingMutex.lock();
}
Editor::EditingMutexLocker::~EditingMutexLocker() {
	if (isActive)
		editingMutex.unlock();
}

//////////////////////////////// Editor::RGB ////////////////////////////////
Editor::RGB::RGB(float pRed, float pGreen, float pBlue)
: red(pRed)
, green(pGreen)
, blue(pBlue) {
}
Editor::RGB::~RGB() {}

//////////////////////////////// Editor::Button ////////////////////////////////
const Editor::RGB Editor::Button::buttonGrayRGB (0.5f, 0.5f, 0.5f);
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
void Editor::Button::setWidthAndHeight(int width, int height) {
	rightX = leftX + width;
	bottomY = topY + height;
}
bool Editor::Button::tryHandleClick(int x, int y) {
	if (x < leftX || x >= rightX || y < topY || y >= bottomY)
		return false;
	onClick();
	return true;
}
void Editor::Button::render() {
	renderRGBRect(buttonGrayRGB, 1.0f, leftX, topY, rightX, bottomY);
	renderOverButton();
}
void Editor::Button::renderFadedOverlay() {
	renderRGBRect(backgroundRGB, 0.5f, leftX, topY, rightX, bottomY);
}
void Editor::Button::renderHighlightOutline() {
	SpriteSheet::renderRectangleOutline(
		1.0f, 1.0f, 1.0f, 1.0f, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
}

//////////////////////////////// Editor::TextButton ////////////////////////////////
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
void Editor::TextButton::renderOverButton() {
	Text::render(text.c_str(), textLeftX, textBaselineY, textMetrics.fontScale);
	renderOverTextButton();
}

//////////////////////////////// Editor::SaveButton ////////////////////////////////
Editor::SaveButton::SaveButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
: TextButton(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY, "Save") {
}
Editor::SaveButton::~SaveButton() {}
void Editor::SaveButton::renderOverTextButton() {
	if (saveButtonDisabled)
		renderFadedOverlay();
}
void Editor::SaveButton::onClick() {
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
				char railSwitchData = (Uint8)MapState::editorGetRailSwitchFloorSaveData(mapX, mapY);
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

	FileUtils::saveImage(floorSurface, MapState::floorFileName);
	SDL_DestroyRenderer(floorRenderer);
	SDL_FreeSurface(floorSurface);

	//request to save the game becuase rail/switch ids may have changed
	needsGameStateSave = true;

	saveButtonDisabled = true;
}

//////////////////////////////// Editor::ExportMapButton ////////////////////////////////
Editor::ExportMapButton::ExportMapButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
: TextButton(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY, "Export Map") {
}
Editor::ExportMapButton::~ExportMapButton() {}
void Editor::ExportMapButton::renderOverTextButton() {
	if (exportMapButtonDisabled)
		renderFadedOverlay();
}
void Editor::ExportMapButton::onClick() {
	if (exportMapButtonDisabled)
		return;

	int mapWidth = MapState::mapWidth();
	int mapHeight = MapState::mapHeight();

	SDL_Surface* tilesSurface = FileUtils::loadImage(SpriteRegistry::tilesFileName);
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

	FileUtils::saveImage(mapSurface, mapFileName);
	SDL_DestroyTexture(tilesTexture);
	SDL_DestroyRenderer(mapRenderer);
	SDL_FreeSurface(mapSurface);
	SDL_FreeSurface(tilesSurface);

	exportMapButtonDisabled = true;
}

//////////////////////////////// Editor::TileButton ////////////////////////////////
Editor::TileButton::TileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pTile)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
, tile(pTile) {
	setWidthAndHeight(buttonSize, buttonSize);
}
Editor::TileButton::~TileButton() {}
void Editor::TileButton::renderOverButton() {
	glDisable(GL_BLEND);
	SpriteRegistry::tiles->renderSpriteAtScreenPosition((int)tile, 0, (GLint)leftX + 1, (GLint)topY + 1);
}
void Editor::TileButton::onClick() {
	if (selectedButton == noiseButton)
		addNoiseTile(tile);
	else
		Button::onClick();
}
void Editor::TileButton::paintMap(int x, int y) {
	MapState::editorSetTile(x, y, tile);
}

//////////////////////////////// Editor::HeightButton ////////////////////////////////
Editor::HeightButton::HeightButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pHeight)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
, height(pHeight) {
	setWidthAndHeight(buttonWidth, buttonHeight);
}
Editor::HeightButton::~HeightButton() {}
void Editor::HeightButton::renderOverButton() {
	if (height == MapState::emptySpaceHeight)
		renderRGBRect(whiteRGB, 1.0f, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
	else {
		renderRGBRect(blackRGB, 1.0f, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
		const RGB& heightRGB = (height & 1) == 0 ? heightFloorRGB : heightWallRGB;
		GLint heightY = topY + (GLint)MapState::heightCount / 2 - (GLint)((height + 1) / 2);
		renderRGBRect(heightRGB, 1.0f, leftX + 1, heightY, rightX - 1, heightY + 1);
	}
}
void Editor::HeightButton::onClick() {
	Button::onClick();
	lastSelectedHeightButton = this;
}
void Editor::HeightButton::paintMap(int x, int y) {
	MapState::editorSetHeight(x, y, height);
}

//////////////////////////////// Editor::PaintBoxRadiusButton ////////////////////////////////
const Editor::RGB Editor::PaintBoxRadiusButton::boxRGB (9.0f / 16.0f, 3.0f / 8.0f, 5.0f / 8.0f);
Editor::PaintBoxRadiusButton::PaintBoxRadiusButton(
	objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pRadius, bool pIsXRadius)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
, radius(pRadius)
, isXRadius(pIsXRadius) {
	setWidthAndHeight(buttonSize, buttonSize);
}
Editor::PaintBoxRadiusButton::~PaintBoxRadiusButton() {}
void Editor::PaintBoxRadiusButton::renderOverButton() {
	renderRGBRect(blackRGB, 1.0f, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
	if (isXRadius) {
		int boxLeftX = leftX + 1;
		int boxTopY = topY + 3;
		renderRGBRect(boxRGB, 1.0f, boxLeftX, boxTopY, boxLeftX + radius + 1, boxTopY + 3);
	} else {
		int boxLeftX = leftX + 3;
		int boxTopY = topY + 1;
		renderRGBRect(boxRGB, 1.0f, boxLeftX, boxTopY, boxLeftX + 3, boxTopY + radius + 1);
	}
}
void Editor::PaintBoxRadiusButton::onClick() {
	if (isXRadius)
		selectedPaintBoxXRadiusButton = this;
	else
		selectedPaintBoxYRadiusButton = this;
}

//////////////////////////////// Editor::EvenPaintBoxRadiusButton ////////////////////////////////
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
void Editor::EvenPaintBoxRadiusButton::renderOverButton() {
	renderRGBRect(blackRGB, 1.0f, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
	if (isXEvenRadius) {
		int boxLeftX = leftX + 3;
		int boxTopY = topY + 1;
		int boxBottomY = bottomY - 1;
		renderRGBRect(boxRGB, 1.0f, boxLeftX, boxTopY, boxLeftX + 3, boxBottomY);
		renderRGBRect(lineRGB, 1.0f, boxLeftX + 3, boxTopY, boxLeftX + 4, boxBottomY);
	} else {
		int boxTopY = topY + 3;
		int boxLeftX = leftX + 1;
		int boxRightX = rightX - 1;
		renderRGBRect(boxRGB, 1.0f, boxLeftX, boxTopY, boxRightX, boxTopY + 3);
		renderRGBRect(lineRGB, 1.0f, boxLeftX, boxTopY + 3, boxRightX, boxTopY + 4);
	}
}
void Editor::EvenPaintBoxRadiusButton::onClick() {
	isSelected = !isSelected;
}

//////////////////////////////// Editor::NoiseButton ////////////////////////////////
Editor::NoiseButton::NoiseButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
: TextButton(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY, "Noise") {
}
Editor::NoiseButton::~NoiseButton() {}
void Editor::NoiseButton::paintMap(int x, int y) {
	discrete_distribution<int>& d = *noiseTilesDistribution;
	MapState::editorSetTile(x, y, noiseTileButtons[d(*randomEngine)]->tile);
}

//////////////////////////////// Editor::NoiseTileButton ////////////////////////////////
Editor::NoiseTileButton::NoiseTileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
, tile(-1)
, count(0) {
	setWidthAndHeight(buttonWidth, buttonHeight);
}
Editor::NoiseTileButton::~NoiseTileButton() {}
void Editor::NoiseTileButton::renderOverButton() {
	if (tile >= 0) {
		glDisable(GL_BLEND);
		SpriteRegistry::tiles->renderSpriteAtScreenPosition((int)tile, 0, (GLint)leftX + 1, (GLint)topY + 1);
		int drawCount = count;
		for (int i = 0; i < MapState::tileSize; i++) {
			float digitRGB = (float)(drawCount % 4) / 3.0f;
			drawCount /= 4;
			GLint dotLeft = (GLint)(leftX + i + 1);
			GLint dotTop = (GLint)(topY + MapState::tileSize + 1);
			SpriteSheet::renderFilledRectangle(digitRGB, digitRGB, digitRGB, 1.0f, dotLeft, dotTop, dotLeft + 1, dotTop + 2);
		}
	} else
		renderRGBRect(blackRGB, 1.0f, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
}
void Editor::NoiseTileButton::onClick() {
	if (tile >= 0)
		removeNoiseTile(tile);
}

//////////////////////////////// Editor::RaiseLowerTileButton ////////////////////////////////
Editor::RaiseLowerTileButton::RaiseLowerTileButton(
	objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, bool pIsRaiseTileButton)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
, isRaiseTileButton(pIsRaiseTileButton) {
	setWidthAndHeight(buttonWidth, buttonHeight);
}
Editor::RaiseLowerTileButton::~RaiseLowerTileButton() {}
void Editor::RaiseLowerTileButton::renderOverButton() {
	renderRGBRect(blackRGB, 1.0f, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
	if (isRaiseTileButton) {
		renderRGBRect(heightFloorRGB, 1.0f, leftX + 2, topY + 2, rightX - 2, bottomY - 4);
		renderRGBRect(heightWallRGB, 1.0f, leftX + 2, bottomY - 4, rightX - 2, bottomY - 2);
	} else {
		renderRGBRect(heightWallRGB, 1.0f, leftX + 2, topY + 2, rightX - 2, topY + 4);
		renderRGBRect(heightFloorRGB, 1.0f, leftX + 2, topY + 4, rightX - 2, bottomY - 2);
	}
}
void Editor::RaiseLowerTileButton::paintMap(int x, int y) {
	//don't raise or lower this tile if it's a wall tile, or if we're dragging
	char height = MapState::getHeight(x, y);
	if (lastMouseDragAction != MouseDragAction::None || height % 2 != 0)
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

		char topHeight = MapState::getHeight(x, y - 1);
		//always match an empty space height
		if (topHeight == MapState::emptySpaceHeight)
			replacementHeight = MapState::emptySpaceHeight;
		//the top tile is a floor tile
		else if (topHeight % 2 == 0) {
			//if it's below where this was, match it
			if (topHeight < height)
				replacementHeight = topHeight;
			//otherwise use the wall below where this was
			else
				replacementHeight = height - 1;
		//the top tile is a wall tile
		} else {
			//if it's a lower wall and it's one above a side floor tile, use that
			if (topHeight < height &&
					(topHeight - 1 == MapState::getHeight(x - 1, y) || topHeight - 1 == MapState::getHeight(x + 1, y)))
				replacementHeight = topHeight - 1;
			//otherwise use the wall below it, or the lowest floor for the lowest wall
			else
				replacementHeight = MathUtils::max(0, topHeight - 2);
		}
	}
	MapState::editorSetHeight(x, y, replacementHeight);
	MapState::editorSetHeight(x, newFloorY, newHeight);

	//we replaced the floor with a wall, just set the tile
	if (replacementHeight % 2 != 0) {
		MapState::editorSetTile(x, y, MapState::tileWallFirst);
		ShuffleTileButton::shuffleTile(x, y);
	//we copied an adjacent floor tile, set the right one
	} else
		setAppropriateDefaultFloorTile(x, y, replacementHeight);
	setAppropriateDefaultFloorTile(x, newFloorY, newHeight);
	//set the heights for the old adjacent tiles, whether they match the replacement height or the old height
	setAppropriateDefaultFloorTile(x - 1, y, height);
	setAppropriateDefaultFloorTile(x + 1, y, height);
	if (replacementHeight % 2 == 0) {
		setAppropriateDefaultFloorTile(x - 1, y, replacementHeight);
		setAppropriateDefaultFloorTile(x + 1, y, replacementHeight);
	}
	//set the heights for the new adjacent tiles, whether they match the new height or the old height
	setAppropriateDefaultFloorTile(x - 1, newFloorY, height);
	setAppropriateDefaultFloorTile(x - 1, newFloorY, newHeight);
	setAppropriateDefaultFloorTile(x + 1, newFloorY, height);
	setAppropriateDefaultFloorTile(x + 1, newFloorY, newHeight);
	//and also set the tile below if we lowered this tile and the one below is a floor tile
	if (!isRaiseTileButton) {
		char belowHeight = MapState::getHeight(x, newFloorY + 1);
		if (belowHeight % 2 == 0)
			setAppropriateDefaultFloorTile(x, newFloorY + 1, belowHeight);
	}
}
void Editor::RaiseLowerTileButton::postPaint() {
	lastMouseDragAction = MouseDragAction::RaiseLowerTile;
}
void Editor::RaiseLowerTileButton::setAppropriateDefaultFloorTile(int x, int y, char expectedFloorHeight) {
	MapState::editorSetAppropriateDefaultFloorTile(x, y, expectedFloorHeight);
	ShuffleTileButton::shuffleTile(x, y);
}

//////////////////////////////// Editor::ShuffleTileButton ////////////////////////////////
const Editor::RGB Editor::ShuffleTileButton::arrowRGB (0.75f, 0.75f, 0.75f);
Editor::ShuffleTileButton::ShuffleTileButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY) {
	setWidthAndHeight(buttonWidth, buttonHeight);
}
Editor::ShuffleTileButton::~ShuffleTileButton() {}
void Editor::ShuffleTileButton::renderOverButton() {
	renderRGBRect(blackRGB, 1.0f, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
	renderRGBRect(heightFloorRGB, 1.0f, leftX + 2, topY + 2, leftX + 5, topY + 5);
	renderRGBRect(heightFloorRGB, 1.0f, rightX - 5, topY + 2, rightX - 2, topY + 5);
	renderRGBRect(heightWallRGB, 1.0f, leftX + 2, bottomY - 5, leftX + 5, bottomY - 2);
	renderRGBRect(heightWallRGB, 1.0f, rightX - 5, bottomY - 5, rightX - 2, bottomY - 2);
	renderRGBRect(arrowRGB, 1.0f, leftX + 6, topY + 2, leftX + 7, topY + 5);
	renderRGBRect(arrowRGB, 1.0f, leftX + 7, topY + 3, leftX + 8, topY + 4);
	renderRGBRect(arrowRGB, 1.0f, leftX + 6, bottomY - 4, leftX + 7, bottomY - 3);
	renderRGBRect(arrowRGB, 1.0f, leftX + 7, bottomY - 5, leftX + 8, bottomY - 2);
}
void Editor::ShuffleTileButton::paintMap(int x, int y) {
	//if we're only painting one tile, ensure it changes
	int attempts =
			selectedPaintBoxXRadiusButton->getRadius() == 0
				&& selectedPaintBoxYRadiusButton->getRadius() == 0
				&& !evenPaintBoxXRadiusButton->isSelected
				&& !evenPaintBoxYRadiusButton->isSelected
		? 1000
		: 1;
	for (; attempts > 0; attempts--) {
		if (shuffleTile(x, y))
			return;
	}
}
bool Editor::ShuffleTileButton::shuffleTile(int x, int y) {
	char oldTile = MapState::getTile(x, y);
	char newTile;
	if (oldTile >= MapState::tileFloorFirst && oldTile <= MapState::tileFloorLast)
		newTile = (char)floorTileDistribution(*randomEngine);
	else if (oldTile >= MapState::tileWallFirst && oldTile <= MapState::tileWallLast)
		newTile = (char)wallTileDistribution(*randomEngine);
	else if (oldTile >= MapState::tilePlatformRightFloorFirst && oldTile <= MapState::tilePlatformRightFloorLast)
		newTile = (char)platformRightFloorTileDistribution(*randomEngine);
	else if (oldTile >= MapState::tilePlatformLeftFloorFirst && oldTile <= MapState::tilePlatformLeftFloorLast)
		newTile = (char)platformLeftFloorTileDistribution(*randomEngine);
	else if (oldTile >= MapState::tilePlatformTopFloorFirst && oldTile <= MapState::tilePlatformTopFloorLast)
		newTile = (char)platformTopFloorTileDistribution(*randomEngine);
	else if (oldTile >= MapState::tileGroundLeftFloorFirst && oldTile <= MapState::tileGroundLeftFloorLast)
		newTile = (char)groundLeftFloorTileDistribution(*randomEngine);
	else if (oldTile >= MapState::tileGroundRightFloorFirst && oldTile <= MapState::tileGroundRightFloorLast)
		newTile = (char)groundRightFloorTileDistribution(*randomEngine);
	else
		return true;
	if (newTile == oldTile)
		return false;
	MapState::editorSetTile(x, y, newTile);
	return true;
}

//////////////////////////////// Editor::SwitchButton ////////////////////////////////
Editor::SwitchButton::SwitchButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pColor)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
, color(pColor) {
	setWidthAndHeight(buttonSize, buttonSize);
}
Editor::SwitchButton::~SwitchButton() {}
void Editor::SwitchButton::renderOverButton() {
	renderRGBRect(blackRGB, 1.0f, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
	glEnable(GL_BLEND);
	SpriteRegistry::switches->renderSpriteAtScreenPosition((int)(color * 2 + 1), 0, (GLint)leftX + 1, (GLint)topY + 1);
}
void Editor::SwitchButton::onClick() {
	Button::onClick();
	lastSelectedSwitchButton = this;
}
void Editor::SwitchButton::paintMap(int x, int y) {
	if (clickedNewTile(x, y, MouseDragAction::AddRemoveSwitch))
		MapState::editorSetSwitch(x, y, color, selectedRailSwitchGroupButton->getRailSwitchGroup());
}

//////////////////////////////// Editor::RailButton ////////////////////////////////
Editor::RailButton::RailButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pColor)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
, color(pColor) {
	setWidthAndHeight(buttonSize, buttonSize);
}
Editor::RailButton::~RailButton() {}
void Editor::RailButton::renderOverButton() {
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
void Editor::RailButton::onClick() {
	Button::onClick();
	lastSelectedRailButton = this;
}
void Editor::RailButton::paintMap(int x, int y) {
	if (clickedNewTile(x, y, MouseDragAction::AddRemoveRail))
		MapState::editorSetRail(x, y, color, selectedRailSwitchGroupButton->getRailSwitchGroup());
}

//////////////////////////////// Editor::RailToggleMovementDirectionButton ////////////////////////////////
const Editor::RGB Editor::RailToggleMovementDirectionButton::arrowRGB (0.75f, 0.75f, 0.75f);
Editor::RailToggleMovementDirectionButton::RailToggleMovementDirectionButton(
	objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY) {
	setWidthAndHeight(buttonSize, buttonSize);
}
Editor::RailToggleMovementDirectionButton::~RailToggleMovementDirectionButton() {}
void Editor::RailToggleMovementDirectionButton::renderOverButton() {
	renderRGBRect(blackRGB, 1.0f, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
	renderRGBRect(arrowRGB, 1.0f, leftX + 3, topY + 1, rightX - 3, topY + 2);
	renderRGBRect(arrowRGB, 1.0f, leftX + 2, topY + 2, rightX - 2, topY + 3);
	renderRGBRect(arrowRGB, 1.0f, leftX + 2, bottomY - 3, rightX - 2, bottomY - 2);
	renderRGBRect(arrowRGB, 1.0f, leftX + 3, bottomY - 2, rightX - 3, bottomY - 1);
}
void Editor::RailToggleMovementDirectionButton::paintMap(int x, int y) {
	MapState::editorToggleRailMovementDirection(x, y);
}

//////////////////////////////// Editor::RailTileOffsetButton ////////////////////////////////
const Editor::RGB Editor::RailTileOffsetButton::arrowRGB (0.75f, 0.75f, 0.75f);
Editor::RailTileOffsetButton::RailTileOffsetButton(
	objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pTileOffset)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
, tileOffset(pTileOffset) {
	setWidthAndHeight(buttonSize, buttonSize);
}
Editor::RailTileOffsetButton::~RailTileOffsetButton() {}
void Editor::RailTileOffsetButton::renderOverButton() {
	renderRGBRect(blackRGB, 1.0f, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
	glEnable(GL_BLEND);
	SpriteRegistry::rails->renderSpriteAtScreenPosition(0, 0, (GLint)leftX + 1, (GLint)topY + 1);
	int arrowTopY1 = tileOffset < 0 ? topY + 1 : bottomY - 2;
	int arrowTopY2 = tileOffset < 0 ? topY + 2 : bottomY - 3;
	renderRGBRect(arrowRGB, 1.0f, leftX + 3, arrowTopY1, rightX - 3, arrowTopY1 + 1);
	renderRGBRect(arrowRGB, 1.0f, leftX + 2, arrowTopY2, rightX - 2, arrowTopY2 + 1);
}
void Editor::RailTileOffsetButton::onClick() {
	Button::onClick();
	lastSelectedRailTileOffsetButton = this;
}
void Editor::RailTileOffsetButton::paintMap(int x, int y) {
	MapState::editorAdjustRailInitialTileOffset(x, y, tileOffset);
}

//////////////////////////////// Editor::ResetSwitchButton ////////////////////////////////
Editor::ResetSwitchButton::ResetSwitchButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY) {
	setWidthAndHeight(buttonWidth, buttonHeight);
}
Editor::ResetSwitchButton::~ResetSwitchButton() {}
void Editor::ResetSwitchButton::renderOverButton() {
	renderRGBRect(blackRGB, 1.0f, leftX + 1, topY + 1, rightX - 1, bottomY - 1);
	glEnable(GL_BLEND);
	SpriteRegistry::resetSwitch->renderSpriteAtScreenPosition(0, 0, (GLint)leftX + 1, (GLint)topY + 1);
}
void Editor::ResetSwitchButton::onClick() {
	Button::onClick();
	lastSelectedResetSwitchButton = this;
}
void Editor::ResetSwitchButton::paintMap(int x, int y) {
	if (lastMouseDragAction == MouseDragAction::None) {
		lastMouseDragAction = MouseDragAction::AddRemoveResetSwitch;
		MapState::editorSetResetSwitch(x, y);
	}
}

//////////////////////////////// Editor::RailSwitchGroupButton ////////////////////////////////
Editor::RailSwitchGroupButton::RailSwitchGroupButton(
	objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY, char pRailSwitchGroup)
: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
, railSwitchGroup(pRailSwitchGroup) {
	setWidthAndHeight(buttonSize, buttonSize);
}
Editor::RailSwitchGroupButton::~RailSwitchGroupButton() {}
void Editor::RailSwitchGroupButton::renderOverButton() {
	MapState::renderGroupRect(railSwitchGroup, (GLint)leftX + 1, (GLint)topY + 1, (GLint)rightX - 1, (GLint)bottomY - 1);
}
void Editor::RailSwitchGroupButton::onClick() {
	selectedRailSwitchGroupButton = this;
}

//////////////////////////////// Editor ////////////////////////////////
const Editor::RGB Editor::blackRGB (0.0f, 0.0f, 0.0f);
const Editor::RGB Editor::whiteRGB (1.0f, 1.0f, 1.0f);
const Editor::RGB Editor::backgroundRGB (0.25f, 0.75f, 0.75f);
const Editor::RGB Editor::heightFloorRGB (0.0f, 0.75f, 9.0f / 16.0f);
const Editor::RGB Editor::heightWallRGB (5.0f / 8.0f, 3.0f / 8.0f, 0.25f);
bool Editor::isActive = false;
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
Editor::ResetSwitchButton* Editor::lastSelectedResetSwitchButton = nullptr;
Editor::MouseDragAction Editor::lastMouseDragAction = Editor::MouseDragAction::None;
int Editor::lastMouseDragMapX = -1;
int Editor::lastMouseDragMapY = -1;
default_random_engine* Editor::randomEngine = nullptr;
discrete_distribution<int>* Editor::noiseTilesDistribution = nullptr;
discrete_distribution<int> Editor::floorTileDistribution ({ 6.25, 6.25, 6.25, 6.25, 1, 1, 1, 1, 1 });
uniform_int_distribution<int> Editor::wallTileDistribution (MapState::tileWallFirst, MapState::tileWallLast);
uniform_int_distribution<int> Editor::platformRightFloorTileDistribution (
	MapState::tilePlatformRightFloorFirst, MapState::tilePlatformRightFloorLast);
uniform_int_distribution<int> Editor::platformLeftFloorTileDistribution (
	MapState::tilePlatformLeftFloorFirst, MapState::tilePlatformLeftFloorLast);
uniform_int_distribution<int> Editor::platformTopFloorTileDistribution (
	MapState::tilePlatformTopFloorFirst, MapState::tilePlatformTopFloorLast);
uniform_int_distribution<int> Editor::groundLeftFloorTileDistribution (
	MapState::tileGroundLeftFloorFirst, MapState::tileGroundLeftFloorLast);
uniform_int_distribution<int> Editor::groundRightFloorTileDistribution (
	MapState::tileGroundRightFloorFirst, MapState::tileGroundRightFloorLast);
bool Editor::saveButtonDisabled = true;
bool Editor::exportMapButtonDisabled = false;
bool Editor::needsGameStateSave = false;
void Editor::loadButtons() {
	buttons.push_back(newSaveButton(Zone::Right, 5, 5));
	buttons.push_back(newExportMapButton(Zone::Right, 45, 5));
	for (char i = 0; i < (char)MapState::tileCount; i++)
		buttons.push_back(
			newTileButton(Zone::Right, 5 + TileButton::buttonSize * (i % 16), 23 + TileButton::buttonSize * (i / 16), i));
	for (char i = 0; i < (char)MapState::heightCount; i++)
		buttons.push_back(
			newHeightButton(Zone::Right, 5 + HeightButton::buttonWidth * i / 2, 58 + HeightButton::buttonHeight * (i % 2), i));
	for (char i = 0; i < (char)PaintBoxRadiusButton::maxRadius; i++) {
		PaintBoxRadiusButton* button =
			newPaintBoxRadiusButton(Zone::Right, 5 + PaintBoxRadiusButton::buttonSize * i, 81, i, true);
		if (i == 0)
			selectedPaintBoxXRadiusButton = button;
		buttons.push_back(button);
	}
	for (char i = 0; i < (char)PaintBoxRadiusButton::maxRadius; i++) {
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
	buttons.push_back(newShuffleTileButton(Zone::Right, 81, 95));
	for (char i = 0; i < 4; i++)
		buttons.push_back(newSwitchButton(Zone::Right, 5 + SwitchButton::buttonSize * i, 108, i));
	for (char i = 0; i < 4; i++)
		buttons.push_back(newRailButton(Zone::Right, 64 + RailButton::buttonSize * i, 114, i));
	buttons.push_back(newRailToggleMovementDirectionButton(Zone::Right, 107, 103));
	for (char i = -1; i <= 1; i += 2)
		buttons.push_back(newRailTileOffsetButton(Zone::Right, 103 + RailTileOffsetButton::buttonSize * i / 2, 114, i));
	buttons.push_back(newResetSwitchButton(Zone::Right, 118, 108));
	for (char i = 0; i < (char)railSwitchGroupCount; i++) {
		RailSwitchGroupButton* button = newRailSwitchGroupButton(
			Zone::Right, 5 + TileButton::buttonSize * (i % 16), 125 + TileButton::buttonSize * (i / 16), i);
		if (i == 0)
			selectedRailSwitchGroupButton = button;
		buttons.push_back(button);
	}
}
void Editor::unloadButtons() {
	for (Button* button : buttons)
		delete button;
	buttons.clear();
	delete[] noiseTileButtons;
	noiseTileButtons = nullptr;
	delete randomEngine;
	randomEngine = nullptr;
	delete noiseTilesDistribution;
	noiseTilesDistribution = nullptr;
}
char Editor::getSelectedHeight() {
	return selectedButton != nullptr && selectedButton == lastSelectedHeightButton
		? lastSelectedHeightButton->getHeight()
		: MapState::invalidHeight;
}
void Editor::getMouseMapXY(int screenLeftWorldX, int screenTopWorldY, int* outMapX, int* outMapY) {
	int mouseX;
	int mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);
	int scaledMouseX = (int)((float)mouseX / Config::currentPixelWidth);
	int scaledMouseY = (int)((float)mouseY / Config::currentPixelHeight);
	*outMapX = (scaledMouseX + screenLeftWorldX) / MapState::tileSize;
	*outMapY = (scaledMouseY + screenTopWorldY) / MapState::tileSize;
}
void Editor::handleClick(SDL_MouseButtonEvent& clickEvent, bool isDrag, EntityState* camera, int ticksTime) {
	EditingMutexLocker editingMutexLocker;
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

		if (selectedButton == noiseButton) {
			vector<double> tilesDistribution;
			for (int i = 0; i < noiseTileButtonMaxCount; i++)
				tilesDistribution.push_back((double)(noiseTileButtons[i]->count));
			delete noiseTilesDistribution;
			noiseTilesDistribution = new discrete_distribution<int>(tilesDistribution.begin(), tilesDistribution.end());
		}
		if (randomEngine == nullptr)
			randomEngine = new default_random_engine((ticksTime << 16) | (screenLeftWorldX << 8) | screenTopWorldY);

		//we need to iterate from bottom to top if we're lowering tiles, otherwise changes in one row would overwrite changes in
		//	another row
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
		} else if (selectedButton == lastSelectedResetSwitchButton) {
			boxMapWidth = 1;
			boxMapHeight = 2;
			boxTopMapOffset = 1;
		} else {
			boxLeftMapOffset = (int)selectedPaintBoxXRadiusButton->getRadius();
			boxTopMapOffset = (int)selectedPaintBoxYRadiusButton->getRadius();
			boxMapWidth = (boxLeftMapOffset * 2 + (evenPaintBoxXRadiusButton->isSelected ? 2 : 1));
			boxMapHeight = (boxTopMapOffset * 2 + (evenPaintBoxYRadiusButton->isSelected ? 2 : 1));
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
	//we found either a matching tile or an empty tile
	NoiseTileButton* button = noiseTileButtons[foundTileIndex];
	if (foundFreeTile) {
		button->tile = tile;
		button->count = 1;
	} else
		button->count++;
}
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
bool Editor::clickedNewTile(int x, int y, MouseDragAction clickedAction) {
	if (lastMouseDragAction == MouseDragAction::None)
		lastMouseDragAction = clickedAction;
	else if (x == lastMouseDragMapX && y == lastMouseDragMapY)
		return false;

	lastMouseDragMapX = x;
	lastMouseDragMapY = y;
	return true;
}
