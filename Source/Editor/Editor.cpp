#include "Editor.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

#ifdef EDITOR
	#define newSaveButton(zone, leftX, topY) newWithArgs(Editor::SaveButton, zone, leftX, topY)
	#define newExportMapButton(zone, leftX, topY) newWithArgs(Editor::ExportMapButton, zone, leftX, topY)

	//////////////////////////////// Editor::Button ////////////////////////////////
	const float Editor::Button::buttonFontScale = 1.0f;
	const float Editor::Button::buttonGrayRGB = 0.5f;
	Editor::Button::Button(objCounterParametersComma() string pText, Zone zone, int zoneLeftX, int zoneTopY)
	: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
	text(pText)
	, textMetrics(Text::getMetrics(pText.c_str(), buttonFontScale))
	, leftX(zone == Zone::Right ? zoneLeftX + Config::gameScreenWidth : zoneLeftX)
	, topY(zone == Zone::Bottom ? zoneTopY + Config::gameScreenHeight : zoneTopY)
	, rightX(0)
	, bottomY(0)
	, textLeftX(0)
	, textBaselineY(0) {
		int textWidth = (int)textMetrics.charactersWidth;
		int leftRightPadding = MathUtils::min(textWidth / 5 + 1, buttonMaxLeftRightPadding);
		rightX = leftX + textWidth + leftRightPadding * 2;
		bottomY = topY + (int)(textMetrics.aboveBaseline + textMetrics.belowBaseline) + buttonTopBottomPadding * 2;
		textLeftX = (float)(leftX + leftRightPadding);
		textBaselineY = (float)(topY + buttonTopBottomPadding) + textMetrics.aboveBaseline;
	}
	Editor::Button::~Button() {}
	//render this button
	void Editor::Button::render() {
		SpriteSheet::renderRectangle(
			buttonGrayRGB, buttonGrayRGB, buttonGrayRGB, 1.0f, (GLint)leftX, (GLint)topY, (GLint)rightX, (GLint)bottomY);
		Text::render(text.c_str(), textLeftX, textBaselineY, textMetrics.fontScale);
	}
	//check if the click is within this button's bounds, and if it is, do the action for this button
	//returns whether the click was handled by this button
	bool Editor::Button::tryHandleClick(int x, int y) {
		if (x < leftX || x >= rightX || y < topY || y >= bottomY)
			return false;
		doAction();
		return true;
	}

	//////////////////////////////// Editor::SaveButton ////////////////////////////////
	Editor::SaveButton::SaveButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
	: Button(objCounterArgumentsComma() "Save", zone, zoneLeftX, zoneTopY) {
	}
	Editor::SaveButton::~SaveButton() {}
	//save the game
	void Editor::SaveButton::doAction() {
	}

	//////////////////////////////// Editor::ExportMapButton ////////////////////////////////
	Editor::ExportMapButton::ExportMapButton(objCounterParametersComma() Zone zone, int zoneLeftX, int zoneTopY)
	: Button(objCounterArgumentsComma() "Export Map", zone, zoneLeftX, zoneTopY) {
	}
	Editor::ExportMapButton::~ExportMapButton() {}
	//export the map
	void Editor::ExportMapButton::doAction() {
	}

	//////////////////////////////// Editor ////////////////////////////////
	const float Editor::backgroundRed = 0.25f;
	const float Editor::backgroundGreen = 0.75f;
	const float Editor::backgroundBlue = 0.75f;
	Editor::SaveButton* Editor::saveButton = nullptr;
	Editor::ExportMapButton* Editor::exportMapButton = nullptr;
	//build all the editor buttons
	void Editor::loadButtons() {
		saveButton = newSaveButton(Zone::Right, 10, 5);
		exportMapButton = newExportMapButton(Zone::Right, 50, 5);
	}
	//delete all the editor buttons
	void Editor::unloadButtons() {
		delete saveButton;
		delete exportMapButton;
	}
	//see if we clicked on any buttons
	void Editor::handleClick(SDL_MouseButtonEvent& clickEvent) {
		int screenX = (int)((float)clickEvent.x / Config::currentPixelWidth);
		int screenY = (int)((float)clickEvent.y / Config::currentPixelHeight);
		if (saveButton->tryHandleClick(screenX, screenY) || exportMapButton->tryHandleClick(screenX, screenY))
			return;
	}
	//draw the editor interface
	void Editor::render() {
		//draw the 2 background rectangles around the game view
		//right zone
		SpriteSheet::renderRectangle(
			backgroundRed,
			backgroundGreen,
			backgroundBlue,
			1.0f,
			(GLint)Config::gameScreenWidth,
			0,
			(GLint)Config::windowScreenWidth,
			(GLint)Config::windowScreenHeight);
		//bottom zone
		SpriteSheet::renderRectangle(
			backgroundRed,
			backgroundGreen,
			backgroundBlue,
			1.0f,
			0,
			(GLint)Config::gameScreenHeight,
			(GLint)Config::gameScreenWidth,
			(GLint)Config::windowScreenHeight);

		//draw the buttons
		saveButton->render();
		exportMapButton->render();
	}
#endif
