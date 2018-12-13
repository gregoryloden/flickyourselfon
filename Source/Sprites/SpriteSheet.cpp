#include "SpriteSheet.h"

SpriteSheet::SpriteSheet(
	objCounterParametersComma() SDL_Surface* imageSurface, int horizontalSpriteCount, int verticalSpriteCount)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
textureId(0)
, spriteWidth(1)
, spriteHeight(1)
, spriteSheetWidth(1)
, spriteSheetHeight(1)
, spriteTexWidth(1.0f)
, spriteTexHeight(1.0f) {
	spriteWidth = imageSurface->w / horizontalSpriteCount;
	spriteHeight = imageSurface->h / verticalSpriteCount;
	spriteSheetWidth = imageSurface->w;
	spriteSheetHeight = imageSurface->h;
	spriteTexWidth = (float)spriteWidth / (float)spriteSheetWidth;
	spriteTexHeight = (float)spriteHeight / (float)spriteSheetHeight;

	glGenTextures(1, &textureId);

	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA, imageSurface->w, imageSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageSurface->pixels);
}
SpriteSheet::~SpriteSheet() {}
//load the surface at the image path and then build a SpriteSheet
SpriteSheet* SpriteSheet::produce(objCounterParametersComma() const char* imagePath, int horizontalSpriteCount, int verticalSpriteCount) {
	SDL_Surface* surface = IMG_Load(imagePath);
	SpriteSheet* spriteSheet = newSpriteSheet(surface, horizontalSpriteCount, verticalSpriteCount);
	SDL_FreeSurface(surface);
	return spriteSheet;
}
//the last row of pixels shouldn't get drawn as part of the sprite
//TODO: make this correct/more general purpose if we ever use this for more than just the tiles sprite
void SpriteSheet::clampSpriteRectForTilesSprite() {
	spriteHeight--;
	spriteTexHeight = (float)spriteHeight / (float)spriteSheetHeight;
}
//draw the specified sprite image at the specified coordinate
void SpriteSheet::render(GLint leftX, GLint topY, int spriteHorizontalIndex, int spriteVerticalIndex) {
	GLint rightX = leftX + spriteWidth;
	GLint bottomY = topY + spriteHeight;
	float texMinX = (float)spriteHorizontalIndex * spriteTexWidth;
	float texMinY = (float)spriteVerticalIndex * spriteTexHeight;
	float texMaxX = texMinX + spriteTexWidth;
	float texMaxY = texMinY + spriteTexHeight;

	glBindTexture(GL_TEXTURE_2D, textureId);
	glBegin(GL_QUADS);
	glTexCoord2f(texMinX, texMinY);
	glVertex2i(leftX, topY);
	glTexCoord2f(texMaxX, texMinY);
	glVertex2i(rightX, topY);
	glTexCoord2f(texMaxX, texMaxY);
	glVertex2i(rightX, bottomY);
	glTexCoord2f(texMinX, texMaxY);
	glVertex2i(leftX, bottomY);
	glEnd();
}
//draw the specified sprite image with its center at the specified coordinate
void SpriteSheet::renderUsingCenter(float centerX, float centerY, int spriteHorizontalIndex, int spriteVerticalIndex) {
	render(
		(GLint)(centerX - (float)spriteWidth * 0.5f),
		(GLint)(centerY - (float)spriteHeight * 0.5f),
		spriteHorizontalIndex,
		spriteVerticalIndex);
}
