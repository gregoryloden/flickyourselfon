#include "General/General.h"

#define newSpriteSheet(imageSurface, horizontalSpriteCount, verticalSpriteCount) \
	newWithArgs(SpriteSheet, imageSurface, horizontalSpriteCount, verticalSpriteCount)
#define newSpriteSheetWithImagePath(imagePath, horizontalSpriteCount, verticalSpriteCount) \
	produceWithArgs(SpriteSheet, imagePath, horizontalSpriteCount, verticalSpriteCount)

class SpriteSheet onlyInDebug(: public ObjCounter) {
private:
	GLuint textureId;
	int spriteSheetWidth;
	int spriteSheetHeight;
	int spriteWidth;
	int spriteHeight;
	float spriteTexPixelWidth;
	float spriteTexPixelHeight;

public:
	SpriteSheet(objCounterParametersComma() SDL_Surface* imageSurface, int horizontalSpriteCount, int verticalSpriteCount);
	~SpriteSheet();

	static SpriteSheet* produce(
		objCounterParametersComma() const char* imagePath, int horizontalSpriteCount, int verticalSpriteCount);
	int getSpriteSheetWidth() { return spriteSheetWidth; }
	int getSpriteSheetHeight() { return spriteSheetHeight; }
	void clampSpriteRectForTilesSprite();
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
};
