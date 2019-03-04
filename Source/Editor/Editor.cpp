#include "Editor.h"
#include "GameState/EntityState.h"
#include "GameState/MapState.h"
#include "Sprites/SpriteRegistry.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

#ifdef EDITOR
	#define newSaveButton(zone, leftX, topY) newWithArgs(Editor::SaveButton, zone, leftX, topY)
	#define newExportMapButton(zone, leftX, topY) newWithArgs(Editor::ExportMapButton, zone, leftX, topY)
	#define newTileButton(tile, zone, leftX, topY) newWithArgs(Editor::TileButton, tile, zone, leftX, topY)
	#define newHeightButton(height, zone, leftX, topY) newWithArgs(Editor::HeightButton, height, zone, leftX, topY)

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
	//render the button background
	void Editor::Button::render() {
		SpriteSheet::renderFilledRectangle(
			buttonGrayRGB, buttonGrayRGB, buttonGrayRGB, 1.0f, (GLint)leftX, (GLint)topY, (GLint)rightX, (GLint)bottomY);
	}
	//check if the click is within this button's bounds, and if it is, do the action for this button
	//returns whether the click was handled by this button
	bool Editor::Button::tryHandleClick(int x, int y) {
		if (x < leftX || x >= rightX || y < topY || y >= bottomY)
			return false;
		doAction();
		return true;
	}

	//////////////////////////////// Editor::TextButton ////////////////////////////////
	const float Editor::TextButton::buttonFontScale = 1.0f;
	Editor::TextButton::TextButton(objCounterParametersComma() string pText, Zone zone, int zoneLeftX, int zoneTopY)
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
	: TextButton(objCounterArgumentsComma() "Save", zone, zoneLeftX, zoneTopY) {
	}
	Editor::SaveButton::~SaveButton() {}
	//save the floor image
	void Editor::SaveButton::doAction() {
		int mapWidth = MapState::mapWidth();
		int mapHeight = MapState::mapHeight();

		SDL_Surface* floorSurface =
			SDL_CreateRGBSurface(
				0,
				mapWidth,
				mapHeight,
				32,
				0xFF0000,
				0xFF00,
				0xFF,
				0xFF000000);
		SDL_Renderer* floorRenderer = SDL_CreateSoftwareRenderer(floorSurface);

		for (int mapY = 0; mapY < mapHeight; mapY++) {
			for (int mapX = 0; mapX < mapWidth; mapX++) {
				char height = MapState::getHeight(mapX, mapY);
				if (height == MapState::emptySpaceHeight)
					SDL_SetRenderDrawColor(floorRenderer, 255, 255, 255, 255);
				else {
					char tile = MapState::getTile(mapX, mapY);
					SDL_SetRenderDrawColor(
						floorRenderer,
						0,
						(Uint8)(((int)tile + 1) * MapState::tileDivisor - 1),
						(Uint8)(((int)height + 1) * MapState::heightDivisor - 1),
						255);
				}
				SDL_RenderDrawPoint(floorRenderer, mapX, mapY);
			}
		}

		IMG_SavePNG(floorSurface, MapState::floorFileName);
		SDL_DestroyRenderer(floorRenderer);
		SDL_FreeSurface(floorSurface);
	}

	//////////////////////////////// Editor::ExportMapButton ////////////////////////////////
	const char* Editor::ExportMapButton::mapFileName = "images/map.png";
	Editor::ExportMapButton::ExportMapButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
	: TextButton(objCounterArgumentsComma() "Export Map", zone, zoneLeftX, zoneTopY) {
	}
	Editor::ExportMapButton::~ExportMapButton() {}
	//export the map
	void Editor::ExportMapButton::doAction() {
		int mapWidth = MapState::mapWidth();
		int mapHeight = MapState::mapHeight();

		SDL_Surface* tilesSurface = IMG_Load(SpriteRegistry::tilesFileName);
		SDL_Surface* mapSurface =
			SDL_CreateRGBSurface(
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
	}

	//////////////////////////////// Editor::TileButton ////////////////////////////////
	const int Editor::TileButton::buttonSize = MapState::tileSize + 2;
	Editor::TileButton::TileButton(objCounterParametersComma() char pTile, Zone zone, int zoneLeftX, int zoneTopY)
	: Button(objCounterArgumentsComma() zone, zoneLeftX, zoneTopY)
	, tile(pTile) {
		setWidthAndHeight(buttonSize, buttonSize);
	}
	Editor::TileButton::~TileButton() {}
	//render the tile above the button
	void Editor::TileButton::render() {
		Button::render();
		SpriteRegistry::tiles->renderSpriteAtScreenPosition((int)tile, 0, (GLint)leftX + 1, (GLint)topY + 1);
	}
	//if noisy tile selection is off, highlight this tile for tile painting
	//if noisy tile selection is on, add this tile to the noisy tile list
	void Editor::TileButton::doAction() {
		//TODO: handle clicks appropriately
	}

	//////////////////////////////// Editor::HeightButton ////////////////////////////////
	const int Editor::HeightButton::buttonWidth = MapState::tileSize + 2;
	const int Editor::HeightButton::buttonHeight = 10;
	Editor::HeightButton::HeightButton(objCounterParametersComma() char pHeight, Zone zone, int zoneLeftX, int zoneTopY)
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
		//TODO: handle clicks appropriately
	}

	//////////////////////////////// Editor ////////////////////////////////
	const Editor::RGB Editor::backgroundRGB (0.25f, 0.75f, 0.75f);
	const Editor::RGB Editor::heightFloorRGB (0.0f, 0.75f, 9.0f / 16.0f);
	const Editor::RGB Editor::heightWallRGB (5.0f / 8.0f, 3.0f / 8.0f, 0.25f);
	Editor::SaveButton* Editor::saveButton = nullptr;
	Editor::ExportMapButton* Editor::exportMapButton = nullptr;
	Editor::TileButton** Editor::tileButtons = nullptr;
	Editor::HeightButton** Editor::heightButtons = nullptr;
	//build all the editor buttons
	void Editor::loadButtons() {
		saveButton = newSaveButton(Zone::Right, 5, 5);
		exportMapButton = newExportMapButton(Zone::Right, 45, 5);
		tileButtons = new TileButton*[MapState::tileCount];
		for (char i = 0; i < (char)MapState::tileCount; i++)
			tileButtons[i] = newTileButton(
				i, Zone::Right, 5 + TileButton::buttonSize * (i % 16), 25 + TileButton::buttonSize * (i / 16));
		heightButtons = new HeightButton*[MapState::heightCount];
		for (char i = 0; i < (char)MapState::heightCount; i++)
			heightButtons[i] = newHeightButton(
				i, Zone::Right, 5 + HeightButton::buttonWidth * (i / 2), 60 + HeightButton::buttonHeight * (i % 2));
	}
	//delete all the editor buttons
	void Editor::unloadButtons() {
		delete saveButton;
		delete exportMapButton;
		for (char i = 0; i < (char)MapState::tileCount; i++)
			delete tileButtons[i];
		delete[] tileButtons;
		for (char i = 0; i < (char)MapState::heightCount; i++)
			delete heightButtons[i];
		delete[] heightButtons;
	}
	//see if we clicked on any buttons
	void Editor::handleClick(SDL_MouseButtonEvent& clickEvent) {
		int screenX = (int)((float)clickEvent.x / Config::currentPixelWidth);
		int screenY = (int)((float)clickEvent.y / Config::currentPixelHeight);
		if (saveButton->tryHandleClick(screenX, screenY) || exportMapButton->tryHandleClick(screenX, screenY))
			return;
	}
	//draw the editor interface
	void Editor::render(EntityState* camera, int ticksTime) {
		//draw a mouse selection box
		int mouseX;
		int mouseY;
		SDL_GetMouseState(&mouseX, &mouseY);
		mouseX = (int)((float)mouseX / Config::currentPixelWidth);
		mouseY = (int)((float)mouseY / Config::currentPixelHeight);

		int screenLeftWorldX = MapState::getScreenLeftWorldX(camera, ticksTime);
		int screenTopWorldY = MapState::getScreenTopWorldY(camera, ticksTime);
		int mouseMapX = (mouseX + screenLeftWorldX) / MapState::tileSize;
		int mouseMapY = (mouseY + screenTopWorldY) / MapState::tileSize;
		GLint boxLeftX = (GLint)(mouseMapX * MapState::tileSize - screenLeftWorldX);
		GLint boxTopY = (GLint)(mouseMapY * MapState::tileSize - screenTopWorldY);
		GLint boxRightX = boxLeftX + (GLint)MapState::tileSize;
		GLint boxBottomY = boxTopY + (GLint)MapState::tileSize;
		SpriteSheet::renderRectangleOutline(1.0f, 1.0f, 1.0f, 1.0f, boxLeftX, boxTopY, boxRightX, boxBottomY);

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
		saveButton->render();
		exportMapButton->render();

		//draw the tiles
		for (int i = 0; i < MapState::tileCount; i++)
			tileButtons[i]->render();

		//draw the heights
		for (int i = 0; i < MapState::heightCount; i++)
			heightButtons[i]->render();
	}
#endif
