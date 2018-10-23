#include "General/General.h"

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
	SpriteSheet(objCounterParametersComma() const char* imagePath, int horizontalSpriteCount, int verticalSpriteCount);
	SpriteSheet(objCounterParametersComma() SDL_Surface* imageSurface, int horizontalSpriteCount, int verticalSpriteCount);
	~SpriteSheet();

private:
	void initializeWithSurface(SDL_Surface* imageSurface, int horizontalSpriteCount, int verticalSpriteCount);
public:
	void clampSpriteRectForTilesSprite();
	void render(int spriteHorizontalIndex, int spriteVerticalIndex, GLint leftX, GLint topY);
	void renderUsingCenter(int spriteHorizontalIndex, int spriteVerticalIndex, float centerX, float centerY);
};
