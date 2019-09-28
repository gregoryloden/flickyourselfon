#include "Text.h"
#include "Sprites/SpriteSheet.h"

#define newGlyph(spriteX, spriteY, spriteWidth, spriteHeight, baselineOffset) \
	newWithArgs(Text::Glyph, spriteX, spriteY, spriteWidth, spriteHeight, baselineOffset)
#define newGlyphRow(unicodeStart, firstGlyph) newWithArgs(Text::GlyphRow, unicodeStart, firstGlyph)

//////////////////////////////// Text::Glyph ////////////////////////////////
Text::Glyph::Glyph(
	objCounterParametersComma() int pSpriteX, int pSpriteY, int pSpriteWidth, int pSpriteHeight, int pBaselineOffset)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
spriteX(pSpriteX)
, spriteY(pSpriteY)
, spriteWidth(pSpriteWidth)
, spriteHeight(pSpriteHeight)
, baselineOffset(pBaselineOffset) {
}
Text::Glyph::~Glyph() {}

//////////////////////////////// Text::GlyphRow ////////////////////////////////
Text::GlyphRow::GlyphRow(objCounterParametersComma() int pUnicodeStart, Glyph* firstGlyph)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
unicodeStart(pUnicodeStart)
, unicodeEnd(pUnicodeStart + 1)
, glyphs() {
	glyphs.push_back(firstGlyph);
}
Text::GlyphRow::~GlyphRow() {
	for (Glyph* glyph : glyphs)
		delete glyph;
}
//add a glyph to this row and bump up the unicode range
void Text::GlyphRow::addGlyph(Glyph* glyph) {
	glyphs.push_back(glyph);
	unicodeEnd++;
}
//returns whether this row has a unicode range that ends after the provided value
//used to binary search for the row containing the glyph for a value-
//	only the first row that ends after the value could contain the right glyph
bool Text::GlyphRow::endsAfter(int unicodeValue) {
	return unicodeEnd > unicodeValue;
}
//returns the glyph associated with the given unicode value, or nullptr if this row does not contain it
Text::Glyph* Text::GlyphRow::getGlyph(int unicodeValue) {
	return (unicodeValue >= unicodeStart && unicodeValue < unicodeEnd) ? glyphs[unicodeValue - unicodeStart] : nullptr;
}

//////////////////////////////// Text::Metrics ////////////////////////////////
Text::Metrics::Metrics()
: charactersWidth(0)
, aboveBaseline(0)
, belowBaseline(0)
, topPadding(0)
, bottomPadding(0) {
}
Text::Metrics::~Metrics() {}

