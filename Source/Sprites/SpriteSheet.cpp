#include "SpriteSheet.h"
#include "Util/FileUtils.h"
#include "Util/Logger.h"

void (SpriteSheet::* SpriteSheet::renderSpriteSheetRegionAtScreenRegion)(
		int spriteLeftX,
		int spriteTopY,
		int spriteRightX,
		int spriteBottomY,
		GLint drawLeftX,
		GLint drawTopY,
		GLint drawRightX,
		GLint drawBottomY) =
	&SpriteSheet::renderSpriteSheetRegionAtScreenRegionOpenGL;
void (SpriteSheet::* SpriteSheet::renderSpriteAtScreenPosition)(
		int spriteHorizontalIndex, int spriteVerticalIndex, GLint drawLeftX, GLint drawTopY) =
	&SpriteSheet::renderSpriteAtScreenPositionOpenGL;
void (SpriteSheet::* SpriteSheet::setSpriteColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) =
	&SpriteSheet::setSpriteColorOpenGL;
SpriteSheet::SpriteSheet(
	objCounterParametersComma()
	SDL_Surface* imageSurface,
	int horizontalSpriteCount,
	int verticalSpriteCount,
	bool hasBottomRightPixelBorder)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
textureId(0)
, renderSurface(imageSurface)
, activeRenderer(nullptr)
, activeRenderTexture(nullptr)
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
	#ifdef DEBUG
		//don't delete activeRenderer, it's managed elsewhere; it should be nullptr by now
		if (activeRenderer != nullptr)
			Logger::debugLogger.log("ERROR: SpriteSheet activeRenderer not null");
		//don't delete renderTexture, it's managed elsewhere; it should be nullptr by now
		if (activeRenderTexture != nullptr)
			Logger::debugLogger.log("ERROR: SpriteSheet renderTexture not null");
	#endif
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
void SpriteSheet::renderWithOpenGL() {
	renderSpriteSheetRegionAtScreenRegion = &renderSpriteSheetRegionAtScreenRegionOpenGL;
	renderSpriteAtScreenPosition = &renderSpriteAtScreenPositionOpenGL;
	setSpriteColor = &SpriteSheet::setSpriteColorOpenGL;
}
void SpriteSheet::renderWithRenderer() {
	renderSpriteSheetRegionAtScreenRegion = &renderSpriteSheetRegionAtScreenRegionRenderer;
	renderSpriteAtScreenPosition = &renderSpriteAtScreenPositionRenderer;
	setSpriteColor = &SpriteSheet::setSpriteColorRenderer;
}
void SpriteSheet::loadRenderTexture(SDL_Renderer* renderer, SDL_Renderer** outOldRenderer, SDL_Texture** outOldTexture) {
	if (outOldRenderer != nullptr)
		*outOldRenderer = activeRenderer;
	if (outOldTexture != nullptr)
		*outOldTexture = activeRenderTexture;
	activeRenderer = renderer;
	activeRenderTexture = SDL_CreateTextureFromSurface(renderer, renderSurface);
}
void SpriteSheet::unloadRenderTexture(SDL_Renderer* renderer, SDL_Texture* texture) {
	SDL_DestroyTexture(activeRenderTexture);
	activeRenderer = renderer;
	activeRenderTexture = texture;
}
void SpriteSheet::withRenderTexture(SDL_Renderer* renderer, function<void()> renderWithTexture) {
	SDL_Renderer* oldRenderer;
	SDL_Texture* oldTexture;
	loadRenderTexture(renderer, &oldRenderer, &oldTexture);
	renderWithTexture();
	unloadRenderTexture(oldRenderer, oldTexture);
}
void SpriteSheet::setSpriteColorOpenGL(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	glColor4f(red, green, blue, alpha);
}
void SpriteSheet::setSpriteColorRenderer(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	SDL_SetTextureColorMod(activeRenderTexture, (Uint8)(red * 255), (Uint8)(green * 255), (Uint8)(blue * 255));
	SDL_SetTextureAlphaMod(activeRenderTexture, (Uint8)(alpha * 255));
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
void SpriteSheet::renderSpriteSheetRegionAtScreenRegionRenderer(
	int spriteLeftX,
	int spriteTopY,
	int spriteRightX,
	int spriteBottomY,
	GLint drawLeftX,
	GLint drawTopY,
	GLint drawRightX,
	GLint drawBottomY)
{
	SDL_Rect source { spriteLeftX, spriteTopY, spriteRightX - spriteLeftX, spriteBottomY - spriteTopY };
	SDL_Rect destination { (int)drawLeftX, (int)drawTopY, (int)(drawRightX - drawLeftX), (int)(drawBottomY - drawTopY) };
	SDL_RenderCopy(activeRenderer, activeRenderTexture, &source, &destination);
}
void SpriteSheet::renderSpriteAtScreenPositionOpenGL(
	int spriteHorizontalIndex, int spriteVerticalIndex, GLint drawLeftX, GLint drawTopY)
{
	int spriteLeftX = spriteHorizontalIndex * spriteWidth;
	int spriteTopY = spriteVerticalIndex * spriteHeight;
	renderSpriteSheetRegionAtScreenRegionOpenGL(
		spriteLeftX,
		spriteTopY,
		spriteLeftX + spriteWidth,
		spriteTopY + spriteHeight,
		drawLeftX,
		drawTopY,
		drawLeftX + (GLint)spriteWidth,
		drawTopY + (GLint)spriteHeight);
}
void SpriteSheet::renderSpriteAtScreenPositionRenderer(
	int spriteHorizontalIndex, int spriteVerticalIndex, GLint drawLeftX, GLint drawTopY)
{
	SDL_Rect source { spriteHorizontalIndex * spriteWidth, spriteVerticalIndex * spriteHeight, spriteWidth, spriteHeight };
	SDL_Rect destination { (int)drawLeftX, (int)drawTopY, spriteWidth, spriteHeight };
	SDL_RenderCopy(activeRenderer, activeRenderTexture, &source, &destination);
}
void SpriteSheet::renderSpriteCenteredAtScreenPosition(
	int spriteHorizontalIndex, int spriteVerticalIndex, float drawCenterX, float drawCenterY)
{
	(this->*renderSpriteAtScreenPosition)(
		spriteHorizontalIndex, spriteVerticalIndex, (GLint)(drawCenterX - centerAnchorX), (GLint)(drawCenterY - centerAnchorY));
}
void SpriteSheet::renderFilledRectangle(
	GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint leftX, GLint topY, GLint rightX, GLint bottomY)
{
	setRectangleColor(red, green, blue, alpha);
	renderPreColoredRectangle(leftX, topY, rightX, bottomY);
	setRectangleColor(1.0f, 1.0f, 1.0f, 1.0f);
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
	setRectangleColor(red, green, blue, alpha);
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
void SpriteSheet::setRectangleColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	glColor4f(red, green, blue, alpha);
}
