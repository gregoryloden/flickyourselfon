#include "SpriteSheet.h"

SpriteSheet::SpriteSheet(
	objCounterParametersComma() SDL_Surface* imageSurface, int horizontalSpriteCount, int verticalSpriteCount)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
textureId(0)
, spriteSheetWidth(imageSurface->w)
, spriteSheetHeight(imageSurface->h)
, spriteWidth(imageSurface->w / horizontalSpriteCount)
, spriteHeight(imageSurface->h / verticalSpriteCount)
, spriteTexPixelWidth(1.0f / (float)imageSurface->w)
, spriteTexPixelHeight(1.0f / (float)imageSurface->h) {
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
SpriteSheet* SpriteSheet::produce(
	objCounterParametersComma() const char* imagePath, int horizontalSpriteCount, int verticalSpriteCount)
{
	SDL_Surface* surface = IMG_Load(imagePath);
	SpriteSheet* spriteSheet = new SpriteSheet(objCounterArgumentsComma() surface, horizontalSpriteCount, verticalSpriteCount);
	SDL_FreeSurface(surface);
	return spriteSheet;
}
//the last row of pixels shouldn't get drawn as part of the sprite
//TODO: make this correct/more general purpose if we ever use this for more than just the tiles sprite
void SpriteSheet::clampSpriteRectForTilesSprite() {
	spriteHeight--;
}
//draw the specified region of the sprite sheet
void SpriteSheet::renderSheetRegionAtScreenRegion(
	int spriteLeftX,
	int spriteTopY,
	int spriteRightX,
	int spriteBottomY,
	GLint drawLeftX,
	GLint drawTopY,
	GLint drawRightX,
	GLint drawBottomY)
{
	float texLeftX = spriteLeftX * spriteTexPixelWidth;
	float texTopY = spriteTopY * spriteTexPixelHeight;
	float texRightX = spriteRightX * spriteTexPixelWidth;
	float texBottomY = spriteBottomY * spriteTexPixelHeight;
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glBegin(GL_QUADS);
	glTexCoord2f(texLeftX, texTopY);
	glVertex2i(drawLeftX, drawTopY);
	glTexCoord2f(texRightX, texTopY);
	glVertex2i(drawRightX, drawTopY);
	glTexCoord2f(texRightX, texBottomY);
	glVertex2i(drawRightX, drawBottomY);
	glTexCoord2f(texLeftX, texBottomY);
	glVertex2i(drawLeftX, drawBottomY);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}
//draw the specified sprite image with its top-left corner at the specified coordinate
void SpriteSheet::renderSpriteAtScreenPosition(
	int spriteHorizontalIndex, int spriteVerticalIndex, GLint drawLeftX, GLint drawTopY)
{
	int spriteLeftX = (GLint)(spriteHorizontalIndex * spriteWidth);
	int spriteTopY = (GLint)(spriteVerticalIndex * spriteHeight);
	renderSheetRegionAtScreenRegion(
		spriteLeftX,
		spriteTopY,
		spriteLeftX + (GLint)spriteWidth,
		spriteTopY + (GLint)spriteHeight,
		drawLeftX,
		drawTopY,
		drawLeftX + spriteWidth,
		drawTopY + spriteHeight);
}
//draw the specified sprite image with its center at the specified coordinate
void SpriteSheet::renderSpriteCenteredAtScreenPosition(
	int spriteHorizontalIndex, int spriteVerticalIndex, float drawCenterX, float drawCenterY)
{
	renderSpriteAtScreenPosition(
		spriteHorizontalIndex,
		spriteVerticalIndex,
		(GLint)(drawCenterX - (float)spriteWidth * 0.5f),
		(GLint)(drawCenterY - (float)spriteHeight * 0.5f));
}
