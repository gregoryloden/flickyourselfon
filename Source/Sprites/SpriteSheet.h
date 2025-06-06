#include "General/General.h"

#define newSpriteSheet(imageSurface, horizontalSpriteCount, verticalSpriteCount, hasBottomRightPixelBorder) \
	newWithArgs(SpriteSheet, imageSurface, horizontalSpriteCount, verticalSpriteCount, hasBottomRightPixelBorder)
#define newSpriteSheetWithImagePath(imagePath, horizontalSpriteCount, verticalSpriteCount, hasBottomRightPixelBorder) \
	produceWithArgs(SpriteSheet, imagePath, horizontalSpriteCount, verticalSpriteCount, hasBottomRightPixelBorder)

class SpriteSheet onlyInDebug(: public ObjCounter) {
private:
	GLuint textureId;
	int spriteWidth;
	int spriteHeight;
	float spriteTexPixelWidth;
	float spriteTexPixelHeight;
	float centerAnchorX;
	float centerAnchorY;

public:
	SpriteSheet(
		objCounterParametersComma()
		SDL_Surface* imageSurface,
		int horizontalSpriteCount,
		int verticalSpriteCount,
		bool hasBottomRightPixelBorder);
	virtual ~SpriteSheet();

	int getSpriteWidth() { return spriteWidth; }
	int getSpriteHeight() { return spriteHeight; }
	void setBottomAnchorY() { centerAnchorY = (float)spriteHeight; }
	//load the surface at the image path and then build a SpriteSheet
	static SpriteSheet* produce(
		objCounterParametersComma()
		const char* imagePath,
		int horizontalSpriteCount,
		int verticalSpriteCount,
		bool hasBottomRightPixelBorder);
	//draw the specified region of the sprite sheet
	static void (SpriteSheet::* renderSpriteSheetRegionAtScreenRegion)(
		int spriteLeftX,
		int spriteTopY,
		int spriteRightX,
		int spriteBottomY,
		GLint drawLeftX,
		GLint drawTopY,
		GLint drawRightX,
		GLint drawBottomY);
	//draw the specified region of the sprite sheet to the screen
	void renderSpriteSheetRegionAtScreenRegionOpenGL(
		int spriteLeftX,
		int spriteTopY,
		int spriteRightX,
		int spriteBottomY,
		GLint drawLeftX,
		GLint drawTopY,
		GLint drawRightX,
		GLint drawBottomY);
	//draw the specified sprite image with its top-left corner at the specified coordinate
	void renderSpriteAtScreenPosition(int spriteHorizontalIndex, int spriteVerticalIndex, GLint drawLeftX, GLint drawTopY);
	//draw the specified sprite image with its center at the specified coordinate
	void renderSpriteCenteredAtScreenPosition(
		int spriteHorizontalIndex, int spriteVerticalIndex, float drawCenterX, float drawCenterY);
	//render a rectangle filled with the specified color at the specified region of the screen
	static void renderFilledRectangle(
		GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint leftX, GLint topY, GLint rightX, GLint bottomY);
	//render a rectangle at the specified region of the screen with the current color
	static void renderPreColoredRectangle(GLint leftX, GLint topY, GLint rightX, GLint bottomY);
	//render a rectangle outline using the specified color at the specified region of the screen
	static void renderRectangleOutline(
		GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint leftX, GLint topY, GLint rightX, GLint bottomY);
private:
	//set the color and enable blending as determined by the alpha
	static void setColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
};