//////////////////////////////// Text ////////////////////////////////
SpriteSheet* Text::font = nullptr;
SpriteSheet* Text::keyBackground = nullptr;
vector<Text::GlyphRow*> Text::glyphRows;
//load the font sprite sheet and find which glyphs we have
void Text::loadFont() {
	SDL_Surface* fontSurface = IMG_Load("images/font.png");
	font = newSpriteSheet(fontSurface, 1, 1);
	keyBackground = newSpriteSheetWithImagePath("images/keybackground.png", 1, 1);
	int imageWidth = fontSurface->w;

	int* pixels = static_cast<int*>(fontSurface->pixels);
	int alphaMask = (int)fontSurface->format->Amask;
	int redMask = (int)fontSurface->format->Rmask;
	int redShift = (int)fontSurface->format->Rshift;
	int greenMask = (int)fontSurface->format->Gmask;
	int greenShift = (int)fontSurface->format->Gshift;
	int blueMask = (int)fontSurface->format->Bmask;
	int blueShift = (int)fontSurface->format->Bshift;
	int solidWhite = redMask | greenMask | blueMask | alphaMask;
	int solidBlack = alphaMask;

	//the font file consists of multiple rows of glyphs
	//assumes rows are laid only in increasing order
	int lastUnicodeValue = -1;
	int headerRow = 0;
	GlyphRow* lastGlyphRow = nullptr;
	while (true) {
		int lowestGlyphBottom = -1;
		int baselineOffset = 0;
		//iterate through all the glyphs in this row, if there are any
		//each glyph consists of its rectangle, plus a row above it and column to the left of it
		//the row above it contains black pixels spanning the width of the glyph rectangle, plus a white pixel in the top-left
		//	corner at the intersection with the column to the left of the glyph
		//the column to the left has a pixel at the top with the unicode value of the glyph (clear if it's the one after the
		//	glyph to the left of it), a pixel below it with how many pixels the baseline is above the bottom (clear if it's the
		//	same as the glyph to the left of it), and gray pixels the rest of the way down to a black pixel marking the last
		//	pixel row of the glyph
		//glyphs can be different heights but each header row is on one line of pixels; header rows will be below the tallest
		//	glyph of the row above
		for (int headerCol = 0; pixels[headerRow * imageWidth + headerCol] == solidWhite; ) {
			//remember which column holds the glyph data, then find the first pixel column to the right of the glyph
			int glyphDataCol = headerCol;
			for (headerCol++; pixels[headerRow * imageWidth + headerCol] == solidBlack; headerCol++)
				;

			//get the encoded unicode value (for a solid pixel), or use the last value + 1
			int unicodeValuePixel = pixels[(headerRow + 1) * imageWidth + glyphDataCol];
			int unicodeValue = (unicodeValuePixel & alphaMask) == 0
				? lastUnicodeValue + 1
				: ((unicodeValuePixel & redMask) >> redShift << 16)
					| ((unicodeValuePixel & greenMask) >> greenShift << 8)
					| ((unicodeValuePixel & blueMask) >> blueShift);

			//get the encoded baseline offset (for a solid pixel), or use the last value
			int baselineOffsetPixel = pixels[(headerRow + 2) * imageWidth + glyphDataCol];
			if ((baselineOffsetPixel & alphaMask) != 0)
				baselineOffset = (baselineOffsetPixel & blueMask) >> blueShift;

			//find the last pixel row of the glyph
			int glyphBottom = headerRow + 3;
			for (; pixels[glyphBottom * imageWidth + glyphDataCol] != solidBlack; glyphBottom++)
				;

			Glyph* glyph = newGlyph(
				glyphDataCol + 1, headerRow + 1, headerCol - glyphDataCol - 1, glyphBottom - headerRow, baselineOffset);
			if (unicodeValue == lastUnicodeValue + 1 && lastGlyphRow != nullptr)
				lastGlyphRow->addGlyph(glyph);
			else {
				lastGlyphRow = newGlyphRow(unicodeValue, glyph);
				glyphRows.push_back(lastGlyphRow);
			}
			lastUnicodeValue = unicodeValue;
			lowestGlyphBottom = MathUtils::max(lowestGlyphBottom, glyphBottom);
		}
		if (lowestGlyphBottom == -1)
			break;
		else
			headerRow = lowestGlyphBottom + 1;
	}

	SDL_FreeSurface(fontSurface);
}
//delete the font sprite sheet
void Text::unloadFont() {
	delete font;
	delete keyBackground;
	for (GlyphRow* glyphRow : glyphRows)
		delete glyphRow;
	glyphRows.clear();
}
//return the glyph as indicated by the character at the given index, and increment the index to the following character
Text::Glyph* Text::getNextGlyph(const char* text, int* inOutCharIndexPointer) {
	int charIndex = *inOutCharIndexPointer;
	char c = text[charIndex];
	int unicodeValue;
	if ((c & 0x80) == 0) {
		unicodeValue = (int)c;
		*inOutCharIndexPointer = charIndex + 1;
	//2-byte utf-8
	} else if ((c & 0xE0) == 0xC0) {
		unicodeValue = (((int)c & 0x1F) << 6) | ((int)text[charIndex + 1] & 0x3F);
		*inOutCharIndexPointer = charIndex + 2;
	//3-byte utf-8
	} else if ((c & 0xF0) == 0xE0) {
		unicodeValue =
			(((int)c & 0xF) << 12) | (((int)text[charIndex + 1] & 0x3F) << 6) | ((int)text[charIndex + 2] & 0x3F);
		*inOutCharIndexPointer = charIndex + 3;
	//skip 4-byte utf-8 and any utf-8 continuation bytes
	} else {
		*inOutCharIndexPointer = charIndex + 1;
		return nullptr;
	}

	//binary search for the right glyph row
	//low equals the lowest index row that could contain it
	//high equals the highest index row that could contain it
	int glyphRowLow = 0;
	int glyphRowHigh = (int)glyphRows.size() - 1;
	while (glyphRowLow < glyphRowHigh) {
		int glyphRowMid = (glyphRowLow + glyphRowHigh) / 2;
		//this row ends after our glyph, so search for an earlier row
		if (glyphRows[glyphRowMid]->endsAfter(unicodeValue))
			glyphRowHigh = glyphRowMid;
		//if it does not end after our glyph, skip it and mark the next row up as our new low
		else
			glyphRowLow = glyphRowMid + 1;
	}

	//this row may or may not contain the glyph, but all lower rows definitely do not contain it
	return glyphRows[glyphRowLow]->getGlyph(unicodeValue);
}
//get the metrics of the text that would be drawn by drawing the given text at the given font scale
Text::Metrics Text::getMetrics(const char* text, float fontScale) {
	int charIndex = 0;
	int charactersWidth = 0;
	int aboveBaseline = 0;
	int belowBaseline = 0;
	while (text[charIndex] != 0) {
		Glyph* glyph = getNextGlyph(text, &charIndex);
		int glyphBaselineOffset = glyph->getBaselineOffset();

		charactersWidth += glyph->getWidth() + defaultInterCharacterSpacing;
		aboveBaseline = MathUtils::max(aboveBaseline, glyph->getHeight() - glyphBaselineOffset);
		belowBaseline = MathUtils::max(belowBaseline, glyphBaselineOffset);
	}

	if (charactersWidth > 0)
		charactersWidth -= defaultInterCharacterSpacing;

	Metrics metrics;
	metrics.charactersWidth = (float)charactersWidth * fontScale;
	metrics.aboveBaseline = (float)aboveBaseline * fontScale;
	metrics.belowBaseline = (float)belowBaseline * fontScale;
	metrics.topPadding = (float)defaultTopPadding * fontScale;
	metrics.bottomPadding = (float)defaultBottomPadding * fontScale;
	metrics.fontScale = fontScale;
	return metrics;
}
//get the metrics of the key background for text of the given width
Text::Metrics Text::getKeyBackgroundMetrics(Metrics* textMetrics) {
	const int belowBaselineSpacing = 5;

	int textOriginalWidth = (int)(textMetrics->charactersWidth / textMetrics->fontScale);
	int targetKeyBackgroundWidth = textOriginalWidth / 4 * 2 + textOriginalWidth + 6;

	Metrics metrics;
	metrics.fontScale = textMetrics->fontScale;
	metrics.charactersWidth =
		MathUtils::max(targetKeyBackgroundWidth, keyBackground->getSpriteSheetWidth()) * metrics.fontScale;
	metrics.aboveBaseline = (float)(keyBackground->getSpriteSheetHeight() - belowBaselineSpacing) * metrics.fontScale;
	metrics.belowBaseline = (float)belowBaselineSpacing * metrics.fontScale;
	metrics.topPadding = metrics.fontScale;
	metrics.bottomPadding = 0.0f;
	return metrics;
}
//render the given text, scaling it as specified
void Text::render(const char* text, float leftX, float baselineY, float fontScale) {
	glEnable(GL_BLEND);
	int charIndex = 0;
	while (text[charIndex] != 0) {
		Glyph* glyph = getNextGlyph(text, &charIndex);
		int glyphSpriteX = glyph->getSpriteX();
		int glyphSpriteY = glyph->getSpriteY();
		int glyphWidth = glyph->getWidth();
		int glyphHeight = glyph->getHeight();
		float topY = baselineY - (float)(glyphHeight - glyph->getBaselineOffset()) * fontScale;

		font->renderSpriteSheetRegionAtScreenRegion(
			glyphSpriteX,
			glyphSpriteY,
			glyphSpriteX + glyphWidth,
			glyphSpriteY + glyphHeight,
			(int)leftX,
			(int)topY,
			(int)(leftX + (float)glyphWidth * fontScale),
			(int)(topY + (float)glyphHeight * fontScale));

		leftX += (float)(glyphWidth + defaultInterCharacterSpacing) * fontScale;
	}
}
//draw a key background to be drawn behind text
void Text::renderKeyBackground(float leftX, float baselineY, Metrics* keyBackgroundMetrics) {
	int originalBackgroundWidth = (int)(keyBackgroundMetrics->charactersWidth / keyBackgroundMetrics->fontScale);
	int leftHalfSpriteWidth = keyBackground->getSpriteSheetWidth() / 2;
	int rightHalfSpriteWidth = keyBackground->getSpriteSheetWidth() - leftHalfSpriteWidth - 1;

	GLint drawLeftMiddleX = (GLint)(leftX + (float)leftHalfSpriteWidth * keyBackgroundMetrics->fontScale);
	GLint drawRightMiddleX =
		(GLint)(leftX + (float)(originalBackgroundWidth - rightHalfSpriteWidth) * keyBackgroundMetrics->fontScale);

	GLint topY = (GLint)(baselineY - keyBackgroundMetrics->aboveBaseline);
	GLint bottomY = (GLint)(baselineY + keyBackgroundMetrics->belowBaseline);

	//draw the left side
	glEnable(GL_BLEND);
	keyBackground->renderSpriteSheetRegionAtScreenRegion(
		0, 0, leftHalfSpriteWidth, keyBackground->getSpriteSheetHeight(), (GLint)leftX, topY, drawLeftMiddleX, bottomY);
	//draw the middle section
	keyBackground->renderSpriteSheetRegionAtScreenRegion(
		leftHalfSpriteWidth,
		0,
		leftHalfSpriteWidth + 1,
		keyBackground->getSpriteSheetHeight(),
		drawLeftMiddleX,
		topY,
		drawRightMiddleX,
		bottomY);
	//draw the right side
	keyBackground->renderSpriteSheetRegionAtScreenRegion(
		leftHalfSpriteWidth + 1,
		0,
		keyBackground->getSpriteSheetWidth(),
		keyBackground->getSpriteSheetHeight(),
		drawRightMiddleX,
		topY,
		(GLint)(leftX + keyBackgroundMetrics->charactersWidth),
		bottomY);
}
