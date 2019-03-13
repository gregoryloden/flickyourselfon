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
	#define newPaintBoxRadiusButton(radius, zone, leftX, topY) \
		newWithArgs(Editor::PaintBoxRadiusButton, radius, zone, leftX, topY)
	#define newNoiseButton(zone, leftX, topY) newWithArgs(Editor::NoiseButton, zone, leftX, topY)
	#define newNoiseTileButton(zone, leftX, topY) newWithArgs(Editor::NoiseTileButton, zone, leftX, topY)

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
	//fade the save button if it's disabled
	void Editor::SaveButton::render() {
		TextButton::render();
		if (saveButtonDisabled)
			renderFadedOverlay();
	}
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

		saveButtonDisabled = true;
	}

	//////////////////////////////// Editor::ExportMapButton ////////////////////////////////
	const char* Editor::ExportMapButton::mapFileName = "images/map.png";
	Editor::ExportMapButton::ExportMapButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
	: TextButton(objCounterArgumentsComma() "Export Map", zone, zoneLeftX, zoneTopY) {
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

		exportMapButtonDisabled = true;
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
		objCounterParametersComma() char pRadius, Zone zone, int zoneLeftX, int zoneTopY)
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
	: TextButton(objCounterArgumentsComma() "Noise", zone, zoneLeftX, zoneTopY) {
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

	//////////////////////////////// Editor ////////////////////////////////
	const Editor::RGB Editor::backgroundRGB (0.25f, 0.75f, 0.75f);
	vector<Editor::Button*> Editor::buttons;
	Editor::NoiseButton* Editor::noiseButton = nullptr;
	Editor::NoiseTileButton** Editor::noiseTileButtons = nullptr;
	Editor::Button* Editor::selectedButton = nullptr;
	Editor::PaintBoxRadiusButton* Editor::selectedPaintBoxRadiusButton = nullptr;
	Editor::HeightButton* Editor::lastSelectedHeightButton = nullptr;
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
				newTileButton(i, Zone::Right, 5 + TileButton::buttonSize * (i % 16), 23 + TileButton::buttonSize * (i / 16)));
		for (char i = 0; i < (char)MapState::heightCount; i++)
			buttons.push_back(
				newHeightButton(
					i, Zone::Right, 5 + HeightButton::buttonWidth * (i / 2), 58 + HeightButton::buttonHeight * (i % 2)));
		for (char i = 0; i < (char)paintBoxMaxRadius; i++) {
			PaintBoxRadiusButton* button =
				newPaintBoxRadiusButton(i, Zone::Right, 5 + PaintBoxRadiusButton::buttonSize * i, 81);
			if (i == 0)
				selectedPaintBoxRadiusButton = button;
			buttons.push_back(button);
		}
		noiseButton = newNoiseButton(Zone::Right, 73, 58);
		buttons.push_back(noiseButton);
		noiseTileButtons = new NoiseTileButton*[noiseTileButtonMaxCount];
		for (char i = 0; i < noiseTileButtonMaxCount; i++) {
			NoiseTileButton* button = newNoiseTileButton(
				Zone::Right, 73 + NoiseTileButton::buttonWidth * (i % 8), 74 + NoiseTileButton::buttonHeight * (i / 8));
			noiseTileButtons[i] = button;
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
		for (Button* button : buttons)
			if (button->tryHandleClick(screenX, screenY))
				return;
		//try to paint part of the map
		if (screenX < Config::gameScreenWidth && screenY < Config::gameScreenHeight
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

			int radius = (int)selectedPaintBoxRadiusButton->getRadius();
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
			int radius = (int)selectedPaintBoxRadiusButton->getRadius();
			int worldDiameter = (radius * 2 + 1) * MapState::tileSize;
			GLint boxLeftX = (GLint)((mouseMapX - radius) * MapState::tileSize - screenLeftWorldX);
			GLint boxTopY = (GLint)((mouseMapY - radius) * MapState::tileSize - screenTopWorldY);
			GLint boxRightX = boxLeftX + (GLint)worldDiameter;
			GLint boxBottomY = boxTopY + (GLint)worldDiameter;
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
		selectedPaintBoxRadiusButton->renderHighlightOutline();
	}
	//return the height of the selected height button, or -1 if it's not selected
	char Editor::getSelectedHeight() {
		return selectedButton != nullptr && selectedButton == lastSelectedHeightButton
			? lastSelectedHeightButton->getHeight()
			: -1;
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
