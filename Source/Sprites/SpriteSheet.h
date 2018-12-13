#include "General/General.h"

#define newSpriteSheet(imageSurface, horizontalSpriteCount, verticalSpriteCount) \
	newWithArgs(SpriteSheet, imageSurface, horizontalSpriteCount, verticalSpriteCount)
#define newSpriteSheetWithImagePath(imagePath, horizontalSpriteCount, verticalSpriteCount) \
	produceWithArgs(SpriteSheet, imagePath, horizontalSpriteCount, verticalSpriteCount)

class SpriteSheet onlyInDebug(: public ObjCounter) {
private:
	GLuint textureId;
	int spriteWidth;
	int spriteHeight;
	int spriteSheetWidth;
	int spriteSheetHeight;
	float spriteTexWidth;
	float spriteTexHeight;

public:
	SpriteSheet(objCounterParametersComma() SDL_Surface* imageSurface, int horizontalSpriteCount, int verticalSpriteCount);
	~SpriteSheet();

	static SpriteSheet* produce(
		objCounterParametersComma() const char* imagePath, int horizontalSpriteCount, int verticalSpriteCount);
	void clampSpriteRectForTilesSprite();
	void render(GLint leftX, GLint topY, int spriteHorizontalIndex, int spriteVerticalIndex);
	void renderUsingCenter(float centerX, float centerY, int spriteHorizontalIndex, int spriteVerticalIndex);
};
