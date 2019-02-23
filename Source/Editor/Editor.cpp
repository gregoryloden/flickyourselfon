#include "Editor.h"
#include "Sprites/SpriteSheet.h"
#include "Util/Config.h"

#ifdef EDITOR
	#define newSaveButton(zone, leftX, topY) newWithArgs(Editor::SaveButton, zone, leftX, topY)
	#define newExportMapButton(zone, leftX, topY) newWithArgs(Editor::ExportMapButton, zone, leftX, topY)

	//////////////////////////////// Editor::Button ////////////////////////////////
	const float Editor::Button::buttonFontScale = 1.0f;
	const float Editor::Button::buttonGrayRGB = 0.5f;
	Editor::Button::Button(objCounterParametersComma() string pText, Zone zone, GLint pLeftX, GLint pTopY)
	: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
	text(pText)
	, textMetrics(Text::getMetrics(pText.c_str(), buttonFontScale))
	, leftX(zone == Zone::Right ? pLeftX + (GLint)Config::gameScreenWidth : pLeftX)
	, topY(zone == Zone::Bottom ? pTopY + (GLint)Config::gameScreenHeight : pTopY) {
	}
	Editor::Button::~Button() {}
	//render this button
	void Editor::Button::render() {
		SpriteSheet::renderRectangle(
			buttonGrayRGB,
			buttonGrayRGB,
			buttonGrayRGB,
			1.0f,
			leftX,
			topY,
			leftX + (GLint)textMetrics.charactersWidth + 2,
			topY + (GLint)(textMetrics.aboveBaseline + textMetrics.belowBaseline) + 2);
		Text::render(text.c_str(), (float)leftX + 1.0f, (float)topY + 1.0f + textMetrics.aboveBaseline, textMetrics.fontScale);
	}

	//////////////////////////////// Editor::SaveButton ////////////////////////////////
	Editor::SaveButton::SaveButton(objCounterParametersComma() Zone zone, GLint pLeftX, GLint pTopY)
	: Button(objCounterArgumentsComma() "Save", zone, pLeftX, pTopY) {
	}
	Editor::SaveButton::~SaveButton() {}

	//////////////////////////////// Editor::ExportMapButton ////////////////////////////////
	Editor::ExportMapButton::ExportMapButton(objCounterParametersComma() Zone zone, GLint pLeftX, GLint pTopY)
	: Button(objCounterArgumentsComma() "Export Map", zone, pLeftX, pTopY) {
	}
	Editor::ExportMapButton::~ExportMapButton() {}

	//////////////////////////////// Editor ////////////////////////////////
	const float Editor::backgroundRed = 0.25f;
	const float Editor::backgroundGreen = 0.75f;
	const float Editor::backgroundBlue = 0.75f;
	Editor::SaveButton* Editor::saveButton = nullptr;
	Editor::ExportMapButton* Editor::exportMapButton = nullptr;
	//build all the editor buttons
	void Editor::loadButtons() {
		saveButton = newSaveButton(Zone::Right, 10, 5);
		exportMapButton = newExportMapButton(Zone::Right, 40, 5);
	}
	//delete all the editor buttons
	void Editor::unloadButtons() {
		delete saveButton;
		delete exportMapButton;
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
			(GLint)Config::gameAndEditorScreenWidth,
			(GLint)Config::gameAndEditorScreenHeight);
		//bottom zone
		SpriteSheet::renderRectangle(
			backgroundRed,
			backgroundGreen,
			backgroundBlue,
			1.0f,
			0,
			(GLint)Config::gameScreenHeight,
			(GLint)Config::gameScreenWidth,
			(GLint)Config::gameAndEditorScreenHeight);

		//draw the buttons
		saveButton->render();
		exportMapButton->render();
	}
#endif
