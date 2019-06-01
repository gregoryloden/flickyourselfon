#ifndef TEXT_H
#define TEXT_H
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

		int getWidth() { return spriteWidth; }
		int getHeight() { return spriteHeight; }
		int getBaselineOffset() { return baselineOffset; }
		int getSpriteX() { return spriteX; }
		int getSpriteY() { return spriteY; }
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
		Glyph* getGlyph(int unicodeValue);
	};
public:
	//Should only be allocated within an object, on the stack, or as a static object
	class Metrics {
	public:
		float charactersWidth;
		float aboveBaseline;
		float belowBaseline;
		float topPadding;
		float bottomPadding;
		float fontScale;

		Metrics();
		~Metrics();

		float getTotalHeight() { return topPadding + aboveBaseline + belowBaseline + bottomPadding; }
		float getBaselineDistanceBelow(Metrics* aboveMetrics) {
			return aboveMetrics->belowBaseline + aboveMetrics->bottomPadding + topPadding + aboveBaseline;
		}
	};

private:
	static const int defaultTopPadding = 1;
	static const int defaultBottomPadding = 1;
	static const int defaultInterCharacterSpacing = 1;

	static SpriteSheet* font;
	static SpriteSheet* keyBackground;
	static vector<GlyphRow*> glyphRows;

public:
	static void loadFont();
	static void unloadFont();
	static Glyph* getNextGlyph(const char* text, int* inOutCharIndexPointer);
	static Metrics getMetrics(const char* text, float fontScale);
	static Metrics getKeyBackgroundMetrics(Metrics* textMetrics);
	static void render(const char* text, float leftX, float baselineY, float fontScale);
	static void renderKeyBackground(float leftX, float baselineY, Metrics* keyBackgroundMetrics);
};
#endif
