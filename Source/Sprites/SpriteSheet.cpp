#include "SpriteSheet.h"
#include "Util/FileUtils.h"

void (SpriteSheet::* SpriteSheet::renderSpriteSheetRegionAtScreenRegion)(
		int spriteLeftX,
		int spriteTopY,
		int spriteRightX,
		int spriteBottomY,
		GLint drawLeftX,
		GLint drawTopY,
		GLint drawRightX,
		GLint drawBottomY)
	= &SpriteSheet::renderSpriteSheetRegionAtScreenRegionOpenGL;
SpriteSheet::SpriteSheet(
	objCounterParametersComma()
	SDL_Surface* imageSurface,
	int horizontalSpriteCount,
	int verticalSpriteCount,
	bool hasBottomRightPixelBorder)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
textureId(0)
, renderSurface(imageSurface)
, spriteWidth(imageSurface->w / horizontalSpriteCount)
, spriteHeight(imageSurface->h / verticalSpriteCount)
, spriteTexPixelWidth(1.0f / (float)imageSurface->w)
, spriteTexPixelHeight(1.0f / (float)imageSurface->h)
, centerAnchorX(0)
, centerAnchorY(0) {
	glGenTextures(1, &textureId);

	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	#ifdef WIN32
		GLenum texFormat = GL_RGBA;
	#else
		GLenum texFormat = GL_BGRA;
	#endif
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA, imageSurface->w, imageSurface->h, 0, texFormat, GL_UNSIGNED_BYTE, imageSurface->pixels);

	//the last row and column of pixels shouldn't get drawn as part of the sprite
	if (hasBottomRightPixelBorder) {
		//because of floored int division, and assuming (sheet size = 1 + sprite size * sprites), any sprite with 2 or more in a
		//	direction should already have the proper value
		if (horizontalSpriteCount == 1)
			spriteWidth--;
		if (verticalSpriteCount == 1)
			spriteHeight--;
	}
	centerAnchorX = spriteWidth * 0.5f;
	centerAnchorY = spriteHeight * 0.5f;
}
SpriteSheet::~SpriteSheet() {
	SDL_FreeSurface(renderSurface);
}
SpriteSheet* SpriteSheet::produce(
	objCounterParametersComma()
	const char* imagePath,
	int horizontalSpriteCount,
	int verticalSpriteCount,
	bool hasBottomRightPixelBorder)
{
	return newSpriteSheet(
		FileUtils::loadImage(imagePath), horizontalSpriteCount, verticalSpriteCount, hasBottomRightPixelBorder);
}
void SpriteSheet::renderSpriteSheetRegionAtScreenRegionOpenGL(
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
void SpriteSheet::renderSpriteAtScreenPosition(
	int spriteHorizontalIndex, int spriteVerticalIndex, GLint drawLeftX, GLint drawTopY)
{
	int spriteLeftX = (GLint)(spriteHorizontalIndex * spriteWidth);
	int spriteTopY = (GLint)(spriteVerticalIndex * spriteHeight);
	(this->*renderSpriteSheetRegionAtScreenRegion)(
		spriteLeftX,
		spriteTopY,
		spriteLeftX + (GLint)spriteWidth,
		spriteTopY + (GLint)spriteHeight,
		drawLeftX,
		drawTopY,
		drawLeftX + spriteWidth,
		drawTopY + spriteHeight);
}
void SpriteSheet::renderSpriteCenteredAtScreenPosition(
	int spriteHorizontalIndex, int spriteVerticalIndex, float drawCenterX, float drawCenterY)
{
	renderSpriteAtScreenPosition(
		spriteHorizontalIndex, spriteVerticalIndex, (GLint)(drawCenterX - centerAnchorX), (GLint)(drawCenterY - centerAnchorY));
}
void SpriteSheet::renderFilledRectangle(
	GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint leftX, GLint topY, GLint rightX, GLint bottomY)
{
	setColor(red, green, blue, alpha);
	renderPreColoredRectangle(leftX, topY, rightX, bottomY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void SpriteSheet::renderPreColoredRectangle(GLint leftX, GLint topY, GLint rightX, GLint bottomY) {
	glBegin(GL_QUADS);
	glVertex2i(leftX, topY);
	glVertex2i(rightX, topY);
	glVertex2i(rightX, bottomY);
	glVertex2i(leftX, bottomY);
	glEnd();
}
//TODO: this doesn't draw it pixellated, do something different so that its pixel grid matches the game screen pixel grid
void SpriteSheet::renderRectangleOutline(
	GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint leftX, GLint topY, GLint rightX, GLint bottomY)
{
	GLfloat lineLeftX = (GLfloat)leftX - 0.5f;
	GLfloat lineTopY = (GLfloat)topY - 0.5f;
	GLfloat lineRightX = (GLfloat)rightX + 0.5f;
	GLfloat lineBottomY = (GLfloat)bottomY + 0.5f;
	setColor(red, green, blue, alpha);
	//TODO: make this correct
	glLineWidth(3.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(lineLeftX, lineTopY);
	glVertex2f(lineRightX, lineTopY);
	glVertex2f(lineRightX, lineBottomY);
	glVertex2f(lineLeftX, lineBottomY);
	glEnd();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void SpriteSheet::setColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	if (alpha >= 1.0f) {
		glDisable(GL_BLEND);
		glColor3f(red, green, blue);
	} else {
		glEnable(GL_BLEND);
		glColor4f(red, green, blue, alpha);
	}
}
