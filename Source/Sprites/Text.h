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
		virtual ~Glyph();

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
		virtual ~GlyphRow();

		//add a glyph to this row and bump up the unicode range
		void addGlyph(Glyph* glyph);
		//returns whether this row has a unicode range that ends after the provided value
		//used to binary search for the row containing the glyph for a value- only the first row that ends after the value could
		//	contain the right glyph
		bool endsAfter(int unicodeValue);
		//returns the glyph associated with the given unicode value, or nullptr if this row does not contain it
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
		virtual ~Metrics();

		float getTotalHeight() { return topPadding + aboveBaseline + belowBaseline + bottomPadding; }
		float getBaselineDistanceBelow(Metrics* aboveMetrics) {
			return aboveMetrics->belowBaseline + aboveMetrics->bottomPadding + topPadding + aboveBaseline;
		}
	};

private:
	static constexpr int defaultInterCharacterSpacing = 1;

	static SpriteSheet* font;
	static SpriteSheet* keyBackground;
	static vector<GlyphRow*> glyphRows;

public:
	//Prevent allocation
	Text() = delete;
	//get the inter-character spacing at this font scale
	static float getInterCharacterSpacing(float fontScale) { return fontScale * defaultInterCharacterSpacing; }
	//load the font sprite sheet and find which glyphs we have
	static void loadFont();
	//delete the font sprite sheet
	static void unloadFont();
	//return the glyph as indicated by the character at the given index, and increment the index to the following character
	static Glyph* getNextGlyph(const char* text, int* inOutCharIndexPointer);
	//get the metrics of the text that would be drawn by drawing the given text at the given font scale
	static Metrics getMetrics(const char* text, float fontScale);
	//get the metrics of the key background for text of the given width
	static Metrics getKeyBackgroundMetrics(Metrics* textMetrics);
	//render the given text, scaling it as specified
	static void render(const char* text, float leftX, float baselineY, float fontScale);
	//draw text with a key background behind it, with the key background placed at the left x
	static void renderWithKeyBackground(const char* text, float leftX, float baselineY, float fontScale);
	//draw text with a key background behind it, with the key background placed at the left x, using the pre-computed metrics
	static void renderWithKeyBackgroundWithMetrics(
		const char* text, float leftX, float baselineY, Metrics* textMetrics, Metrics* keyBackgroundMetrics);
	//render lines of text vertically and horizontally centered
	static void renderLines(vector<string>& lines, vector<Metrics>& linesMetrics);
	//set the color to use for the font and the key background
	static void setRenderColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
};
#endif
