#include "General.h"

class SpriteSheet onlyInDebug(: public ObjCounter) {
private:
	GLuint textureId;
	int spriteWidth;
	int spriteHeight;
	float spriteSheetWidth;
	float spriteSheetHeight;

public:
	SpriteSheet(objCounterParametersComma() const char* imagePath, int pSpriteWidth, int pSpriteHeight);
	~SpriteSheet();

	void render(int spriteHorizontalIndex, int spriteVerticalIndex);
};
