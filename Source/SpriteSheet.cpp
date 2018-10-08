#include "SpriteSheet.h"
#include <SDL_opengl.h>

SpriteSheet::SpriteSheet(objCounterParametersComma() const char* imagePath, int horizontalSpriteCount, int verticalSpriteCount)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
textureId(0)
, spriteWidth(1)
, spriteHeight(1)
, spriteSheetWidth(1.0f)
, spriteSheetHeight(1.0f) {
	glGenTextures(1, &textureId);

	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	SDL_Surface* surface = IMG_Load(imagePath);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
	spriteSheetWidth = (float)(surface->w);
	spriteSheetHeight = (float)(surface->h);
	spriteWidth = surface->w / horizontalSpriteCount;
	spriteHeight = surface->h / verticalSpriteCount;
	SDL_FreeSurface(surface);
}
SpriteSheet::~SpriteSheet() {}
void SpriteSheet::render(int spriteHorizontalIndex, int spriteVerticalIndex) {
	float minX = (float)(spriteHorizontalIndex * spriteWidth);
	float minY = (float)(spriteVerticalIndex * spriteHeight);
	float maxX = minX + (float)(spriteWidth);
	float maxY = minY + (float)(spriteHeight);
	float texMinX = minX / spriteSheetWidth;
	float texMinY = minY / spriteSheetHeight;
	float texMaxX = maxX / spriteSheetWidth;
	float texMaxY = maxY / spriteSheetHeight;

	glBindTexture(GL_TEXTURE_2D, textureId);
	glBegin(GL_QUADS);
	glTexCoord2f(texMinX, texMinY);
	glVertex2f(minX, minY);
	glTexCoord2f(texMaxX, texMinY);
	glVertex2f(maxX, minY);
	glTexCoord2f(texMaxX, texMaxY);
	glVertex2f(maxX, maxY);
	glTexCoord2f(texMinX, texMaxY);
	glVertex2f(minX, maxY);
	glEnd();
}
