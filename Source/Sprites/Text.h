#include "General/General.h"

class SpriteSheet;

class Text {
private:
	class Glyph onlyInDebug(: public ObjCounter) {
	private:
		int spriteX;
		int spriteY;
		int spriteWidth;
		int spriteHeight;
		int baselineOffset;

	public:
		Glyph(objCounterParametersComma() int pSpriteX, int pSpriteY, int pSpriteWidth, int pSpriteHeight, int pBaselineOffset);
		~Glyph();
	};
	class GlyphRow onlyInDebug(: public ObjCounter) {
	private:
		int unicodeStart;
		int unicodeEnd;
		vector<Glyph*> glyphs;

	public:
		GlyphRow(objCounterParametersComma() int pUnicodeStart, Glyph* firstGlyph);
		~GlyphRow();

		void addGlyph(Glyph* glyph);
		bool endsAfter(int unicodeValue);
	};

	static vector<GlyphRow*> glyphRows;
	static SpriteSheet* font;

public:
	static void loadFont();
	static void unloadFont();
};
