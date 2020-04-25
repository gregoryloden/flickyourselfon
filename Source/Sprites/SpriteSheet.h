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
	static SpriteSheet* produce(
		objCounterParametersComma()
		const char* imagePath,
		int horizontalSpriteCount,
		int verticalSpriteCount,
		bool hasBottomRightPixelBorder);
	void renderSpriteSheetRegionAtScreenRegion(
		int spriteLeftX,
		int spriteTopY,
		int spriteRightX,
		int spriteBottomY,
		GLint drawLeftX,
		GLint drawTopY,
		GLint drawRightX,
		GLint drawBottomY);
	void renderSpriteAtScreenPosition(int spriteHorizontalIndex, int spriteVerticalIndex, GLint drawLeftX, GLint drawTopY);
	void renderSpriteCenteredAtScreenPosition(
		int spriteHorizontalIndex, int spriteVerticalIndex, float drawCenterX, float drawCenterY);
	static void renderFilledRectangle(
		GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint leftX, GLint topY, GLint rightX, GLint bottomY);
	static void renderRectangleOutline(
		GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint leftX, GLint topY, GLint rightX, GLint bottomY);
private:
	static void setColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
};
