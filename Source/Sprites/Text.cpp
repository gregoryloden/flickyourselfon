#include "Text.h"
#include "Sprites/SpriteSheet.h"

#define newGlyph(spriteX, spriteY, spriteWidth, spriteHeight, baselineOffset) \
	newWithArgs(Glyph, spriteX, spriteY, spriteWidth, spriteHeight, baselineOffset)
#define newGlyphRow(unicodeStart, firstGlyph) newWithArgs(GlyphRow, unicodeStart, firstGlyph)

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
vector<Text::GlyphRow*> Text::glyphRows;
SpriteSheet* Text::font = nullptr;
//load the font sprite sheet and find which glyphs we have
void Text::loadFont() {
	SDL_Surface* fontSurface = IMG_Load("images/font.png");
	font = newSpriteSheet(fontSurface, 1, 1);
	int imageWidth = fontSurface->w;

	int* pixels = static_cast<int*>(fontSurface->pixels);
	int alphaMask = (int)fontSurface->format->Amask;
	int redMask = (int)fontSurface->format->Rmask;
	int redShift = (int)fontSurface->format->Rshift;
	int greenMask = (int)fontSurface->format->Gmask;
	int greenShift = (int)fontSurface->format->Gshift;
	int blueMask = (int)fontSurface->format->Bmask;
	int blueShift = (int)fontSurface->format->Bshift;
	const int solidWhite = 0xFFFFFFFF;
	int solidBlack = alphaMask;

	//the font file consists of multiple rows of glyphs
	int lastUnicodeValue = -1;
	int headerRow = 0;
	GlyphRow* lastGlyphRow = nullptr;
	while (true) {
		int glyphBottom = -1;
		int lastBaselineOffset = 0;
		//iterate through all the glyphs in this row, if there are any
		//each glyph consists of its rectangle, plus a row above it and column to the left of it
		//the row above it marks the width of the glyph rectangle
		//the column to the left marks the height of the glyph rectangle, and also contains the unicode value of the glyph as
		//	well as how many pixels it extends below the baseline
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
			int baselineOffset = (baselineOffsetPixel & alphaMask) == 0
				? lastBaselineOffset
				: (baselineOffsetPixel & blueMask) >> blueShift;

			//find the last pixel row of the glyph
			for (glyphBottom = headerRow + 3; pixels[glyphBottom * imageWidth + glyphDataCol] != solidBlack; glyphBottom++)
				;

			Glyph* glyph =
				newGlyph(
					glyphDataCol + 1, headerRow + 1, headerCol - glyphDataCol - 1, glyphBottom - headerRow, baselineOffset);
			if (unicodeValue == lastUnicodeValue + 1 && lastGlyphRow != nullptr)
				lastGlyphRow->addGlyph(glyph);
			else {
				lastGlyphRow = newGlyphRow(unicodeValue, glyph);
				glyphRows.push_back(lastGlyphRow);
			}
			lastUnicodeValue = unicodeValue;
		}
		if (glyphBottom == -1)
			break;
		else
			headerRow = glyphBottom + 1;
	}

	SDL_FreeSurface(fontSurface);
}
//delete the font sprite sheet
void Text::unloadFont() {
	delete font;
	for (GlyphRow* glyphRow : glyphRows)
		delete glyphRow;
	glyphRows.clear();
}
