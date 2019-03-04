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
void SpriteSheet::renderSpriteSheetRegionAtScreenRegion(
	int spriteLeftX,
	int spriteTopY,
	int spriteRightX,
	int spriteBottomY,
	GLint drawLeftX,
	GLint drawTopY,
	GLint drawRightX,
	GLint drawBottomY)
{
	GLfloat texLeftX = (GLfloat)(spriteLeftX * spriteTexPixelWidth);
	GLfloat texTopY = (GLfloat)(spriteTopY * spriteTexPixelHeight);
	GLfloat texRightX = (GLfloat)(spriteRightX * spriteTexPixelWidth);
	GLfloat texBottomY = (GLfloat)(spriteBottomY * spriteTexPixelHeight);
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
	renderSpriteSheetRegionAtScreenRegion(
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
//render a rectangle filled with the specified color at the specified region of the screen
void SpriteSheet::renderFilledRectangle(
	GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint leftX, GLint topY, GLint rightX, GLint bottomY)
{
	setColor(red, green, blue, alpha);
	glBegin(GL_QUADS);
	glVertex2i(leftX, topY);
	glVertex2i(rightX, topY);
	glVertex2i(rightX, bottomY);
	glVertex2i(leftX, bottomY);
	glEnd();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
//render a rectangle outline using the specified color at the specified region of the screen
//TODO: this doesn't draw it pixellated, do something different so that its pixel grid matches the game screen pixel grid
void SpriteSheet::renderRectangleOutline(
	GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint leftX, GLint topY, GLint rightX, GLint bottomY)
{
	setColor(red, green, blue, alpha);
	//glLineWidth(3.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2i(leftX, topY);
	glVertex2i(rightX, topY);
	glVertex2i(rightX, bottomY);
	glVertex2i(leftX, bottomY);
	glEnd();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
//set the color and enable blending as determined by the alpha
void SpriteSheet::setColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	if (alpha >= 1.0f) {
		glDisable(GL_BLEND);
		glColor3f(red, green, blue);
	} else {
		glEnable(GL_BLEND);
		glColor4f(red, green, blue, alpha);
	}
}
